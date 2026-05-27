/*
 * fp2.h -- F_{p^2} = F_p[i], i^2 = -1 (p ≡ 3 mod 4)
 */
#ifndef FP2_H
#define FP2_H

#include "field.h"

/* An F_{p^2} element: (re, im) = re + im * i */
typedef struct { fp_t re, im; } fp2_t;

void fp2_set_zero(fp2_t *a);
void fp2_set_one(fp2_t *a);
void fp2_set_small(fp2_t *a, uint64_t re, uint64_t im);
void fp2_set_fp(fp2_t *a, const fp_t re);   /* im = 0 */
void fp2_copy(fp2_t *dst, const fp2_t *src);
void fp2_random(fp2_t *a);

void fp2_add(fp2_t *r, const fp2_t *a, const fp2_t *b);
void fp2_sub(fp2_t *r, const fp2_t *a, const fp2_t *b);
void fp2_neg(fp2_t *r, const fp2_t *a);
void fp2_mul(fp2_t *r, const fp2_t *a, const fp2_t *b);
void fp2_sqr(fp2_t *r, const fp2_t *a);
void fp2_inv(fp2_t *r, const fp2_t *a);
void fp2_mul_fp(fp2_t *r, const fp2_t *a, const fp_t b);  /* scalar mul */

int  fp2_is_zero(const fp2_t *a);
int  fp2_is_one(const fp2_t *a);
int  fp2_is_equal(const fp2_t *a, const fp2_t *b);

void fp2_cswap(fp2_t *a, fp2_t *b, uint32_t ctl);

/* Serialisation: 2 * FIELD_BYTES bytes (re || im, big-endian each) */
#define FP2_BYTES (2 * FIELD_BYTES)
void fp2_encode(uint8_t out[FP2_BYTES], const fp2_t *a);
int  fp2_decode(fp2_t *a, const uint8_t in[FP2_BYTES]);

#endif /* FP2_H */
