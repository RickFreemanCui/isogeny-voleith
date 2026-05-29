/*
 * shake.c -- SHAKE128 using XKCP KeccakP-1600 reference implementation
 */
#include "shake.h"
#include <string.h>

/* XKCP reference Keccak API */
extern void KeccakP1600_AddBytes(void *state, const unsigned char *data,
                                  unsigned int offset, unsigned int length);
extern void KeccakP1600_ExtractBytes(const void *state, unsigned char *data,
                                      unsigned int offset, unsigned int length);
extern void KeccakP1600_Permute_24rounds(void *state);
extern void KeccakP1600_Initialize(void *state);

#define RATE 168  /* SHAKE128 rate in bytes */

void shake128_init(shake128_state_t *st) {
    KeccakP1600_Initialize(st->state);
    st->rate = RATE;
    st->byte_idx = 0;
}

void shake128_absorb(shake128_state_t *st, const void *data, size_t len) {
    const uint8_t *p = (const uint8_t *)data;
    while (len > 0) {
        size_t space = st->rate - st->byte_idx;
        size_t chunk = (len < space) ? len : space;
        KeccakP1600_AddBytes(st->state, p, (unsigned int)st->byte_idx, (unsigned int)chunk);
        st->byte_idx += (unsigned int)chunk;
        p += chunk; len -= chunk;
        if (st->byte_idx == st->rate) {
            KeccakP1600_Permute_24rounds(st->state);
            st->byte_idx = 0;
        }
    }
}

void shake128_absorb_byte(shake128_state_t *st, uint8_t byte) {
    shake128_absorb(st, &byte, 1);
}

void shake128_finalise(shake128_state_t *st, uint8_t *out, size_t out_len) {
    /* SHAKE domain separator + padding:
     * Append bits 1111, then pad 10*1 to rate boundary.
     * KeccakP1600_AddBytes handles the byte-to-lane mapping correctly. */
    uint8_t pad = 0x1F;
    if (st->byte_idx == st->rate - 1) {
        /* No room for delimiter + final bit in same block */
        KeccakP1600_AddBytes(st->state, &pad, (unsigned int)st->byte_idx, 1);
        KeccakP1600_Permute_24rounds(st->state);
        st->byte_idx = 0;
    }
    KeccakP1600_AddBytes(st->state, &pad, (unsigned int)st->byte_idx, 1);
    /* pad final bit: 0x80 at last byte of rate block */
    uint8_t final = 0x80;
    KeccakP1600_AddBytes(st->state, &final, RATE - 1, 1);
    KeccakP1600_Permute_24rounds(st->state);

    /* Squeeze */
    st->byte_idx = 0;
    while (out_len > 0) {
        if (st->byte_idx == st->rate) {
            KeccakP1600_Permute_24rounds(st->state);
            st->byte_idx = 0;
        }
        size_t avail = st->rate - st->byte_idx;
        size_t chunk = (out_len < avail) ? out_len : avail;
        KeccakP1600_ExtractBytes(st->state, out, (unsigned int)st->byte_idx, (unsigned int)chunk);
        st->byte_idx += (unsigned int)chunk;
        out += chunk; out_len -= chunk;
    }
}

void shake128_xof(uint8_t *out, size_t out_len,
                  const void *in, size_t in_len) {
    shake128_state_t st;
    shake128_init(&st);
    shake128_absorb(&st, in, in_len);
    shake128_finalise(&st, out, out_len);
}
