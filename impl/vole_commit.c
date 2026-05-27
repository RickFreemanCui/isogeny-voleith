/*
 * vole_commit.c -- Subspace VOLE over F_{p^2}
 *
 * Matrix layout: all matrices are row-major (row r, column c).
 * U: L_HAT_VOLE rows × K_C cols
 * Enc_U: L_HAT_VOLE rows × N_C cols (RS-encoded)
 * C: L_HAT_VOLE rows × (N_C - K_C) cols (= parity of Enc_U - raw_U_parity)
 * V: L_HAT_VOLE rows × N_C cols
 */
#include "vole_commit.h"
#include "prg.h"
#include "rs_code.h"
#include <string.h>
#include <stdlib.h>

#define ROWS L_HAT_VOLE
#define K    K_C
#define N    N_C
#define D    (N - K)

/* Access row r, col c in row-major matrix */
#define IDX(r, c, stride) ((r) * (stride) + (c))

void vole_gen_alphas(fp2_t alpha[N_LEAVES])
{
    for (uint32_t j = 0; j < N_LEAVES; j++)
        fp2_set_small(&alpha[j], j + 1, 0);
}

/* ── sender ────────────────────────────────────────────────── */

void vole_sender(fp2_t *U,            /* ROWS × K */
                 fp2_t *V,            /* ROWS × N */
                 fp2_t *C,            /* ROWS × D */
                 const uint8_t leaf_seeds[BAVC_TAU][BAVC_N][SECPAR_BYTES],
                 const fp2_t alpha[N_LEAVES],
                 const bavc_iv_t iv)
{
    (void)iv;

    /* accumulate per-column sums u_col[r] and v_col[r] */
    fp2_t *u_cols = calloc(N * ROWS, sizeof(fp2_t));   /* N × ROWS, col-major temp */
    fp2_t *v_cols = calloc(N * ROWS, sizeof(fp2_t));

    for (int i = 0; i < N; i++) {
        for (uint32_t j = 0; j < BAVC_N; j++) {
            uint8_t prg_out[ROWS * FP2_BYTES];
            prg_gen(prg_out, ROWS, leaf_seeds[i][j], (uint32_t)j);
            for (int r = 0; r < ROWS; r++) {
                fp2_t val;
                fp2_decode(&val, &prg_out[r * FP2_BYTES]);
                fp2_add(&u_cols[i * ROWS + r], &u_cols[i * ROWS + r], &val);
                fp2_t term;
                fp2_mul(&term, &alpha[j], &val);
                fp2_sub(&v_cols[i * ROWS + r], &v_cols[i * ROWS + r], &term);
            }
        }
    }

    /* U[r][c] = u_cols[c][r] (transpose from col-major to row-major) */
    for (int c = 0; c < K; c++)
        for (int r = 0; r < ROWS; r++)
            fp2_copy(&U[IDX(r, c, K)], &u_cols[c * ROWS + r]);

    /* V[r][c] = v_cols[c][r] */
    for (int c = 0; c < N; c++)
        for (int r = 0; r < ROWS; r++)
            fp2_copy(&V[IDX(r, c, N)], &v_cols[c * ROWS + r]);

    /* RS encode each row of U, compute C */
    static fp2_t msg[K];    /* static: large array, avoid stack overflow */
    for (int r = 0; r < ROWS; r++) {
        for (int c = 0; c < K; c++)
            fp2_copy(&msg[c], &U[IDX(r, c, K)]);

        static fp2_t enc[N];
        rs_encode(enc, msg);

        /* C[r][c-K] = enc[c] - v_cols[c][r] for c = K..N-1 */
        for (int c = K; c < N; c++) {
            fp2_sub(&C[IDX(r, c - K, D)], &enc[c], &v_cols[c * ROWS + r]);
        }
    }

    free(u_cols); free(v_cols);
}

/* ── receiver ──────────────────────────────────────────────── */

void vole_receiver(fp2_t *Q,            /* ROWS × N */
                   const bavc_opening_t *opening,
                   const uint32_t delta[BAVC_TAU],
                   const fp2_t *C,      /* ROWS × D */
                   const fp2_t alpha[N_LEAVES],
                   const bavc_iv_t iv)
{
    /* reconstruct leaf seeds */
    uint8_t (*leaf_seeds)[BAVC_N][SECPAR_BYTES] = malloc(BAVC_TAU * sizeof(*leaf_seeds));
    bavc_commit_t dummy;
    bavc_reconstruct(dummy, leaf_seeds, opening, delta, iv);

    /* q_cols[i][r] = Σ_{j≠δ_i} (α_{δ_i} - α_j)·PRG(sd_{i,j})[r] */
    fp2_t *q_cols = calloc(N * ROWS, sizeof(fp2_t));

    for (int i = 0; i < N; i++) {
        fp2_t a_delta, diff;
        fp2_copy(&a_delta, &alpha[delta[i]]);
        for (uint32_t j = 0; j < BAVC_N; j++) {
            if (j == delta[i]) continue;
            uint8_t prg_out[ROWS * FP2_BYTES];
            prg_gen(prg_out, ROWS, leaf_seeds[i][j], (uint32_t)j);
            fp2_copy(&diff, &a_delta);
            fp2_sub(&diff, &diff, &alpha[j]);
            for (int r = 0; r < ROWS; r++) {
                fp2_t val, term;
                fp2_decode(&val, &prg_out[r * FP2_BYTES]);
                fp2_mul(&term, &diff, &val);
                fp2_add(&q_cols[i * ROWS + r], &q_cols[i * ROWS + r], &term);
            }
        }
    }
    free(leaf_seeds);

    /* Q[r][c] = q_cols[c][r] - C[r][c-K] * α_{δ_c} for c ≥ K */
    for (int c = 0; c < N; c++)
        for (int r = 0; r < ROWS; r++)
            fp2_copy(&Q[IDX(r, c, N)], &q_cols[c * ROWS + r]);

    for (int r = 0; r < ROWS; r++) {
        for (int c = K; c < N; c++) {
            fp2_t corr;
            fp2_mul(&corr, &C[IDX(r, c - K, D)], &alpha[delta[c]]);
            fp2_sub(&Q[IDX(r, c, N)], &Q[IDX(r, c, N)], &corr);
        }
    }

    free(q_cols);
}
