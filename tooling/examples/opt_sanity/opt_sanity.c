#include <stddef.h>
#include <stdint.h>

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

static inline uint32_t rotl32(uint32_t v, unsigned s) {
    return (uint32_t)((v << s) | (v >> (32 - s)));
}

static uint32_t mix32(uint32_t acc, uint32_t v) {
    acc ^= v;
    acc = acc * 0x9E3779B1u + 0x7F4A7C15u;
    acc ^= acc >> 16;
    return acc;
}

__attribute__((noinline)) static void halt_ok(void) { __asm__ volatile("hlt"); }
__attribute__((noinline)) static void fail_loop(void) {
    for (;;) {
    }
}

int main(void) {
    uint8_t buf[32] = {0x3C, 0xA5, 0x7E, 0x19, 0x42, 0xD0, 0x01, 0xFF,
                       0x10, 0x20, 0x30, 0x40, 0x55, 0xAA, 0x5A, 0xA5,
                       0x99, 0x00, 0x13, 0x37, 0xDE, 0xAD, 0xBE, 0xEF,
                       0x11, 0x22, 0x33, 0x44, 0x77, 0x88, 0xCC, 0xDD};
    uint32_t acc = 0x12345678u;

    for (unsigned i = 0; i < sizeof(buf); ++i) {
        acc = mix32(acc, (uint32_t)(buf[i] + i));
    }

    struct Packed p = {0x12, 0x3456, 0x78};
    struct Aligned al = {0xAA, 0x11223344u};
    struct Bits bits = {0, 0, 0, 0};
    union Pun pun;

    bits.a = 5;
    bits.b = 17;
    bits.c = 45;
    bits.d = 2;

    pun.u = 0x89ABCDEFu;

    acc = mix32(acc, p.a);
    acc = mix32(acc, p.b);
    acc = mix32(acc, p.c);
    acc = mix32(acc, (uint32_t)offsetof(struct Packed, c));
    acc = mix32(acc, al.b);
    acc = mix32(acc, (uint32_t)offsetof(struct Aligned, b));
    acc = mix32(acc, (uint32_t)(bits.a + bits.b + bits.c + bits.d));
    acc = mix32(acc, pun.u);

    uint64_t x = 0x123456789ABCDEF0ULL;
    uint64_t y = 0x0FEDCBA987654321ULL;
    uint64_t z = x + y;
    uint64_t m = (x ^ y) + (x >> 3) - (y << 1);

    acc = mix32(acc, (uint32_t)z);
    acc = mix32(acc, (uint32_t)(z >> 32));
    acc = mix32(acc, (uint32_t)m);
    acc = mix32(acc, (uint32_t)(m >> 32));

    void *vp = buf;
    uint8_t *bp = (uint8_t *)vp;
    uint32_t word = (uint32_t)bp[5] | ((uint32_t)bp[6] << 8) | ((uint32_t)bp[7] << 16) |
                    ((uint32_t)bp[8] << 24);
    acc = mix32(acc, word);

    acc ^= rotl32(acc, 7);

    if (acc == 0x88CA679Bu) {
        halt_ok();
    }

    fail_loop();
    return 0;
}
