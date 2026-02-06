#include <stdint.h>

// Single Q7.8 fixed-point multiply
// Input: two Q7.8 values at 0x0100 and 0x0102
// Output: result at 0x0200

static int16_t q7_8_mul(int16_t a, int16_t b) {
    int32_t res = 0;
    int32_t aa = a;
    int32_t bb = b;
    int32_t sign = 1;

    if (aa < 0) {
        aa = -aa;
        sign = -sign;
    }
    if (bb < 0) {
        bb = -bb;
        sign = -sign;
    }

    while (bb) {
        if (bb & 1) {
            res += aa;
        }
        aa <<= 1;
        bb >>= 1;
    }

    if (sign < 0) {
        res = -res;
    }
    return (int16_t)(res >> 8);
}

int main(void) {
    int16_t a = *(volatile int16_t *)0x0100;
    int16_t b = *(volatile int16_t *)0x0102;

    int16_t result = q7_8_mul(a, b);

    *(volatile int16_t *)0x0200 = result;

    return 0;
}
