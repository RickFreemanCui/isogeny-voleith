/*
 * itmac.c -- IT-MAC implementation over F_{p^2}
 */
#include "itmac.h"
#include <string.h>

/* ── prover ────────────────────────────────────────────────── */

void itmac_from_value_mask(itmac_poly_t *out, const fp2_t *value, const fp2_t *mask)
{
    out->deg = 1;
    fp2_copy(&out->coeffs[0], mask);      /* constant term */
    fp2_copy(&out->coeffs[1], value);     /* leading = value */
}

void itmac_mul(itmac_poly_t *out, const itmac_poly_t *a, const itmac_poly_t *b)
{
    int da = a->deg, db = b->deg;
    out->deg = da + db;
    memset(out->coeffs, 0, (da + db + 1) * sizeof(fp2_t));
    for (int i = 0; i <= da; i++)
        for (int j = 0; j <= db; j++) {
            fp2_t prod;
            fp2_mul(&prod, &a->coeffs[i], &b->coeffs[j]);
            fp2_add(&out->coeffs[i + j], &out->coeffs[i + j], &prod);
        }
}

void itmac_add(itmac_poly_t *out, const itmac_poly_t *a, const itmac_poly_t *b)
{
    /* p_{a+b}^{(maxdeg)} = p_a * x^{maxdeg-da} + p_b * x^{maxdeg-db} */
    int maxdeg = (a->deg > b->deg) ? a->deg : b->deg;
    out->deg = maxdeg;
    memset(out->coeffs, 0, (maxdeg + 1) * sizeof(fp2_t));
    int shift_a = maxdeg - a->deg;
    int shift_b = maxdeg - b->deg;
    for (int i = 0; i <= a->deg; i++)
        fp2_copy(&out->coeffs[i + shift_a], &a->coeffs[i]);
    for (int i = 0; i <= b->deg; i++)
        fp2_add(&out->coeffs[i + shift_b], &out->coeffs[i + shift_b], &b->coeffs[i]);
}

void itmac_const(itmac_poly_t *out, const fp2_t *c, int deg)
{
    out->deg = deg;
    memset(out->coeffs, 0, (deg + 1) * sizeof(fp2_t));
    fp2_copy(&out->coeffs[deg], c); /* leading coefficient = c */
}

void itmac_lift(itmac_poly_t *out, const itmac_poly_t *rands, int num_rands)
{
    /* Combine num_rands degree-1 IT-MACs into degree-(num_rands+1) auth of 0.
     * rands[i] has coeffs: [m_i, r_i] (constant, leading).
     * out(x) = Σ_{i=0}^{n-1} (r_i·x + m_i) · x^i
     *         = r_{n-1}·x^n + (r_{n-2}+m_{n-1})·x^{n-1} + ... + (r_0+m_1)·x + m_0
     * Interpret as degree n+1 with leading coeff 0.
     */
    int n = num_rands;
    int out_deg = n + 1;
    out->deg = out_deg;
    memset(out->coeffs, 0, (out_deg + 1) * sizeof(fp2_t));

    /* coeff[k] for k=0..n: sum of r_i·x^{i+1} + m_i·x^i contributions */
    for (int i = 0; i < n; i++) {
        /* rands[i].coeffs[0] = m_i (constant), rands[i].coeffs[1] = r_i */
        /* contributes m_i to x^i term */
        if (i <= out_deg)
            fp2_add(&out->coeffs[i], &out->coeffs[i], &rands[i].coeffs[0]);
        /* contributes r_i to x^{i+1} term */
        if (i + 1 <= out_deg)
            fp2_add(&out->coeffs[i + 1], &out->coeffs[i + 1], &rands[i].coeffs[1]);
    }
    /* leading coefficient out->coeffs[out_deg] stays 0 */
}

void itmac_eval(fp2_t *result, const itmac_poly_t *p, const fp2_t *x)
{
    /* Horner: result = Σ coeffs[i] * x^i */
    fp2_set_zero(result);
    for (int i = p->deg; i >= 0; i--) {
        fp2_mul(result, result, x);
        fp2_add(result, result, &p->coeffs[i]);
    }
}

/* ── verifier keys ─────────────────────────────────────────── */

void itmac_derandomize(itmac_key_t *k_x, const itmac_key_t *k_r,
                       const fp2_t *delta, const fp2_t *Delta)
{
    /* k_x = k_r + δ·Δ  (IT-MAC of x = r + δ, where degree=1) */
    fp2_t term;
    fp2_mul(&term, delta, Delta);
    fp2_add(k_x, k_r, &term);
}

void itmac_key_mul(itmac_key_t *k_ab, const itmac_key_t *k_a, const itmac_key_t *k_b)
{
    fp2_mul(k_ab, k_a, k_b);
}

void itmac_key_add(itmac_key_t *k_sum,
                   const itmac_key_t *k_a, int da,
                   const itmac_key_t *k_b, int db,
                   const fp2_t *Delta)
{
    if (da >= db) {
        /* k_{a+b} = k_a + k_b·Δ^{da-db} */
        fp2_t Delta_pow, term;
        fp2_set_one(&Delta_pow);
        for (int i = 0; i < da - db; i++)
            fp2_mul(&Delta_pow, &Delta_pow, Delta);
        fp2_mul(&term, k_b, &Delta_pow);
        fp2_add(k_sum, k_a, &term);
    } else {
        fp2_t Delta_pow, term;
        fp2_set_one(&Delta_pow);
        for (int i = 0; i < db - da; i++)
            fp2_mul(&Delta_pow, &Delta_pow, Delta);
        fp2_mul(&term, k_a, &Delta_pow);
        fp2_add(k_sum, &term, k_b);
    }
}

void itmac_key_const(itmac_key_t *k_c, const fp2_t *c, int deg, const fp2_t *Delta)
{
    /* k_c = c·Δ^deg */
    fp2_t Delta_pow;
    fp2_set_one(&Delta_pow);
    for (int i = 0; i < deg; i++)
        fp2_mul(&Delta_pow, &Delta_pow, Delta);
    fp2_mul(k_c, c, &Delta_pow);
}

void itmac_key_lift(itmac_key_t *k_out, const itmac_key_t *k_rands,
                    int num_rands, const fp2_t *Delta)
{
    /* Same as prover lift but with k values:
     * k_out = Σ_{i=0}^{n-1} k_{r_i} · Δ^i
     */
    fp2_set_zero(k_out);
    fp2_t Delta_pow;
    fp2_set_one(&Delta_pow);
    for (int i = 0; i < num_rands; i++) {
        fp2_t term;
        fp2_mul(&term, &k_rands[i], &Delta_pow);
        fp2_add(k_out, k_out, &term);
        fp2_mul(&Delta_pow, &Delta_pow, Delta);
    }
}
