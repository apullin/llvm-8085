#include <stdint.h>

// Minimal test: count how many times we can right-shift until zero
// Input: 16-bit value in shift_test_input, linked at 0x0100
// Output: count at 0x0200

int main(void) {
    extern const uint16_t shift_test_input[1];
    uint16_t val = shift_test_input[0];
    uint16_t count = 0;

    while (val) {
        count++;
        val >>= 1;
    }

    *(volatile uint16_t *)0x0200 = count;

    return 0;
}
