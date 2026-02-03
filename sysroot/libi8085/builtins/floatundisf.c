// Minimal i8085 uint64 -> float conversion (soft-float).

#include <stdint.h>

static inline float bitcast_u32_to_f32(uint32_t bits) {
  union {
    uint32_t i;
    float f;
  } rep = { .i = bits };
  return rep.f;
}

float __floatundisf(uint64_t a) {
  if (a == 0)
    return 0.0f;

  const uint32_t mantissa_mask = 0x007FFFFFu;
  const int mantissa_bits = 23;
  const int mantissa_digits = mantissa_bits + 1;
  const int exp_bias = 127;

  uint64_t ua = a;
  int sd = 64 - __builtin_clzll(ua);
  int e = sd - 1;

  if (sd > mantissa_digits) {
    if (sd == mantissa_digits + 1) {
      ua <<= 1;
    } else if (sd == mantissa_digits + 2) {
      // no-op
    } else {
      ua = (ua >> (sd - (mantissa_digits + 2))) |
           ((ua & (~0ULL >> ((64 + mantissa_digits + 2) - sd))) != 0);
    }
    ua |= (ua & 4) != 0;
    ++ua;
    ua >>= 2;
    if (ua & (1ULL << mantissa_digits)) {
      ua >>= 1;
      ++e;
    }
  } else {
    ua <<= (mantissa_digits - sd);
  }

  uint32_t result = ((uint32_t)(e + exp_bias) << mantissa_bits) |
                    ((uint32_t)ua & mantissa_mask);
  return bitcast_u32_to_f32(result);
}
