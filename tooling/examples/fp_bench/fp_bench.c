/*
 * FP-heavy benchmark for i8085 soft-float performance comparison.
 *
 * Exercises all major soft-float operations in realistic patterns
 * using native float types. Avoids arrays to stay compatible with
 * i8085 backend at all opt levels.
 *
 * Uses volatile to prevent constant folding so all operations
 * actually exercise the runtime soft-float library.
 *
 * Output: 4-byte pass count at 0x0200. Halts on success.
 */

#include <stdint.h>

/* Union for exact bit-pattern checks where needed. */
typedef union { float f; uint32_t u; } fu32;

#define OUTPUT_ADDR 0x0200
#define TOTAL_TESTS 6

__attribute__((noinline)) static void halt_ok(void) { __asm__ volatile("hlt"); }
__attribute__((noinline)) static void fail_loop(void) { for (;;) {} }

/* Helper: absolute value via bit manipulation. */
static float fabs_soft(float x) {
    fu32 v;
    v.f = x;
    v.u &= 0x7FFFFFFFU;
    return v.f;
}

/*
 * Test 1: Newton-Raphson sqrt(2.0), 5 iterations.
 * x_{n+1} = 0.5 * (x_n + 2.0/x_n)
 * Verify: result^2 should be very close to 2.0.
 * 5 iterations x (1 div + 1 add + 1 mul) + 1 mul + 1 sub + 1 cmp = 18 FP ops
 */
__attribute__((noinline))
static int test_newton_sqrt(void) {
    volatile float two = 2.0f;
    volatile float half = 0.5f;
    volatile float one = 1.0f;
    float x = one;
    volatile int i;
    for (i = 0; i < 5; i++) {
        float d = two / x;
        float s = x + d;
        x = half * s;
    }
    /* x^2 should equal 2.0 within tolerance */
    float sq = x * x;
    float diff = fabs_soft(sq - two);
    /* Tolerance: ~1.9e-6 (allows a few ULP from truncating division) */
    volatile float tol = 1.9e-6f;
    return diff < tol;
}

/*
 * Test 2: Unrolled 8-element dot product.
 * a = [1,2,3,4,5,6,7,8], b = [8,7,6,5,4,3,2,1]
 * Expected = 120.0
 * 8 mul + 7 add = 15 FP ops
 */
__attribute__((noinline))
static int test_dot_product(void) {
    volatile float v1 = 1.0f, v2 = 2.0f, v3 = 3.0f, v4 = 4.0f;
    volatile float v5 = 5.0f, v6 = 6.0f, v7 = 7.0f, v8 = 8.0f;
    volatile float expected = 120.0f;

    float dot;
    dot  = v1 * v8;  /* 1*8=8 */
    dot += v2 * v7;  /* +14=22 */
    dot += v3 * v6;  /* +18=40 */
    dot += v4 * v5;  /* +20=60 */
    dot += v5 * v4;  /* +20=80 */
    dot += v6 * v3;  /* +18=98 */
    dot += v7 * v2;  /* +14=112 */
    dot += v8 * v1;  /* +8=120 */
    return dot == expected;
}

/*
 * Test 3: Horner polynomial p(x) = 1 + 2x + 3x^2 + 4x^3 + 5x^4 at x=2.0
 * = 1 + x*(2 + x*(3 + x*(4 + x*5)))
 * Expected: 129.0
 * 4 mul + 4 add = 8 FP ops
 */
__attribute__((noinline))
static int test_horner(void) {
    volatile float x = 2.0f;
    volatile float c0 = 1.0f, c1 = 2.0f, c2 = 3.0f, c3 = 4.0f, c4 = 5.0f;
    volatile float expected = 129.0f;

    float px;
    px = c4;                     /* 5 */
    px = px * x + c3;           /* 5*2+4 = 14 */
    px = px * x + c2;           /* 14*2+3 = 31 */
    px = px * x + c1;           /* 31*2+2 = 64 */
    px = px * x + c0;           /* 64*2+1 = 129 */
    return px == expected;
}

/*
 * Test 4: Int <-> float round-trip conversions (unrolled).
 * 8 values: 0, 1, -1, 42, -42, 100, -100, 255
 * 8 floatsisf + 8 fixsfsi = 16 conversion ops
 */
__attribute__((noinline))
static int test_int_roundtrip(void) {
    volatile int32_t v0 = 0, v1 = 1, v_m1 = -1, v42 = 42;
    volatile int32_t v_m42 = -42, v100 = 100, v_m100 = -100, v255 = 255;

    if ((int32_t)(float)v0 != 0) return 0;
    if ((int32_t)(float)v1 != 1) return 0;
    if ((int32_t)(float)v_m1 != -1) return 0;
    if ((int32_t)(float)v42 != 42) return 0;
    if ((int32_t)(float)v_m42 != -42) return 0;
    if ((int32_t)(float)v100 != 100) return 0;
    if ((int32_t)(float)v_m100 != -100) return 0;
    if ((int32_t)(float)v255 != 255) return 0;
    return 1;
}

/*
 * Test 5: Comparison-driven min/max finding.
 * Find min and max of 5 values using native comparisons.
 * ~10 comparisons
 */
__attribute__((noinline))
static int test_comparisons(void) {
    volatile float a = 5.0f, b = 2.0f, c = 8.0f;
    volatile float d = 1.0f, e = 3.0f;
    volatile float exp_min = 1.0f, exp_max = 8.0f;

    float mn, mx;

    /* Find min */
    mn = a;
    if (b < mn) mn = b;
    if (c < mn) mn = c;
    if (d < mn) mn = d;
    if (e < mn) mn = e;

    /* Find max */
    mx = a;
    if (b > mx) mx = b;
    if (c > mx) mx = c;
    if (d > mx) mx = d;
    if (e > mx) mx = e;

    return mn == exp_min && mx == exp_max;
}

/*
 * Test 6: Division chain and multiply-back.
 * 1000.0 / 10 / 10 / 10 / 10 = 0.1, then * 10 * 10 * 10 * 10.
 * 4 div + 4 mul + 1 sub + 1 cmp = 10 FP ops
 */
__attribute__((noinline))
static int test_div_chain(void) {
    volatile float start = 1000.0f;
    volatile float ten = 10.0f;
    volatile float tol = 5.0e-4f;

    float v = start;
    v = v / ten;
    v = v / ten;
    v = v / ten;
    v = v / ten;
    v = v * ten;
    v = v * ten;
    v = v * ten;
    v = v * ten;
    /* Check approximate equality with 1000.0 */
    float diff = fabs_soft(v - start);
    return diff < tol;
}

int main(void) {
    volatile uint32_t *output = (volatile uint32_t *)OUTPUT_ADDR;
    volatile uint16_t pass = 0;

    /* Per-test pass/fail flags at 0x0204..0x0209 */
    volatile uint8_t *flags = (volatile uint8_t *)0x0204;
    int r;

    r = test_newton_sqrt();
    flags[0] = r;
    if (r) pass++;

    r = test_dot_product();
    flags[1] = r;
    if (r) pass++;

    r = test_horner();
    flags[2] = r;
    if (r) pass++;

    r = test_int_roundtrip();
    flags[3] = r;
    if (r) pass++;

    r = test_comparisons();
    flags[4] = r;
    if (r) pass++;

    r = test_div_chain();
    flags[5] = r;
    if (r) pass++;

    *output = (uint32_t)pass;

    if (pass == TOTAL_TESTS) {
        halt_ok();
    }

    fail_loop();
    return 0;
}
