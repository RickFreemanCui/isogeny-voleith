/*
 * bavc.h -- Batch All-but-one Vector Commitment (GGM tree based)
 *
 * tau_vc = n_C  vectors, each with N = 2^{k_vc} leaves.
 * Each leaf is a PRG seed; the PRG expands to L_HAT_VOLE F_{p^2} elements.
 */
#ifndef BAVC_H
#define BAVC_H

#include "params.h"
#include <stdint.h>

#define BAVC_TAU      N_C
#define BAVC_N        N_LEAVES
#define BAVC_KVC      K_VC

/* A leaf hash: 2 * SECPAR bits = 32 bytes */
#define LEAF_HASH_BYTES 32

/* Opening size: TAU leaf hashes + TAU * k_vc sibling seeds */
#define BAVC_OPEN_SIZE  (BAVC_TAU * (LEAF_HASH_BYTES + BAVC_KVC * SECPAR_BYTES))

/* ── types ─────────────────────────────────────────────────── */

/* GGM forest: for each tree i, store all 2N-1 node seeds (internal + leaves) */
/* We store only internal nodes during commit; leaves are recomputed on open */
typedef struct {
    uint8_t *nodes[BAVC_TAU];    /* level 0..k_vc-1 internal node seeds, each SECPAR_BYTES */
    uint8_t leaf_seeds[BAVC_TAU][BAVC_N][SECPAR_BYTES];
} bavc_forest_t;

/* Hashed leaf commitments */
typedef uint8_t leaf_hashes_t[BAVC_TAU][BAVC_N][LEAF_HASH_BYTES];

/* Commitment: H(h_0 || ... || h_{tau-1}) where h_i = H(com_{i,0}||...||com_{i,N-1}) */
typedef uint8_t bavc_commit_t[LEAF_HASH_BYTES];

/* Opening: for each i, siblings along path to delta_i, plus leaf hashes */
typedef struct {
    uint8_t leaf_hashes[BAVC_TAU][LEAF_HASH_BYTES];  /* one revealed leaf hash per tree */
    uint8_t siblings[BAVC_TAU][BAVC_KVC][SECPAR_BYTES]; /* sibling seeds per level */
} bavc_opening_t;

/* IV: initialization vector / domain separator */
typedef uint8_t bavc_iv_t[SECPAR_BYTES];

/* ── API ───────────────────────────────────────────────────── */

/* Commit: build GGM forest from root seed, output commitment and leaf hashes.
 * The forest and leaf_seeds are stored for later opening. */
void bavc_commit(bavc_commit_t commit,
                 bavc_forest_t *forest,
                 leaf_hashes_t leaf_hashes,
                 const uint8_t root_seed[SECPAR_BYTES],
                 const bavc_iv_t iv);

/* Open: produce an opening that hides leaf delta_i in each tree i */
void bavc_open(bavc_opening_t *opening,
               const bavc_forest_t *forest,
               const leaf_hashes_t leaf_hashes,
               const uint32_t delta[BAVC_TAU]);

/* Reconstruct: from opening, recover the commitment and all non-hidden leaf seeds */
void bavc_reconstruct(bavc_commit_t commit,
                      uint8_t leaf_seeds[BAVC_TAU][BAVC_N][SECPAR_BYTES],
                      const bavc_opening_t *opening,
                      const uint32_t delta[BAVC_TAU],
                      const bavc_iv_t iv);

/* Free forest memory */
void bavc_forest_free(bavc_forest_t *forest);

#endif /* BAVC_H */
