/*
 * quicksilver.h -- QuickSilver batched polynomial proof
 */
#ifndef QUICKSILVER_H
#define QUICKSILVER_H

#include "fp2.h"
#include "itmac.h"
#include "params.h"

/* Prover: produces D_QS coefficients (c_0..c_{D_QS-1}, leading c_{D_QS}=z=0 omitted) */
void quicksilver_prove(fp2_t proof_out[D_QS + 1],
                       const fp2_t *W, const fp2_t *Vprime,
                       const itmac_poly_t *r_qs,
                       const fp2_t *chi2,
                       const fp2_t *j0, const fp2_t *jk);

/* Verifier: outputs k_z = Σ χ₂ⁱ·f_i(k_w) + Lift(k_r_qs).
 * Caller recovers the constant term of p_z via:
 *   c_0 = k_z - Σ_{i=1}^{D_QS-1} sent_coeffs[i]·Δ'ⁱ
 * (leading c_{D_QS}=z=0 for valid witness, not needed). */
void quicksilver_verify(fp2_t *k_z_out,
                        const fp2_t *Qprime,
                        const fp2_t *k_r_qs,
                        const fp2_t *chi2,
                        const fp2_t *Delta_prime,
                        const fp2_t *j0, const fp2_t *jk);

/* Exposed for testing */
void qs_phi2_itmac(itmac_poly_t *res, const itmac_poly_t *X, const itmac_poly_t *Y);
void qs_phi2_key(itmac_key_t *k_out, const itmac_key_t *kX, const itmac_key_t *kY, const fp2_t *D);

#endif
