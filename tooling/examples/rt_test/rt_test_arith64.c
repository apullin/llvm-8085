/*
 * compiler-rt 64-bit arithmetic unit tests for i8085
 *
 * Tests:
 *   __muldi3   (64-bit multiply)
 *   __divdi3   (64-bit signed division)
 *   __udivdi3  (64-bit unsigned division)
 *   __ashldi3  (64-bit left shift)
 *   __ashrdi3  (64-bit arithmetic right shift)
 *   __lshrdi3  (64-bit logical right shift)
 *
 * Test vectors adapted from:
 *   llvm-project/compiler-rt/test/builtins/Unit/muldi3_test.c
 *   llvm-project/compiler-rt/test/builtins/Unit/divdi3_test.c
 *   llvm-project/compiler-rt/test/builtins/Unit/udivdi3_test.c
 *   llvm-project/compiler-rt/test/builtins/Unit/ashldi3_test.c
 *   llvm-project/compiler-rt/test/builtins/Unit/ashrdi3_test.c
 *   llvm-project/compiler-rt/test/builtins/Unit/lshrdi3_test.c
 *
 * Note: 64-bit operations are very slow on the 8-bit i8085.
 * At -O0, the unoptimized code is too large for ROM, so the
 * division tests are gated with #ifdef __OPTIMIZE__.
 */

#include "rt_test.h"

static volatile di_int  vda, vdb;
static volatile du_int  vdua, vdub;
static volatile int     vshift;

/* ============ __muldi3 tests ============ */

static void test_muldi3(di_int a, di_int b, di_int expected) {
    vda = a; vdb = b;
    di_int x = vda * vdb;
    CHECK(x == expected);
}

/* ============ __divdi3 tests ============ */

static void test_divdi3(di_int a, di_int b, di_int expected) {
    vda = a; vdb = b;
    di_int x = vda / vdb;
    CHECK(x == expected);
}

/* ============ __udivdi3 tests ============ */

static void test_udivdi3(du_int a, du_int b, du_int expected) {
    vdua = a; vdub = b;
    du_int x = vdua / vdub;
    CHECK(x == expected);
}

/* ============ __ashldi3 tests ============ */

static void test_ashldi3(di_int a, int b, di_int expected) {
    vda = a; vshift = b;
    di_int x = vda << vshift;
    CHECK(x == expected);
}

/* ============ __ashrdi3 tests ============ */

static void test_ashrdi3(di_int a, int b, di_int expected) {
    vda = a; vshift = b;
    di_int x = vda >> vshift;
    CHECK(x == expected);
}

/* ============ __lshrdi3 tests ============ */

static void test_lshrdi3(du_int a, int b, du_int expected) {
    vdua = a; vshift = b;
    du_int x = vdua >> vshift;
    CHECK(x == expected);
}

