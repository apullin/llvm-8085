/*
 * Float operations torture test for i8085.
 *
 * Exercises hand-written soft-float functions: negation, comparison,
 * int<->float conversion, addition, multiplication, subtraction, division.
 *
 * All float constants are represented as uint32_t hex values
 * (IEEE 754 binary32) to avoid needing compiler float support.
 * The ABI for float and uint32_t is identical on i8085.
 *
 * Output: 4-byte pass count at 0x0200.
 */

#include <stdint.h>

/* Soft-float function declarations using uint32_t (same ABI as float). */
uint32_t __negsf2(uint32_t a);
uint32_t __addsf3(uint32_t a, uint32_t b);
uint32_t __subsf3(uint32_t a, uint32_t b);
uint32_t __mulsf3(uint32_t a, uint32_t b);
uint32_t __divsf3(uint32_t a, uint32_t b);
int __lesf2(uint32_t a, uint32_t b);
int __gesf2(uint32_t a, uint32_t b);
int __unordsf2(uint32_t a, uint32_t b);
uint32_t __floatsisf(int32_t a);
uint32_t __floatunsisf(uint32_t a);
int32_t __fixsfsi(uint32_t a);
uint32_t __fixunssfsi(uint32_t a);

/* IEEE 754 single-precision constants */
#define F_POS_ZERO  0x00000000U  /* +0.0  */
#define F_NEG_ZERO  0x80000000U  /* -0.0  */
#define F_POS_ONE   0x3F800000U  /* +1.0  */
#define F_NEG_ONE   0xBF800000U  /* -1.0  */
#define F_POS_TWO   0x40000000U  /* +2.0  */
#define F_POS_THREE 0x40400000U  /* +3.0  */
#define F_POS_HALF  0x3F000000U  /* +0.5  */
#define F_POS_SIX   0x40C00000U  /* +6.0  */
#define F_POS_42    0x42280000U  /* +42.0 */
#define F_NEG_42    0xC2280000U  /* -42.0 */
#define F_POS_256   0x43800000U  /* +256.0 */
#define F_POS_INF   0x7F800000U  /* +Inf  */
#define F_NEG_INF   0xFF800000U  /* -Inf  */
#define F_QNAN      0x7FC00000U  /* quiet NaN */

#define OUTPUT_ADDR 0x0200
#define TOTAL_TESTS 24

__attribute__((noinline)) static void halt_ok(void) { __asm__ volatile("hlt"); }
__attribute__((noinline)) static void fail_loop(void) {
    for (;;) {
    }
}

int main(void) {
    volatile uint32_t *output = (volatile uint32_t *)OUTPUT_ADDR;
    volatile uint16_t pass = 0;

    /* ---- Negation tests ---- */

    /* 1. neg(+1.0) == -1.0 */
    if (__negsf2(F_POS_ONE) == F_NEG_ONE) pass++;

    /* 2. neg(-1.0) == +1.0 */
    if (__negsf2(F_NEG_ONE) == F_POS_ONE) pass++;

    /* 3. neg(+0.0) == -0.0 */
    if (__negsf2(F_POS_ZERO) == F_NEG_ZERO) pass++;

    /* ---- Comparison tests ---- */

    /* 4. cmp(1.0, 2.0) < 0 */
    if (__lesf2(F_POS_ONE, F_POS_TWO) < 0) pass++;

    /* 5. cmp(2.0, 1.0) > 0 */
    if (__lesf2(F_POS_TWO, F_POS_ONE) > 0) pass++;

    /* 6. cmp(1.0, 1.0) == 0 */
    if (__lesf2(F_POS_ONE, F_POS_ONE) == 0) pass++;

    /* 7. cmp(-1.0, 1.0) < 0 */
    if (__lesf2(F_NEG_ONE, F_POS_ONE) < 0) pass++;

    /* 8. __lesf2(NaN, 1.0) returns +1 for unordered */
    if (__lesf2(F_QNAN, F_POS_ONE) > 0) pass++;

    /* 9. __gesf2(NaN, 1.0) returns -1 for unordered */
    if (__gesf2(F_QNAN, F_POS_ONE) < 0) pass++;

    /* 10. unord(NaN, 1.0) != 0 */
    if (__unordsf2(F_QNAN, F_POS_ONE) != 0) pass++;

    /* 11. unord(1.0, 2.0) == 0 */
    if (__unordsf2(F_POS_ONE, F_POS_TWO) == 0) pass++;

    /* ---- Int->float conversion tests ---- */

    /* 12. floatsisf(0) == 0.0 */
    if (__floatsisf(0) == F_POS_ZERO) pass++;

    /* 13. floatsisf(1) == 1.0 */
    if (__floatsisf(1) == F_POS_ONE) pass++;

    /* 14. floatsisf(-1) == -1.0 */
    if (__floatsisf(-1) == F_NEG_ONE) pass++;

    /* 15. floatsisf(42) == 42.0 */
    if (__floatsisf(42) == F_POS_42) pass++;

    /* 16. floatunsisf(0) == 0.0 */
    if (__floatunsisf(0) == F_POS_ZERO) pass++;

    /* 17. floatunsisf(1) == 1.0 */
    if (__floatunsisf(1) == F_POS_ONE) pass++;

    /* 18. floatunsisf(256) == 256.0 */
    if (__floatunsisf(256) == F_POS_256) pass++;

    /* ---- Float->int conversion tests ---- */

    /* 19. fixsfsi(1.0) == 1 */
    if (__fixsfsi(F_POS_ONE) == 1) pass++;

    /* 20. fixsfsi(-1.0) == -1 */
    if (__fixsfsi(F_NEG_ONE) == -1) pass++;

    /* 21. fixunssfsi(1.0) == 1 */
    if (__fixunssfsi(F_POS_ONE) == 1) pass++;

    /* 22. fixunssfsi(42.0) == 42 */
    if (__fixunssfsi(F_POS_42) == 42) pass++;

    /* ---- Arithmetic tests ---- */

    /* 23. add(1.0, 2.0) == 3.0 */
    if (__addsf3(F_POS_ONE, F_POS_TWO) == F_POS_THREE) pass++;

    /* 24. mul(2.0, 3.0) == 6.0 */
    if (__mulsf3(F_POS_TWO, F_POS_THREE) == F_POS_SIX) pass++;

    /* Write pass count to output */
    *output = (uint32_t)pass;

    if (pass == TOTAL_TESTS) {
        halt_ok();
    }

    fail_loop();
    return 0;
}
