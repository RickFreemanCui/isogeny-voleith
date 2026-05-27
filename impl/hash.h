/*
 * hash.h -- cryptographic hashing for VOLEitH
 *
 * Uses SHA-256 (CommonCrypto on macOS).
 * Extensible output via counter mode.
 */
#ifndef HASH_H
#define HASH_H

#include <stdint.h>
#include <stddef.h>

#ifdef __APPLE__
#include <CommonCrypto/CommonDigest.h>
typedef CC_SHA256_CTX hash_state_t;
#else
#error "Only macOS (CommonCrypto) supported for now"
#endif

#define HASH_BYTES      CC_SHA256_DIGEST_LENGTH  /* 32 */
#define SECPAR_BYTES    16

void hash_init(hash_state_t *st);
void hash_update(hash_state_t *st, const void *data, size_t len);
void hash_final(hash_state_t *st, uint8_t out[HASH_BYTES]);
void hash_digest(uint8_t out[HASH_BYTES], const void *data, size_t len);

/* Extensible output: SHA-256 in counter mode */
void hash_xof(uint8_t *out, size_t out_len,
              const void *seed, size_t seed_len,
              const void *tweak, size_t tweak_len);

#endif /* HASH_H */
