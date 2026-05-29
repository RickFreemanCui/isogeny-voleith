/*
 * aes128.h -- AES-128-CTR for BAVC GGM tree expansion
 *
 * Uses CommonCrypto on macOS (hardware-accelerated via ARM CE / AES-NI).
 */
#ifndef AES128_H
#define AES128_H

#include <stdint.h>
#include <stddef.h>

#define AES128_KEY_BYTES  16
#define AES128_BLOCK      16

/* Encrypt a single 128-bit block with AES-128 (ECB).
 * Hardware-accelerated via CommonCrypto (ARM CE / AES-NI). */
void aes128_encrypt_block(uint8_t out[AES128_BLOCK],
                          const uint8_t in[AES128_BLOCK],
                          const uint8_t key[AES128_KEY_BYTES]);

#endif /* AES128_H */
