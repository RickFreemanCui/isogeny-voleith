/*
 * bavc.c -- GGM-tree BAVC
 *
 * Tree storage: heap layout (root at 0, children of i at 2i+1, 2i+2).
 * Leaves are nodes at indices N-1 .. 2N-2 (level k_vc).
 * Internal nodes are indices 0 .. N-2.
 */
#include "bavc.h"
#include "aes128.h"
#include "hash.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* ── GGM helpers (AES-128-CTR) ──────────────────────────────── */

/* AES-128-CTR block encryption with domain separation */
static void aes_ctr_block(uint8_t out[AES128_BLOCK],
                          const uint8_t key[AES128_KEY_BYTES],
                          const uint8_t iv[AES128_KEY_BYTES],
                          uint32_t domain, uint8_t ctr)
{
    uint8_t buf[AES128_BLOCK];
    for (int i = 0; i < AES128_BLOCK; i++) buf[i] = iv[i];
    /* mix domain into first bytes */
    buf[0] ^= (uint8_t)(domain);
    buf[1] ^= (uint8_t)(domain >> 8);
    buf[2] ^= (uint8_t)(domain >> 16);
    buf[3] ^= (uint8_t)(domain >> 24);
    buf[12] ^= ctr;
    aes128_encrypt_block(out, buf, key);
}

/* GGM expansion: parent seed -> two child seeds */
static void expand_node(uint8_t left[SECPAR_BYTES], uint8_t right[SECPAR_BYTES],
                        const uint8_t seed[SECPAR_BYTES],
                        const uint8_t iv[SECPAR_BYTES],
                        uint32_t tree_idx, uint32_t node_idx)
{
    uint32_t domain = (tree_idx << 20) | node_idx;
    aes_ctr_block(left,  seed, iv, domain, 0);
    aes_ctr_block(right, seed, iv, domain, 1);
}

/* Leaf commitment: 32 bytes via AES-128-CTR(leaf_seed, iv, ctr=0,1) */
static void leaf_hash_fn(uint8_t out[LEAF_HASH_BYTES],
                         const uint8_t seed[SECPAR_BYTES],
                         const uint8_t iv[SECPAR_BYTES])
{
    aes_ctr_block(out,                    seed, iv, 0xFFFFFFFF, 0);
    aes_ctr_block(out + AES128_BLOCK,     seed, iv, 0xFFFFFFFF, 1);
}

static void tree_root_seed(uint8_t root[SECPAR_BYTES],
                           const uint8_t master_seed[SECPAR_BYTES],
                           const uint8_t iv[SECPAR_BYTES],
                           uint32_t tree_idx)
{
    hash_state_t st;
    hash_init(&st);
    hash_update(&st, master_seed, SECPAR_BYTES);
    hash_update(&st, &tree_idx, sizeof(tree_idx));
    hash_update(&st, iv, SECPAR_BYTES);
    hash_final(&st, root);
}

/* Expand subtree rooted at node_idx (at level L) down to leaves.
 * Writes leaf seeds for all leaves in this subtree. */
static void expand_subtree(
    uint8_t leaf_seeds[BAVC_N][SECPAR_BYTES],
    const uint8_t seed[SECPAR_BYTES],
    const uint8_t iv[SECPAR_BYTES],
    uint32_t tree_idx, uint32_t node_idx)
{
    /* is node_idx a leaf? Leaves are N-1 .. 2N-2 */
    if (node_idx >= BAVC_N - 1) {
        memcpy(leaf_seeds[node_idx - (BAVC_N - 1)], seed, SECPAR_BYTES);
        return;
    }
    uint8_t left[SECPAR_BYTES], right[SECPAR_BYTES];
    expand_node(left, right, seed, iv, tree_idx, node_idx);
    expand_subtree(leaf_seeds, left,  iv, tree_idx, 2 * node_idx + 1);
    expand_subtree(leaf_seeds, right, iv, tree_idx, 2 * node_idx + 2);
}

/* ── commit ─────────────────────────────────────────────────── */

