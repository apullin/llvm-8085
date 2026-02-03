#include <stddef.h>
#include <stdint.h>

#define OUT_ADDR 0x0300

struct Packed {
    uint8_t a;
    uint16_t b;
    uint8_t c;
} __attribute__((packed));

struct Aligned {
    uint8_t a;
    uint32_t b __attribute__((aligned(4)));
};

struct Bits {
    unsigned a : 3;
    unsigned b : 5;
    unsigned c : 6;
    unsigned d : 2;
};

union Pun {
    uint32_t u;
    uint8_t b[4];
};

union F32 {
    float f;
    uint32_t u;
};

static volatile float in_a = 1.5f;
static volatile float in_b = 2.25f;
static volatile int32_t in_i = 12345;

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

static inline void store64(volatile uint8_t *p, uint64_t v) {
    p[0] = (uint8_t)(v & 0xFFu);
    p[1] = (uint8_t)((v >> 8) & 0xFFu);
    p[2] = (uint8_t)((v >> 16) & 0xFFu);
    p[3] = (uint8_t)((v >> 24) & 0xFFu);
    p[4] = (uint8_t)((v >> 32) & 0xFFu);
    p[5] = (uint8_t)((v >> 40) & 0xFFu);
    p[6] = (uint8_t)((v >> 48) & 0xFFu);
    p[7] = (uint8_t)((v >> 56) & 0xFFu);
}

int main(void) {
    volatile uint8_t *out = (volatile uint8_t *)OUT_ADDR;

    struct Packed p = {0x12, 0x3456, 0x78};
    struct Aligned al = {0xAA, 0x11223344};
    struct Bits bits = {0, 0, 0, 0};
    union Pun pun;

    bits.a = 5;
    bits.b = 17;
    bits.c = 45;
    bits.d = 2;

    pun.u = 0x89ABCDEFu;

    uint64_t x = 0x123456789ABCDEF0ULL;
    uint64_t y = 0x0FEDCBA987654321ULL;
    uint64_t z = x + y;
    uint64_t m = (x ^ y) + (x >> 3) - (y << 1);
    uint64_t prod = (x & 0xFFFFu) * (y & 0xFFFFu);

    uint32_t u = 0xA1B2C3D4u;
    uint32_t rot = (u << 3) | (u >> (32 - 3));

    uint8_t sum = ((volatile uint8_t *)&p)[0] + ((volatile uint8_t *)&p)[1] + ((volatile uint8_t *)&p)[2];

    uint16_t off_p_c = (uint16_t)offsetof(struct Packed, c);
    uint16_t off_al_b = (uint16_t)offsetof(struct Aligned, b);

    float fa = in_a;
    float fb = in_b;
    float fc = fa + fb;
    float fd = fb - fa;
    float fe = fa * fb;
    float ff = fb / fa;
    float fg = (float)in_i;
    int32_t fi = (int32_t)fe;
    uint8_t fcmp = (fa < fb) ? 1u : 0u;

    union F32 fc_u = {.f = fc};
    union F32 fd_u = {.f = fd};
    union F32 fe_u = {.f = fe};
    union F32 ff_u = {.f = ff};
    union F32 fg_u = {.f = fg};

    *out++ = p.a;
    store16(out, p.b);
    out += 2;
    *out++ = p.c;
    *out++ = (uint8_t)off_p_c;

    *out++ = al.a;
    store32(out, al.b);
    out += 4;
    store16(out, off_al_b);
    out += 2;

    *out++ = (uint8_t)bits.a;
    *out++ = (uint8_t)bits.b;
    *out++ = (uint8_t)bits.c;
    *out++ = (uint8_t)bits.d;

    *out++ = sum;

    *out++ = pun.b[0];
    *out++ = pun.b[1];
    *out++ = pun.b[2];
    *out++ = pun.b[3];

    store64(out, z);
    out += 8;
    store64(out, m);
    out += 8;
    store64(out, prod);
    out += 8;
    store32(out, rot);
    out += 4;
    store32(out, fc_u.u);
    out += 4;
    store32(out, fd_u.u);
    out += 4;
    store32(out, fe_u.u);
    out += 4;
    store32(out, ff_u.u);
    out += 4;
    store32(out, fg_u.u);
    out += 4;
    store32(out, (uint32_t)fi);
    out += 4;
    *out++ = fcmp;

    return 0;
}
