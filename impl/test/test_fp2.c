/*
 * test_fp2.c -- unit tests for F_{p^2} arithmetic
 */
#include <stdio.h>
#include <assert.h>
#include "../fp2.h"

static int tests = 0, passed = 0;

#define TEST(name) do { tests++; printf("  %s ... ", name); } while(0)
#define OK()       do { passed++; printf("OK\n"); } while(0)
#define FAIL(msg)  do { printf("FAIL: %s\n", msg); return; } while(0)
#define CHECK(cond, msg) do { if (!(cond)) FAIL(msg); } while(0)

void test_basics(void) {
    TEST("set_zero / is_zero");
    fp2_t a; fp2_set_zero(&a);
    CHECK(fp2_is_zero(&a), "zero");
    OK();

    TEST("set_one / is_one");
    fp2_set_one(&a);
    CHECK(fp2_is_one(&a), "one");
    OK();
}

void test_add_sub(void) {
    TEST("add/sub roundtrip");
    fp2_t a, b, s, d;
    fp2_set_small(&a, 100, 50);
    fp2_set_small(&b, 37, 12);
    fp2_add(&s, &a, &b);
    fp2_sub(&d, &s, &b);
    CHECK(fp2_is_equal(&d, &a), "roundtrip");
    OK();

    TEST("negate");
    fp2_t neg, zero;
    fp2_set_zero(&zero);
    fp2_neg(&neg, &a);
    fp2_add(&d, &a, &neg);
    CHECK(fp2_is_zero(&d), "a + (-a) != 0");
    OK();
}

void test_mul_sqr(void) {
    TEST("mul by 1");
    fp2_t a, one, p;
    fp2_set_small(&a, 42, 17);
    fp2_set_one(&one);
    fp2_mul(&p, &a, &one);
    CHECK(fp2_is_equal(&p, &a), "a*1 != a");
    OK();

    TEST("sqr vs mul");
    fp2_t sqr, mul;
    fp2_sqr(&sqr, &a);
    fp2_mul(&mul, &a, &a);
    CHECK(fp2_is_equal(&sqr, &mul), "sqr != mul");
    OK();

    TEST("i^2 = -1");
    fp2_t i, i2, mone;
    fp2_set_small(&i, 0, 1);
    fp2_sqr(&i2, &i);
    /* -1 in the field */
    fp2_t expect;
    fp2_set_small(&expect, 0, 0);
    fp_set_one(expect.re); fp_neg(expect.re, expect.re);
    CHECK(fp2_is_equal(&i2, &expect), "i^2 != -1");
    OK();
}

void test_inv(void) {
    TEST("inverse");
    fp2_t a, inv, p, one;
    fp2_set_small(&a, 123, 456);
    fp2_inv(&inv, &a);
    fp2_set_one(&one);
    fp2_mul(&p, &a, &inv);
    CHECK(fp2_is_one(&p), "a*a^{-1} != 1");
    OK();
}

void test_serialise(void) {
    TEST("encode/decode");
    fp2_t a, b;
    uint8_t buf[FP2_BYTES];
    fp2_set_small(&a, 0xDEAD, 0xBEEF);
    fp2_encode(buf, &a);
    fp2_decode(&b, buf);
    CHECK(fp2_is_equal(&a, &b), "roundtrip");
    OK();
}

int main(void) {
    printf("=== test_fp2 ===\n");
    test_basics();
    test_add_sub();
    test_mul_sqr();
    test_inv();
    test_serialise();
    printf("\n  %d / %d tests passed\n", passed, tests);
    return (passed == tests) ? 0 : 1;
}
