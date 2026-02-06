#include <stdint.h>

// Minimal test: count how many times we can right-shift until zero
// Input: 16-bit value at 0x0100
// Output: count at 0x0200

int main(void) {
    uint16_t val = *(volatile uint16_t *)0x0100;
    uint16_t count = 0;

    while (val) {
        count++;
        val >>= 1;
    }

    *(volatile uint16_t *)0x0200 = count;

    return 0;
}
