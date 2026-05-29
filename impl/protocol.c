/*
 * protocol.c -- 9-round VOLEitH NIZK with Fiat-Shamir + implicit verification
 *
 * Four FS hash points (domain bytes 0x08..0x0b):
 *   chi1   = SHAKE128(com* || C || iv || 0x08)
 *   chi2   = SHAKE128(chi1 || u_tilde || D || 0x09)
 *   Delta' = SHAKE128(chi2 || pz_middle[1..D_QS-1] || 0x0a)
 *   I      = SHAKE128(Delta' || Q' || M[Q'] || pz_c0 || counter || 0x0b)
 *            [grind counter until I has w trailing zero bits]
 *
 * Verification is implicit — the verifier assumes each check equation holds
 * to recover missing values (com*, pz_c0, M[Q']), feeds them into FS hashing,
 * and compares the recomputed I against the I in the proof. PoW on I is also
 * verified.
 */
#include "protocol.h"
#include "prg.h"
#include "rs_code.h"
#include "vole_commit.h"
#include "quicksilver.h"
#include "shake.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/* ── helpers ────────────────────────────────────────────────── */

void witness_generate_trivial(witness_t witness, const fp2_t j0) {
    for (int i = 0; i < L_WIT; i++) fp2_copy(&witness[i], &j0);
}

/* Expand SHAKE output to an F_{p^2} element via rejection sampling.
 * Uses 64 bytes of XOF output, reduces mod p for real part, repeats for imag. */
static void shake_to_fp2(fp2_t *out, shake128_state_t *st) {
    uint8_t buf[64];
    shake128_finalise(st, buf, 64);
    /* Decode as field element via fp_decode which reduces mod 2p */
    fp_decode(out->re, buf);
    fp_decode(out->im, buf + 32);
}

/* Derive random seed from /dev/urandom, hashed with domain string */
static void random_seed(uint8_t seed[SECPAR_BYTES]) {
    uint8_t raw[32];
    FILE *f = fopen("/dev/urandom", "rb");
    if (!f) { for (int i = 0; i < 32; i++) raw[i] = 0; }
    else { fread(raw, 1, 32, f); fclose(f); }
    shake128_xof(seed, SECPAR_BYTES, raw, sizeof(raw));
}

/* Compute the linear combination sum_i chi^i * row_i and write to out.
 * Used for u_tilde computation. */
static void linear_combine_rows(fp2_t *out, int out_len,
                                const fp2_t *mat, int rows, int stride,
                                const fp2_t *chi) {
    for (int c = 0; c < out_len; c++) fp2_set_zero(&out[c]);
    fp2_t chi_pow; fp2_set_one(&chi_pow);
    for (int r = 0; r < rows; r++) {
        for (int c = 0; c < out_len; c++) {
            fp2_t term;
            fp2_mul(&term, &mat[r * stride + c], &chi_pow);
            fp2_add(&out[c], &out[c], &term);
        }
        fp2_mul(&chi_pow, &chi_pow, chi);
    }
}

/* ── prove ──────────────────────────────────────────────────── */

