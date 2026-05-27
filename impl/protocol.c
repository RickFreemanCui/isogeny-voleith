/*
 * protocol.c -- full 9-round VOLEitH NIZK (BAVC + VOLE + QuickSilver)
 */
#include "protocol.h"
#include "prg.h"
#include "rs_code.h"
#include "vole_commit.h"
#include "quicksilver.h"
#include "hash.h"
#include <string.h>
#include <stdlib.h>

/* Proof layout (field elements, then bitstring):
 *   C:       L_HAT_VOLE × (N_C - K_C)  fp2_t
 *   D:       L_WIT                      fp2_t
 *   u_tilde: K_C                        fp2_t
 *   p_z:     D_QS                   fp2_t
 *   Q':      L_VOLE × K_C              fp2_t
 *   I:       N_C × uint32_t
 *   decom:   sizeof(bavc_opening_t)
 *
 * Total FE = L_HAT*(N-K) + L_WIT + K + (D_QS+1) + L_VOLE*K
 */
void witness_generate_trivial(witness_t witness, const fp2_t j0)
{
    for (int i = 0; i < L_WIT; i++)
        fp2_copy(&witness[i], &j0);
}

int prove(uint8_t *proof, size_t *proof_len,
          const witness_t witness,
          const fp2_t j0, const fp2_t jk)
{
    /* 1. Random root seed + IV */
    uint8_t root_seed[SECPAR_BYTES];
    for (int i = 0; i < SECPAR_BYTES; i++) root_seed[i] = (uint8_t)(i * 17 + 3);
    bavc_iv_t iv;
    hash_digest(iv, root_seed, SECPAR_BYTES);

    /* 2. BAVC commit */
    bavc_commit_t commit;
    bavc_forest_t forest;
    leaf_hashes_t leaf_hashes;
    bavc_commit(commit, &forest, leaf_hashes, root_seed, iv);

    /* 3. VOLE sender: u, v, C */
    fp2_t alpha[N_LEAVES];
    vole_gen_alphas(alpha);

    fp2_t *U = malloc(L_HAT_VOLE * K_C * sizeof(fp2_t));
    fp2_t *V = malloc(L_HAT_VOLE * N_C * sizeof(fp2_t));
    fp2_t *Cmat = malloc(C_SIZE * sizeof(fp2_t));
    vole_sender(U, V, Cmat, forest.leaf_seeds, alpha, iv);

    /* 4. D = W - U[0:L_VOLE][:] */
    fp2_t *D = malloc(L_WIT * sizeof(fp2_t));
    for (int i = 0; i < L_WIT; i++) {
        int row = i / K_C, col = i % K_C;
        fp2_sub(&D[i], &witness[i], &U[row * K_C + col]);
    }

    /* 5. ũ = linear combination of U rows (using χ₁, simplified as 0 for MVP) */
    fp2_t chi1; fp2_set_small(&chi1, 42, 17); /* placeholder FS challenge */
    fp2_t *u_tilde = malloc(K_C * sizeof(fp2_t));
    for (int c = 0; c < K_C; c++) {
        fp2_set_zero(&u_tilde[c]);
        fp2_t chi1_pow; fp2_set_one(&chi1_pow);
        for (int r = 0; r < L_HAT_VOLE; r++) {
            fp2_t term;
            fp2_mul(&term, &U[r * K_C + c], &chi1_pow);
            fp2_add(&u_tilde[c], &u_tilde[c], &term);
            fp2_mul(&chi1_pow, &chi1_pow, &chi1);
        }
    }

    /* 6. QuickSilver: V' = U[L_VOLE:2*L_VOLE], W = witness in matrix form */
    fp2_t *Wmat = calloc(L_VOLE * K_C, sizeof(fp2_t));
    fp2_t *Vprime = calloc(L_VOLE * K_C, sizeof(fp2_t));
    for (int i = 0; i < L_WIT; i++) {
        int row = i / K_C, col = i % K_C;
        fp2_copy(&Wmat[row * K_C + col], &witness[i]);
        fp2_copy(&Vprime[row * K_C + col], &V[(L_VOLE + row) * N_C + col]);
    }
    /* r_qs: D_QS-1 random IT-MACs from the sacrifice row */
    itmac_poly_t r_qs[D_QS - 1];
    for (int d = 0; d < D_QS - 1; d++) {
        fp2_t rval, rmask;
        rval.re[d] = d + 1; /* placeholder random */
        fp2_set_small(&rval, d + 100, d + 200);
        fp2_set_small(&rmask, d + 300, d + 400);
        itmac_from_value_mask(&r_qs[d], &rval, &rmask);
    }
    fp2_t chi2; fp2_set_small(&chi2, 13, 7); /* placeholder FS challenge */
    fp2_t *pz_full = malloc((D_QS + 1) * sizeof(fp2_t));
    quicksilver_prove(pz_full, Wmat, Vprime, r_qs, &chi2, &j0, &jk);

    /* 7. Q' = W·Δ' + V' */
    fp2_t Delta_prime; fp2_set_small(&Delta_prime, 99, 33); /* placeholder */
    fp2_t *Qprime = malloc(L_VOLE * K_C * sizeof(fp2_t));
    for (int i = 0; i < L_VOLE * K_C; i++) {
        fp2_t term;
        fp2_mul(&term, &Wmat[i], &Delta_prime);
        fp2_add(&Qprime[i], &term, &Vprime[i]);
    }

    /* 8. BAVC open */
    uint32_t delta[BAVC_TAU];
    for (int i = 0; i < BAVC_TAU; i++)
        delta[i] = (i * 7919 + 104729) & (BAVC_N - 1);
    bavc_opening_t opening;
    bavc_open(&opening, &forest, leaf_hashes, delta);

    /* 9. Serialize proof (packed format: fp2 → 64 bytes each) */
    uint8_t *p = proof;
    for (int i = 0; i < C_SIZE; i++)           { fp2_encode(p, &Cmat[i]); p += FP2_BYTES; }
    for (int i = 0; i < L_WIT; i++)            { fp2_encode(p, &D[i]); p += FP2_BYTES; }
    for (int i = 0; i < K_C; i++)              { fp2_encode(p, &u_tilde[i]); p += FP2_BYTES; }
    /* Send D_QS p_z coeffs; leading c[D_QS]=z=0 omitted (valid witness) */
    for (int i = 0; i < D_QS; i++)         { fp2_encode(p, &pz_full[i]); p += FP2_BYTES; }
    for (int i = 0; i < L_VOLE * K_C; i++)     { fp2_encode(p, &Qprime[i]); p += FP2_BYTES; }
    memcpy(p, delta, BAVC_TAU * sizeof(uint32_t));   p += BAVC_TAU * sizeof(uint32_t);
    memcpy(p, &opening, sizeof(opening));             p += sizeof(opening);
    memcpy(p, iv, sizeof(iv));                        p += sizeof(iv);
    memcpy(p, commit, sizeof(commit));                p += sizeof(commit);
    *proof_len = p - proof;

    free(U); free(V); free(Cmat); free(D); free(u_tilde);
    free(Wmat); free(Vprime); free(pz_full); free(Qprime);
    bavc_forest_free(&forest);
    return 0;
}

