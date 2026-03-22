// Test inputs for Q7.8 multiply
// 2.0 * 2.0 = 4.0
// Q7.8: 2.0 = 0x0200, expected result 4.0 = 0x0400

#include <stdint.h>

__attribute__((section(".input")))
const int16_t q7_8_simple_input[2] = {
    0x0200,  // a = 0x0200 = 2.0 in Q7.8 (at 0x0100)
    0x0200,  // b = 0x0200 = 2.0 in Q7.8 (at 0x0102)
};
