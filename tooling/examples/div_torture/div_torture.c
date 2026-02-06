/*
 * Division/remainder torture test for i8085
 *
 * Exercises all division and remainder routines:
 *   __udiv8,  __urem8,  __sdiv8,  __srem8
 *   __udiv16, __urem16, __sdiv16, __srem16
 *   __udiv32, __urem32, __sdiv32, __srem32
 *
 * Also exercises the i64 shift and add/sub libcalls:
 *   __ashldi3, __lshrdi3, __ashrdi3
 *   __adddi3,  __subdi3
 *
 * Results are written to 0x0200 (output area).
 * Test halts on success; loops forever on failure.
 */

#include <stdint.h>

#define OUTPUT_ADDR 0x0200

static inline void store8(volatile uint8_t *p, uint8_t v) {
    p[0] = v;
}

static inline void store16(volatile uint8_t *p, uint16_t v) {
    p[0] = (uint8_t)(v & 0xFFu);
    p[1] = (uint8_t)((v >> 8) & 0xFFu);
}

static inline void store32(volatile uint8_t *p, uint32_t v) {
    p[0] = (uint8_t)(v & 0xFFu);
    p[1] = (uint8_t)((v >> 8) & 0xFFu);
    p[2] = (uint8_t)((v >> 16) & 0xFFu);
    p[3] = (uint8_t)((v >> 24) & 0xFFu);
}

__attribute__((noinline)) static void halt_ok(void) { __asm__ volatile("hlt"); }
__attribute__((noinline)) static void fail_loop(void) {
    for (;;) {
    }
}

static volatile uint8_t *const out = (volatile uint8_t *)OUTPUT_ADDR;

/* ------------------------------------------------------------ */
/* 8-bit division tests                                          */
/* ------------------------------------------------------------ */
__attribute__((noinline))
static uint8_t test_div8(void) {
    volatile uint8_t *p = out;
    uint8_t ok = 1;

    /* Unsigned: 200 / 7 = 28, 200 % 7 = 4 */
    uint8_t uq = (uint8_t)200u / (uint8_t)7u;
    uint8_t ur = (uint8_t)200u % (uint8_t)7u;
    store8(p, uq); p += 1;
    store8(p, ur); p += 1;
    if (uq != 28) ok = 0;
    if (ur != 4) ok = 0;

    /* Unsigned: 255 / 16 = 15, 255 % 16 = 15 */
    uq = (uint8_t)255u / (uint8_t)16u;
    ur = (uint8_t)255u % (uint8_t)16u;
    store8(p, uq); p += 1;
    store8(p, ur); p += 1;
    if (uq != 15) ok = 0;
    if (ur != 15) ok = 0;

    /* Signed: -100 / 7 = -14, -100 % 7 = -2 */
    int8_t sq = (int8_t)(-100) / (int8_t)7;
    int8_t sr = (int8_t)(-100) % (int8_t)7;
    store8(p, (uint8_t)sq); p += 1;
    store8(p, (uint8_t)sr); p += 1;
    if (sq != -14) ok = 0;
    if (sr != -2) ok = 0;

    /* Signed: 100 / -7 = -14, 100 % -7 = 2 */
    sq = (int8_t)100 / (int8_t)(-7);
    sr = (int8_t)100 % (int8_t)(-7);
    store8(p, (uint8_t)sq); p += 1;
    store8(p, (uint8_t)sr); p += 1;
    if (sq != -14) ok = 0;
    if (sr != 2) ok = 0;

    /* Unsigned: 0 / 5 = 0 */
    uq = (uint8_t)0 / (uint8_t)5;
    store8(p, uq); p += 1;
    if (uq != 0) ok = 0;

    /* Unsigned: 100 / 1 = 100 */
    uq = (uint8_t)100 / (uint8_t)1;
    store8(p, uq); p += 1;
    if (uq != 100) ok = 0;

    return ok;
}

