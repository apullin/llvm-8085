#include <stdint.h>

int main(void) {
    volatile uint16_t *out = (volatile uint16_t *)0x0100;

    out[0] = 0;
    out[1] = 1;
    for (uint16_t i = 2; i < 16; i++) {
        out[i] = out[i - 1] + out[i - 2];
    }

    return 0;
}
