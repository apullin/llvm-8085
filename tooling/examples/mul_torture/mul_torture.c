/*
 * Multiply torture test for i8085
 *
 * Exercises all specialized multiply routines in the runtime library:
 *   __mulsi8        - signed 8x8->16
 *   __mulsi8_hi8    - signed 8x8->16, upper 8 bits (MULHS)
 *   __mulsi16       - signed 16x16->32
 *   __mulsi16_shr8  - signed 16x16->32, >>8, return i16 (Q7.8)
 *   __mulsi16_hi16  - signed 16x16->32, upper 16 bits
 *   __mulsi16_lo16  - signed 16x16->32, lower 16 bits
 *   __mulsi32_shr16 - signed 32x32->64, >>16, return i32 (Q15.16)
 *   __mulsi32_hi32  - signed 32x32->64, upper 32 bits
 *
 * Results are written to 0x0200 (130 bytes total).
 * The test halts on success; loops forever on failure.
 *
 * Each test group is in its own noinline function to keep register
 * pressure low and work around codegen issues in the 8085 backend.
 */

#include <stdint.h>

#define OUTPUT_ADDR 0x0200

/* ------------------------------------------------------------------ */
/* Helper store functions (little-endian byte writes)                  */
/* ------------------------------------------------------------------ */

static inline void store8(volatile uint8_t *p, uint8_t v) {
    p[0] = v;
}

static inline void store16(volatile uint8_t *p, uint16_t v) {
    p[0] = (uint8_t)(v & 0xFFu);
    p[1] = (uint8_t)((v >> 8) & 0xFFu);
}

static inline void store32_halves(volatile uint8_t *p,
                                  uint16_t lo16, uint16_t hi16) {
    p[0] = (uint8_t)(lo16 & 0xFFu);
    p[1] = (uint8_t)((lo16 >> 8) & 0xFFu);
    p[2] = (uint8_t)(hi16 & 0xFFu);
    p[3] = (uint8_t)((hi16 >> 8) & 0xFFu);
}

static inline void store32(volatile uint8_t *p, uint32_t v) {
    p[0] = (uint8_t)(v & 0xFFu);
    p[1] = (uint8_t)((v >> 8) & 0xFFu);
    p[2] = (uint8_t)((v >> 16) & 0xFFu);
    p[3] = (uint8_t)((v >> 24) & 0xFFu);
}

/* ------------------------------------------------------------------ */
/* Volatile test inputs -- prevents constant folding                   */
/* ------------------------------------------------------------------ */

/* 8-bit signed inputs (10 pairs) */
static volatile int8_t i8_a[] = {
    0, 1, 7, -1, -1, 127, -128, -128, 0, 100
};
static volatile int8_t i8_b[] = {
    0, 1, 13, 1, -1, 127, 127, -128, 127, -50
};

/* 8-bit signed inputs for hi8 tests (4 pairs) */
static volatile int8_t hi8_a[] = { 127, -128, 10, -128 };
static volatile int8_t hi8_b[] = { 127, 127, 10, -128 };

/* 16-bit signed inputs (10 pairs) */
static volatile int16_t i16_a[] = {
    0, 1, 256, -1, -1, 32767, -32768, -32768, 1000, 0
};
static volatile int16_t i16_b[] = {
    0, 1, 256, 1, -1, 32767, 32767, -32768, -500, 32767
};

/* 16-bit signed inputs for shr8 tests (5 pairs) */
static volatile int16_t shr8_a[] = { 256, 512, -256, 32767, 100 };
static volatile int16_t shr8_b[] = { 256, 128, 256, 32767, 200 };

/* 16-bit signed inputs for hi16 tests (4 pairs) */
static volatile int16_t hi16_a[] = { 256, 32767, -32768, 100 };
static volatile int16_t hi16_b[] = { 256, 32767, 32767, 100 };

/* 16-bit signed inputs for lo16 tests (4 pairs) */
static volatile int16_t lo16_a[] = { 256, 1000, -1, 32767 };
static volatile int16_t lo16_b[] = { 256, 33, -1, 2 };

