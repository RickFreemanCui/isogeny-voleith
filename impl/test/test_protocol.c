/* test_protocol.c -- end-to-end BAVC+VOLE round-trip */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "../protocol.h"

static int tests=0, passed=0;
#define T(s) do{tests++;printf("  %s ... ",s);}while(0)
#define OK() do{passed++;printf("OK\n");}while(0)
#define F(m) do{printf("FAIL: %s\n",m);return 1;}while(0)
#define C(c,m) do{if(!(c))F(m);}while(0)

int main(void) {
    printf("=== test_protocol (BAVC+VOLE round-trip) ===\n");

    /* setup */
    fp2_t j0, jk;
    fp2_set_small(&j0, 42, 17);
    fp2_set_small(&jk, 99, 33);

    witness_t witness;
    witness_generate_trivial(witness, j0);

    T("prove");
    uint8_t *proof = malloc(PROOF_BYTES);
    size_t proof_len;
    int ret = prove(proof, &proof_len, witness, j0, jk);
    C(ret == 0, "prove failed");
    printf("(proof_len=%zu) ", proof_len);
    OK();

    T("verify (should pass)");
    ret = verify(proof, proof_len, j0, jk);
    C(ret == 1, "verify failed on valid proof");
    OK();

    T("verify tampered proof (corrupt opening)");
    /* The opening is after FE terms + I vector. Corrupt a byte in it. */
    size_t off = PROOF_FE_SIZE * FP2_BYTES + BAVC_TAU * sizeof(uint32_t);
    proof[off] ^= 0xFF;
    ret = verify(proof, proof_len, j0, jk);
    C(ret == 0, "verify should reject tampered proof");
    OK();

    free(proof);
    printf("\n  %d/%d passed\n", passed, tests);
    return (passed==tests)?0:1;
}