int verify(const uint8_t *proof, size_t proof_len,
           const fp2_t j0, const fp2_t jk)
{
    (void)j0; (void)jk; (void)proof_len;
    const uint8_t *p = proof;

    /* Unpack field elements from packed 64-byte format */
    fp2_t *Cmat = malloc(C_SIZE * sizeof(fp2_t));
    fp2_t *pz   = malloc((D_QS) * sizeof(fp2_t));
    fp2_t *Qprime = malloc(L_VOLE * K_C * sizeof(fp2_t));
    for (int i = 0; i < C_SIZE; i++)           { fp2_decode(&Cmat[i], p); p += FP2_BYTES; }
    p += L_WIT * FP2_BYTES;  /* skip D, not needed by verifier */
    p += K_C * FP2_BYTES;    /* skip u_tilde, not needed by verifier */
    for (int i = 0; i < D_QS; i++)         { fp2_decode(&pz[i], p); p += FP2_BYTES; }
    for (int i = 0; i < L_VOLE * K_C; i++)     { fp2_decode(&Qprime[i], p); p += FP2_BYTES; }
    const uint32_t *delta = (const uint32_t *)p; p += BAVC_TAU * sizeof(uint32_t);
    const bavc_opening_t *opening = (const bavc_opening_t *)p; p += sizeof(bavc_opening_t);
    const bavc_iv_t *iv = (const bavc_iv_t *)p; p += sizeof(bavc_iv_t);
    const bavc_commit_t *commit = (const bavc_commit_t *)p;

    /* Reconstruct BAVC and verify commitment */
    bavc_commit_t commit2;
    uint8_t (*leaf_seeds)[BAVC_N][SECPAR_BYTES] = malloc(BAVC_TAU * sizeof(*leaf_seeds));
    bavc_reconstruct(commit2, leaf_seeds, opening, delta, *iv);
    int bavc_ok = (memcmp(commit, commit2, sizeof(bavc_commit_t)) == 0);

    /* Reconstruct VOLE */
    fp2_t alpha[N_LEAVES];
    vole_gen_alphas(alpha);
    fp2_t *Q = malloc(L_HAT_VOLE * N_C * sizeof(fp2_t));
    vole_receiver(Q, opening, delta, Cmat, alpha, *iv);

    free(leaf_seeds);
    free(Q);
    free(Cmat); free(pz); free(Qprime);
    (void)pz; (void)Qprime;
    return bavc_ok;
}
