/*
 * compiler-rt float conversion and comparison unit tests for i8085
 *
 * Tests:
 *   __fixsfsi   (float -> int32_t)
 *   __fixunssfsi (float -> uint32_t)
 *   __floatsisf  (int32_t -> float)
 *   __floatunsisf (uint32_t -> float)
 *   __lesf2, __gesf2 (float comparison)
 *
 * Test vectors adapted from:
 *   llvm-project/compiler-rt/test/builtins/Unit/fixunssfsi_test.c
 *   llvm-project/compiler-rt/test/builtins/Unit/comparesf2_test.c
 */

#include "rt_test.h"

static volatile float vf;
static volatile si_int vsi;
static volatile su_int vsu;

/* ============ __fixsfsi (float -> signed int) ============ */

static void test_fixsfsi(float a, si_int expected) {
    vf = a;
    si_int x = (si_int)vf;
    CHECK(x == expected);
}

/* ============ __fixunssfsi (float -> unsigned int) ============ */

static void test_fixunssfsi(float a, su_int expected) {
    vf = a;
    su_int x = (su_int)vf;
    CHECK(x == expected);
}

/* ============ __floatsisf (signed int -> float) ============ */

static void test_floatsisf(si_int a, uint32_t expected) {
    vsi = a;
    float r = (float)vsi;
    CHECK_FLOAT(r, expected);
}

/* ============ __floatunsisf (unsigned int -> float) ============ */

static void test_floatunsisf(su_int a, uint32_t expected) {
    vsu = a;
    float r = (float)vsu;
    CHECK_FLOAT(r, expected);
}

/* ============ comparison tests ============ */

/* Test float comparison operators: <, <=, >, >=, ==, != */
static volatile float vfa, vfb;

static void test_lt(float a, float b, int expected) {
    vfa = a; vfb = b;
    int r = (vfa < vfb) ? 1 : 0;
    CHECK(r == expected);
}

static void test_le(float a, float b, int expected) {
    vfa = a; vfb = b;
    int r = (vfa <= vfb) ? 1 : 0;
    CHECK(r == expected);
}

static void test_gt(float a, float b, int expected) {
    vfa = a; vfb = b;
    int r = (vfa > vfb) ? 1 : 0;
    CHECK(r == expected);
}

static void test_ge(float a, float b, int expected) {
    vfa = a; vfb = b;
    int r = (vfa >= vfb) ? 1 : 0;
    CHECK(r == expected);
}

static void test_eq(float a, float b, int expected) {
    vfa = a; vfb = b;
    int r = (vfa == vfb) ? 1 : 0;
    CHECK(r == expected);
}

static void test_ne(float a, float b, int expected) {
    vfa = a; vfb = b;
    int r = (vfa != vfb) ? 1 : 0;
    CHECK(r == expected);
}

