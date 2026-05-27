/*
 * rs_code.h -- systematic Reed-Solomon encoder over F_{p^2}
 */
#ifndef RS_CODE_H
#define RS_CODE_H

#include "fp2.h"
#include "params.h"

/* RS code parameters */
#define RS_N  N_C
#define RS_K  K_C
#define RS_D  (N_C - K_C)  /* number of parity symbols = n - k */

/* Encode message msg[RS_K] -> codeword out[RS_N] (systematic) */
void rs_encode(fp2_t out[RS_N], const fp2_t msg[RS_K]);

/* Batch encode: rows × RS_K messages -> rows × RS_N codewords.
 * out[i][j] for i=0..rows-1, j=0..RS_N-1 */
void rs_batch_encode(fp2_t *out, const fp2_t *msg, int rows);

#endif /* RS_CODE_H */
