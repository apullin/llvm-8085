/*
 * FP-heavy benchmark for i8085 soft-float performance comparison.
 *
 * Exercises all major soft-float operations in realistic patterns.
 * Avoids arrays to stay compatible with i8085 backend at all opt levels.
 *
 * All float constants encoded as uint32_t hex (IEEE 754 binary32).
 * Output: 4-byte pass count at 0x0200. Halts on success.
 */

#include <stdint.h>

/* Soft-float function declarations (same ABI as uint32_t on i8085). */
uint32_t __addsf3(uint32_t a, uint32_t b);
uint32_t __subsf3(uint32_t a, uint32_t b);
uint32_t __mulsf3(uint32_t a, uint32_t b);
uint32_t __divsf3(uint32_t a, uint32_t b);
int __lesf2(uint32_t a, uint32_t b);
uint32_t __floatsisf(int32_t a);
int32_t __fixsfsi(uint32_t a);
uint32_t __floatunsisf(uint32_t a);
uint32_t __fixunssfsi(uint32_t a);

/* IEEE 754 binary32 constants */
#define F_ZERO      0x00000000U  /* 0.0   */
#define F_HALF      0x3F000000U  /* 0.5   */
#define F_ONE       0x3F800000U  /* 1.0   */
#define F_TWO       0x40000000U  /* 2.0   */
#define F_THREE     0x40400000U  /* 3.0   */
#define F_FIVE      0x40A00000U  /* 5.0   */
#define F_TEN       0x41200000U  /* 10.0  */

#define OUTPUT_ADDR 0x0200
#define TOTAL_TESTS 6

__attribute__((noinline)) static void halt_ok(void) { __asm__ volatile("hlt"); }
__attribute__((noinline)) static void fail_loop(void) { for (;;) {} }

/*
 * Test 1: Newton-Raphson sqrt(2.0), 5 iterations.
 * x_{n+1} = 0.5 * (x_n + 2.0/x_n)
 * Verify: result^2 should be very close to 2.0.
 * 5 iterations × (1 div + 1 add + 1 mul) + 1 mul + 1 sub + 1 cmp = 18 FP ops
 */
__attribute__((noinline))
static int test_newton_sqrt(void) {
    uint32_t x = F_ONE;
    uint32_t d, s;
    volatile int i;
    for (i = 0; i < 5; i++) {
        d = __divsf3(F_TWO, x);
        s = __addsf3(x, d);
        x = __mulsf3(F_HALF, s);
    }
    /* x^2 should equal 2.0 within tolerance */
    uint32_t sq = __mulsf3(x, x);
    uint32_t diff = __subsf3(sq, F_TWO);
    diff &= 0x7FFFFFFFU;  /* abs */
    /* Tolerance: 0x36000000 ~= 1.9e-6 (allows a few ULP from truncating division) */
    return __lesf2(diff, 0x36000000U) <= 0;
}

/*
 * Test 2: Unrolled 8-element dot product.
 * a = [1,2,3,4,5,6,7,8], b = [8,7,6,5,4,3,2,1]
 * Expected = 120.0 = 0x42F00000
 * 8 mul + 7 add = 15 FP ops
 */
__attribute__((noinline))
static int test_dot_product(void) {
    uint32_t dot, prod;
    dot  = __mulsf3(F_ONE,   0x41000000U);  /* 1*8=8 */
    prod = __mulsf3(F_TWO,   0x40E00000U);  /* 2*7=14 */
    dot  = __addsf3(dot, prod);              /* 22 */
    prod = __mulsf3(F_THREE, 0x40C00000U);  /* 3*6=18 */
    dot  = __addsf3(dot, prod);              /* 40 */
    prod = __mulsf3(0x40800000U, F_FIVE);   /* 4*5=20 */
    dot  = __addsf3(dot, prod);              /* 60 */
    prod = __mulsf3(F_FIVE, 0x40800000U);   /* 5*4=20 */
    dot  = __addsf3(dot, prod);              /* 80 */
    prod = __mulsf3(0x40C00000U, F_THREE);  /* 6*3=18 */
    dot  = __addsf3(dot, prod);              /* 98 */
    prod = __mulsf3(0x40E00000U, F_TWO);    /* 7*2=14 */
    dot  = __addsf3(dot, prod);              /* 112 */
    prod = __mulsf3(0x41000000U, F_ONE);    /* 8*1=8 */
    dot  = __addsf3(dot, prod);              /* 120 */
    return dot == 0x42F00000U;  /* 120.0 */
}

