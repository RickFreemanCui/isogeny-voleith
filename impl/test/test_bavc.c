/*
 * test_bavc.c -- BAVC commit/open/reconstruct round-trip
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "../bavc.h"

static int tests = 0, passed = 0;
#define TEST(name) do { tests++; printf("  %s ... ", name); } while(0)
#define OK()       do { passed++; printf("OK\n"); } while(0)
#define FAIL(msg)  do { printf("FAIL: %s\n", msg); return 1; } while(0)
#define CHECK(cond, msg) do { if (!(cond)) FAIL(msg); } while(0)

int main(void) {
    printf("=== test_bavc (tau=%d, N=%d) ===\n", BAVC_TAU, BAVC_N);

    /* setup */
    uint8_t root_seed[SECPAR_BYTES] = {0};
    bavc_iv_t iv = {0};
    for (int i = 0; i < SECPAR_BYTES; i++) {
        root_seed[i] = (uint8_t)(i * 7 + 13);
        iv[i] = (uint8_t)(i * 3 + 42);
    }

    bavc_commit_t commit;
    bavc_forest_t forest;
    leaf_hashes_t leaf_hashes;

    TEST("commit");
    bavc_commit(commit, &forest, leaf_hashes, root_seed, iv);
    OK();

    TEST("open");
    uint32_t delta[BAVC_TAU];
    bavc_opening_t opening;
    for (uint32_t i = 0; i < BAVC_TAU; i++)
        delta[i] = (i * 123 + 456) & (BAVC_N - 1);
    bavc_open(&opening, &forest, leaf_hashes, delta);
    OK();

    TEST("reconstruct + leaf check (tree 0)");
    bavc_commit_t commit2;
    uint8_t leaf_seeds2[BAVC_TAU][BAVC_N][SECPAR_BYTES];
    bavc_reconstruct(commit2, leaf_seeds2, &opening, delta, iv);
    /* check first tree leaves */
    {
        uint32_t i = 0;
        for (uint32_t j = 0; j < BAVC_N; j++) {
            if (j == delta[i]) continue;
            if (memcmp(forest.leaf_seeds[i][j], leaf_seeds2[i][j], SECPAR_BYTES) != 0) {
                printf("FAIL at tree 0 leaf %u (delta=%u)\n", j, delta[i]);
                printf("  expected: "); for(int k=0;k<8;k++)printf("%02x",forest.leaf_seeds[i][j][k]); printf("\n");
                printf("  got:      "); for(int k=0;k<8;k++)printf("%02x",leaf_seeds2[i][j][k]); printf("\n");
                FAIL("leaf seed mismatch");
            }
        }
    }
    OK();

    TEST("reconstruct commit matches");
    CHECK(memcmp(commit, commit2, sizeof(bavc_commit_t)) == 0,
          "commitment mismatch");
    OK();

    /* check all other trees */
    TEST("leaf seeds match (all trees)");
    {
        int ok = 1;
        for (uint32_t i = 1; i < BAVC_TAU && ok; i++) {
            for (uint32_t j = 0; j < BAVC_N && ok; j++) {
                if (j == delta[i]) continue;
                if (memcmp(forest.leaf_seeds[i][j], leaf_seeds2[i][j], SECPAR_BYTES) != 0)
                    ok = 0;
            }
        }
        CHECK(ok, "leaf seed mismatch in non-zero tree");
    }
    OK();

    bavc_forest_free(&forest);
    printf("\n  %d / %d tests passed\n", passed, tests);
    return (passed == tests) ? 0 : 1;
}