void bavc_commit(bavc_commit_t commit,
                 bavc_forest_t *forest,
                 leaf_hashes_t leaf_hashes,
                 const uint8_t root_seed[SECPAR_BYTES],
                 const bavc_iv_t iv)
{
    hash_state_t st_final;
    hash_init(&st_final);

    for (uint32_t i = 0; i < BAVC_TAU; i++) {
        /* derive root seed */
        uint8_t root[SECPAR_BYTES];
        tree_root_seed(root, root_seed, iv, i);

        /* heap storage: 2*N nodes total (N-1 internal + N leaves + 1 sentinel) */
        /* indices 0..2N-2 used, root at 0, leaves at N-1..2N-2 */
        size_t total = 2 * BAVC_N - 1;
        forest->nodes[i] = malloc(total * SECPAR_BYTES);
        if (!forest->nodes[i]) { fprintf(stderr, "BAVC OOM\n"); exit(1); }
        memcpy(&forest->nodes[i][0], root, SECPAR_BYTES);

        /* expand: for each internal node 0..N-2, expand to children */
        for (uint32_t node = 0; node < BAVC_N - 1; node++) {
            uint32_t lc = 2 * node + 1, rc = 2 * node + 2;
            expand_node(&forest->nodes[i][lc * SECPAR_BYTES],
                        &forest->nodes[i][rc * SECPAR_BYTES],
                        &forest->nodes[i][node * SECPAR_BYTES],
                        iv, i, node);
        }

        /* copy leaf seeds */
        for (uint32_t j = 0; j < BAVC_N; j++) {
            memcpy(forest->leaf_seeds[i][j],
                   &forest->nodes[i][(BAVC_N - 1 + j) * SECPAR_BYTES],
                   SECPAR_BYTES);
        }

        /* per-tree hash */
        uint8_t h_i[LEAF_HASH_BYTES];
        hash_state_t st_hi;
        hash_init(&st_hi);
        for (uint32_t j = 0; j < BAVC_N; j++) {
            leaf_hash_fn(leaf_hashes[i][j], forest->leaf_seeds[i][j], iv);
            hash_update(&st_hi, leaf_hashes[i][j], LEAF_HASH_BYTES);
        }
        hash_final(&st_hi, h_i);
        hash_update(&st_final, h_i, LEAF_HASH_BYTES);
    }
    hash_final(&st_final, commit);
}

/* ── open ───────────────────────────────────────────────────── */

void bavc_open(bavc_opening_t *opening,
               const bavc_forest_t *forest,
               const leaf_hashes_t leaf_hashes,
               const uint32_t delta[BAVC_TAU])
{
    for (uint32_t i = 0; i < BAVC_TAU; i++) {
        uint32_t d = delta[i] & (BAVC_N - 1);
        memcpy(opening->leaf_hashes[i], leaf_hashes[i][d], LEAF_HASH_BYTES);

        /* walk from root (index 0) to leaf (index N-1 + d).
         * At each level, store the sibling seed. */
        uint32_t node = 0; /* root */
        for (int level = 0; level < BAVC_KVC; level++) {
            int bit = (d >> (BAVC_KVC - 1 - level)) & 1;
            uint32_t path_child = 2 * node + 1 + bit;
            uint32_t sibling    = 2 * node + 1 + (1 - bit);
            memcpy(opening->siblings[i][level],
                   &forest->nodes[i][sibling * SECPAR_BYTES], SECPAR_BYTES);
            node = path_child;
        }
    }
}

/* ── reconstruct ────────────────────────────────────────────── */

void bavc_reconstruct(bavc_commit_t commit,
                      uint8_t leaf_seeds[BAVC_TAU][BAVC_N][SECPAR_BYTES],
                      const bavc_opening_t *opening,
                      const uint32_t delta[BAVC_TAU],
                      const bavc_iv_t iv)
{
    hash_state_t st_final;
    hash_init(&st_final);

    for (uint32_t i = 0; i < BAVC_TAU; i++) {
        uint32_t d = delta[i] & (BAVC_N - 1);
        memset(leaf_seeds[i], 0, sizeof(leaf_seeds[i]));

        /* expand sibling subtrees at each level */
        uint32_t node = 0;
        for (int level = 0; level < BAVC_KVC; level++) {
            int bit = (d >> (BAVC_KVC - 1 - level)) & 1;
            uint32_t path_child = 2 * node + 1 + bit;
            uint32_t sibling    = 2 * node + 1 + (1 - bit);
            expand_subtree(leaf_seeds[i],
                           opening->siblings[i][level],
                           iv, i, sibling);
            node = path_child;
        }

        /* per-tree hash: reconstructed leaves + hidden leaf hash */
        uint8_t h_i[LEAF_HASH_BYTES], leaf_h[LEAF_HASH_BYTES];
        hash_state_t st_hi;
        hash_init(&st_hi);
        for (uint32_t j = 0; j < BAVC_N; j++) {
            if (j == d)
                memcpy(leaf_h, opening->leaf_hashes[i], LEAF_HASH_BYTES);
            else
                leaf_hash_fn(leaf_h, leaf_seeds[i][j], iv);
            hash_update(&st_hi, leaf_h, LEAF_HASH_BYTES);
        }
        hash_final(&st_hi, h_i);
        hash_update(&st_final, h_i, LEAF_HASH_BYTES);
    }
    hash_final(&st_final, commit);
}

void bavc_forest_free(bavc_forest_t *forest) {
    for (uint32_t i = 0; i < BAVC_TAU; i++) free(forest->nodes[i]);
}
