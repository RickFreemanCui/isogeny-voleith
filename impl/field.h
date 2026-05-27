/*
 * field.h -- F_p arithmetic for p = 5 * 2^248 - 1 (251-bit prime)
 *
 * Adapted from reference/field/fp_p5248_64.c.
 * 5 limbs of 51 bits each, Montgomery representation, working modulus 2p.
 */
#ifndef FIELD_H
#define FIELD_H

#include <stdint.h>
#include <stddef.h>

#define FIELD_WORDS   5
#define FIELD_BITS    251
#define FIELD_BYTES   32
#define FIELD_RADIX   51

/* An F_p element: 5 uint64_t limbs in Montgomery form */
typedef uint64_t fp_t[FIELD_WORDS];

/* --- Lifecycle --- */
void fp_set_zero(fp_t a);
void fp_set_one(fp_t a);
void fp_set_small(fp_t a, uint64_t val);
void fp_copy(fp_t dst, const fp_t src);
void fp_random(fp_t a);                    /* uniform random mod p */

/* --- Arithmetic --- */
void fp_add(fp_t r, const fp_t a, const fp_t b);   /* r = a + b  (mod 2p) */
void fp_sub(fp_t r, const fp_t a, const fp_t b);   /* r = a - b  (mod 2p) */
void fp_neg(fp_t r, const fp_t a);                 /* r = -a     (mod 2p) */
void fp_mul(fp_t r, const fp_t a, const fp_t b);   /* r = a * b  (mod 2p) */
void fp_sqr(fp_t r, const fp_t a);                 /* r = a^2    (mod 2p) */
void fp_inv(fp_t r, const fp_t a);                 /* r = a^{-1} (mod 2p) */
void fp_mul_small(fp_t r, const fp_t a, uint32_t val); /* r = a * val */

/* --- Tests --- */
int  fp_is_zero(const fp_t a);             /* a == 0 ? */
int  fp_is_one(const fp_t a);              /* a == 1 ? */
int  fp_is_equal(const fp_t a, const fp_t b);  /* a == b ? */
int  fp_is_square(const fp_t a);           /* quadratic residue? */

/* --- Square root (p ≡ 3 mod 4, so sqrt possible via exponentiation) --- */
void fp_sqrt(fp_t r, const fp_t a);        /* r = √a (assumes a is QR) */

/* --- Constant-time ops --- */
void fp_cswap(fp_t a, fp_t b, uint32_t ctl);  /* swap a,b if ctl & 1 */
void fp_cmov(fp_t r, const fp_t a, uint32_t ctl); /* r = a if ctl & 1 */

/* --- Serialisation --- */
void fp_encode(uint8_t out[FIELD_BYTES], const fp_t a);
int  fp_decode(fp_t a, const uint8_t in[FIELD_BYTES]); /* returns 1 if in range */

/* --- Helpers --- */
void fp_exp3div4(fp_t out, const fp_t a);  /* a^{(p+1)/4}, helper for sqrt */
void fp_double(fp_t r, const fp_t a);       /* r = 2a */
void fp_sub_x2(fp_t r, const fp_t a, const fp_t b); /* r = 2a - b */

#endif /* FIELD_H */
