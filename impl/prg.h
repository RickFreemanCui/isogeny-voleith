/*
 * prg.h -- PRG: seed -> F_{p^2}^{hat_l_vole}
 */
#ifndef PRG_H
#define PRG_H

#include "fp2.h"
#include "params.h"
#include <stdint.h>

/* A PRG seed is SECPAR_BYTES bytes */
#define PRG_SEED_BYTES SECPAR_BYTES

/*
 * Generate `count` F_{p^2} elements from (seed, tweak).
 * Output: out[0..count-1], each element is FP2_BYTES bytes.
 */
void prg_gen(uint8_t *out, size_t count,
             const uint8_t seed[PRG_SEED_BYTES],
             uint32_t tweak);

#endif /* PRG_H */
