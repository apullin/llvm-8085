// Minimal i8085 64-bit divide/remainder libcalls.

#include <stdint.h>

static uint64_t i8085_udivmod64(uint64_t num, uint64_t den, uint64_t *rem) {
  if (den == 0) {
    if (rem)
      *rem = 0;
    return 0;
  }

  uint64_t q = 0;
  uint64_t r = 0;
  for (int i = 63; i >= 0; --i) {
    r = (r << 1) | ((num >> i) & 1u);
    if (r >= den) {
      r -= den;
      q |= (1ULL << i);
    }
  }

  if (rem)
    *rem = r;
  return q;
}

static uint64_t i8085_abs_u64(int64_t v) {
  return v < 0 ? (uint64_t)(0ull - (uint64_t)v) : (uint64_t)v;
}

uint64_t __udivdi3(uint64_t a, uint64_t b) {
  if (b == 0)
    return 0;
  return i8085_udivmod64(a, b, 0);
}

uint64_t __umoddi3(uint64_t a, uint64_t b) {
  if (b == 0)
    return 0;
  uint64_t r = 0;
  (void)i8085_udivmod64(a, b, &r);
  return r;
}

uint64_t __udivmoddi4(uint64_t a, uint64_t b, uint64_t *rem) {
  return i8085_udivmod64(a, b, rem);
}

int64_t __divdi3(int64_t a, int64_t b) {
  if (b == 0)
    return 0;
  int neg = (a < 0) ^ (b < 0);
  uint64_t ua = i8085_abs_u64(a);
  uint64_t ub = i8085_abs_u64(b);
  uint64_t q = i8085_udivmod64(ua, ub, 0);
  int64_t res = (int64_t)q;
  return neg ? -res : res;
}

int64_t __moddi3(int64_t a, int64_t b) {
  if (b == 0)
    return 0;
  uint64_t ua = i8085_abs_u64(a);
  uint64_t ub = i8085_abs_u64(b);
  uint64_t r = 0;
  (void)i8085_udivmod64(ua, ub, &r);
  int64_t res = (int64_t)r;
  return (a < 0) ? -res : res;
}
