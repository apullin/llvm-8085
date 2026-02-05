/*
 * JSON parse benchmark input data.
 *
 * JSON string placed at address 0x0100 via the .input section.
 * String: {"name":"i8085","bits":8,"year":1977}
 * Length: 37 chars + null terminator = 38 bytes
 *
 * Expected parse result: 7 tokens, "bits" value = 8
 */

#include <stdint.h>

__attribute__((section(".input")))
const char json_input[48] = "{\"name\":\"i8085\",\"bits\":8,\"year\":1977}";
