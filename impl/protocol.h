/*
 * protocol.h -- 9-round VOLEitH NIZK protocol orchestration
 */
#ifndef PROTOCOL_H
#define PROTOCOL_H

#include "fp2.h"
#include "bavc.h"
#include "params.h"
#include "constraints.h"

/* Proof layout offsets */
#define C_SIZE         (L_HAT_VOLE * (N_C - K_C))
/* Proof size: FE terms + bitstring overhead */
#define PROOF_FE_SIZE  (C_SIZE + L_WIT + K_C + D_QS + L_VOLE*K_C)
/* bitstring: I (N_C * 4 bytes) + opening + iv */
#define PROOF_BS_SIZE  (BAVC_TAU * sizeof(uint32_t) + sizeof(bavc_opening_t) + sizeof(bavc_iv_t) + sizeof(bavc_commit_t))
#define PROOF_BYTES    (PROOF_FE_SIZE * FP2_BYTES + PROOF_BS_SIZE + 4096)

/* Generate a valid isogeny path witness (j_1..j_{k-1}) from j_0.
 * Uses a trivial "identity path" for testing: j_i = j_0 for all i. */
void witness_generate_trivial(witness_t witness, const fp2_t j0);

/* Prover: given witness and public j_0, j_k, produce a NIZK proof.
 * Returns proof length in bytes. */
int prove(uint8_t *proof, size_t *proof_len,
          const witness_t witness,
          const fp2_t j0, const fp2_t jk);

/* Verifier: check a NIZK proof. Returns 1 if valid, 0 if invalid. */
int verify(const uint8_t *proof, size_t proof_len,
           const fp2_t j0, const fp2_t jk);

#endif
