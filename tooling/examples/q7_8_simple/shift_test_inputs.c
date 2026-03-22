// Test input: 0x0200 (512) should give count=10 (2^9 = 512, needs 10 shifts)
#include <stdint.h>

__attribute__((section(".input")))
const uint16_t shift_test_input[1] = {
    0x0200,  // val = 0x0200 = 512
};