/* 32-bit signed inputs for shr16 tests (5 pairs) */
static volatile int32_t shr16_a[] = { 65536, 0, 1, -65536, 256 };
static volatile int32_t shr16_b[] = { 65536, 12345, 65536, 65536, 256 };

/* 32-bit signed inputs for hi32 tests (5 pairs) */
static volatile int32_t hi32_a[] = { 65536, 0, 2147483647L, -1, -1 };
static volatile int32_t hi32_b[] = { 65536, 12345, 2147483647L, 1, -1 };

/* ------------------------------------------------------------------ */
/* noinline wrappers to force library calls at all optimization levels */
/* ------------------------------------------------------------------ */

/* 8x8 -> 16 signed: triggers __mulsi8 */
__attribute__((noinline))
static int16_t mul_s8x8_16(int8_t a, int8_t b) {
    return (int16_t)a * (int16_t)b;
}

/* 8x8 -> upper 8 signed: triggers __mulsi8_hi8 */
__attribute__((noinline))
static int8_t mul_s8x8_hi8(int8_t a, int8_t b) {
    return (int8_t)(((int16_t)a * (int16_t)b) >> 8);
}

/* 16x16 -> 32 signed: triggers __mulsi16
 * Returns lower and upper 16-bit halves via pointers to avoid
 * 32-bit shift codegen issues in the caller. */
__attribute__((noinline))
static void mul_s16x16_32(int16_t a, int16_t b,
                           uint16_t *lo, uint16_t *hi) {
    int32_t r = (int32_t)a * (int32_t)b;
    *lo = (uint16_t)(r & 0xFFFF);
    *hi = (uint16_t)((uint32_t)r >> 16);
}

/* 16x16 -> 32 >> 8, return i16: triggers __mulsi16_shr8 */
__attribute__((noinline))
static int16_t mul_s16x16_shr8(int16_t a, int16_t b) {
    return (int16_t)(((int32_t)a * (int32_t)b) >> 8);
}

/* 16x16 -> upper 16 signed: triggers __mulsi16_hi16 */
__attribute__((noinline))
static int16_t mul_s16x16_hi16(int16_t a, int16_t b) {
    return (int16_t)(((int32_t)a * (int32_t)b) >> 16);
}

/* 16x16 -> lower 16 signed: triggers __mulsi16_lo16 */
__attribute__((noinline))
static int16_t mul_s16x16_lo16(int16_t a, int16_t b) {
    return (int16_t)((int32_t)a * (int32_t)b);
}

/* 32x32 -> 64 >> 16, return i32: triggers __mulsi32_shr16 */
__attribute__((noinline))
static int32_t mul_s32x32_shr16(int32_t a, int32_t b) {
    return (int32_t)(((int64_t)a * (int64_t)b) >> 16);
}

/* 32x32 -> upper 32 signed: triggers __mulsi32_hi32 */
__attribute__((noinline))
static int32_t mul_s32x32_hi32(int32_t a, int32_t b) {
    return (int32_t)(((int64_t)a * (int64_t)b) >> 32);
}

/* ------------------------------------------------------------------ */
/* Halt / fail                                                         */
/* ------------------------------------------------------------------ */

__attribute__((noinline)) static void halt_ok(void) {
    __asm__ volatile("hlt");
}

__attribute__((noinline)) static void fail_loop(void) {
    for (;;) {}
}

/* ------------------------------------------------------------------ */
/* Expected results (computed by hand, used for self-check)            */
/* ------------------------------------------------------------------ */

/* 8x8->16 expected results (10 values) */
static const int16_t exp_s8x8_16[] = {
    0, 1, 91, -1, 1, 16129, -16256, 16384, 0, -5000
};

/* 8x8->hi8 expected results (4 values) */
static const int8_t exp_s8x8_hi8[] = {
    63, -64, 0, 64
};

/* 16x16->32 expected results as (lo16, hi16) pairs */
static const uint16_t exp_s16x16_32_lo[] = {
    0x0000, 0x0001, 0x0000, 0xFFFF, 0x0001,
    0x0001, 0x8000, 0x0000, 0x5EE0, 0x0000
};
static const uint16_t exp_s16x16_32_hi[] = {
    0x0000, 0x0000, 0x0001, 0xFFFF, 0x0000,
    0x3FFF, 0xC000, 0x4000, 0xFFF8, 0x0000
};