/*
 * Test 3: Horner polynomial p(x) = 1 + 2x + 3x^2 + 4x^3 + 5x^4 at x=2.0
 * = 1 + x*(2 + x*(3 + x*(4 + x*5)))
 * Expected: 129.0 = 0x43010000
 * 4 mul + 4 add = 8 FP ops
 */
__attribute__((noinline))
static int test_horner(void) {
    uint32_t px;
    px = F_FIVE;                                    /* 5 */
    px = __addsf3(__mulsf3(px, F_TWO), 0x40800000U); /* 5*2+4 = 14 */
    px = __addsf3(__mulsf3(px, F_TWO), F_THREE);     /* 14*2+3 = 31 */
    px = __addsf3(__mulsf3(px, F_TWO), F_TWO);       /* 31*2+2 = 64 */
    px = __addsf3(__mulsf3(px, F_TWO), F_ONE);        /* 64*2+1 = 129 */
    return px == 0x43010000U;
}

/*
 * Test 4: Int <-> float round-trip conversions (unrolled).
 * 8 values: 0, 1, -1, 42, -42, 100, -100, 255
 * 8 floatsisf + 8 fixsfsi = 16 conversion ops
 */
__attribute__((noinline))
static int test_int_roundtrip(void) {
    if (__fixsfsi(__floatsisf(0)) != 0) return 0;
    if (__fixsfsi(__floatsisf(1)) != 1) return 0;
    if (__fixsfsi(__floatsisf(-1)) != -1) return 0;
    if (__fixsfsi(__floatsisf(42)) != 42) return 0;
    if (__fixsfsi(__floatsisf(-42)) != -42) return 0;
    if (__fixsfsi(__floatsisf(100)) != 100) return 0;
    if (__fixsfsi(__floatsisf(-100)) != -100) return 0;
    if (__fixsfsi(__floatsisf(255)) != 255) return 0;
    return 1;
}

/*
 * Test 5: Comparison-driven min/max finding.
 * Find min and max of 5 values using __lesf2.
 * ~10 comparisons
 */
__attribute__((noinline))
static int test_comparisons(void) {
    uint32_t a = F_FIVE, b = F_TWO, c = 0x41000000U; /* 5, 2, 8 */
    uint32_t d = F_ONE, e = F_THREE;                   /* 1, 3 */
    uint32_t mn, mx;

    /* Find min */
    mn = a;
    if (__lesf2(b, mn) < 0) mn = b;
    if (__lesf2(c, mn) < 0) mn = c;
    if (__lesf2(d, mn) < 0) mn = d;
    if (__lesf2(e, mn) < 0) mn = e;

    /* Find max */
    mx = a;
    if (__lesf2(b, mx) > 0) mx = b;
    if (__lesf2(c, mx) > 0) mx = c;
    if (__lesf2(d, mx) > 0) mx = d;
    if (__lesf2(e, mx) > 0) mx = e;

    return mn == F_ONE && mx == 0x41000000U;  /* min=1, max=8 */
}

/*
 * Test 6: Division chain and multiply-back.
 * 1000.0 / 10 / 10 / 10 / 10 = 0.1, then * 10 * 10 * 10 * 10.
 * 4 div + 4 mul + 1 sub + 1 cmp = 10 FP ops
 * 1000.0 = 0x447A0000
 */
__attribute__((noinline))
static int test_div_chain(void) {
    uint32_t v = 0x447A0000U;  /* 1000.0 */
    v = __divsf3(v, F_TEN);
    v = __divsf3(v, F_TEN);
    v = __divsf3(v, F_TEN);
    v = __divsf3(v, F_TEN);
    v = __mulsf3(v, F_TEN);
    v = __mulsf3(v, F_TEN);
    v = __mulsf3(v, F_TEN);
    v = __mulsf3(v, F_TEN);
    /* Check approximate equality with 1000.0 */
    uint32_t diff = __subsf3(v, 0x447A0000U);
    diff &= 0x7FFFFFFFU;
    /* Tolerance: 0x3A000000 ~= 4.88e-4 */
    return __lesf2(diff, 0x3A000000U) <= 0;
}

int main(void) {
    volatile uint32_t *output = (volatile uint32_t *)OUTPUT_ADDR;
    volatile uint16_t pass = 0;

    /* Per-test pass/fail flags at 0x0204..0x0219 */
    volatile uint8_t *flags = (volatile uint8_t *)0x0204;
    pass = 0;
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