int main(void) {
    test_init();

    /* ============ __muldi3 (from compiler-rt) ============ */
    test_muldi3(0, 0, 0);
    test_muldi3(0, 1, 0);
    test_muldi3(1, 0, 0);
    test_muldi3(0, 10, 0);
    test_muldi3(10, 0, 0);

    test_muldi3(0, -1, 0);
    test_muldi3(-1, 0, 0);
    test_muldi3(0, -10, 0);
    test_muldi3(-10, 0, 0);

    test_muldi3(1, 1, 1);
    test_muldi3(1, 10, 10);
    test_muldi3(10, 1, 10);

    test_muldi3(1, -1, -1);
    test_muldi3(1, -10, -10);
    test_muldi3(-10, 1, -10);

    /* Near-overflow: isqrt(INT64_MAX) ~= 3037000499 */
    test_muldi3(3037000499LL, 3037000499LL, 9223372030926249001LL);
    test_muldi3(-3037000499LL, 3037000499LL, -9223372030926249001LL);
    test_muldi3(3037000499LL, -3037000499LL, -9223372030926249001LL);
    test_muldi3(-3037000499LL, -3037000499LL, 9223372030926249001LL);

    /* Additional small ones */
    test_muldi3(2, 3, 6);
    test_muldi3(-2, 3, -6);
    test_muldi3(100, 100, 10000);
    test_muldi3(0x10000LL, 0x10000LL, 0x100000000LL);

    /* ============ __ashldi3 (from compiler-rt) ============ */
    test_ashldi3(0x0123456789ABCDEFLL, 0, 0x0123456789ABCDEFLL);
    test_ashldi3(0x0123456789ABCDEFLL, 1, 0x02468ACF13579BDELL);
    test_ashldi3(0x0123456789ABCDEFLL, 2, 0x048D159E26AF37BCLL);
    test_ashldi3(0x0123456789ABCDEFLL, 3, 0x091A2B3C4D5E6F78LL);
    test_ashldi3(0x0123456789ABCDEFLL, 4, 0x123456789ABCDEF0LL);

    test_ashldi3(0x0123456789ABCDEFLL, 28, 0x789ABCDEF0000000LL);
    test_ashldi3(0x0123456789ABCDEFLL, 29, (di_int)0xF13579BDE0000000LL);
    test_ashldi3(0x0123456789ABCDEFLL, 30, (di_int)0xE26AF37BC0000000LL);
    test_ashldi3(0x0123456789ABCDEFLL, 31, (di_int)0xC4D5E6F780000000LL);

    test_ashldi3(0x0123456789ABCDEFLL, 32, (di_int)0x89ABCDEF00000000LL);

    test_ashldi3(0x0123456789ABCDEFLL, 33, 0x13579BDE00000000LL);
    test_ashldi3(0x0123456789ABCDEFLL, 34, 0x26AF37BC00000000LL);
    test_ashldi3(0x0123456789ABCDEFLL, 35, 0x4D5E6F7800000000LL);
    test_ashldi3(0x0123456789ABCDEFLL, 36, (di_int)0x9ABCDEF000000000LL);

    test_ashldi3(0x0123456789ABCDEFLL, 60, (di_int)0xF000000000000000LL);
    test_ashldi3(0x0123456789ABCDEFLL, 61, (di_int)0xE000000000000000LL);
    test_ashldi3(0x0123456789ABCDEFLL, 62, (di_int)0xC000000000000000LL);
    test_ashldi3(0x0123456789ABCDEFLL, 63, (di_int)0x8000000000000000LL);

    /* ============ __ashrdi3 (from compiler-rt) ============ */
    /* Positive value */
    test_ashrdi3(0x0123456789ABCDEFLL, 0, 0x0123456789ABCDEFLL);
    test_ashrdi3(0x0123456789ABCDEFLL, 1, 0x0091A2B3C4D5E6F7LL);
    test_ashrdi3(0x0123456789ABCDEFLL, 2, 0x0048D159E26AF37BLL);
    test_ashrdi3(0x0123456789ABCDEFLL, 3, 0x002468ACF13579BDLL);
    test_ashrdi3(0x0123456789ABCDEFLL, 4, 0x00123456789ABCDELL);

    test_ashrdi3(0x0123456789ABCDEFLL, 28, 0x0000000012345678LL);
    test_ashrdi3(0x0123456789ABCDEFLL, 32, 0x0000000001234567LL);
    test_ashrdi3(0x0123456789ABCDEFLL, 36, 0x0000000000123456LL);

    test_ashrdi3(0x0123456789ABCDEFLL, 60, 0);
    test_ashrdi3(0x0123456789ABCDEFLL, 63, 0);

    /* Negative value */
    test_ashrdi3((di_int)0xFEDCBA9876543210LL, 0, (di_int)0xFEDCBA9876543210LL);
    test_ashrdi3((di_int)0xFEDCBA9876543210LL, 1, (di_int)0xFF6E5D4C3B2A1908LL);
    test_ashrdi3((di_int)0xFEDCBA9876543210LL, 4, (di_int)0xFFEDCBA987654321LL);

    test_ashrdi3((di_int)0xFEDCBA9876543210LL, 28, (di_int)0xFFFFFFFFEDCBA987LL);
    test_ashrdi3((di_int)0xFEDCBA9876543210LL, 32, (di_int)0xFFFFFFFFFEDCBA98LL);
    test_ashrdi3((di_int)0xFEDCBA9876543210LL, 36, (di_int)0xFFFFFFFFFFEDCBA9LL);

    test_ashrdi3((di_int)0xAEDCBA9876543210LL, 60, (di_int)0xFFFFFFFFFFFFFFFALL);
    test_ashrdi3((di_int)0xAEDCBA9876543210LL, 63, (di_int)0xFFFFFFFFFFFFFFFFLL);

    /* ============ __lshrdi3 (from compiler-rt) ============ */
    /* Positive value */
    test_lshrdi3(0x0123456789ABCDEFULL, 0, 0x0123456789ABCDEFULL);
    test_lshrdi3(0x0123456789ABCDEFULL, 1, 0x0091A2B3C4D5E6F7ULL);
    test_lshrdi3(0x0123456789ABCDEFULL, 4, 0x00123456789ABCDEULL);

    test_lshrdi3(0x0123456789ABCDEFULL, 28, 0x0000000012345678ULL);
    test_lshrdi3(0x0123456789ABCDEFULL, 32, 0x0000000001234567ULL);
    test_lshrdi3(0x0123456789ABCDEFULL, 36, 0x0000000000123456ULL);

    test_lshrdi3(0x0123456789ABCDEFULL, 60, 0);
    test_lshrdi3(0x0123456789ABCDEFULL, 63, 0);

    /* Value with high bit set (logical shift should NOT sign-extend) */
    test_lshrdi3(0xFEDCBA9876543210ULL, 0, 0xFEDCBA9876543210ULL);
    test_lshrdi3(0xFEDCBA9876543210ULL, 1, 0x7F6E5D4C3B2A1908ULL);
    test_lshrdi3(0xFEDCBA9876543210ULL, 4, 0x0FEDCBA987654321ULL);

    test_lshrdi3(0xFEDCBA9876543210ULL, 28, 0x0000000FEDCBA987ULL);
    test_lshrdi3(0xFEDCBA9876543210ULL, 32, 0x00000000FEDCBA98ULL);
    test_lshrdi3(0xFEDCBA9876543210ULL, 36, 0x000000000FEDCBA9ULL);

    test_lshrdi3(0xAEDCBA9876543210ULL, 60, 0x000000000000000AULL);
    test_lshrdi3(0xAEDCBA9876543210ULL, 63, 0x0000000000000001ULL);

    /* ============ __divdi3 and __udivdi3 ============ */
    /* These are gated at -O0 to avoid ROM overflow */
#ifdef __OPTIMIZE__
    /* __divdi3 (from compiler-rt) */
    test_divdi3(0, 1, 0);
    test_divdi3(0, -1, 0);
    test_divdi3(2, 1, 2);
    test_divdi3(2, -1, -2);
    test_divdi3(-2, 1, -2);
    test_divdi3(-2, -1, 2);
    test_divdi3((di_int)0x8000000000000000LL, 1, (di_int)0x8000000000000000LL);
    test_divdi3((di_int)0x8000000000000000LL, -1, (di_int)0x8000000000000000LL);
    test_divdi3((di_int)0x8000000000000000LL, -2, 0x4000000000000000LL);
    test_divdi3((di_int)0x8000000000000000LL, 2, (di_int)0xC000000000000000LL);

    /* Additional divdi3 */
    test_divdi3(100, 7, 14);
    test_divdi3(-100, 7, -14);
    test_divdi3(100, -7, -14);
    test_divdi3(-100, -7, 14);

    /* __udivdi3 (from compiler-rt) */
    test_udivdi3(0, 1, 0);
    test_udivdi3(2, 1, 2);
    test_udivdi3(0x8000000000000000ULL, 1, 0x8000000000000000ULL);
    test_udivdi3(0x8000000000000000ULL, 2, 0x4000000000000000ULL);
    test_udivdi3(0xFFFFFFFFFFFFFFFFULL, 2, 0x7FFFFFFFFFFFFFFFULL);

    /* Additional udivdi3 */
    test_udivdi3(100, 7, 14);
    test_udivdi3(0xFFFFFFFFFFFFFFFFULL, 1, 0xFFFFFFFFFFFFFFFFULL);
#endif

    return 0;
}
