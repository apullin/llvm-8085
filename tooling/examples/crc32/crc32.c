/*
 * CRC32 benchmark for i8085
 *
 * Computes CRC32 (ISO 3309 / ITU-T V.42) over a 32-byte buffer loaded
 * from external memory at 0x0100.  Uses the bit-by-bit algorithm (no
 * lookup table) to keep code size small while exercising 32-bit shifts,
 * XOR, and byte processing.
 *
 * Input:  32 bytes at 0x0100
 * Output: 4-byte CRC32 (little-endian) at 0x0200
 */

#include <stdint.h>

#define INPUT_ADDR   0x0100
#define OUTPUT_ADDR  0x0200
#define INPUT_LEN    32

/* Standard CRC32 polynomial (reversed / reflected) */
#define CRC32_POLY   0xEDB88320u

/* Expected CRC32 for the test input vector */
#define EXPECTED_CRC 0x116656A0u

__attribute__((noinline)) static void halt_ok(void) { __asm__ volatile("hlt"); }
__attribute__((noinline)) static void fail_loop(void) {
    for (;;) {
    }
}

static uint32_t crc32_byte(uint32_t crc, uint8_t byte) {
    crc ^= (uint32_t)byte;
    for (uint8_t bit = 0; bit < 8; bit++) {
        if (crc & 1u) {
            crc = (crc >> 1) ^ CRC32_POLY;
        } else {
            crc >>= 1;
        }
    }
    return crc;
}

int main(void) {
    const uint8_t *input = (const uint8_t *)INPUT_ADDR;
    volatile uint32_t *output = (volatile uint32_t *)OUTPUT_ADDR;

    uint32_t crc = 0xFFFFFFFFu;

    for (uint8_t i = 0; i < INPUT_LEN; i++) {
        crc = crc32_byte(crc, input[i]);
    }

    crc ^= 0xFFFFFFFFu;

    *output = crc;

    if (crc == EXPECTED_CRC) {
        halt_ok();
    }

    fail_loop();
    return 0;
}
