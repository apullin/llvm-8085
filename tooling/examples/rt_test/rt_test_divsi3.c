/*
 * compiler-rt __divsi3/__udivsi3/__modsi3/__umodsi3 unit tests for i8085
 *
 * Test vectors adapted from:
 *   llvm-project/compiler-rt/test/builtins/Unit/divsi3_test.c
 *   llvm-project/compiler-rt/test/builtins/Unit/udivsi3_test.c
 *   llvm-project/compiler-rt/test/builtins/Unit/modsi3_test.c
 *   llvm-project/compiler-rt/test/builtins/Unit/umodsi3_test.c
 *
 * Tests 32-bit signed/unsigned division and modulus.
 */

#include "rt_test.h"

static volatile si_int vsa, vsb;
static volatile su_int vua, vub;

/* ---- __divsi3 tests (signed 32-bit division) ---- */

static void test__divsi3(si_int a, si_int b, si_int expected) {
    vsa = a; vsb = b;
    si_int x = vsa / vsb;
    CHECK(x == expected);
}

/* ---- __udivsi3 tests (unsigned 32-bit division) ---- */

static void test__udivsi3(su_int a, su_int b, su_int expected) {
    vua = a; vub = b;
    su_int x = vua / vub;
    CHECK(x == expected);
}

/* ---- __modsi3 tests (signed 32-bit remainder) ---- */

static void test__modsi3(si_int a, si_int b, si_int expected) {
    vsa = a; vsb = b;
    si_int x = vsa % vsb;
    CHECK(x == expected);
}

/* ---- __umodsi3 tests (unsigned 32-bit remainder) ---- */

static void test__umodsi3(su_int a, su_int b, su_int expected) {
    vua = a; vub = b;
    su_int x = vua % vub;
    CHECK(x == expected);
}

