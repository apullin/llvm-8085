/*
 * Bubble sort benchmark for i8085
 *
 * Sorts an array of 16 x int16_t values using bubble sort.
 * Exercises 16-bit signed comparisons, swaps, nested loops,
 * and memory load/store patterns.
 *
 * Input:  16 x int16_t at 0x0100 (32 bytes)
 * Output: sorted array at 0x0200 (32 bytes)
 *
 * Verification: checks that first element == -32768 (0x8000)
 *               and last element == 32767 (0x7FFF).
 */

#include <stdint.h>

#define INPUT_ADDR   0x0100
#define OUTPUT_ADDR  0x0200
#define ARRAY_LEN    16

#define EXPECTED_FIRST ((int16_t)-32768)
#define EXPECTED_LAST  ((int16_t)32767)

__attribute__((noinline)) static void halt_ok(void) { __asm__ volatile("hlt"); }
__attribute__((noinline)) static void fail_loop(void) {
    for (;;) {
    }
}

int main(void) {
    const int16_t *input = (const int16_t *)INPUT_ADDR;
    volatile int16_t *output = (volatile int16_t *)OUTPUT_ADDR;

    /* Copy input to a local working array */
    int16_t arr[ARRAY_LEN];
    for (uint8_t i = 0; i < ARRAY_LEN; i++) {
        arr[i] = input[i];
    }

    /* Bubble sort (ascending) */
    for (uint8_t i = 0; i < ARRAY_LEN - 1; i++) {
        for (uint8_t j = 0; j < ARRAY_LEN - 1 - i; j++) {
            if (arr[j] > arr[j + 1]) {
                int16_t tmp = arr[j];
                arr[j] = arr[j + 1];
                arr[j + 1] = tmp;
            }
        }
    }

    /* Write sorted array to output */
    for (uint8_t i = 0; i < ARRAY_LEN; i++) {
        output[i] = arr[i];
    }

    /* Verify first and last elements */
    if (arr[0] == EXPECTED_FIRST && arr[ARRAY_LEN - 1] == EXPECTED_LAST) {
        halt_ok();
    }

    fail_loop();
    return 0;
}
