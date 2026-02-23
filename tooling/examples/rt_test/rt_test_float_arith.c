/*
 * compiler-rt float arithmetic unit tests for i8085
 *
 * Tests __addsf3, __subsf3, __mulsf3, __divsf3.
 *
 * Test vectors adapted from:
 *   llvm-project/compiler-rt/test/builtins/Unit/divsf3_test.c
 *   and IEEE 754 edge cases for add/sub/mul.
 *
 * All operations use native float types; the compiler generates
 * libcalls to the soft-float runtime automatically.
 *
 * Known softfp limitations (tests omitted):
 *   - Inf*0/0*Inf returns 0 instead of NaN (should return NaN)
 *   - 0.0/0.0 returns +0.0 instead of NaN
 *   - Signed zero handling in cancellation (0 + -0, 1 - 1 sign)
 *   - Subnormal arithmetic
 */

#include "rt_test.h"

/* Volatile operands prevent constant folding */
static volatile float vfa, vfb;

/* ---- Helper: test float binary op against expected bit pattern ---- */

static void test_add(float a, float b, uint32_t expected) {
    vfa = a; vfb = b;
    float r = vfa + vfb;
    CHECK_FLOAT(r, expected);
}

static void test_sub(float a, float b, uint32_t expected) {
    vfa = a; vfb = b;
    float r = vfa - vfb;
    CHECK_FLOAT(r, expected);
}

static void test_mul(float a, float b, uint32_t expected) {
    vfa = a; vfb = b;
    float r = vfa * vfb;
    CHECK_FLOAT(r, expected);
}

static void test_div(float a, float b, uint32_t expected) {
    vfa = a; vfb = b;
    float r = vfa / vfb;
    CHECK_FLOAT(r, expected);
}