int main(void) {
    test_init();

    /* ============ __divsi3 ============ */
    test__divsi3(0, 1, 0);
    test__divsi3(0, -1, 0);
    test__divsi3(2, 1, 2);
    test__divsi3(2, -1, -2);
    test__divsi3(-2, 1, -2);
    test__divsi3(-2, -1, 2);
    test__divsi3((si_int)0x80000000L, 1, (si_int)0x80000000L);
    test__divsi3((si_int)0x80000000L, -1, (si_int)0x80000000L);
    test__divsi3((si_int)0x80000000L, -2, 0x40000000L);
    test__divsi3((si_int)0x80000000L, 2, (si_int)0xC0000000L);

    /* Additional divsi3 */
    test__divsi3(5, 3, 1);
    test__divsi3(-5, 3, -1);
    test__divsi3(5, -3, -1);
    test__divsi3(-5, -3, 1);
    test__divsi3(1000000L, 127, 7874L);
    test__divsi3(-1000000L, 127, -7874L);

    /* ============ __udivsi3 (subset of original 121-entry table) ============ */
    test__udivsi3(0x00000000UL, 0x00000001UL, 0x00000000UL);
    test__udivsi3(0x00000000UL, 0x00000002UL, 0x00000000UL);
    test__udivsi3(0x00000001UL, 0x00000001UL, 0x00000001UL);
    test__udivsi3(0x00000001UL, 0x00000002UL, 0x00000000UL);
    test__udivsi3(0x00000002UL, 0x00000001UL, 0x00000002UL);
    test__udivsi3(0x00000002UL, 0x00000002UL, 0x00000001UL);
    test__udivsi3(0x00000003UL, 0x00000002UL, 0x00000001UL);
    test__udivsi3(0x00000003UL, 0x00000003UL, 0x00000001UL);
    test__udivsi3(0x00000010UL, 0x00000001UL, 0x00000010UL);
    test__udivsi3(0x00000010UL, 0x00000002UL, 0x00000008UL);
    test__udivsi3(0x00000010UL, 0x00000003UL, 0x00000005UL);
    test__udivsi3(0x00000010UL, 0x00000010UL, 0x00000001UL);
    test__udivsi3(0x078644FAUL, 0x00000001UL, 0x078644FAUL);
    test__udivsi3(0x078644FAUL, 0x00000002UL, 0x03C3227DUL);
    test__udivsi3(0x078644FAUL, 0x078644FAUL, 0x00000001UL);
    test__udivsi3(0x7FFFFFFFUL, 0x00000001UL, 0x7FFFFFFFUL);
    test__udivsi3(0x7FFFFFFFUL, 0x00000002UL, 0x3FFFFFFFUL);
    test__udivsi3(0x7FFFFFFFUL, 0x7FFFFFFFUL, 0x00000001UL);
    test__udivsi3(0x80000000UL, 0x00000001UL, 0x80000000UL);
    test__udivsi3(0x80000000UL, 0x00000002UL, 0x40000000UL);
    test__udivsi3(0x80000000UL, 0x80000000UL, 0x00000001UL);
    test__udivsi3(0xFFFFFFFFUL, 0x00000001UL, 0xFFFFFFFFUL);
    test__udivsi3(0xFFFFFFFFUL, 0x00000002UL, 0x7FFFFFFFUL);
    test__udivsi3(0xFFFFFFFFUL, 0x00000003UL, 0x55555555UL);
    test__udivsi3(0xFFFFFFFFUL, 0x00000010UL, 0x0FFFFFFFUL);
    test__udivsi3(0xFFFFFFFFUL, 0x7FFFFFFFUL, 0x00000002UL);
    test__udivsi3(0xFFFFFFFFUL, 0xFFFFFFFFUL, 0x00000001UL);

    /* ============ __modsi3 ============ */
    test__modsi3(0, 1, 0);
    test__modsi3(0, -1, 0);
    test__modsi3(5, 3, 2);
    test__modsi3(5, -3, 2);
    test__modsi3(-5, 3, -2);
    test__modsi3(-5, -3, -2);
    test__modsi3((si_int)0x80000000L, 1, 0);
    test__modsi3((si_int)0x80000000L, 2, 0);
    test__modsi3((si_int)0x80000000L, -2, 0);
    test__modsi3((si_int)0x80000000L, 3, -2);
    test__modsi3((si_int)0x80000000L, -3, -2);

    /* ============ __umodsi3 (subset) ============ */
    test__umodsi3(0x00000000UL, 0x00000001UL, 0x00000000UL);
    test__umodsi3(0x00000001UL, 0x00000001UL, 0x00000000UL);
    test__umodsi3(0x00000001UL, 0x00000002UL, 0x00000001UL);
    test__umodsi3(0x00000002UL, 0x00000001UL, 0x00000000UL);
    test__umodsi3(0x00000002UL, 0x00000003UL, 0x00000002UL);
    test__umodsi3(0x00000010UL, 0x00000003UL, 0x00000001UL);
    test__umodsi3(0x078644FAUL, 0x00000003UL, 0x00000000UL);
    test__umodsi3(0x7FFFFFFFUL, 0x00000002UL, 0x00000001UL);
    test__umodsi3(0x7FFFFFFFUL, 0x00000003UL, 0x00000001UL);
    test__umodsi3(0x80000000UL, 0x00000002UL, 0x00000000UL);
    test__umodsi3(0x80000000UL, 0x00000003UL, 0x00000002UL);
    test__umodsi3(0xFFFFFFFFUL, 0x00000002UL, 0x00000001UL);
    test__umodsi3(0xFFFFFFFFUL, 0x00000003UL, 0x00000000UL);
    test__umodsi3(0xFFFFFFFFUL, 0x00000010UL, 0x0000000FUL);
    test__umodsi3(0xFFFFFFFFUL, 0xFFFFFFFFUL, 0x00000000UL);
    test__umodsi3(0xFFFFFFFDUL, 0xFFFFFFFDUL, 0x00000000UL);
    test__umodsi3(0xFFFFFFFEUL, 0xFFFFFFFDUL, 0x00000001UL);

    return 0;
}
