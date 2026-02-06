// Test input: 0x0200 (512) should give count=10 (2^9 = 512, needs 10 shifts)
__attribute__((section(".input")))
const unsigned char input_data[] = {
    0x00, 0x02,  // val = 0x0200 = 512
};