/* ------------------------------------------------------------ */
/* 16-bit division tests                                         */
/* ------------------------------------------------------------ */
__attribute__((noinline))
static uint8_t test_div16(void) {
    volatile uint8_t *p = out + 16;
    uint8_t ok = 1;

    /* Unsigned: 50000 / 123 = 406, 50000 % 123 = 62 */
    uint16_t uq = (uint16_t)50000u / (uint16_t)123u;
    uint16_t ur = (uint16_t)50000u % (uint16_t)123u;
    store16(p, uq); p += 2;
    store16(p, ur); p += 2;
    if (uq != 406) ok = 0;
    if (ur != 62) ok = 0;

    /* Unsigned: 65535 / 256 = 255, 65535 % 256 = 255 */
    uq = (uint16_t)65535u / (uint16_t)256u;
    ur = (uint16_t)65535u % (uint16_t)256u;
    store16(p, uq); p += 2;
    store16(p, ur); p += 2;
    if (uq != 255) ok = 0;
    if (ur != 255) ok = 0;

    /* Signed: -30000 / 123 = -243, -30000 % 123 = -111 */
    int16_t sq = (int16_t)(-30000) / (int16_t)123;
    int16_t sr = (int16_t)(-30000) % (int16_t)123;
    store16(p, (uint16_t)sq); p += 2;
    store16(p, (uint16_t)sr); p += 2;
    if (sq != -243) ok = 0;
    if (sr != -111) ok = 0;

    /* Signed: 30000 / -123 = -243, 30000 % -123 = 111 */
    sq = (int16_t)30000 / (int16_t)(-123);
    sr = (int16_t)30000 % (int16_t)(-123);
    store16(p, (uint16_t)sq); p += 2;
    store16(p, (uint16_t)sr); p += 2;
    if (sq != -243) ok = 0;
    if (sr != 111) ok = 0;

    return ok;
}

/* ------------------------------------------------------------ */
/* 32-bit division tests                                         */
/* ------------------------------------------------------------ */
__attribute__((noinline))
static uint8_t test_div32(void) {
    volatile uint8_t *p = out + 48;
    uint8_t ok = 1;

    /* Unsigned: 1000000 / 127 = 7874, 1000000 % 127 = 2 */
    uint32_t uq = (uint32_t)1000000ul / (uint32_t)127u;
    uint32_t ur = (uint32_t)1000000ul % (uint32_t)127u;
    store32(p, uq); p += 4;
    store32(p, ur); p += 4;
    if (uq != 7874) ok = 0;
    if (ur != 2) ok = 0;

    /* Unsigned: 0xFFFFFFFF / 0x10000 = 0xFFFF, 0xFFFFFFFF % 0x10000 = 0xFFFF */
    uq = 0xFFFFFFFFul / 0x10000ul;
    ur = 0xFFFFFFFFul % 0x10000ul;
    store32(p, uq); p += 4;
    store32(p, ur); p += 4;
    if (uq != 0xFFFF) ok = 0;
    if (ur != 0xFFFF) ok = 0;

    /* Signed: -1000000 / 127 = -7874, -1000000 % 127 = -2 */
    int32_t sq = (int32_t)(-1000000) / (int32_t)127;
    int32_t sr = (int32_t)(-1000000) % (int32_t)127;
    store32(p, (uint32_t)sq); p += 4;
    store32(p, (uint32_t)sr); p += 4;
    if (sq != -7874) ok = 0;
    if (sr != -2) ok = 0;

    return ok;
}

/* ------------------------------------------------------------ */
/* i64 shift tests                                               */
/* ------------------------------------------------------------ */
__attribute__((noinline))
static uint8_t test_shift64(void) {
    volatile uint8_t *p = out + 80;
    uint8_t ok = 1;

    uint64_t v = 0x0123456789ABCDEFull;

    /* Left shift by 4 */
    uint64_t shl4 = v << 4;
    store32(p, (uint32_t)(shl4 & 0xFFFFFFFFul)); p += 4;
    store32(p, (uint32_t)(shl4 >> 32)); p += 4;
    if (shl4 != 0x123456789ABCDEF0ull) ok = 0;

    /* Logical right shift by 8 */
    uint64_t lshr8 = v >> 8;
    store32(p, (uint32_t)(lshr8 & 0xFFFFFFFFul)); p += 4;
    store32(p, (uint32_t)(lshr8 >> 32)); p += 4;
    if (lshr8 != 0x000123456789ABCDull) ok = 0;

    /* Arithmetic right shift by 4 on negative value */
    int64_t sv = (int64_t)0xFEDCBA9876543210ull;
    int64_t ashr4 = sv >> 4;
    store32(p, (uint32_t)((uint64_t)ashr4 & 0xFFFFFFFFul)); p += 4;
    store32(p, (uint32_t)((uint64_t)ashr4 >> 32)); p += 4;
    if (ashr4 != (int64_t)0xFFEDCBA987654321ull) ok = 0;

    /* Left shift by 32 (byte shuffle only) */
    uint64_t shl32 = v << 32;
    store32(p, (uint32_t)(shl32 & 0xFFFFFFFFul)); p += 4;
    store32(p, (uint32_t)(shl32 >> 32)); p += 4;
    if (shl32 != 0x89ABCDEF00000000ull) ok = 0;

    return ok;
}

