/*
 * shake.h -- SHAKE128 wrapper using XKCP Keccak
 */
#ifndef SHAKE_H
#define SHAKE_H

#include <stdint.h>
#include <stddef.h>

#define SHAKE128_RATE   168    /* bytes */
#define SHAKE128_BYTES  32     /* default output for challenges */

/* Absorbing state for Fiat-Shamir transcript hashing */
typedef struct {
    uint64_t state[25];
    unsigned int rate;
    unsigned int byte_idx;
    uint8_t  partial;
} shake128_state_t;

void shake128_init(shake128_state_t *st);
void shake128_absorb(shake128_state_t *st, const void *data, size_t len);
void shake128_absorb_byte(shake128_state_t *st, uint8_t byte);
void shake128_finalise(shake128_state_t *st, uint8_t *out, size_t out_len);

/* Convenience: one-shot XOF */
void shake128_xof(uint8_t *out, size_t out_len,
                  const void *in, size_t in_len);

#endif /* SHAKE_H */
