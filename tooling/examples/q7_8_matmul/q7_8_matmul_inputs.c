#include <stdint.h>

__attribute__((section(".input")))
const int16_t q7_8_inputs[8] = {
    0x0100, 0x0080, // A: 1.0, 0.5
    0xFF00, 0x0200, // A: -1.0, 2.0
    0x0100, 0xFF80, // B: 1.0, -0.5
    0x0040, 0x0100  // B: 0.25, 1.0
};
