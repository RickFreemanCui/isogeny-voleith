/*
 * aes128.c -- AES-128 via CommonCrypto (hardware-accelerated)
 */
#include "aes128.h"

#ifdef __APPLE__
#include <CommonCrypto/CommonCryptor.h>

void aes128_encrypt_block(uint8_t out[AES128_BLOCK],
                          const uint8_t in[AES128_BLOCK],
                          const uint8_t key[AES128_KEY_BYTES])
{
    size_t moved = 0;
    CCCryptorStatus rv = CCCrypt(kCCEncrypt, kCCAlgorithmAES128, kCCOptionECBMode,
                                  key, kCCKeySizeAES128, NULL,
                                  in, AES128_BLOCK, out, AES128_BLOCK, &moved);
    (void)rv; (void)moved;
}

#else
#error "AES-128 requires CommonCrypto (macOS) or OpenSSL"
#endif
