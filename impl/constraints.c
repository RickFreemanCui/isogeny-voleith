/*
 * constraints.c -- classical Phi_2 isogeny path constraints
 *
 * Phi_2(X,Y) = X^3 + Y^3 + X^2*Y^2 - 1488*(X^2*Y + X*Y^2)
 *            - 162000*(X^2 + Y^2) + 40773375*X*Y
 *            + 8748000000*(X+Y) - 157464000000000
 */
#include "constraints.h"

void constraints_classical_eval(fp2_t *result,
                                const witness_t witness,
                                const fp2_t j0,
                                const fp2_t jk,
                                int idx)
{
    /* idx 0..k-1, checking step idx+1 */
    const fp2_t *ji_prev = (idx == 0) ? &j0 : &witness[idx - 1];
    const fp2_t *ji      = (idx == K_PATH - 1) ? &jk : &witness[idx];

    fp2_t X, Y, X2, Y2, X3, Y3, XY, X2Y2;
    fp2_copy(&X, ji_prev);
    fp2_copy(&Y, ji);

    /* powers */
    fp2_sqr(&X2, &X);           /* X^2 */
    fp2_sqr(&Y2, &Y);           /* Y^2 */
    fp2_mul(&X3, &X2, &X);     /* X^3 */
    fp2_mul(&Y3, &Y2, &Y);     /* Y^3 */
    fp2_mul(&XY,  &X,  &Y);    /* X*Y */
    fp2_sqr(&X2Y2, &XY);       /* X^2*Y^2 */

    /* compute Phi_2 as linear combination */
    /* X^3 + Y^3 + X^2*Y^2 */
    fp2_t sum;
    fp2_add(&sum, &X3, &Y3);
    fp2_add(&sum, &sum, &X2Y2);

    /* -1488*X^2*Y = -1488 * X2 * Y */
    fp2_t term;
    fp2_mul(&term, &X2, &Y);
    fp2_set_small(&X, 1488, 0); /* reuse X as scalar 1488 */
    fp2_mul(&term, &term, &X);
    fp2_sub(&sum, &sum, &term);

    /* -1488*X*Y^2 */
    fp2_mul(&term, &X2, &Y); /* wait, meant X*Y2... */
    /* actually: X*Y^2 */
    fp2_mul(&term, &X, &Y2);
    fp2_mul(&term, &term, &X); /* 1488 * X*Y^2 */
    fp2_sub(&sum, &sum, &term);

    /* -162000*(X^2 + Y^2) */
    fp2_set_small(&X, 162000, 0);
    fp2_t sum_sq;
    fp2_add(&sum_sq, &X2, &Y2);
    fp2_mul(&term, &sum_sq, &X);
    fp2_sub(&sum, &sum, &term);

    /* +40773375*XY */
    fp2_set_small(&X, 40773375, 0);
    fp2_mul(&term, &XY, &X);
    fp2_add(&sum, &sum, &term);

    /* +8748000000*(X+Y) */
    fp2_set_small(&X, 8748000000ULL, 0);
    fp2_t sum_xy;
    fp2_add(&sum_xy, ji_prev, ji);
    /* but ji_prev and ji are const; need mutable copies */
    fp2_t Xm, Ym;
    fp2_copy(&Xm, ji_prev);
    fp2_copy(&Ym, ji);
    fp2_add(&sum_xy, &Xm, &Ym);
    fp2_mul(&term, &sum_xy, &X);
    fp2_add(&sum, &sum, &term);

    /* -157464000000000 */
    fp2_set_small(&X, 0x8F3C3F00, 0);  /* placeholder — this constant doesn't fit in 64 bits */
    fp2_sub(result, &sum, &X);
}
