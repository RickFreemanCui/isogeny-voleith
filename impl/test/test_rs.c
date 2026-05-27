/* test_rs.c -- RS code tests (all large arrays are static to avoid stack issues) */
#include <stdio.h>
#include "../rs_code.h"

int tests=0, passed=0;
#define T(s) do{tests++;printf("  %s ... ",s);}while(0)
#define OK() do{passed++;printf("OK\n");}while(0)
#define F(m) do{printf("FAIL: %s\n",m);return 1;}while(0)
#define C(c,m) do{if(!(c))F(m);}while(0)

static fp2_t msg[RS_K], out[RS_N];
static fp2_t m1[RS_K], m2[RS_K], e1[RS_N], e2[RS_N], e3[RS_N];
static fp2_t batch_msg[3*RS_K], batch_out[3*RS_N];

int main(void) {
    printf("=== test_rs (N=%d, K=%d) ===\n", RS_N, RS_K);

    T("systematic");
    for (int i = 0; i < RS_K; i++) fp2_set_small(&msg[i], i*10+1, i*3+7);
    rs_encode(out, msg);
    int ok = 1;
    for (int i = 0; i < RS_K; i++)
        if (!fp2_is_equal(&out[i], &msg[i])) ok = 0;
    C(ok, "systematic");
    OK();

    T("linearity");
    for (int i = 0; i < RS_K; i++) { fp2_random(&m1[i]); fp2_random(&m2[i]); }
    rs_encode(e1, m1); rs_encode(e2, m2);
    fp2_t a; fp2_set_small(&a, 3, 1);
    static fp2_t am1[RS_K], sum[RS_K];
    for (int i = 0; i < RS_K; i++) { fp2_mul(&am1[i], &m1[i], &a); fp2_add(&sum[i], &am1[i], &m2[i]); }
    rs_encode(e3, sum);
    ok = 1;
    for (int i = 0; i < RS_N; i++) {
        fp2_t expected, tmp;
        fp2_mul(&tmp, &e1[i], &a);
        fp2_add(&expected, &tmp, &e2[i]);
        if (!fp2_is_equal(&e3[i], &expected)) ok = 0;
    }
    C(ok, "linearity");
    OK();

    T("batch encode");
    for (int r = 0; r < 3; r++)
        for (int i = 0; i < RS_K; i++)
            fp2_set_small(&batch_msg[r*RS_K+i], r*100+i*10+1, r*50+i*3+7);
    rs_batch_encode(batch_out, batch_msg, 3);
    ok = 1;
    for (int r = 0; r < 3; r++) {
        rs_encode(out, &batch_msg[r*RS_K]);
        for (int i = 0; i < RS_N; i++)
            if (!fp2_is_equal(&batch_out[r*RS_N+i], &out[i])) ok = 0;
    }
    C(ok, "batch");
    OK();

    printf("\n  %d/%d passed\n", passed, tests);
    return (passed==tests)?0:1;
}
