// Test inputs for Q7.8 multiply
// 2.0 * 2.0 = 4.0
// Q7.8: 2.0 = 0x0200, expected result 4.0 = 0x0400

__attribute__((section(".input")))
const unsigned char input_data[] = {
    0x00, 0x02,  // a = 0x0200 = 2.0 in Q7.8 (at 0x0100)
    0x00, 0x02,  // b = 0x0200 = 2.0 in Q7.8 (at 0x0102)
};
