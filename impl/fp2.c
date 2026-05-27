/*
 * fp2.c -- F_{p^2} = F_p[i], i^2 = -1 (p = 5*2^248 - 1 ≡ 3 mod 4)
 */
#include "fp2.h"
#include <string.h>
#include <stdio.h>

void fp2_set_zero(fp2_t *a) {
    fp_set_zero(a->re); fp_set_zero(a->im);
}

void fp2_set_one(fp2_t *a) {
    fp_set_one(a->re); fp_set_zero(a->im);
}

void fp2_set_small(fp2_t *a, uint64_t re, uint64_t im) {
    fp_set_small(a->re, re); fp_set_small(a->im, im);
}

void fp2_set_fp(fp2_t *a, const fp_t re) {
    fp_copy(a->re, re); fp_set_zero(a->im);
}

void fp2_copy(fp2_t *dst, const fp2_t *src) {
    fp_copy(dst->re, src->re); fp_copy(dst->im, src->im);
}

void fp2_random(fp2_t *a) {
    fp_random(a->re); fp_random(a->im);
}

/* ── arithmetic ───────────────────────────────────────────────── */

void fp2_add(fp2_t *r, const fp2_t *a, const fp2_t *b) {
    fp_add(r->re, a->re, b->re);
    fp_add(r->im, a->im, b->im);
}

void fp2_sub(fp2_t *r, const fp2_t *a, const fp2_t *b) {
    fp_sub(r->re, a->re, b->re);
    fp_sub(r->im, a->im, b->im);
}

void fp2_neg(fp2_t *r, const fp2_t *a) {
    fp_neg(r->re, a->re);
    fp_neg(r->im, a->im);
}

void fp2_mul(fp2_t *r, const fp2_t *a, const fp2_t *b) {
    /* (a+bi)(c+di) = (ac - bd) + (ad + bc)i
     * Karatsuba: ac, bd, (a+b)(c+d) */
    fp_t ac, bd, sum;
    fp_mul(ac, a->re, b->re);
    fp_mul(bd, a->im, b->im);

    fp_t a_sum, b_sum, cross;
    fp_add(a_sum, a->re, a->im);
    fp_add(b_sum, b->re, b->im);
    fp_mul(cross, a_sum, b_sum);

    /* re = ac - bd, im = cross - ac - bd */
    fp_sub(r->re, ac, bd);
    fp_sub(sum, cross, ac);
    fp_sub(r->im, sum, bd);
}

void fp2_sqr(fp2_t *r, const fp2_t *a) {
    /* (a+bi)^2 = (a^2 - b^2) + 2ab·i */
    fp_t a2, b2, two_ab;
    fp_sqr(a2, a->re);
    fp_sqr(b2, a->im);
    fp_sub(r->re, a2, b2);
    fp_mul(two_ab, a->re, a->im);
    fp_double(r->im, two_ab);
}

void fp2_inv(fp2_t *r, const fp2_t *a) {
    /* (a+bi)^{-1} = (a - bi) / (a^2 + b^2) */
    fp_t a2, b2, norm, inv_norm;
    fp_sqr(a2, a->re);
    fp_sqr(b2, a->im);
    fp_add(norm, a2, b2);
    fp_inv(inv_norm, norm);
    fp_mul(r->re, a->re, inv_norm);
    fp_neg(r->im, a->im);
    fp_mul(r->im, r->im, inv_norm);
}

void fp2_mul_fp(fp2_t *r, const fp2_t *a, const fp_t b) {
    fp_mul(r->re, a->re, b);
    fp_mul(r->im, a->im, b);
}

/* ── tests ────────────────────────────────────────────────────── */

int fp2_is_zero(const fp2_t *a) {
    return fp_is_zero(a->re) && fp_is_zero(a->im);
}

int fp2_is_one(const fp2_t *a) {
    return fp_is_one(a->re) && fp_is_zero(a->im);
}

int fp2_is_equal(const fp2_t *a, const fp2_t *b) {
    return fp_is_equal(a->re, b->re) && fp_is_equal(a->im, b->im);
}

/* ── constant-time ────────────────────────────────────────────── */

void fp2_cswap(fp2_t *a, fp2_t *b, uint32_t ctl) {
    fp_cswap(a->re, b->re, ctl);
    fp_cswap(a->im, b->im, ctl);
}

/* ── serialisation ────────────────────────────────────────────── */

void fp2_encode(uint8_t out[FP2_BYTES], const fp2_t *a) {
    fp_encode(out, a->re);
    fp_encode(out + FIELD_BYTES, a->im);
}

int fp2_decode(fp2_t *a, const uint8_t in[FP2_BYTES]) {
    int ok = fp_decode(a->re, in);
    ok &= fp_decode(a->im, in + FIELD_BYTES);
    return ok;
}
