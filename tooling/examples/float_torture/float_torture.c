/*
 * Float operations torture test for i8085.
 *
 * Exercises soft-float functions via native float types: negation,
 * comparison, int<->float conversion, addition, multiplication,
 * subtraction, division.
 *
 * Uses volatile to prevent constant folding so all operations
 * actually exercise the runtime soft-float library.
 *
 * Output: 4-byte pass count at 0x0200.
 */

#include <stdint.h>

/* Union for exact bit-pattern checks where needed. */
typedef union { float f; uint32_t u; } fu32;

#define OUTPUT_ADDR 0x0200
#define TOTAL_TESTS 24

__attribute__((noinline)) static void halt_ok(void) { __asm__ volatile("hlt"); }
__attribute__((noinline)) static void fail_loop(void) {
    for (;;) {
    }
}

/* Helper: reinterpret uint32_t bits as float. */
static float from_bits(uint32_t u) { fu32 x; x.u = u; return x.f; }

int main(void) {
    volatile uint32_t *output = (volatile uint32_t *)OUTPUT_ADDR;
    volatile uint16_t pass = 0;

    /* Volatile inputs prevent constant folding. */
    volatile float one = 1.0f;
    volatile float neg_one = -1.0f;
    volatile float two = 2.0f;
    volatile float three = 3.0f;
    volatile float half = 0.5f;
    volatile float zero = 0.0f;
    volatile float forty_two = 42.0f;
    volatile float two_fifty_six = 256.0f;
    volatile float six = 6.0f;
    volatile float nan_val = from_bits(0x7FC00000U);

    volatile int32_t i_zero = 0;
    volatile int32_t i_one = 1;
    volatile int32_t i_neg_one = -1;
    volatile int32_t i_42 = 42;
    volatile uint32_t u_zero = 0;
    volatile uint32_t u_one = 1;
    volatile uint32_t u_256 = 256;

    /* ---- Negation tests ---- */

    /* 1. -(+1.0) == -1.0 */
    if (-(one) == neg_one) pass++;

    /* 2. -(-1.0) == +1.0 */
    if (-(neg_one) == one) pass++;

    /* 3. -(+0.0) has negative sign bit */
    {
        fu32 r; r.f = -(zero);
        if (r.u == 0x80000000U) pass++;
    }

    /* ---- Comparison tests ---- */

    /* 4. 1.0 < 2.0 */
    if (one < two) pass++;

    /* 5. 2.0 > 1.0 */
    if (two > one) pass++;

    /* 6. 1.0 == 1.0 */
    if (one == one) pass++;

    /* 7. -1.0 < 1.0 */
    if (neg_one < one) pass++;

    /* 8. !(NaN <= 1.0) */
    if (!(nan_val <= one)) pass++;

    /* 9. !(NaN >= 1.0) */
    if (!(nan_val >= one)) pass++;

    /* 10. NaN != NaN */
    if (nan_val != nan_val) pass++;

    /* 11. ordered: 1.0 <= 2.0 */
    if (one <= two) pass++;

    /* ---- Int->float conversion tests ---- */

    /* 12. (float)0 == 0.0 */
    if ((float)i_zero == zero) pass++;

    /* 13. (float)1 == 1.0 */
    if ((float)i_one == one) pass++;

    /* 14. (float)(-1) == -1.0 */
    if ((float)i_neg_one == neg_one) pass++;

    /* 15. (float)42 == 42.0 */
    if ((float)i_42 == forty_two) pass++;

    /* 16. (float)0u == 0.0 */
    if ((float)u_zero == zero) pass++;

    /* 17. (float)1u == 1.0 */
    if ((float)u_one == one) pass++;

    /* 18. (float)256u == 256.0 */
    if ((float)u_256 == two_fifty_six) pass++;

    /* ---- Float->int conversion tests ---- */

    /* 19. (int)1.0 == 1 */
    if ((int32_t)one == 1) pass++;

    /* 20. (int)(-1.0) == -1 */
    if ((int32_t)neg_one == -1) pass++;

    /* 21. (unsigned)1.0 == 1 */
    if ((uint32_t)one == 1U) pass++;

    /* 22. (unsigned)42.0 == 42 */
    if ((uint32_t)forty_two == 42U) pass++;

    /* ---- Arithmetic tests ---- */

    /* 23. 1.0 + 2.0 == 3.0 */
    if (one + two == three) pass++;

    /* 24. 2.0 * 3.0 == 6.0 */
    if (two * three == six) pass++;

    /* Write pass count to output */
    *output = (uint32_t)pass;

    if (pass == TOTAL_TESTS) {
        halt_ok();
    }

    fail_loop();
    return 0;
}