/* 16x16->shr8 expected results (5 values) */
static const int16_t exp_s16x16_shr8[] = {
    256, 256, -256, (int16_t)0xFF00, 78
};

/* 16x16->hi16 expected results (4 values) */
static const int16_t exp_s16x16_hi16[] = {
    1, 16383, -16384, 0
};

/* 16x16->lo16 expected results (4 values) */
static const int16_t exp_s16x16_lo16[] = {
    0, (int16_t)33000u, 1, (int16_t)65534u
};

/* 32x32->shr16 expected results (5 values)
 * 65536*65536=4294967296 >> 16 = 65536 = 0x00010000
 * 0*12345=0 >> 16 = 0
 * 1*65536=65536 >> 16 = 1
 * -65536*65536=-4294967296 >> 16 = -65536 = 0xFFFF0000
 * 256*256=65536 >> 16 = 1 */
static const int32_t exp_s32x32_shr16[] = {
    65536, 0, 1, -65536, 1
};

/* 32x32->hi32 expected results (5 values)
 * 65536*65536=4294967296 >> 32 = 1
 * 0*12345=0 >> 32 = 0
 * 2147483647*2147483647=4611686014132420609 >> 32 = 0x3FFFFFFF = 1073741823
 * -1*1=-1 >> 32 = -1 (0xFFFFFFFF)
 * -1*-1=1 >> 32 = 0 */
static const int32_t exp_s32x32_hi32[] = {
    1, 0, 1073741823L, -1, 0
};

/* ------------------------------------------------------------------ */
/* Test group functions -- noinline to reduce register pressure        */
/* ------------------------------------------------------------------ */

/* Test group 1: 8x8 -> 16 (__mulsi8)
 * 10 tests, 2 bytes each = 20 bytes */
__attribute__((noinline))
static uint8_t test_mulsi8(volatile uint8_t *out) {
    uint8_t ok = 1;
    for (uint8_t i = 0; i < 10; i++) {
        int16_t r = mul_s8x8_16(i8_a[i], i8_b[i]);
        store16(out, (uint16_t)r);
        out += 2;
        if (r != exp_s8x8_16[i]) ok = 0;
    }
    return ok;
}

/* Test group 2: 8x8 -> hi8 (__mulsi8_hi8)
 * 4 tests, 1 byte each = 4 bytes */
__attribute__((noinline))
static uint8_t test_mulsi8_hi8(volatile uint8_t *out) {
    uint8_t ok = 1;
    for (uint8_t i = 0; i < 4; i++) {
        int8_t r = mul_s8x8_hi8(hi8_a[i], hi8_b[i]);
        store8(out, (uint8_t)r);
        out += 1;
        if (r != exp_s8x8_hi8[i]) ok = 0;
    }
    return ok;
}

/* Test group 3: 16x16 -> 32 (__mulsi16)
 * 10 tests, 4 bytes each = 40 bytes */
__attribute__((noinline))
static uint8_t test_mulsi16(volatile uint8_t *out) {
    uint8_t ok = 1;
    for (uint8_t i = 0; i < 10; i++) {
        uint16_t lo, hi;
        mul_s16x16_32(i16_a[i], i16_b[i], &lo, &hi);
        store32_halves(out, lo, hi);
        out += 4;
        if (lo != exp_s16x16_32_lo[i] || hi != exp_s16x16_32_hi[i]) ok = 0;
    }
    return ok;
}

/* Test group 4: 16x16 -> shr8 (__mulsi16_shr8)
 * 5 tests, 2 bytes each = 10 bytes */
__attribute__((noinline))
static uint8_t test_mulsi16_shr8(volatile uint8_t *out) {
    uint8_t ok = 1;
    for (uint8_t i = 0; i < 5; i++) {
        int16_t r = mul_s16x16_shr8(shr8_a[i], shr8_b[i]);
        store16(out, (uint16_t)r);
        out += 2;
        if (r != exp_s16x16_shr8[i]) ok = 0;
    }
    return ok;
}

