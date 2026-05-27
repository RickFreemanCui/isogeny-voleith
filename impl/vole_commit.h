/*
 * vole_commit.h -- Subspace VOLE sender and receiver
 */
#ifndef VOLE_COMMIT_H
#define VOLE_COMMIT_H

#include "fp2.h"
#include "bavc.h"
#include "params.h"

/* Generate alpha values: α_j = (j+1, 0) for j = 0..N-1 */
void vole_gen_alphas(fp2_t alpha[N_LEAVES]);

/* Prover side: generate VOLE correlations from BAVC leaves. */
void vole_sender(fp2_t *U_out,        /* L_HAT_VOLE × K_C */
                 fp2_t *V_out,        /* L_HAT_VOLE × N_C */
                 fp2_t *C_out,        /* L_HAT_VOLE × (N_C - K_C) */
                 const uint8_t leaf_seeds[BAVC_TAU][BAVC_N][SECPAR_BYTES],
                 const fp2_t alpha[N_LEAVES],  /* soft-spoken OT points */
                 const bavc_iv_t iv);

/* Verifier side: from BAVC opening, reconstruct Q matrix */
void vole_receiver(fp2_t *Q_out,      /* L_HAT_VOLE × N_C */
                   const bavc_opening_t *opening,
                   const uint32_t delta[BAVC_TAU],
                   const fp2_t *C,    /* from prover: L_HAT_VOLE × (N_C - K_C) */
                   const fp2_t alpha[N_LEAVES],
                   const bavc_iv_t iv);

#endif
