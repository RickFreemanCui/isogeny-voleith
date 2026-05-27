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

/* Verifier: checks p_z(Δ') == k_z */
int quicksilver_verify(const fp2_t proof_in[D_QS],
                       const fp2_t *Qprime,
                       const fp2_t *k_r_qs,
                       const fp2_t *chi2,
                       const fp2_t *Delta_prime,
                       const fp2_t *j0, const fp2_t *jk);

/* Exposed for testing */
void qs_phi2_itmac(itmac_poly_t *res, const itmac_poly_t *X, const itmac_poly_t *Y);
void qs_phi2_key(itmac_key_t *k_out, const itmac_key_t *kX, const itmac_key_t *kY, const fp2_t *D);

#endif
