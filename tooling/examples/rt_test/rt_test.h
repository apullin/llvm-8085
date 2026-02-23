/*
 * Common header for compiler-rt builtin unit tests on i8085.
 *
 * Provides:
 *   - Test counter macros (pass/fail/total at 0x0200)
 *   - Float bit-manipulation helpers (fromRep32, toRep32, etc.)
 *   - CHECK() macro for test assertions
 *
 * This is freestanding: no printf, no stdlib, no stdio.
 */

#ifndef RT_TEST_H
#define RT_TEST_H

#include <stdint.h>

/* ---- Test result counters at fixed RAM address ---- */
#define PASS_COUNT  (*(volatile unsigned int *)0x0200)
#define FAIL_COUNT  (*(volatile unsigned int *)0x0202)
#define TOTAL_COUNT (*(volatile unsigned int *)0x0204)

/* Additional: store the index of the first failing test */
#define FIRST_FAIL  (*(volatile unsigned int *)0x0206)

static inline void test_init(void) {
    PASS_COUNT  = 0;
    FAIL_COUNT  = 0;
    TOTAL_COUNT = 0;
    FIRST_FAIL  = 0xFFFF;
}

static inline void test_pass(void) {
    TOTAL_COUNT++;
    PASS_COUNT++;
}

static inline void test_fail(void) {
    if (FIRST_FAIL == 0xFFFF)
        FIRST_FAIL = TOTAL_COUNT;
    TOTAL_COUNT++;
    FAIL_COUNT++;
}

#define CHECK(cond) do { if (cond) test_pass(); else test_fail(); } while(0)

/* ---- Float helpers (adapted from compiler-rt fp_test.h) ---- */

typedef union { float f; uint32_t u; } float_bits;

static inline float fromRep32(uint32_t x) {
    float_bits fb;
    fb.u = x;
    return fb.f;
}

static inline uint32_t toRep32(float x) {
    float_bits fb;
    fb.f = x;
    return fb.u;
}

/* Compare float result against expected bit pattern.
 * Returns 0 if match (including NaN equivalence). */
static inline int compareResultF(float result, uint32_t expected) {
    uint32_t rep = toRep32(result);
    if (rep == expected)
        return 0;
    /* Any NaN is acceptable when expecting qNaN */
    if (expected == 0x7FC00000UL) {
        if ((rep & 0x7F800000UL) == 0x7F800000UL &&
            (rep & 0x007FFFFFUL) > 0)
            return 0;
    }
    return 1;
}

static inline float makeQNaN32(void) { return fromRep32(0x7FC00000UL); }
static inline float makeNaN32(uint32_t rand) {
    return fromRep32(0x7F800000UL | (rand & 0x007FFFFFUL));
}
static inline float makeInf32(void) { return fromRep32(0x7F800000UL); }
static inline float makeNegativeInf32(void) { return fromRep32(0xFF800000UL); }

/* CHECK_FLOAT: compare float result to expected uint32_t bit pattern */
#define CHECK_FLOAT(result, expected) \
    CHECK(compareResultF((result), (expected)) == 0)

/* ---- Type definitions matching compiler-rt ---- */
/* On i8085: int is 16-bit, long is 32-bit, long long is 64-bit */
typedef int32_t  si_int;
typedef uint32_t su_int;
typedef int64_t  di_int;
typedef uint64_t du_int;

#endif /* RT_TEST_H */
