/*
 * hash.c -- SHA-256 based hashing with extensible output
 *
 * On macOS, uses CommonCrypto. On other platforms, uses a built-in SHA-256.
 */
#include "hash.h"
#include <string.h>

#ifdef __APPLE__

void hash_init(hash_state_t *st)    { CC_SHA256_Init(st); }
void hash_update(hash_state_t *st, const void *data, size_t len) { CC_SHA256_Update(st, data, (CC_LONG)len); }
void hash_final(hash_state_t *st, uint8_t out[HASH_BYTES]) { CC_SHA256_Final(out, st); }
void hash_digest(uint8_t out[HASH_BYTES], const void *data, size_t len) { CC_SHA256(data, (CC_LONG)len, out); }

#else
#error "Non-macOS hash not yet implemented"
#endif

/* ── extensible output (counter mode) ─────────────────────────── */

void hash_xof(uint8_t *out, size_t out_len,
              const void *seed, size_t seed_len,
              const void *tweak, size_t tweak_len)
{
    uint8_t buf[HASH_BYTES + 16]; /* space for seed + tweak + counter */
    hash_state_t st;
    size_t offset = 0;
    uint32_t ctr = 0;

    while (offset < out_len) {
        hash_init(&st);
        hash_update(&st, seed, seed_len);
        if (tweak && tweak_len > 0)
            hash_update(&st, tweak, tweak_len);
        hash_update(&st, &ctr, sizeof(ctr));
        hash_final(&st, buf);

        size_t chunk = HASH_BYTES;
        if (offset + chunk > out_len) chunk = out_len - offset;
        memcpy(out + offset, buf, chunk);
        offset += chunk;
        ctr++;
    }
}