int prove(uint8_t *proof, size_t *proof_len,
          const witness_t witness,
          const fp2_t j0, const fp2_t jk)
{
    /* 1. Random root seed + IV (public, sent in proof) */
    uint8_t root_seed[SECPAR_BYTES];
    random_seed(root_seed);
    bavc_iv_t iv;
    /* Derive IV from root_seed via SHAKE128 */ {
        shake128_state_t st_iv;
        shake128_init(&st_iv);
        shake128_absorb(&st_iv, root_seed, SECPAR_BYTES);
        shake128_absorb_byte(&st_iv, 0x00);
        shake128_finalise(&st_iv, iv, sizeof(iv));
    }

    /* 2. BAVC commit */
    bavc_commit_t commit;
    bavc_forest_t forest;
    leaf_hashes_t leaf_hashes;
    bavc_commit(commit, &forest, leaf_hashes, root_seed, iv);

    /* 3. VOLE sender: U (L_HAT × K_C), V (L_HAT × N_C), C (L_HAT × D) */
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

    /* ── FS Hash 1: chi1 = SHAKE128(commit || C || iv || 0x08) ── */
    shake128_state_t st;
    shake128_init(&st);
    shake128_absorb(&st, commit, sizeof(commit));
    for (int i = 0; i < C_SIZE; i++) {
        uint8_t buf[FP2_BYTES]; fp2_encode(buf, &Cmat[i]);
        shake128_absorb(&st, buf, FP2_BYTES);
    }
    shake128_absorb(&st, iv, sizeof(iv));
    shake128_absorb_byte(&st, 0x08);
    fp2_t chi1; shake_to_fp2(&chi1, &st);

    /* 5. u_tilde = sum_i chi1^i * U[i][:]  (L_HAT rows, K_C cols) */
    fp2_t *u_tilde = malloc(K_C * sizeof(fp2_t));
    linear_combine_rows(u_tilde, K_C, U, L_HAT_VOLE, K_C, &chi1);

    /* ── FS Hash 2: chi2 = SHAKE128(chi1 || u_tilde || D || 0x09) ── */
    shake128_init(&st);
    { uint8_t buf[FP2_BYTES]; fp2_encode(buf, &chi1); shake128_absorb(&st, buf, FP2_BYTES); }
    for (int i = 0; i < K_C; i++) {
        uint8_t buf[FP2_BYTES]; fp2_encode(buf, &u_tilde[i]);
        shake128_absorb(&st, buf, FP2_BYTES);
    }
    for (int i = 0; i < L_WIT; i++) {
        uint8_t buf[FP2_BYTES]; fp2_encode(buf, &D[i]);
        shake128_absorb(&st, buf, FP2_BYTES);
    }
    shake128_absorb_byte(&st, 0x09);
    fp2_t chi2; shake_to_fp2(&chi2, &st);

    /* 6. QuickSilver: Wmat = witness in matrix form, V' = U[L_VOLE:2L_VOLE] */
    fp2_t *Wmat = calloc(L_VOLE * K_C, sizeof(fp2_t));
    fp2_t *Vprime = calloc(L_VOLE * K_C, sizeof(fp2_t));
    for (int i = 0; i < L_WIT; i++) {
        int row = i / K_C, col = i % K_C;
        fp2_copy(&Wmat[row * K_C + col], &witness[i]);
        fp2_copy(&Vprime[row * K_C + col], &V[(L_VOLE + row) * N_C + col]);
    }
    /* r_qs: D_QS-1 trailing elements past the witness in U (ZK) */
    itmac_poly_t r_qs[D_QS - 1];
    for (int d = 0; d < D_QS - 1; d++) {
        int idx = L_WIT + d;
        int row = idx / K_C, col = idx % K_C;
        itmac_from_value_mask(&r_qs[d],
            &U[row * K_C + col],
            &V[(L_VOLE + row) * N_C + col]);
    }
    fp2_t *pz_full = malloc((D_QS + 1) * sizeof(fp2_t));
    quicksilver_prove(pz_full, Wmat, Vprime, r_qs, &chi2, &j0, &jk);

    /* ── FS Hash 3: Delta' = SHAKE128(chi2 || pz_coeffs[0..D_QS-1] || 0x0a) ── */
    shake128_init(&st);
    { uint8_t buf[FP2_BYTES]; fp2_encode(buf, &chi2); shake128_absorb(&st, buf, FP2_BYTES); }
    /* Hash 3: Delta' = SHAKE128(chi2 || pz_middle[1..D_QS-1] || 0x0a) */
    shake128_init(&st);
    { uint8_t buf[FP2_BYTES]; fp2_encode(buf, &chi2); shake128_absorb(&st, buf, FP2_BYTES); }
    for (int i = 1; i < D_QS; i++) {
        uint8_t buf[FP2_BYTES]; fp2_encode(buf, &pz_full[i]);
        shake128_absorb(&st, buf, FP2_BYTES);
    }
    shake128_absorb_byte(&st, 0x0a);
    fp2_t Delta_prime; shake_to_fp2(&Delta_prime, &st);

    /* 7. Q' = W·Δ' + V' */
    fp2_t *Qprime = malloc(L_VOLE * K_C * sizeof(fp2_t));
    for (int i = 0; i < L_VOLE * K_C; i++) {
        fp2_mul(&Qprime[i], &Wmat[i], &Delta_prime);
        fp2_add(&Qprime[i], &Qprime[i], &Vprime[i]);
    }

    /* M[Q'] = V₁·Δ' + V₂  (subspace VOLE authentication, L_VOLE × N_C) */
    fp2_t *MQ = malloc(L_VOLE * N_C * sizeof(fp2_t));
    for (int r = 0; r < L_VOLE; r++) {
        for (int c = 0; c < N_C; c++) {
            fp2_mul(&MQ[r * N_C + c], &V[r * N_C + c], &Delta_prime);
            fp2_add(&MQ[r * N_C + c], &MQ[r * N_C + c], &V[(L_VOLE + r) * N_C + c]);
        }
    }

    /* ── FS Hash 4: I = SHAKE128(Delta' || Q' || M[Q'] || pz_c0 || counter || 0x0b) ── */
    uint32_t I_vec[BAVC_TAU];
    uint32_t counter = 0;
    int found = 0;
    while (!found) {
        shake128_init(&st);
        { uint8_t buf[FP2_BYTES]; fp2_encode(buf, &Delta_prime); shake128_absorb(&st, buf, FP2_BYTES); }
        for (int i = 0; i < L_VOLE * K_C; i++) {
            uint8_t buf[FP2_BYTES]; fp2_encode(buf, &Qprime[i]);
            shake128_absorb(&st, buf, FP2_BYTES);
        }
        for (int i = 0; i < L_VOLE * N_C; i++) {
            uint8_t buf[FP2_BYTES]; fp2_encode(buf, &MQ[i]);
            shake128_absorb(&st, buf, FP2_BYTES);
        }
        { uint8_t buf[FP2_BYTES]; fp2_encode(buf, &pz_full[0]); shake128_absorb(&st, buf, FP2_BYTES); }
        shake128_absorb(&st, &counter, sizeof(counter));
        shake128_absorb_byte(&st, 0x0b);

        uint8_t ibuf[BAVC_TAU * 4];
        shake128_finalise(&st, ibuf, sizeof(ibuf));
        for (int i = 0; i < BAVC_TAU; i++)
            I_vec[i] = ((uint32_t*)ibuf)[i] & (BAVC_N - 1);

        /* Check w=8 trailing zero bits on the concatenated I bits */
        unsigned int zero_bits = 0;
        for (int i = BAVC_TAU - 1; i >= 0 && zero_bits < POW_W; i--) {
            uint32_t v = I_vec[i];
            for (int b = 0; b < 12 && zero_bits < POW_W; b++) {
                if ((v >> b) & 1) break;
                zero_bits++;
            }
            if (zero_bits >= POW_W) break;
        }
        if (zero_bits >= POW_W) { found = 1; break; }
        counter++;
    }

    /* 8. BAVC open at I */
    bavc_opening_t opening;
    bavc_open(&opening, &forest, leaf_hashes, I_vec);

    /* 9. Serialize proof */
    uint8_t *p = proof;
    for (int i = 0; i < C_SIZE; i++)           { fp2_encode(p, &Cmat[i]); p += FP2_BYTES; }
    for (int i = 0; i < L_WIT; i++)            { fp2_encode(p, &D[i]); p += FP2_BYTES; }
    for (int i = 0; i < K_C; i++)              { fp2_encode(p, &u_tilde[i]); p += FP2_BYTES; }
    /* Send p_z[1..D_QS-1] (middle coeffs). Constant term p_z[0] recovered by verifier. */
    for (int i = 1; i < D_QS; i++)             { fp2_encode(p, &pz_full[i]); p += FP2_BYTES; }
    for (int i = 0; i < L_VOLE * K_C; i++)     { fp2_encode(p, &Qprime[i]); p += FP2_BYTES; }
    memcpy(p, I_vec, sizeof(I_vec));            p += sizeof(I_vec);
    memcpy(p, &opening, sizeof(opening));       p += sizeof(opening);
    memcpy(p, iv, sizeof(iv));                  p += sizeof(iv);
    memcpy(p, commit, sizeof(commit));          p += sizeof(commit);
    memcpy(p, &counter, sizeof(counter));       p += sizeof(counter);
    *proof_len = p - proof;

    free(U); free(V); free(Cmat); free(D); free(u_tilde);
    free(Wmat); free(Vprime); free(pz_full); free(Qprime); free(MQ);
    bavc_forest_free(&forest);
    return 0;
}

