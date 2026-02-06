#include <stdint.h>

// Minimal test with volatile to prevent loop optimization
// Input: 16-bit value at 0x0100
// Output: count at 0x0200

int main(void) {
    volatile uint16_t val = *(volatile uint16_t *)0x0100;
    uint16_t count = 0;

    while (val) {
        count++;
        val = val >> 1;  // Use volatile to prevent loop xform
    }

    *(volatile uint16_t *)0x0200 = count;

    return 0;
}
