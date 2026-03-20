/*
 * i8085 helper ABI matrix test.
 *
 * Goal:
 *   Catch ABI mismatches between frontend/backend call lowering and the
 *   hand-written builtin/helper entrypoints.
 *
 * Coverage buckets:
 *   - direct C calls into normal-C and sret helpers
 *   - indirect coverage for combined div/rem helpers that use the special
 *     i8085 builtin register-return ABI and cannot be spelled directly in C
 */

#include "rt_test.h"

extern di_int __adddi3(di_int, di_int);
extern di_int __subdi3(di_int, di_int);
extern di_int __anddi3(di_int, di_int);
extern di_int __ordi3(di_int, di_int);
extern di_int __xordi3(di_int, di_int);
extern di_int __mulsi32(si_int, si_int);
extern du_int __mului32(su_int, su_int);
extern di_int __divdi3(di_int, di_int);
extern di_int __moddi3(di_int, di_int);
extern du_int __udivdi3(du_int, du_int);
extern du_int __umoddi3(du_int, du_int);
extern du_int __udivmoddi4(du_int, du_int, du_int *);
extern di_int __ashldi3(di_int, int);
extern di_int __ashrdi3(di_int, int);
extern du_int __lshrdi3(du_int, int);

static volatile uint8_t  vu8a, vu8b;
static volatile int8_t   vs8a, vs8b;
static volatile uint16_t vu16a, vu16b;
static volatile int16_t  vs16a, vs16b;

static __attribute__((noinline)) uint16_t pack_udivrem8_const(uint8_t a) {
    uint8_t q = a / 3u;
    uint8_t r = a % 3u;
    return (uint16_t)q | ((uint16_t)r << 8);
}

static __attribute__((noinline)) uint16_t pack_sdivrem8_const(int8_t a) {
    int8_t q = a / 3;
    int8_t r = a % 3;
    return (uint16_t)(uint8_t)q | ((uint16_t)(uint8_t)r << 8);
}

static __attribute__((noinline)) uint16_t pack_udivrem8_var(uint8_t a,
                                                            uint8_t b) {
    uint8_t q = a / b;
    uint8_t r = a % b;
    return (uint16_t)q | ((uint16_t)r << 8);
}

static __attribute__((noinline)) uint16_t pack_sdivrem8_var(int8_t a,
                                                            int8_t b) {
    int8_t q = a / b;
    int8_t r = a % b;
    return (uint16_t)(uint8_t)q | ((uint16_t)(uint8_t)r << 8);
}

static __attribute__((noinline)) uint32_t pack_udivrem16_const(uint16_t a) {
    uint16_t q = a / 13u;
    uint16_t r = a % 13u;
    return (uint32_t)q | ((uint32_t)r << 16);
}

static __attribute__((noinline)) uint32_t pack_sdivrem16_const(int16_t a) {
    int16_t q = a / 13;
    int16_t r = a % 13;
    return (uint32_t)(uint16_t)q | ((uint32_t)(uint16_t)r << 16);
}

static __attribute__((noinline)) uint32_t pack_udivrem16_var(uint16_t a,
                                                             uint16_t b) {
    uint16_t q = a / b;
    uint16_t r = a % b;
    return (uint32_t)q | ((uint32_t)r << 16);
}

static __attribute__((noinline)) uint32_t pack_sdivrem16_var(int16_t a,
                                                             int16_t b) {
    int16_t q = a / b;
    int16_t r = a % b;
    return (uint32_t)(uint16_t)q | ((uint32_t)(uint16_t)r << 16);
}

static __attribute__((noinline)) void test_divrem_builtin_abi(void) {
    vu8a = 26u;
    vu8b = 13u;
    vs8a = -26;
    vs8b = 13;
    vu16a = 500u;
    vu16b = 37u;
    vs16a = -500;
    vs16b = 37;

    CHECK(pack_udivrem8_const(0u) == 0x0000u);
    CHECK(pack_udivrem8_const(26u) == 0x0208u);
    CHECK(pack_sdivrem8_const(-26) == 0xFEF8u);
    CHECK(pack_udivrem8_var(vu8a, vu8b) == 0x0002u);
    CHECK(pack_sdivrem8_var(vs8a, vs8b) == 0x00FEu);

    CHECK(pack_udivrem16_const(0u) == 0x00000000UL);
    CHECK(pack_udivrem16_const(500u) == 0x00060026UL);
    CHECK(pack_sdivrem16_const(-500) == 0xFFFAFFDAUL);
    CHECK(pack_udivrem16_var(vu16a, vu16b) == 0x0013000DUL);
    CHECK(pack_sdivrem16_var(vs16a, vs16b) == 0xFFEDFFF3UL);
}

static __attribute__((noinline)) void test_i64_c_abi_helpers(void) {
    CHECK(__adddi3(0x0123456789ABCDEFLL, 0x1111111111111111LL) ==
          0x123456789ABCDF00LL);
    CHECK(__subdi3(0x123456789ABCDEF0LL, 0x1111111111111111LL) ==
          0x0123456789ABCDDFLL);
    CHECK(__anddi3(0xF0F0F0F00FF00FF0LL, 0x12345678FFFF0000LL) ==
          0x103050700FF00000LL);
    CHECK(__ordi3(0xF0F0F0F00FF00FF0LL, 0x12345678FFFF0000LL) ==
          (di_int)0xF2F4F6F8FFFF0FF0LL);
    CHECK(__xordi3(0xF0F0F0F00FF00FF0LL, 0x12345678FFFF0000LL) ==
          (di_int)0xE2C4A688F00F0FF0LL);

    CHECK(__ashldi3(0x0123456789ABCDEFLL, 8) == 0x23456789ABCDEF00LL);
    CHECK(__ashrdi3((di_int)0xFEDCBA9876543210LL, 8) ==
          (di_int)0xFFFEDCBA98765432LL);
    CHECK(__lshrdi3(0xFEDCBA9876543210ULL, 8) == 0x00FEDCBA98765432ULL);

    CHECK(__divdi3(-1000LL, 7LL) == -142LL);
    CHECK(__moddi3(-1000LL, 7LL) == -6LL);
    CHECK(__udivdi3(1000ULL, 7ULL) == 142ULL);
    CHECK(__umoddi3(1000ULL, 7ULL) == 6ULL);

    CHECK(__mulsi32(30000L, -7L) == -210000LL);
    CHECK(__mului32(70000UL, 600UL) == 42000000ULL);
}

static __attribute__((noinline)) void test_divmoddi4_helper(void) {
    du_int rem = 0;
    du_int quo = __udivmoddi4(1000ULL, 37ULL, &rem);
    CHECK(quo == 27ULL);
    CHECK(rem == 1ULL);
}

int main(void) {
    test_init();

    test_divrem_builtin_abi();
    test_i64_c_abi_helpers();
    test_divmoddi4_helper();

    return 0;
}
