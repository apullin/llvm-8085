#include <stdint.h>

static int16_t q7_8_mul(int16_t a, int16_t b) {
    return (int16_t)(((int32_t)a * (int32_t)b) >> 8);
}

int main(void) {
    const int16_t *A = (const int16_t *)0x0100;
    const int16_t *B = (const int16_t *)0x0108;
    volatile int16_t *C = (volatile int16_t *)0x0200;

    int16_t a0 = A[0];
    int16_t a1 = A[1];
    int16_t a2 = A[2];
    int16_t a3 = A[3];
    int16_t b0 = B[0];
    int16_t b1 = B[1];
    int16_t b2 = B[2];
    int16_t b3 = B[3];

    C[0] = q7_8_mul(a0, b0) + q7_8_mul(a1, b2);
    C[1] = q7_8_mul(a0, b1) + q7_8_mul(a1, b3);
    C[2] = q7_8_mul(a2, b0) + q7_8_mul(a3, b2);
    C[3] = q7_8_mul(a2, b1) + q7_8_mul(a3, b3);

    return 0;
}