/* ── verify ──────────────────────────────────────────────────── */

int verify(const uint8_t *proof, size_t proof_len,
           const fp2_t j0, const fp2_t jk)
{
    (void)j0; (void)jk; (void)proof_len;
    const uint8_t *p = proof;

    /* Unpack */
    fp2_t *Cmat = malloc(C_SIZE * sizeof(fp2_t));
    fp2_t *D    = malloc(L_WIT * sizeof(fp2_t));
    fp2_t *u_tilde = malloc(K_C * sizeof(fp2_t));
    fp2_t *pz_coeffs = malloc((D_QS - 1) * sizeof(fp2_t));  /* middle coeffs only */
    fp2_t *Qprime = malloc(L_VOLE * K_C * sizeof(fp2_t));

    for (int i = 0; i < C_SIZE; i++)           { fp2_decode(&Cmat[i], p); p += FP2_BYTES; }
    for (int i = 0; i < L_WIT; i++)            { fp2_decode(&D[i], p); p += FP2_BYTES; }
    for (int i = 0; i < K_C; i++)              { fp2_decode(&u_tilde[i], p); p += FP2_BYTES; }
    for (int i = 0; i < D_QS - 1; i++)         { fp2_decode(&pz_coeffs[i], p); p += FP2_BYTES; }
    for (int i = 0; i < L_VOLE * K_C; i++)     { fp2_decode(&Qprime[i], p); p += FP2_BYTES; }

    const uint32_t *I_vec = (const uint32_t *)p; p += sizeof(uint32_t) * BAVC_TAU;
    const bavc_opening_t *opening = (const bavc_opening_t *)p; p += sizeof(bavc_opening_t);
    const bavc_iv_t *iv = (const bavc_iv_t *)p; p += sizeof(bavc_iv_t);
    const bavc_commit_t *commit = (const bavc_commit_t *)p; p += sizeof(bavc_commit_t);
    const uint32_t *counter = (const uint32_t *)p;

    /* ── Reconstruct BAVC → com*, leaf_seeds ── */
    bavc_commit_t com_star;
    uint8_t (*leaf_seeds)[BAVC_N][SECPAR_BYTES] = malloc(BAVC_TAU * sizeof(*leaf_seeds));
    bavc_reconstruct(com_star, leaf_seeds, opening, I_vec, *iv);

    /* ── FS Hash 1: chi1 = SHAKE128(com* || C || iv || 0x08) ── */
    shake128_state_t st;
    shake128_init(&st);
    shake128_absorb(&st, com_star, sizeof(com_star));
    for (int i = 0; i < C_SIZE; i++) {
        uint8_t buf[FP2_BYTES]; fp2_encode(buf, &Cmat[i]);
        shake128_absorb(&st, buf, FP2_BYTES);
    }
    shake128_absorb(&st, iv, sizeof(*iv));
    shake128_absorb_byte(&st, 0x08);
    fp2_t chi1; shake_to_fp2(&chi1, &st);

    /* ── FS Hash 2: chi2 = SHAKE128(chi1 || u_tilde || D || 0x09) ── */
    shake128_init(&st);
    { uint8_t buf[FP2_BYTES]; fp2_encode(buf, &chi1); shake128_absorb(&st, buf, FP2_BYTES); }
    for (int i = 0; i < K_C; i++) {
        uint8_t buf[FP2_BYTES]; fp2_encode(buf, &u_tilde[i]);
        shake128_absorb(&st, buf, FP2_BYTES);
    }
    for (int i = 0; i < L_WIT; i++) {
        uint8_t buf[FP2_BYTES]; fp2_encode(buf, &D[i]);
        shake128_absorb(&st, buf, FP2_BYTES);
    }
    shake128_absorb_byte(&st, 0x09);
    fp2_t chi2; shake_to_fp2(&chi2, &st);

    /* ── FS Hash 3: Delta' = SHAKE128(chi2 || pz_middle || 0x0a) ── */
    shake128_init(&st);
    { uint8_t buf[FP2_BYTES]; fp2_encode(buf, &chi2); shake128_absorb(&st, buf, FP2_BYTES); }
    for (int i = 0; i < D_QS - 1; i++) {
        uint8_t buf[FP2_BYTES]; fp2_encode(buf, &pz_coeffs[i]);
        shake128_absorb(&st, buf, FP2_BYTES);
    }
    shake128_absorb_byte(&st, 0x0a);
    fp2_t Delta_prime; shake_to_fp2(&Delta_prime, &st);

    /* ── Implicit QuickSilver check: recover p_z[0] (constant term) ──
     * Compute k_z using verifier's Q' values, then:
     *   p_z[0] = k_z - sum_{i=1}^{D_QS-1} pz_coeffs[i-1] * Delta'^i
     * (leading coeff = z = 0, we only have D_QS middle coeffs)
     * But we need p_z[0] for the FS hash... Actually we don't need p_z[0]
     * for the FS hash — we already hashed the D_QS coeffs in Hash 3.
     * The QuickSilver check is implicit: the verifier just needs to
     * reconstruct the same transcript, and I-equality confirms correctness. */

    /* ── Reconstruct VOLE → Q matrix ── */
    fp2_t alpha[N_LEAVES];
    vole_gen_alphas(alpha);
    fp2_t *Q = malloc(L_HAT_VOLE * N_C * sizeof(fp2_t));
    vole_receiver(Q, opening, I_vec, Cmat, alpha, *iv);

    /* ── Recover r_qs keys from Q' (trailing past witness) ── */
    fp2_t k_r_qs[D_QS - 1];
    for (int d = 0; d < D_QS - 1; d++) {
        int idx = L_WIT + d;
        int row = idx / K_C, col = idx % K_C;
        fp2_copy(&k_r_qs[d], &Qprime[row * K_C + col]);
    }

    /* ── QuickSilver: compute k_z, recover p_z constant term ── */
    fp2_t kz;
    quicksilver_verify(&kz, Qprime, k_r_qs, &chi2, &Delta_prime, &j0, &jk);
    /* pz_coeffs[0..D_QS-2] holds coeffs for x^1..x^{D_QS-1}.
     * Recover c_0 = k_z - Σ_{i=1}^{D_QS-1} pz_coeffs[i-1]·Δ'ⁱ */
    fp2_t pz_c0, sum_hi;
    fp2_set_zero(&sum_hi);
    fp2_t dp; fp2_set_one(&dp);
    for (int i = 1; i < D_QS; i++) {
        fp2_mul(&dp, &dp, &Delta_prime);  /* Δ'ⁱ */
        fp2_t term;
        fp2_mul(&term, &pz_coeffs[i - 1], &dp);
        fp2_add(&sum_hi, &sum_hi, &term);
    }
    fp2_sub(&pz_c0, &kz, &sum_hi);

    /* ── Subspace VOLE (implicit): recover M[Q'] = Q₁·Δ' + Q₂ − Enc_C(Q')·Δ ── */
    fp2_t *MQ_recovered = malloc(L_VOLE * N_C * sizeof(fp2_t));
    /* RS-encode D (reshaped to L_VOLE × K_C) → L_VOLE × N_C */
    for (int r = 0; r < L_VOLE; r++) {
        fp2_t d_row[K_C], enc_d[N_C], qp_row[K_C], enc_qp[N_C];
        for (int c = 0; c < K_C; c++) {
            int idx = r * K_C + c;
            fp2_copy(&d_row[c], (idx < L_WIT) ? &D[idx] : &pz_c0 /* dummy, zero */);
            fp2_copy(&qp_row[c], (idx < L_WIT) ? &Qprime[idx] : &pz_c0);
        }
        rs_encode(enc_d, d_row);
        rs_encode(enc_qp, qp_row);
        for (int c = 0; c < N_C; c++) {
            /* Q₁ = Q[r][c] + enc_d[c] * α_{δ_c} */
            fp2_t q1, term;
            fp2_mul(&term, &enc_d[c], &alpha[I_vec[c]]);
            fp2_add(&q1, &Q[r * N_C + c], &term);
            /* M[r][c] = q1 * Δ' + Q₂ − enc_qp[c] * α_{δ_c} */
            fp2_mul(&MQ_recovered[r * N_C + c], &q1, &Delta_prime);
            fp2_add(&MQ_recovered[r * N_C + c], &MQ_recovered[r * N_C + c],
                    &Q[(L_VOLE + r) * N_C + c]);
            fp2_mul(&term, &enc_qp[c], &alpha[I_vec[c]]);
            fp2_sub(&MQ_recovered[r * N_C + c], &MQ_recovered[r * N_C + c], &term);
        }
    }

    /* ── FS Hash 4: I = SHAKE128(Delta' || Q' || M[Q'] || pz_c0 || counter || 0x0b) ── */
    shake128_init(&st);
    { uint8_t buf[FP2_BYTES]; fp2_encode(buf, &Delta_prime); shake128_absorb(&st, buf, FP2_BYTES); }
    for (int i = 0; i < L_VOLE * K_C; i++) {
        uint8_t buf[FP2_BYTES]; fp2_encode(buf, &Qprime[i]);
        shake128_absorb(&st, buf, FP2_BYTES);
    }
    for (int i = 0; i < L_VOLE * N_C; i++) {
        uint8_t buf[FP2_BYTES]; fp2_encode(buf, &MQ_recovered[i]);
        shake128_absorb(&st, buf, FP2_BYTES);
    }
    { uint8_t buf[FP2_BYTES]; fp2_encode(buf, &pz_c0); shake128_absorb(&st, buf, FP2_BYTES); }
    shake128_absorb(&st, counter, sizeof(*counter));
    shake128_absorb_byte(&st, 0x0b);

    uint8_t ibuf[BAVC_TAU * 4];
    shake128_finalise(&st, ibuf, sizeof(ibuf));
    uint32_t I_recomp[BAVC_TAU];
    for (int i = 0; i < BAVC_TAU; i++)
        I_recomp[i] = ((uint32_t*)ibuf)[i] & (BAVC_N - 1);

    /* ── Final check: I_recomp == I_vec and PoW ── */
    int ok = (memcmp(I_recomp, I_vec, BAVC_TAU * sizeof(uint32_t)) == 0);
    if (ok) {
        /* PoW check: I has w trailing zero bits */
        unsigned int zero_bits = 0;
        const uint32_t *ivp = I_vec;
        for (int i = BAVC_TAU - 1; i >= 0 && zero_bits < POW_W; i--) {
            uint32_t v = ivp[i];
            for (int b = 0; b < 12 && zero_bits < POW_W; b++) {
                if ((v >> b) & 1) break;
                zero_bits++;
            }
        }
        ok = (zero_bits >= POW_W);
    }

    free(leaf_seeds);
    free(Q);
    free(Cmat); free(D); free(u_tilde); free(pz_coeffs); free(Qprime);
    (void)commit; /* com* used in place of commit for FS hashing */
    return ok;
}
