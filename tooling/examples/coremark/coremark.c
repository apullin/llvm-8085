/*
 * Single-TU wrapper for CoreMark on Intel 8085
 *
 * The benchmark.sh infrastructure compiles one .c file per benchmark.
 * This file #includes all CoreMark sources into a single translation unit.
 *
 * CoreMark's main() is redirected to coremark_main() so we can wrap it
 * with our own main() that stores the result and halts.
 *
 * TOTAL_DATA_SIZE reduced to 200 for i8085 simulator feasibility.
 * Standard 2000 requires billions of instructions on an 8-bit CPU.
 * Even 460 takes >5B steps on this 8-bit CPU with no MUL instruction.
 * 200 bytes: each algo gets 66 bytes (1 list node, 2x2 matrix, state).
 * Minimum viable is ~180 (per_item=20 in list, need blksize/20 >= 3).
 * Must be defined before any includes since coremark.h has an #ifndef guard.
 */
#define TOTAL_DATA_SIZE 200

/* Port layer (must come first — provides types, seeds, stubs) */
#include "core_portme.c"

/* CoreMark algorithm modules */
#include "core_list_join.c"
#include "core_matrix.c"
#include "core_state.c"
#include "core_util.c"

/* Redirect CoreMark's main() to a different name */
#define main coremark_main
#include "core_main.c"
#undef main

/* Our actual main() — calls CoreMark and reports result */
int main(void) {
    volatile unsigned int *result = (volatile unsigned int *)0x0200;
    int ret = coremark_main();
    *result = (unsigned int)ret;
    if (ret == 0) {
        __asm__ volatile("hlt");
    }
    for (;;) {}
    return 0;
}
