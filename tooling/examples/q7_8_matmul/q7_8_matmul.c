#include <stdint.h>

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
    const int16_t A[4] = {
        0x0100, 0x0080, // 1.0, 0.5
        0xFF00, 0x0200  // -1.0, 2.0
    };
    const int16_t B[4] = {
        0x0100, 0xFF80, // 1.0, -0.5
        0x0040, 0x0100  // 0.25, 1.0
    };

    volatile int16_t *C = (volatile int16_t *)0x0200;

    C[0] = q7_8_mul(A[0], B[0]) + q7_8_mul(A[1], B[2]);
    C[1] = q7_8_mul(A[0], B[1]) + q7_8_mul(A[1], B[3]);
    C[2] = q7_8_mul(A[2], B[0]) + q7_8_mul(A[3], B[2]);
    C[3] = q7_8_mul(A[2], B[1]) + q7_8_mul(A[3], B[3]);

    return 0;
}