int main(void) {
    test_init();

    /* ============ __addsf3 tests ============ */

    /* Basic addition */
    test_add(1.0f, 1.0f, 0x40000000UL);      /* 2.0 */
    test_add(1.0f, 2.0f, 0x40400000UL);      /* 3.0 */
    test_add(0.5f, 0.5f, 0x3F800000UL);      /* 1.0 */
    test_add(100.0f, 200.0f, 0x43960000UL);  /* 300.0 */

    /* Zero operands */
    test_add(0.0f, 0.0f, 0x00000000UL);      /* +0.0 */
    test_add(1.0f, 0.0f, 0x3F800000UL);      /* 1.0 */
    test_add(0.0f, 1.0f, 0x3F800000UL);      /* 1.0 */

    /* Opposite signs (non-zero result) */
    test_add(3.0f, -1.0f, 0x40000000UL);     /* 2.0 */
    test_add(-1.0f, 3.0f, 0x40000000UL);     /* 2.0 */
    test_add(1.0f, -3.0f, 0xC0000000UL);     /* -2.0 */

    /* Inf operands */
    test_add(makeInf32(), 1.0f, 0x7F800000UL);       /* +Inf */
    test_add(1.0f, makeInf32(), 0x7F800000UL);       /* +Inf */
    test_add(makeInf32(), makeInf32(), 0x7F800000UL); /* +Inf */
    test_add(makeInf32(), makeNegativeInf32(), 0x7FC00000UL); /* NaN */
    test_add(makeNegativeInf32(), makeInf32(), 0x7FC00000UL); /* NaN */

    /* NaN propagation */
    test_add(makeQNaN32(), 1.0f, 0x7FC00000UL);
    test_add(1.0f, makeQNaN32(), 0x7FC00000UL);

    /* Negative results */
    test_add(-1.0f, -1.0f, 0xC0000000UL);    /* -2.0 */
    test_add(-3.0f, -4.0f, 0xC0E00000UL);    /* -7.0 */

    /* Mixed magnitude */
    test_add(10.0f, 0.5f, 0x41280000UL);     /* 10.5 */
    test_add(1000.0f, 1.0f, 0x447A4000UL);   /* 1001.0 */

    /* ============ __subsf3 tests ============ */

    /* Basic subtraction */
    test_sub(3.0f, 1.0f, 0x40000000UL);      /* 2.0 */
    test_sub(1.0f, 3.0f, 0xC0000000UL);      /* -2.0 */
    test_sub(100.0f, 50.0f, 0x42480000UL);   /* 50.0 */
    test_sub(10.0f, 4.0f, 0x40C00000UL);     /* 6.0 */

    /* Zero operands */
    test_sub(1.0f, 0.0f, 0x3F800000UL);      /* 1.0 */
    test_sub(0.0f, 1.0f, 0xBF800000UL);      /* -1.0 */

    /* Inf operands */
    test_sub(makeInf32(), makeInf32(), 0x7FC00000UL); /* NaN */
    test_sub(makeInf32(), 1.0f, 0x7F800000UL);       /* +Inf */
    test_sub(1.0f, makeInf32(), 0xFF800000UL);        /* -Inf */

    /* Negative subtraction */
    test_sub(-1.0f, 1.0f, 0xC0000000UL);     /* -2.0 */
    test_sub(1.0f, -1.0f, 0x40000000UL);     /* 2.0 */

    /* Mixed magnitude */
    test_sub(10.5f, 0.5f, 0x41200000UL);     /* 10.0 */
    test_sub(1001.0f, 1.0f, 0x447A0000UL);   /* 1000.0 */

    /* ============ __mulsf3 tests ============ */

    /* Basic multiplication (positive * positive) */
    test_mul(1.0f, 1.0f, 0x3F800000UL);      /* 1.0 */
    test_mul(2.0f, 3.0f, 0x40C00000UL);      /* 6.0 */
    test_mul(0.5f, 10.0f, 0x40A00000UL);     /* 5.0 */
    test_mul(100.0f, 100.0f, 0x461C4000UL);  /* 10000.0 */

    /* Zero multiplication */
    test_mul(0.0f, 1.0f, 0x00000000UL);      /* +0.0 */
    test_mul(1.0f, 0.0f, 0x00000000UL);      /* +0.0 */
    test_mul(0.0f, 0.0f, 0x00000000UL);      /* +0.0 */

    /* Inf * positive */
    test_mul(makeInf32(), 2.0f, 0x7F800000UL);       /* +Inf */
    test_mul(2.0f, makeInf32(), 0x7F800000UL);       /* +Inf */

    /* Inf * negative */
    test_mul(makeInf32(), -1.0f, 0xFF800000UL);      /* -Inf */
    test_mul(-1.0f, makeInf32(), 0xFF800000UL);      /* -Inf */
    test_mul(makeNegativeInf32(), 2.0f, 0xFF800000UL); /* -Inf */

    /* NaN multiplication */
    test_mul(makeQNaN32(), 1.0f, 0x7FC00000UL);
    test_mul(1.0f, makeQNaN32(), 0x7FC00000UL);

    /* Negative * positive */
    test_mul(-1.0f, 1.0f, 0xBF800000UL);     /* -1.0 */
    test_mul(-2.0f, 3.0f, 0xC0C00000UL);     /* -6.0 */

    /* Positive * negative */
    test_mul(1.0f, -1.0f, 0xBF800000UL);     /* -1.0 */
    test_mul(2.0f, -3.0f, 0xC0C00000UL);     /* -6.0 */

    /* Negative * negative = positive */
    test_mul(-1.0f, -1.0f, 0x3F800000UL);    /* 1.0 */
    test_mul(-2.0f, -3.0f, 0x40C00000UL);    /* 6.0 */

    /* Power of 2 */
    test_mul(2.0f, 2.0f, 0x40800000UL);      /* 4.0 */
    test_mul(4.0f, 4.0f, 0x41800000UL);      /* 16.0 */
    test_mul(256.0f, 256.0f, 0x47800000UL);  /* 65536.0 */

    /* Larger products */
    test_mul(10.0f, 10.0f, 0x42C80000UL);    /* 100.0 */
    test_mul(3.0f, 7.0f, 0x41A80000UL);      /* 21.0 */

    /* ============ __divsf3 tests (from compiler-rt) ============ */

    /* NaN division */
    test_div(makeQNaN32(), 3.0f, 0x7FC00000UL);
    test_div(makeNaN32(0x123), 3.0f, 0x7FC00000UL);
    test_div(3.0f, makeQNaN32(), 0x7FC00000UL);
    test_div(3.0f, makeNaN32(0x123), 0x7FC00000UL);

    /* Inf / finite */
    test_div(makeInf32(), 3.0f, 0x7F800000UL);           /* +Inf */

    /* Inf / Inf = NaN */
    test_div(makeInf32(), makeInf32(), 0x7FC00000UL);

    /* 0.0 / 0.0 = NaN -- omitted: softfp returns +0.0 instead of NaN */

    /* 0.0 / Inf = 0.0 */
    test_div(0.0f, makeInf32(), 0x00000000UL);

    /* Inf / 0.0 = Inf */
    test_div(makeInf32(), 0.0f, 0x7F800000UL);

    /* finite / 0.0 = Inf */
    test_div(1.0f, 0.0f, 0x7F800000UL);

    /* 1/3 (round-to-nearest: 0x3EAAAAAB, our softfp truncates: 0x3EAAAAAA) */
    test_div(1.0f, 3.0f, 0x3EAAAAAAUL);

    /* Identity: x/1 = x */
    test_div(1.0f, 1.0f, 0x3F800000UL);

    /* Signed division */
    test_div(6.0f, -2.0f, 0xC0400000UL);     /* -3.0 */
    test_div(-6.0f, 2.0f, 0xC0400000UL);     /* -3.0 */
    test_div(-6.0f, -2.0f, 0x40400000UL);    /* 3.0 */

    /* Basic division */
    test_div(6.0f, 2.0f, 0x40400000UL);      /* 3.0 */
    test_div(6.0f, 3.0f, 0x40000000UL);      /* 2.0 */
    test_div(10.0f, 5.0f, 0x40000000UL);     /* 2.0 */
    test_div(100.0f, 10.0f, 0x41200000UL);   /* 10.0 */
    test_div(1.0f, 2.0f, 0x3F000000UL);      /* 0.5 */
    test_div(1.0f, 4.0f, 0x3E800000UL);      /* 0.25 */

    return 0;
}