/* ------------------------------------------------------------ */
/* i64 add/sub tests                                             */
/* ------------------------------------------------------------ */
__attribute__((noinline))
static uint8_t test_addsub64(void) {
    volatile uint8_t *p = out + 120;
    uint8_t ok = 1;

    int64_t a = (int64_t)0x123456789ABCDEFull;
    int64_t b = (int64_t)0xFEDCBA9876543210ull;
    int64_t sum = a + b;
    store32(p, (uint32_t)((uint64_t)sum & 0xFFFFFFFFul)); p += 4;
    store32(p, (uint32_t)((uint64_t)sum >> 32)); p += 4;
    /* 0x0123456789ABCDEF + 0xFEDCBA9876543210 = 0xFFFFFFFFFFFFFFFF */
    if ((uint64_t)sum != 0xFFFFFFFFFFFFFFFFull) ok = 0;

    int64_t diff = a - b;
    store32(p, (uint32_t)((uint64_t)diff & 0xFFFFFFFFul)); p += 4;
    store32(p, (uint32_t)((uint64_t)diff >> 32)); p += 4;
    /* 0x0123456789ABCDEF - 0xFEDCBA9876543210 = 0x02468ACF13579BDF */
    if ((uint64_t)diff != 0x02468ACF13579BDFull) ok = 0;

    return ok;
}

/* ------------------------------------------------------------ */
/* i64 bitwise tests (AND, OR, XOR)                              */
/* ------------------------------------------------------------ */
__attribute__((noinline))
static uint8_t test_bitwise64(void) {
    volatile uint8_t *p = out + 152;
    uint8_t ok = 1;

    uint64_t a = 0x0123456789ABCDEFull;
    uint64_t b = 0xFEDCBA9876543210ull;

    /* AND: 0x0123456789ABCDEF & 0xFEDCBA9876543210 = 0x0000000000000000
     * (b is the bitwise complement of a, so AND is zero) */
    uint64_t r_and = a & b;
    store32(p, (uint32_t)(r_and & 0xFFFFFFFFul)); p += 4;
    store32(p, (uint32_t)(r_and >> 32)); p += 4;
    if (r_and != 0x0000000000000000ull) ok = 0;

    /* OR: a | ~a = all ones */
    uint64_t r_or = a | b;
    store32(p, (uint32_t)(r_or & 0xFFFFFFFFul)); p += 4;
    store32(p, (uint32_t)(r_or >> 32)); p += 4;
    if (r_or != 0xFFFFFFFFFFFFFFFFull) ok = 0;

    /* XOR: a ^ ~a = all ones */
    uint64_t r_xor = a ^ b;
    store32(p, (uint32_t)(r_xor & 0xFFFFFFFFul)); p += 4;
    store32(p, (uint32_t)(r_xor >> 32)); p += 4;
    if (r_xor != 0xFFFFFFFFFFFFFFFFull) ok = 0;

    /* AND with all-ones */
    uint64_t all_ones = 0xFFFFFFFFFFFFFFFFull;
    uint64_t r_and2 = a & all_ones;
    if (r_and2 != a) ok = 0;

    /* XOR with self = 0 */
    uint64_t r_xor2 = a ^ a;
    if (r_xor2 != 0) ok = 0;

    /* OR with zero = self */
    uint64_t r_or2 = a | 0;
    if (r_or2 != a) ok = 0;

    return ok;
}

/* ------------------------------------------------------------ */
/* i64 compare tests                                             */
/* ------------------------------------------------------------ */
__attribute__((noinline))
static uint8_t test_compare64(void) {
    uint8_t ok = 1;

    /* Unsigned compare */
    uint64_t a = 0x0000000100000000ull;
    uint64_t b = 0x00000000FFFFFFFFull;
    if (!(a > b)) ok = 0;
    if (!(b < a)) ok = 0;

    /* Equal */
    uint64_t c = 0x123456789ABCDEFull;
    uint64_t d = 0x123456789ABCDEFull;
    if (!(c == d)) ok = 0;
    if (c != d) ok = 0;

    /* Signed compare: positive vs negative */
    int64_t sp = 1;
    int64_t sn = -1;
    if (!(sp > sn)) ok = 0;
    if (!(sn < sp)) ok = 0;

    /* Signed compare: both negative */
    int64_t sn1 = -100;
    int64_t sn2 = -200;
    if (!(sn1 > sn2)) ok = 0;
    if (!(sn2 < sn1)) ok = 0;

    /* Off-by-one */
    uint64_t x = 0x8000000000000000ull;
    uint64_t y = 0x7FFFFFFFFFFFFFFFull;
    if (!(x > y)) ok = 0;  /* unsigned: 0x8... > 0x7... */

    /* Signed: 0x8000... is INT64_MIN */
    int64_t sx = (int64_t)x;
    int64_t sy = (int64_t)y;
    if (!(sx < sy)) ok = 0;  /* signed: INT64_MIN < INT64_MAX */

    return ok;
}

