/*
 * Bubble sort benchmark input data.
 *
 * 16 x int16_t values placed at address 0x0100 via the .input section.
 * Includes extremes (INT16_MIN, INT16_MAX), zero, positives, negatives,
 * and duplicates (-1 appears twice).
 *
 * Unsorted: 1024 -512 32767 -32768 0 255 -1 100 -100 500 -500 1 -1 42 -42 7777
 * Sorted:  -32768 -512 -500 -100 -42 -1 -1 0 1 42 100 255 500 1024 7777 32767
 */

#include <stdint.h>

__attribute__((section(".input")))
const int16_t bubble_sort_input[16] = {
    1024, -512, 32767, -32768,
    0,    255,  -1,    100,
    -100, 500,  -500,  1,
    -1,   42,   -42,   7777
};
