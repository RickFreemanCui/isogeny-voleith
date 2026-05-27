/*
 * prg.c -- PRG implementation using hash_xof
 */
#include "prg.h"
#include "hash.h"
#include <string.h>

void prg_gen(uint8_t *out, size_t count,
             const uint8_t seed[PRG_SEED_BYTES],
             uint32_t tweak)
{
    size_t byte_len = count * FP2_BYTES;
    hash_xof(out, byte_len, seed, PRG_SEED_BYTES, &tweak, sizeof(tweak));
}
