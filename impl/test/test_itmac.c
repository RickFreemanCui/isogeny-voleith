/* test_itmac.c -- IT-MAC polynomial operations */
#include <stdio.h>
#include "../itmac.h"

static int tests=0, passed=0;
#define T(s) do{tests++;printf("  %s ... ",s);}while(0)
#define OK() do{passed++;printf("OK\n");}while(0)
#define F(m) do{printf("FAIL: %s\n",m);return 1;}while(0)
#define C(c,m) do{if(!(c))F(m);}while(0)

int main(void) {
    printf("=== test_itmac ===\n");

    /* setup: degree-1 IT-MACs for values a, b with masks */
    T("from_value_mask");
    fp2_t a_val, a_mask, b_val, b_mask;
    fp2_set_small(&a_val, 10, 0);
    fp2_set_small(&a_mask, 3, 0);
    fp2_set_small(&b_val, 5, 0);
    fp2_set_small(&b_mask, 7, 0);

    itmac_poly_t A, B;
    itmac_from_value_mask(&A, &a_val, &a_mask);
    itmac_from_value_mask(&B, &b_val, &b_mask);
    C(A.deg == 1 && B.deg == 1, "deg");
    OK();

    /* mul: (10x+3)*(5x+7) = 50x^2 + 85x + 21 */
    T("mul");
    itmac_poly_t M;
    itmac_mul(&M, &A, &B);
    C(M.deg == 2, "deg");
    fp2_t e50, e85, e21;
    fp2_set_small(&e50, 50, 0);
    fp2_set_small(&e85, 85, 0);
    fp2_set_small(&e21, 21, 0);
    C(fp2_is_equal(&M.coeffs[2], &e50), "coeff[2]");
    C(fp2_is_equal(&M.coeffs[1], &e85), "coeff[1]");
    C(fp2_is_equal(&M.coeffs[0], &e21), "coeff[0]");
    OK();

    /* add: (10x+3) + (5x+7) = 15x + 10 */
    T("add");
    itmac_poly_t S;
    itmac_add(&S, &A, &B);
    C(S.deg == 1, "deg");
    fp2_t e15, e10;
    fp2_set_small(&e15, 15, 0);
    fp2_set_small(&e10, 10, 0);
    C(fp2_is_equal(&S.coeffs[1], &e15), "coeff[1]");
    C(fp2_is_equal(&S.coeffs[0], &e10), "coeff[0]");
    OK();

    /* eval: at x=2, A(2)=10*2+3=23 */
    T("eval");
    fp2_t x, result, e23;
    fp2_set_small(&x, 2, 0);
    fp2_set_small(&e23, 23, 0);
    itmac_eval(&result, &A, &x);
    C(fp2_is_equal(&result, &e23), "eval");
    OK();

    /* constant itmac */
    T("const");
    itmac_poly_t Cmac;
    fp2_t c5; fp2_set_small(&c5, 5, 0);
    itmac_const(&Cmac, &c5, 3);
    C(Cmac.deg == 3, "deg");
    C(fp2_is_equal(&Cmac.coeffs[3], &c5), "leading");
    C(fp2_is_zero(&Cmac.coeffs[0]), "const is 0");
    OK();

    /* lift: combine 2 degree-1 randoms into degree-3 0-IT-MAC */
    T("lift");
    itmac_poly_t rands[2], lifted;
    fp2_t r0, r1, m0, m1;
    fp2_set_small(&r0, 1, 0); fp2_set_small(&m0, 2, 0);
    fp2_set_small(&r1, 3, 0); fp2_set_small(&m1, 4, 0);
    itmac_from_value_mask(&rands[0], &r0, &m0);
    itmac_from_value_mask(&rands[1], &r1, &m1);
    itmac_lift(&lifted, rands, 2);
    /* deg should be 3 (2+1), leading coeff = 0 */
    C(lifted.deg == 3, "deg");
    C(fp2_is_zero(&lifted.coeffs[3]), "leading=0");
    /* coeff[2] = r1 = 3, coeff[1] = r0 + m1 = 5, coeff[0] = m0 = 2 */
    fp2_t e3, e5, e2;
    fp2_set_small(&e3, 3, 0); fp2_set_small(&e5, 5, 0); fp2_set_small(&e2, 2, 0);
    C(fp2_is_equal(&lifted.coeffs[2], &e3), "c2");
    C(fp2_is_equal(&lifted.coeffs[1], &e5), "c1");
    C(fp2_is_equal(&lifted.coeffs[0], &e2), "c0");
    OK();

    /* verifier key operations */
    T("key mul");
    fp2_t Delta, ka, kb, kab;
    fp2_set_small(&Delta, 7, 0);
    fp2_set_small(&ka, 23, 0);  /* A(7) = 10*7+3 = 73... hmm */
    /* Let me recompute: A(x) = 10x + 3, A(7) = 73 */
    fp2_set_small(&ka, 73, 0);
    fp2_set_small(&kb, 42, 0);  /* B(7) = 5*7+7 = 42 */
    itmac_key_mul(&kab, &ka, &kb);
    /* k_ab should equal M(7) where M = A*B = 50x^2+85x+21, M(7)=50*49+85*7+21=2450+595+21=3066 */
    fp2_t e3066; fp2_set_small(&e3066, 3066, 0);
    C(fp2_is_equal(&kab, &e3066), "key mul");
    OK();

    printf("\n  %d/%d passed\n", passed, tests);
    return (passed==tests)?0:1;
}
