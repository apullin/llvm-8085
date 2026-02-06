/*
 * String operations torture test for i8085.
 *
 * Exercises hand-written strlen and strcmp with various inputs.
 *
 * Input layout (32 bytes at 0x0100):
 *   +0:  "Hello"   (len 5)
 *   +6:  "Hello"   (duplicate)
 *   +12: "Hellp"   (last-char differs)
 *   +18: "Hel"     (prefix)
 *   +22: "World!"  (len 6)
 *   +29: ""        (empty)
 *
 * Output: 4-byte pass count at 0x0200.
 */

#include <stdint.h>

/* Declare strlen/strcmp directly — clang's freestanding <string.h>
   only provides memcpy/memset/memmove/memcmp as builtins. */
typedef unsigned int size_t;
size_t strlen(const char *s);
int strcmp(const char *s1, const char *s2);

#define INPUT_ADDR  0x0100
#define OUTPUT_ADDR 0x0200

#define TOTAL_TESTS 12

__attribute__((noinline)) static void halt_ok(void) { __asm__ volatile("hlt"); }
__attribute__((noinline)) static void fail_loop(void) {
    for (;;) {
    }
}

int main(void) {
    const char *hello1 = (const char *)(INPUT_ADDR + 0);   /* "Hello" */
    const char *hello2 = (const char *)(INPUT_ADDR + 6);   /* "Hello" */
    const char *hellp  = (const char *)(INPUT_ADDR + 12);  /* "Hellp" */
    const char *hel    = (const char *)(INPUT_ADDR + 18);  /* "Hel" */
    const char *world  = (const char *)(INPUT_ADDR + 22);  /* "World!" */
    const char *empty  = (const char *)(INPUT_ADDR + 29);  /* "" */

    volatile uint32_t *output = (volatile uint32_t *)OUTPUT_ADDR;

    /*
     * Use a volatile pass counter so the optimizer cannot coalesce
     * the conditional increments into select/lshr/icmp-sgt patterns
     * that expose codegen bugs in the i8085 back-end (wrong results
     * for 16-bit logical-right-shift-by-15 and signed-greater-than
     * when used inside chained selects).
     */
    volatile uint16_t pass = 0;

    /* ---- strlen tests ---- */

    /* 1. strlen("Hello") == 5 */
    if (strlen(hello1) == 5) pass++;

    /* 2. strlen("Hellp") == 5 */
    if (strlen(hellp) == 5) pass++;

    /* 3. strlen("Hel") == 3 */
    if (strlen(hel) == 3) pass++;

    /* 4. strlen("World!") == 6 */
    if (strlen(world) == 6) pass++;

    /* 5. strlen("") == 0 */
    if (strlen(empty) == 0) pass++;

    /* ---- strcmp tests ---- */

    /* 6. strcmp("Hello", "Hello") == 0 */
    if (strcmp(hello1, hello2) == 0) pass++;

    /* 7. strcmp("Hello", "Hellp") < 0  ('o' < 'p') */
    if (strcmp(hello1, hellp) < 0) pass++;

    /* 8. strcmp("Hellp", "Hello") > 0  ('p' > 'o') */
    if (strcmp(hellp, hello1) > 0) pass++;

    /* 9. strcmp("Hello", "Hel") > 0  ('l' > '\0') */
    if (strcmp(hello1, hel) > 0) pass++;

    /* 10. strcmp("Hel", "Hello") < 0  ('\0' < 'l') */
    if (strcmp(hel, hello1) < 0) pass++;

    /* 11. strcmp("Hello", "World!") < 0  ('H' < 'W') */
    if (strcmp(hello1, world) < 0) pass++;

    /* 12. strcmp("", "") == 0 */
    if (strcmp(empty, empty) == 0) pass++;

    /* Write pass count to output */
    *output = (uint32_t)pass;

    if (pass == TOTAL_TESTS) {
        halt_ok();
    }

    fail_loop();
    return 0;
}
