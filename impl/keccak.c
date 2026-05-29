/*
 * keccak.c -- Keccak-f[1600] permutation
 *
 * Adapted from XKCP ref-64bits (public domain / CC0).
 * https://github.com/XKCP/XKCP
 */
#include <stdint.h>
#include <string.h>

#define NROUNDS 24
#define ROL64(a, n) (((a) << (n)) | ((a) >> (64 - (n))))

static const uint64_t KeccakF1600RoundConstants[NROUNDS] = {
    0x0000000000000001ULL, 0x0000000000008082ULL, 0x800000000000808aULL,
    0x8000000080008000ULL, 0x000000000000808bULL, 0x0000000080000001ULL,
    0x8000000080008081ULL, 0x8000000000008009ULL, 0x000000000000008aULL,
    0x0000000000000088ULL, 0x0000000080008009ULL, 0x000000008000000aULL,
    0x000000008000808bULL, 0x800000000000008bULL, 0x8000000000008089ULL,
    0x8000000000008003ULL, 0x8000000000008002ULL, 0x8000000000000080ULL,
    0x000000000000800aULL, 0x800000008000000aULL, 0x8000000080008081ULL,
    0x8000000000008080ULL, 0x0000000080000001ULL, 0x8000000080008008ULL,
};

/* ρ offsets for lanes (x,y) in π-step traversal order:
 * (0,0), (1,0), (0,2), (2,1), (1,2), (2,3), (3,3), (3,0), (0,1), (1,1),
 * (1,4), (4,4), (4,0), (0,3), (3,4), (4,2), (2,0), (0,4), (4,3), (3,2),
 * (2,4), (4,1), (1,3), (3,1), (1,0) [back to start, not stored] */
static const unsigned int KeccakF1600RhoOffsets[25] = {
     0,  1,  3, 43, 45,  8, 55, 28, 62, 44,
    61,  2, 14, 27, 41, 56, 25,  6, 39, 21,
    18, 20, 15, 36,  1  /* [24] is a repeat of [1] for code simplicity */
};

void KeccakP1600_Permute_24rounds(void *state) {
    uint64_t *A = (uint64_t *)state;
    uint64_t C[5], D[5], B[25];

    for (unsigned int r = 0; r < NROUNDS; r++) {
        /* θ */
        for (unsigned int x = 0; x < 5; x++)
            C[x] = A[x] ^ A[5 + x] ^ A[10 + x] ^ A[15 + x] ^ A[20 + x];
        for (unsigned int x = 0; x < 5; x++) {
            D[x] = C[(x + 4) % 5] ^ ROL64(C[(x + 1) % 5], 1);
            for (unsigned int y = 0; y < 5; y++)
                A[y * 5 + x] ^= D[x];
        }

        /* ρ + π */
        uint64_t cur = A[1];
        unsigned int x = 1, y = 0;
        for (unsigned int t = 0; t < 24; t++) {
            unsigned int nx = y;
            unsigned int ny = (2 * x + 3 * y) % 5;
            unsigned int idx = ny * 5 + nx;
            uint64_t tmp = A[idx];
            A[idx] = ROL64(cur, KeccakF1600RhoOffsets[t + 1]);
            cur = tmp;
            x = nx; y = ny;
        }

        /* χ */
        for (unsigned int y = 0; y < 5; y++) {
            for (unsigned int x = 0; x < 5; x++)
                B[x] = A[y * 5 + x];
            for (unsigned int x = 0; x < 5; x++)
                A[y * 5 + x] = B[x] ^ ((~B[(x + 1) % 5]) & B[(x + 2) % 5]);
        }

        /* ι */
        A[0] ^= KeccakF1600RoundConstants[r];
    }
}
