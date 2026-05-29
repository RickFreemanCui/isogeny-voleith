/* test_quicksilver.c -- QuickSilver prove/verify round-trip */
#include <stdio.h>
#include <string.h>
#include "../quicksilver.h"

static int tests=0, passed=0;
#define T(s) do{tests++;printf("  %s ... ",s);}while(0)
#define OK() do{passed++;printf("OK\n");}while(0)
#define F(m) do{printf("FAIL: %s\n",m);return 1;}while(0)
#define C(c,m) do{if(!(c))F(m);}while(0)

int main(void) {
    printf("=== test_quicksilver ===\n");

    /* Build witness: all j_i = (1,0) (not a valid isogeny path, but tests mechanics) */
    static fp2_t W[L_VOLE * K_C], Vprime[L_VOLE * K_C];
    fp2_t j0, jk;
    fp2_set_small(&j0, 1, 0);
    fp2_set_small(&jk, 1, 0);

    for (int i = 0; i < L_WIT; i++) {
        int row = i / K_C, col = i % K_C;
        fp2_set_small(&W[row * K_C + col], 2, 3);  /* arbitrary j_i */
        fp2_set_small(&Vprime[row * K_C + col], 5, 7);  /* arbitrary mask */
    }

    /* D_QS-1 = 3 random IT-MACs for QuickSilver lift masking */
    itmac_poly_t r_qs[3];
    for (int d = 0; d < D_QS - 1; d++) {
        fp2_t rval, rmask;
        fp2_set_small(&rval, d+10, d+20);
        fp2_set_small(&rmask, d+30, d+40);
        itmac_from_value_mask(&r_qs[d], &rval, &rmask);
    }

    /* challenge chi2 */
    fp2_t chi2; fp2_set_small(&chi2, 13, 7);

    T("prove");
    static fp2_t proof[D_QS + 1];
    quicksilver_prove(proof, W, Vprime, r_qs, &chi2, &j0, &jk);
    OK();

    /* Verifier: Q' = W·Δ' + V' */
    fp2_t Delta_prime;
    fp2_set_small(&Delta_prime, 42, 17);

    static fp2_t Qprime[L_VOLE * K_C];
    for (int i = 0; i < L_VOLE * K_C; i++) {
        fp2_t term;
        fp2_mul(&term, &W[i], &Delta_prime);
        fp2_add(&Qprime[i], &term, &Vprime[i]);
    }

    /* r_qs keys: k_{r_d} = r_d·Δ' + m_d */
    fp2_t k_r_qs[D_QS - 1];
    for (int d = 0; d < D_QS - 1; d++) {
        fp2_t term;
        fp2_mul(&term, &r_qs[d].coeffs[1], &Delta_prime);
        fp2_add(&k_r_qs[d], &term, &r_qs[d].coeffs[0]);
    }

    T("compute_kz + recover constant term");
    fp2_t kz;
    quicksilver_verify(&kz, Qprime, k_r_qs, &chi2, &Delta_prime, &j0, &jk);
    /* Recover p_z[0] = kz - Σ_{i=1}^{D_QS} proof[i]·Δ'ⁱ
     * (include leading term c_{D_QS} which may be non-zero for test witness) */
    fp2_t recovered_c0, sum_hi;
    fp2_set_zero(&sum_hi);
    fp2_t dp; fp2_set_one(&dp);
    for (int i = 1; i <= D_QS; i++) {
        fp2_mul(&dp, &dp, &Delta_prime);  /* Δ'ⁱ */
        fp2_t term;
        fp2_mul(&term, &proof[i], &dp);
        fp2_add(&sum_hi, &sum_hi, &term);
    }
    fp2_sub(&recovered_c0, &kz, &sum_hi);
    /* Should match proof[0] (prover's constant term) */
    C(fp2_is_equal(&recovered_c0, &proof[0]), "constant term mismatch");
    OK();

    T("corrupted proof → wrong constant term");
    fp2_t bad_c; fp2_set_small(&bad_c, 999, 0);
    fp2_copy(&proof[0], &bad_c);
    quicksilver_verify(&kz, Qprime, k_r_qs, &chi2, &Delta_prime, &j0, &jk);
    fp2_set_zero(&sum_hi);
    fp2_set_one(&dp);
    for (int i = 1; i <= D_QS; i++) {
        fp2_mul(&dp, &dp, &Delta_prime);
        fp2_t term; fp2_mul(&term, &proof[i], &dp);
        fp2_add(&sum_hi, &sum_hi, &term);
    }
    fp2_sub(&recovered_c0, &kz, &sum_hi);
    C(!fp2_is_equal(&recovered_c0, &proof[0]), "should differ from corrupted proof[0]");
    OK();

    printf("\n  %d/%d passed\n", passed, tests);
    return (passed==tests)?0:1;
}