/* Test group 5: 16x16 -> hi16 (__mulsi16_hi16)
 * 4 tests, 2 bytes each = 8 bytes */
__attribute__((noinline))
static uint8_t test_mulsi16_hi16(volatile uint8_t *out) {
    uint8_t ok = 1;
    for (uint8_t i = 0; i < 4; i++) {
        int16_t r = mul_s16x16_hi16(hi16_a[i], hi16_b[i]);
        store16(out, (uint16_t)r);
        out += 2;
        if (r != exp_s16x16_hi16[i]) ok = 0;
    }
    return ok;
}

/* Test group 6: 16x16 -> lo16 (__mulsi16_lo16)
 * 4 tests, 2 bytes each = 8 bytes */
__attribute__((noinline))
static uint8_t test_mulsi16_lo16(volatile uint8_t *out) {
    uint8_t ok = 1;
    for (uint8_t i = 0; i < 4; i++) {
        int16_t r = mul_s16x16_lo16(lo16_a[i], lo16_b[i]);
        store16(out, (uint16_t)r);
        out += 2;
        if (r != exp_s16x16_lo16[i]) ok = 0;
    }
    return ok;
}

/* Test group 7: 32x32 -> shr16 (__mulsi32_shr16)
 * 5 tests, 4 bytes each = 20 bytes */
__attribute__((noinline))
static uint8_t test_mulsi32_shr16(volatile uint8_t *out) {
    uint8_t ok = 1;
    for (uint8_t i = 0; i < 5; i++) {
        int32_t r = mul_s32x32_shr16(shr16_a[i], shr16_b[i]);
        store32(out, (uint32_t)r);
        out += 4;
        if (r != exp_s32x32_shr16[i]) ok = 0;
    }
    return ok;
}

/* Test group 8: 32x32 -> hi32 (__mulsi32_hi32)
 * 5 tests, 4 bytes each = 20 bytes */
__attribute__((noinline))
static uint8_t test_mulsi32_hi32(volatile uint8_t *out) {
    uint8_t ok = 1;
    for (uint8_t i = 0; i < 5; i++) {
        int32_t r = mul_s32x32_hi32(hi32_a[i], hi32_b[i]);
        store32(out, (uint32_t)r);
        out += 4;
        if (r != exp_s32x32_hi32[i]) ok = 0;
    }
    return ok;
}

/* ------------------------------------------------------------------ */
/* Main                                                                */
/* ------------------------------------------------------------------ */

int main(void) {
    volatile uint8_t *out = (volatile uint8_t *)OUTPUT_ADDR;
    uint8_t ok = 1;

    /* Group 1: 20 bytes (0x0200 - 0x0213) */
    if (!test_mulsi8(out)) ok = 0;
    out += 20;

    /* Group 2: 4 bytes (0x0214 - 0x0217) */
    if (!test_mulsi8_hi8(out)) ok = 0;
    out += 4;

    /* Group 3: 40 bytes (0x0218 - 0x023F) */
    if (!test_mulsi16(out)) ok = 0;
    out += 40;

    /* Group 4: 10 bytes (0x0240 - 0x0249) */
    if (!test_mulsi16_shr8(out)) ok = 0;
    out += 10;

    /* Group 5: 8 bytes (0x024A - 0x0251) */
    if (!test_mulsi16_hi16(out)) ok = 0;
    out += 8;

    /* Group 6: 8 bytes (0x0252 - 0x0259) */
    if (!test_mulsi16_lo16(out)) ok = 0;
    out += 8;

    /* Group 7: 20 bytes (0x025A - 0x026D) */
    if (!test_mulsi32_shr16(out)) ok = 0;
    out += 20;

    /* Group 8: 20 bytes (0x026E - 0x0281) */
    if (!test_mulsi32_hi32(out)) ok = 0;

    /* Total: 20 + 4 + 40 + 10 + 8 + 8 + 20 + 20 = 130 bytes */

    if (ok) {
        halt_ok();
    }

    fail_loop();
    return 0;
}
