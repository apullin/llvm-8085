// Minimal i8085 integer multiply libcalls.
// These avoid using native * to prevent recursive libcalls.

#include <stdint.h>

uint8_t __mul8(uint8_t a, uint8_t b) {
  uint16_t res = 0;
  uint16_t aa = a;
  uint16_t bb = b;
  while (bb) {
    if (bb & 1u)
      res += aa;
    aa <<= 1;
    bb >>= 1;
  }
  return (uint8_t)res;
}

uint16_t __mul16(uint16_t a, uint16_t b) {
  uint32_t res = 0;
  uint32_t aa = a;
  uint32_t bb = b;
  while (bb) {
    if (bb & 1u)
      res += aa;
    aa <<= 1;
    bb >>= 1;
  }
  return (uint16_t)res;
}

uint32_t __mul32(uint32_t a, uint32_t b) {
  uint32_t res = 0;
  uint32_t aa = a;
  uint32_t bb = b;
  while (bb) {
    if (bb & 1u)
      res += aa;
    aa <<= 1;
    bb >>= 1;
  }
  return res;
}
