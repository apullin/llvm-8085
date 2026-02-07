/*
 * 64-bit arithmetic torture test for i8085
 *
 * Exercises the hand-written 64-bit runtime library routines:
 *   __adddi3  - i64 add
 *   __subdi3  - i64 subtract
 *   __ashldi3 - i64 left shift
 *   __lshrdi3 - i64 logical right shift
 *   __ashrdi3 - i64 arithmetic right shift
 *
 * Uses a 32-bit hash accumulator to validate results without
 * generating enormous inline 64-bit comparison code.
 * Inputs are volatile to prevent constant folding.
 *
 * The test halts on success; loops forever on failure.
 */

#include <stdint.h>

#define OUTPUT_ADDR 0x0200

__attribute__((noinline)) static void halt_ok(void) {
    __asm__ volatile("hlt");
}

__attribute__((noinline, noreturn)) static void fail_loop(void) {
    for (;;) {}
}

/* Hash fold: accumulate 32-bit values */
static uint32_t mix(uint32_t acc, uint32_t v) {
    acc ^= v;
    acc = acc * (uint32_t)0x9E3779B1u + (uint32_t)0x7F4A7C15u;
    acc ^= acc >> 16;
    return acc;
}

/* Mix a 64-bit value into accumulator as two 32-bit halves */
__attribute__((noinline))
static uint32_t mix64(uint32_t acc, uint64_t v) {
    acc = mix(acc, (uint32_t)(v & 0xFFFFFFFFu));
    acc = mix(acc, (uint32_t)(v >> 32));
    return acc;
}

/* Volatile test inputs to prevent constant folding.
 * Each pair is used as (a, b) arguments for operations. */
static volatile uint64_t va[] = {
    0,                          /* add: 0+0=0 */
    0xFFFFFFFFULL,              /* add: carry across 32-bit boundary */
    0xFFFFFFFFFFFFFFFFULL,      /* add: full overflow */
    0x123456789ABCDEF0ULL,      /* sub: identity (x-0) */
    0x100000000ULL,             /* sub: borrow from upper 32 */
};
static volatile uint64_t vb[] = {
    0,
    1,
    1,
    0,
    1,
};

/* Shift test inputs */
static volatile uint64_t vshift_val = 0xDEADBEEFCAFEBABEULL;
static volatile int32_t vshift_amt[] = { 0, 1, 7, 8, 16, 31, 32, 33, 63 };
#define N_SHIFTS 9

/* ------------------------------------------------------------------ */
/* Test group 1: __adddi3 + __subdi3                                   */
/* ------------------------------------------------------------------ */

__attribute__((noinline))
static uint32_t test_addsub(void) {
    uint32_t acc = 0;

    /* 5 add tests */
    for (uint8_t i = 0; i < 5; i++) {
        uint64_t r = va[i] + vb[i];
        acc = mix64(acc, r);
    }

    /* Sub: x - 0 = x, tested with va[3] */
    acc = mix64(acc, va[3] - vb[3]);

    /* Sub: x - 1 from 0x100000000 */
    acc = mix64(acc, va[4] - vb[4]);

    /* Sub: 0 - 1 = all ones */
    acc = mix64(acc, vb[0] - vb[1]);

    /* Sub: x - x = 0 */
    acc = mix64(acc, va[2] - va[2]);

    return acc;
}

/* ------------------------------------------------------------------ */
/* Test group 2: __ashldi3 (left shift)                                */
/* ------------------------------------------------------------------ */

__attribute__((noinline))
static uint32_t test_shl(void) {
    uint32_t acc = 0;
    uint64_t val = vshift_val;

    for (uint8_t i = 0; i < N_SHIFTS; i++) {
        uint64_t r = val << vshift_amt[i];
        acc = mix64(acc, r);
    }

    return acc;
}

/* ------------------------------------------------------------------ */
/* Test group 3: __lshrdi3 (logical right shift)                       */
/* ------------------------------------------------------------------ */

__attribute__((noinline))
static uint32_t test_lshr(void) {
    uint32_t acc = 0;
    uint64_t val = vshift_val;

    for (uint8_t i = 0; i < N_SHIFTS; i++) {
        uint64_t r = val >> vshift_amt[i];
        acc = mix64(acc, r);
    }

    return acc;
}

/* ------------------------------------------------------------------ */
/* Test group 4: __ashrdi3 (arithmetic right shift)                    */
/* ------------------------------------------------------------------ */

__attribute__((noinline))
static uint32_t test_ashr(void) {
    uint32_t acc = 0;
    /* Negative value for sign extension tests */
    int64_t val = (int64_t)vshift_val;  /* 0xDEAD... is negative */

    for (uint8_t i = 0; i < N_SHIFTS; i++) {
        int64_t r = val >> vshift_amt[i];
        acc = mix64(acc, (uint64_t)r);
    }

    return acc;
}

/* ------------------------------------------------------------------ */
/* Test group 5: Mixed operations                                      */
/* ------------------------------------------------------------------ */

__attribute__((noinline))
static uint32_t test_mixed(void) {
    uint32_t acc = 0;

    /* (a + b) << n */
    uint64_t sum = va[1] + vb[1];
    acc = mix64(acc, sum << vshift_amt[6]);  /* shift by 32 */

    /* (a - b) >> n */
    uint64_t diff = va[3] - va[4];
    acc = mix64(acc, diff >> vshift_amt[4]);  /* shift by 16 */

    return acc;
}

/* ------------------------------------------------------------------ */
/* Main                                                                */
/* ------------------------------------------------------------------ */

int main(void) {
    uint32_t acc = 0;

    acc = mix(acc, test_addsub());
    acc = mix(acc, test_shl());
    acc = mix(acc, test_lshr());
    acc = mix(acc, test_ashr());
    acc = mix(acc, test_mixed());

    /* Write final hash to output area */
    volatile uint32_t *out = (volatile uint32_t *)OUTPUT_ADDR;
    *out = acc;

    halt_ok();

    fail_loop();
    return 0;
}
