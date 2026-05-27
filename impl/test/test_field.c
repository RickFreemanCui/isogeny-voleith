/*
 * test_field.c -- unit tests for F_p arithmetic
 */
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "../field.h"

static int tests = 0, passed = 0;

#define TEST(name) do { tests++; printf("  %s ... ", name); } while(0)
#define OK()       do { passed++; printf("OK\n"); } while(0)
#define FAIL(msg)  do { printf("FAIL: %s\n", msg); return; } while(0)
#define CHECK(cond, msg) do { if (!(cond)) FAIL(msg); } while(0)

void test_set_get(void) {
    TEST("set_zero / is_zero");
    fp_t a; fp_set_zero(a);
    CHECK(fp_is_zero(a), "zero not zero");
    OK();

    TEST("set_one / is_one");
    fp_set_one(a);
    CHECK(fp_is_one(a), "one not one");
    CHECK(!fp_is_zero(a), "one is zero");
    OK();

    TEST("set_small");
    fp_set_small(a, 42);
    CHECK(!fp_is_zero(a), "42 is zero");
    CHECK(!fp_is_one(a), "42 is one");
    OK();
}

void test_copy(void) {
    TEST("copy");
    fp_t a, b;
    fp_set_small(a, 12345);
    fp_copy(b, a);
    CHECK(fp_is_equal(a, b), "copy not equal");
    OK();
}

void test_add_sub(void) {
    TEST("add then sub");
    fp_t a, b, sum, diff, zero;
    fp_set_small(a, 100);
    fp_set_small(b, 37);
    fp_add(sum, a, b);
    fp_sub(diff, sum, b);
    CHECK(fp_is_equal(diff, a), "add/sub roundtrip");
    OK();

    TEST("negate");
    fp_t neg_a, back;
    fp_neg(neg_a, a);
    fp_add(back, a, neg_a);
    CHECK(fp_is_zero(back), "a + (-a) != 0");
    OK();
}

void test_mul_sqr(void) {
    TEST("mul by 1");
    fp_t a, one, prod;
    fp_set_small(a, 12345);
    fp_set_one(one);
    fp_mul(prod, a, one);
    CHECK(fp_is_equal(prod, a), "a * 1 != a");
    OK();

    TEST("mul by 0");
    fp_t zero;
    fp_set_zero(zero);
    fp_mul(prod, a, zero);
    CHECK(fp_is_zero(prod), "a * 0 != 0");
    OK();

    TEST("sqr vs mul");
    fp_t sqr, mul;
    fp_set_small(a, 9999);
    fp_sqr(sqr, a);
    fp_mul(mul, a, a);
    CHECK(fp_is_equal(sqr, mul), "sqr != mul");
    OK();

    TEST("commutative mul");
    fp_t b, ab, ba;
    fp_set_small(a, 42);
    fp_set_small(b, 17);
    fp_mul(ab, a, b);
    fp_mul(ba, b, a);
    CHECK(fp_is_equal(ab, ba), "ab != ba");
    OK();
}

void test_inv(void) {
    TEST("inverse");
    fp_t a, inv_a, prod, one;
    fp_set_small(a, 12345);
    fp_inv(inv_a, a);
    fp_set_one(one);
    fp_mul(prod, a, inv_a);
    CHECK(fp_is_one(prod), "a * a^{-1} != 1");
    OK();

    TEST("inverse of 1");
    fp_t inv_one;
    fp_set_one(a);
    fp_inv(inv_one, a);
    CHECK(fp_is_one(inv_one), "1^{-1} != 1");
    OK();
}

void test_serialise(void) {
    TEST("encode/decode roundtrip");
    fp_t a, b;
    uint8_t buf[FIELD_BYTES];
    fp_set_small(a, 0xDEADBEEFCAFEull);
    fp_encode(buf, a);
    fp_decode(b, buf);
    CHECK(fp_is_equal(a, b), "encode/decode roundtrip");
    OK();
}

void test_random(void) {
    TEST("random (basic)");
    fp_t a, b;
    fp_random(a);
    fp_random(b);
    /* extremely unlikely to be equal */
    CHECK(!fp_is_equal(a, b), "two randoms equal");
    CHECK(!fp_is_zero(a), "random is zero");
    OK();
}

void test_cswap(void) {
    TEST("cswap");
    fp_t a, b, a0, b0;
    fp_set_small(a, 1); fp_set_small(b, 2);
    fp_copy(a0, a); fp_copy(b0, b);

    fp_cswap(a, b, 0);
    CHECK(fp_is_equal(a, a0) && fp_is_equal(b, b0), "cswap(0) changed values");

    fp_cswap(a, b, 1);
    CHECK(fp_is_equal(a, b0) && fp_is_equal(b, a0), "cswap(1) didn't swap");
    OK();
}

int main(void) {
    printf("=== test_field ===\n");

    test_set_get();
    test_copy();
    test_add_sub();
    test_mul_sqr();
    test_inv();
    test_serialise();
    test_random();
    test_cswap();

    printf("\n  %d / %d tests passed\n", passed, tests);
    return (passed == tests) ? 0 : 1;
}
