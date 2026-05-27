/*
 * itmac.h -- Information-Theoretic MAC over F_{p^2}
 *
 * Prover holds polynomial p_a(x) = a·x^d + ... + c_0  (leading coeff = a)
 * Verifier holds evaluation k_a = p_a(Δ) at global key Δ
 *
 * Operations are homomorphic: add, mul on polynomials/keys.
 */
#ifndef ITMAC_H
#define ITMAC_H

#include "fp2.h"

/* Max degree we support for IT-MAC polynomials */
#define ITMAC_MAX_DEG 16

/* Prover's IT-MAC: polynomial coefficients c_0 .. c_{d-1}, a */
typedef struct {
    int     deg;                          /* polynomial degree */
    fp2_t   coeffs[ITMAC_MAX_DEG + 1];    /* coeffs[0] = const, coeffs[deg] = leading (= a) */
} itmac_poly_t;

/* Verifier's IT-MAC: just the evaluation k */
typedef fp2_t itmac_key_t;

/* ── prover operations ─────────────────────────────────────── */

/* Create degree-1 IT-MAC from (value, mask). p(x) = value*x + mask */
void itmac_from_value_mask(itmac_poly_t *out, const fp2_t *value, const fp2_t *mask);

/* Multiply two IT-MACs: deg = da + db */
void itmac_mul(itmac_poly_t *out,
               const itmac_poly_t *a,
               const itmac_poly_t *b);

/* Add two IT-MACs: deg = max(da, db) */
void itmac_add(itmac_poly_t *out,
               const itmac_poly_t *a,
               const itmac_poly_t *b);

/* Constant IT-MAC of degree d: p(x) = c * x^d */
void itmac_const(itmac_poly_t *out, const fp2_t *c, int deg);

/* Lift d-1 random degree-1 IT-MACs into degree-d authentication of 0.
 * rands[i] = (r_i, mask_i) with r_i random.
 * Produces: out(x) = Σ_{i=0}^{d-2} (r_i·x + mask_i)·x^i (degree d)
 * whose leading coefficient is 0. */
void itmac_lift(itmac_poly_t *out,
                const itmac_poly_t *rands,
                int num_rands);

/* Evaluate polynomial at point x */
void itmac_eval(fp2_t *result, const itmac_poly_t *p, const fp2_t *x);

/* ── verifier operations (operating on keys) ────────────────── */

/* Derandomize: k_x = k_r + δ·Δ  (for publishing δ = x - r) */
void itmac_derandomize(itmac_key_t *k_x,
                       const itmac_key_t *k_r,
                       const fp2_t *delta,
                       const fp2_t *Delta);

/* Verifier multiply: k_ab = k_a · k_b */
void itmac_key_mul(itmac_key_t *k_ab,
                   const itmac_key_t *k_a,
                   const itmac_key_t *k_b);

/* Verifier add: k_{a+b} = k_a + k_b·Δ^{da-db} */
void itmac_key_add(itmac_key_t *k_sum,
                   const itmac_key_t *k_a, int da,
                   const itmac_key_t *k_b, int db,
                   const fp2_t *Delta);

/* Verifier constant: k_c = c·Δ^d */
void itmac_key_const(itmac_key_t *k_c,
                     const fp2_t *c, int deg,
                     const fp2_t *Delta);

/* Verifier lift: k_0 = Σ k_{r_i}·Δ^i  (same formula, uses Δ) */
void itmac_key_lift(itmac_key_t *k_out,
                    const itmac_key_t *k_rands, int num_rands,
                    const fp2_t *Delta);

#endif /* ITMAC_H */
