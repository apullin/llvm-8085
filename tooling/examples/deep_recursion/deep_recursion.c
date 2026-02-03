#include <stdint.h>

#ifndef DEPTH
#define DEPTH 64u
#endif

__attribute__((noinline)) static uint8_t sum_down(uint8_t n) {
    if (n == 0) {
        return 0;
    }

    return (uint8_t)(n + sum_down((uint8_t)(n - 1u)));
}

__attribute__((noinline)) static void halt_ok(void) { __asm__ volatile("hlt"); }
__attribute__((noinline)) static void fail_loop(void) {
    for (;;) {
    }
}

int main(void) {
    const uint8_t depth = (uint8_t)DEPTH;
    const uint8_t expected = 0x20u;
    uint8_t res = sum_down(depth);
    volatile uint8_t *out = (volatile uint8_t *)0x0400u;
    *out = res;

    if (res == expected) {
        halt_ok();
    }

    fail_loop();
    return 0;
}
