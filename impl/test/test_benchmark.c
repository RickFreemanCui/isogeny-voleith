/*
 * test_benchmark.c -- performance benchmark: 100 prove/verify runs
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "../protocol.h"

#define WARMUP 5
#define RUNS   100

static double now_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000.0 + ts.tv_nsec / 1e6;
}

int main(void) {
    fp2_t j0, jk;
    fp2_set_small(&j0, 42, 17);
    fp2_set_small(&jk, 99, 33);

    witness_t witness;
    witness_generate_trivial(witness, j0);

    uint8_t *proof = malloc(PROOF_BYTES);
    size_t proof_len;
    double prove_times[RUNS], verify_times[RUNS];

    printf("=== VOLEitH Benchmark (%d warmup + %d runs) ===\n", WARMUP, RUNS);

    for (int i = -WARMUP; i < RUNS; i++) {
        double t0 = now_ms();
        prove(proof, &proof_len, witness, j0, jk);
        double t1 = now_ms();
        int ok = verify(proof, proof_len, j0, jk);
        double t2 = now_ms();

        if (i >= 0) {
            prove_times[i]  = t1 - t0;
            verify_times[i] = t2 - t1;
        }
        if (!ok) { printf("FAIL at run %d\n", i); free(proof); return 1; }
    }

    /* statistics */
    double p_min = 1e9, p_max = 0, p_sum = 0;
    double v_min = 1e9, v_max = 0, v_sum = 0;
    for (int i = 0; i < RUNS; i++) {
        if (prove_times[i] < p_min) p_min = prove_times[i];
        if (prove_times[i] > p_max) p_max = prove_times[i];
        p_sum += prove_times[i];
        if (verify_times[i] < v_min) v_min = verify_times[i];
        if (verify_times[i] > v_max) v_max = verify_times[i];
        v_sum += verify_times[i];
    }
    double p_avg = p_sum / RUNS;
    double v_avg = v_sum / RUNS;

    printf("Proof size: %.1f kB\n", proof_len / 1024.0);
    printf("Prove:   avg %7.1f ms  min %7.1f ms  max %7.1f ms\n", p_avg, p_min, p_max);
    printf("Verify:  avg %7.1f ms  min %7.1f ms  max %7.1f ms\n", v_avg, v_min, v_max);
    printf("Total:   avg %7.1f ms\n", p_avg + v_avg);

    free(proof);
    return 0;
}
