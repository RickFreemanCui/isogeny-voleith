/*
 * rs_code.c -- systematic Reed-Solomon encoder over F_{p^2}
 *
 * Evaluation points: β_j = j+1 for j = 0..RS_N-1.
 * Encodes by evaluating the message polynomial m(X) = Σ msg[t]·X^t
 * at each parity evaluation point β_{K+j} using Horner's method.
 */
#include "rs_code.h"
#include <string.h>

void rs_encode(fp2_t out[RS_N], const fp2_t msg[RS_K])
{
    /* systematic: first K positions = message */
    for (int i = 0; i < RS_K; i++)
        fp2_copy(&out[i], &msg[i]);

    /* parity: out[K+j] = m(β_{K+j}) = Σ_{t=0}^{K-1} msg[t] * (K+j+1)^t */
    for (int j = 0; j < RS_D; j++) {
        fp2_t beta, acc;
        fp2_set_small(&beta, RS_K + j + 1, 0);
        fp2_set_zero(&acc);

        /* Horner's method: evaluate from highest degree down */
        for (int t = RS_K - 1; t >= 0; t--) {
            fp2_mul(&acc, &acc, &beta);
            fp2_add(&acc, &acc, &msg[t]);
        }
        fp2_copy(&out[RS_K + j], &acc);
    }
}

void rs_batch_encode(fp2_t *out, const fp2_t *msg, int rows)
{
    for (int i = 0; i < rows; i++)
        rs_encode(&out[i * RS_N], &msg[i * RS_K]);
}
