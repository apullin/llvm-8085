/*
 * Test input data for string_torture benchmark.
 *
 * Layout (32 bytes at 0x0100):
 *   offset 0:  "Hello\0"       (6 bytes)
 *   offset 6:  "Hello\0"       (6 bytes)  -- duplicate for strcmp equal test
 *   offset 12: "Hellp\0"       (6 bytes)  -- last-char differs
 *   offset 18: "Hel\0"         (4 bytes)  -- prefix of "Hello"
 *   offset 22: "World!\0"      (7 bytes)  -- totally different
 *   offset 29: "\0"            (1 byte)   -- empty string
 *   offset 30: padding (2 bytes)
 */

#include <stdint.h>

__attribute__((section(".input")))
const char string_torture_input[32] = {
    'H','e','l','l','o','\0',           /* offset 0: "Hello" */
    'H','e','l','l','o','\0',           /* offset 6: "Hello" (dup) */
    'H','e','l','l','p','\0',           /* offset 12: "Hellp" */
    'H','e','l','\0',                   /* offset 18: "Hel" */
    'W','o','r','l','d','!','\0',       /* offset 22: "World!" */
    '\0',                               /* offset 29: "" (empty) */
    0, 0                                /* padding */
};