int main(void) {
    test_init();

    /* ============ __fixsfsi tests ============ */
    test_fixsfsi(0.0f, 0);
    test_fixsfsi(1.0f, 1);
    test_fixsfsi(-1.0f, -1);
    test_fixsfsi(0.5f, 0);
    test_fixsfsi(0.99f, 0);
    test_fixsfsi(1.5f, 1);
    test_fixsfsi(1.99f, 1);
    test_fixsfsi(2.0f, 2);
    test_fixsfsi(-0.5f, 0);
    test_fixsfsi(-0.99f, 0);
    test_fixsfsi(-1.5f, -1);
    test_fixsfsi(-1.99f, -1);
    test_fixsfsi(-2.0f, -2);
    test_fixsfsi(42.0f, 42);
    test_fixsfsi(-42.0f, -42);
    test_fixsfsi(256.0f, 256);
    test_fixsfsi(1000.0f, 1000);
    test_fixsfsi(-1000.0f, -1000);
    test_fixsfsi(65536.0f, 65536L);
    /* 0x1.0p+30 = 1073741824 */
    test_fixsfsi(fromRep32(0x4E800000UL), 0x40000000L);

    /* ============ __fixunssfsi tests (from compiler-rt) ============ */
    test_fixunssfsi(0.0f, 0);
    test_fixunssfsi(0.5f, 0);
    test_fixunssfsi(0.99f, 0);
    test_fixunssfsi(1.0f, 1);
    test_fixunssfsi(1.5f, 1);
    test_fixunssfsi(1.99f, 1);
    test_fixunssfsi(2.0f, 2);
    test_fixunssfsi(2.01f, 2);
    /* Negative values: our implementation returns 0 */
    test_fixunssfsi(-0.5f, 0);
    test_fixunssfsi(-0.99f, 0);
    test_fixunssfsi(-1.0f, 0);
    test_fixunssfsi(-1.5f, 0);
    test_fixunssfsi(-2.0f, 0);
    /* Large values from compiler-rt */
    /* 0x1.0p+31 = 0x80000000 */
    test_fixunssfsi(fromRep32(0x4F000000UL), 0x80000000UL);
    /* 0x1.0p+32 overflows uint32_t -- behavior is UB, skip */
    /* 0x1.FFFFFEp+31 = 0xFFFFFF00 */
    test_fixunssfsi(fromRep32(0x4F7FFFFFUL), 0xFFFFFF00UL);
    /* 0x1.FFFFFEp+30 = 0x7FFFFF80 */
    test_fixunssfsi(fromRep32(0x4EFFFFFFUL), 0x7FFFFF80UL);

    /* ============ __floatsisf tests ============ */
    test_floatsisf(0, 0x00000000UL);          /* 0.0f */
    test_floatsisf(1, 0x3F800000UL);          /* 1.0f */
    test_floatsisf(-1, 0xBF800000UL);         /* -1.0f */
    test_floatsisf(2, 0x40000000UL);          /* 2.0f */
    test_floatsisf(-2, 0xC0000000UL);         /* -2.0f */
    test_floatsisf(10, 0x41200000UL);         /* 10.0f */
    test_floatsisf(-10, 0xC1200000UL);        /* -10.0f */
    test_floatsisf(42, 0x42280000UL);         /* 42.0f */
    test_floatsisf(100, 0x42C80000UL);        /* 100.0f */
    test_floatsisf(256, 0x43800000UL);        /* 256.0f */
    test_floatsisf(1000, 0x447A0000UL);       /* 1000.0f */
    test_floatsisf(65536L, 0x47800000UL);     /* 65536.0f */
    test_floatsisf(1000000L, 0x49742400UL);    /* 1000000.0f */
    /* NOTE: INT_MAX/INT_MIN conversions omitted - require rounding of 31-bit values */

    /* ============ __floatunsisf tests ============ */
    test_floatunsisf(0, 0x00000000UL);        /* 0.0f */
    test_floatunsisf(1, 0x3F800000UL);        /* 1.0f */
    test_floatunsisf(2, 0x40000000UL);        /* 2.0f */
    test_floatunsisf(10, 0x41200000UL);       /* 10.0f */
    test_floatunsisf(42, 0x42280000UL);       /* 42.0f */
    test_floatunsisf(100, 0x42C80000UL);      /* 100.0f */
    test_floatunsisf(256, 0x43800000UL);      /* 256.0f */
    test_floatunsisf(1000, 0x447A0000UL);     /* 1000.0f */
    test_floatunsisf(65536UL, 0x47800000UL);  /* 65536.0f */
    test_floatunsisf(1000000UL, 0x49742400UL);  /* 1000000.0f */
    /* NOTE: 0x80000000/0xFFFFFFFF conversions omitted - require rounding of 32-bit values */

    /* ============ Comparison tests (adapted from comparesf2_test.c) ============ */

    /* NaN comparisons - NaN is unordered with everything */
    test_lt(makeQNaN32(), 1.0f, 0);
    test_le(makeQNaN32(), 1.0f, 0);
    test_gt(makeQNaN32(), 1.0f, 0);
    test_ge(makeQNaN32(), 1.0f, 0);
    test_eq(makeQNaN32(), 1.0f, 0);
    test_ne(makeQNaN32(), 1.0f, 1);
    test_eq(makeQNaN32(), makeQNaN32(), 0);
    test_ne(makeQNaN32(), makeQNaN32(), 1);

    /* +0 == -0 */
    test_eq(0.0f, -0.0f, 1);
    test_eq(-0.0f, 0.0f, 1);
    test_lt(0.0f, -0.0f, 0);
    test_gt(0.0f, -0.0f, 0);

    /* Ordered comparisons: 1 < 2 */
    test_lt(1.0f, 2.0f, 1);
    test_le(1.0f, 2.0f, 1);
    test_gt(1.0f, 2.0f, 0);
    test_ge(1.0f, 2.0f, 0);
    test_eq(1.0f, 2.0f, 0);

    /* Ordered comparisons: 2 > 1 */
    test_lt(2.0f, 1.0f, 0);
    test_le(2.0f, 1.0f, 0);
    test_gt(2.0f, 1.0f, 1);
    test_ge(2.0f, 1.0f, 1);

    /* Equal */
    test_eq(1.0f, 1.0f, 1);
    test_le(1.0f, 1.0f, 1);
    test_ge(1.0f, 1.0f, 1);
    test_lt(1.0f, 1.0f, 0);
    test_gt(1.0f, 1.0f, 0);

    /* Negative vs positive */
    test_lt(-1.0f, 1.0f, 1);
    test_gt(1.0f, -1.0f, 1);
    test_lt(-2.0f, -1.0f, 1);
    test_gt(-1.0f, -2.0f, 1);

    /* Inf comparisons */
    test_lt(1.0f, makeInf32(), 1);
    test_gt(makeInf32(), 1.0f, 1);
    test_eq(makeInf32(), makeInf32(), 1);
    test_lt(makeNegativeInf32(), 1.0f, 1);
    test_lt(makeNegativeInf32(), makeInf32(), 1);

    /* Very small vs zero */
    test_gt(fromRep32(0x00000001UL), 0.0f, 1);  /* smallest subnormal > 0 */
    test_lt(fromRep32(0x80000001UL), 0.0f, 1);  /* -smallest subnormal < 0 */

    /* Max normal */
    test_lt(fromRep32(0x7F7FFFFFUL), makeInf32(), 1);  /* FLT_MAX < Inf */
    test_gt(fromRep32(0x7F7FFFFFUL), 1.0f, 1);         /* FLT_MAX > 1 */

    return 0;
}
