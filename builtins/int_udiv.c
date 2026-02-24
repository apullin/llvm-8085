// Minimal i8085 unsigned divide/remainder libcalls.

#include <stdint.h>

static uint32_t i8085_udivmod32(uint32_t num, uint32_t den, uint32_t *rem) {
  if (den == 0) {
    if (rem)
      *rem = 0;
    return 0;
  }

  uint32_t q = 0;
  uint32_t r = 0;
  for (int i = 31; i >= 0; --i) {
    r = (r << 1) | ((num >> i) & 1u);
    if (r >= den) {
      r -= den;
      q |= (1u << i);
    }
  }

  if (rem)
    *rem = r;
  return q;
}

uint8_t __udiv8(uint8_t a, uint8_t b) {
  if (b == 0)
    return 0;
  return (uint8_t)i8085_udivmod32(a, b, 0);
}

uint16_t __udiv16(uint16_t a, uint16_t b) {
  if (b == 0)
    return 0;
  return (uint16_t)i8085_udivmod32(a, b, 0);
}

uint32_t __udiv32(uint32_t a, uint32_t b) {
  if (b == 0)
    return 0;
  return i8085_udivmod32(a, b, 0);
}

uint8_t __urem8(uint8_t a, uint8_t b) {
  if (b == 0)
    return 0;
  uint32_t r = 0;
  (void)i8085_udivmod32(a, b, &r);
  return (uint8_t)r;
}

uint16_t __urem16(uint16_t a, uint16_t b) {
  if (b == 0)
    return 0;
  uint32_t r = 0;
  (void)i8085_udivmod32(a, b, &r);
  return (uint16_t)r;
}

uint32_t __urem32(uint32_t a, uint32_t b) {
  if (b == 0)
    return 0;
  uint32_t r = 0;
  (void)i8085_udivmod32(a, b, &r);
  return r;
}