/* ------------------------------------------------------------ */
/* i64 negate tests                                              */
/* ------------------------------------------------------------ */
__attribute__((noinline))
static uint8_t test_negate64(void) {
    uint8_t ok = 1;

    /* Negate 1 -> -1 */
    int64_t a = 1;
    int64_t na = -a;
    if (na != -1) ok = 0;

    /* Negate -1 -> 1 */
    int64_t b = -1;
    int64_t nb = -b;
    if (nb != 1) ok = 0;

    /* Negate 0 -> 0 */
    int64_t c = 0;
    int64_t nc = -c;
    if (nc != 0) ok = 0;

    /* Negate a larger value */
    int64_t d = 0x0123456789ABCDEFll;
    int64_t nd = -d;
    if ((uint64_t)nd != 0xFEDCBA9876543211ull) ok = 0;

    return ok;
}

/* ------------------------------------------------------------ */
/* i64 division tests                                            */
/* ------------------------------------------------------------ */
__attribute__((noinline))
static uint8_t test_div64(void) {
    volatile uint8_t *p = out + 192;
    uint8_t ok = 1;

    /* Unsigned: 100 / 7 = 14, 100 % 7 = 2 */
    uint64_t uq = (uint64_t)100ull / (uint64_t)7ull;
    uint64_t ur = (uint64_t)100ull % (uint64_t)7ull;
    store32(p, (uint32_t)(uq & 0xFFFFFFFFul)); p += 4;
    store32(p, (uint32_t)(uq >> 32)); p += 4;
    if (uq != 14) ok = 0;
    if (ur != 2) ok = 0;

    /* Unsigned: 0xFFFFFFFFFFFFFFFF / 2 = 0x7FFFFFFFFFFFFFFF */
    uq = 0xFFFFFFFFFFFFFFFFull / 2ull;
    ur = 0xFFFFFFFFFFFFFFFFull % 2ull;
    store32(p, (uint32_t)(uq & 0xFFFFFFFFul)); p += 4;
    store32(p, (uint32_t)(uq >> 32)); p += 4;
    if (uq != 0x7FFFFFFFFFFFFFFFull) ok = 0;
    if (ur != 1) ok = 0;

    /* Unsigned: divide by 1 */
    uq = 0x123456789ABCDEFull / 1ull;
    if (uq != 0x123456789ABCDEFull) ok = 0;

    /* Unsigned: 0 / anything = 0 */
    uq = 0ull / 42ull;
    if (uq != 0) ok = 0;

    /* Signed: -100 / 7 = -14, -100 % 7 = -2 */
    int64_t sq = (int64_t)(-100) / (int64_t)7;
    int64_t sr = (int64_t)(-100) % (int64_t)7;
    if (sq != -14) ok = 0;
    if (sr != -2) ok = 0;

    /* Signed: 100 / -7 = -14, 100 % -7 = 2 */
    sq = (int64_t)100 / (int64_t)(-7);
    sr = (int64_t)100 % (int64_t)(-7);
    if (sq != -14) ok = 0;
    if (sr != 2) ok = 0;

    /* Signed: -100 / -7 = 14, -100 % -7 = -2 */
    sq = (int64_t)(-100) / (int64_t)(-7);
    sr = (int64_t)(-100) % (int64_t)(-7);
    if (sq != 14) ok = 0;
    if (sr != -2) ok = 0;

    return ok;
}

int main(void) {
    uint8_t ok = 1;

    if (!test_div8()) ok = 0;
    if (!test_div16()) ok = 0;
    if (!test_div32()) ok = 0;
    if (!test_shift64()) ok = 0;
    if (!test_addsub64()) ok = 0;
    if (!test_bitwise64()) ok = 0;
    if (!test_compare64()) ok = 0;
    if (!test_negate64()) ok = 0;
    if (!test_div64()) ok = 0;

    /* Write pass/fail indicator */
    store8(out + 255, ok);

    if (ok) {
        halt_ok();
    }
    fail_loop();
    return 0;
}
