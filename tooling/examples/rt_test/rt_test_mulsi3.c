/*
 * compiler-rt __mulsi3 unit test for i8085
 *
 * Test vectors adapted from:
 *   llvm-project/compiler-rt/test/builtins/Unit/mulsi3_test.c
 *
 * Tests 32-bit integer multiplication via the __mulsi3 builtin.
 */

#include "rt_test.h"

/* Prevent the compiler from constant-folding: force through memory */
static volatile si_int va, vb;

static void test__mulsi3(si_int a, si_int b, si_int expected) {
    va = a; vb = b;
    si_int x = va * vb;
    CHECK(x == expected);
}

int main(void) {
    test_init();

    /* Zero * anything = 0 */
    test__mulsi3(0, 0, 0);
    test__mulsi3(0, 1, 0);
    test__mulsi3(1, 0, 0);
    test__mulsi3(0, 10, 0);
    test__mulsi3(10, 0, 0);
    test__mulsi3(0, 0x7FFFFFFFL, 0);
    test__mulsi3(0x7FFFFFFFL, 0, 0);

    /* Zero * negative = 0 */
    test__mulsi3(0, -1, 0);
    test__mulsi3(-1, 0, 0);
    test__mulsi3(0, -10, 0);
    test__mulsi3(-10, 0, 0);
    test__mulsi3(0, (si_int)0x80000000L, 0);
    test__mulsi3((si_int)0x80000000L, 0, 0);

    /* Identity */
    test__mulsi3(1, 1, 1);
    test__mulsi3(1, 10, 10);
    test__mulsi3(10, 1, 10);
    test__mulsi3(1, 0x7FFFFFFFL, 0x7FFFFFFFL);
    test__mulsi3(0x7FFFFFFFL, 1, 0x7FFFFFFFL);

    /* Negative identity */
    test__mulsi3(1, -1, -1);
    test__mulsi3(1, -10, -10);
    test__mulsi3(-10, 1, -10);
    test__mulsi3(1, (si_int)0x80000000L, (si_int)0x80000000L);
    test__mulsi3((si_int)0x80000000L, 1, (si_int)0x80000000L);

    /* Near-overflow: isqrt(INT_MAX) ~= 46340 */
    test__mulsi3(46340L, 46340L, 2147395600L);
    test__mulsi3(-46340L, 46340L, -2147395600L);
    test__mulsi3(46340L, -46340L, -2147395600L);
    test__mulsi3(-46340L, -46340L, 2147395600L);

    /* Additional small multiplications */
    test__mulsi3(2, 3, 6);
    test__mulsi3(-2, 3, -6);
    test__mulsi3(2, -3, -6);
    test__mulsi3(-2, -3, 6);

    test__mulsi3(100, 100, 10000L);
    test__mulsi3(256, 256, 65536L);
    test__mulsi3(1000, 1000, 1000000L);
    test__mulsi3(-1000, 1000, -1000000L);

    /* Power of 2 tests */
    test__mulsi3(1, 0x10000L, 0x10000L);
    test__mulsi3(0x100L, 0x100L, 0x10000L);
    test__mulsi3(0x1000L, 0x1000L, 0x1000000L);

    /* Negative * negative */
    test__mulsi3(-1, -1, 1);
    test__mulsi3(-256, -256, 65536L);

    return 0;
}
