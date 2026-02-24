// i8085 soft-float single-precision division (standalone implementation).

#define SINGLE_PRECISION

#include "fp_lib.h"

static __inline rep_t i8085_pack_result(rep_t sign, int exp, rep_t sig) {
  if (exp >= (int)maxExponent)
    return infRep | sign;

  if (exp > 0) {
    sig &= significandMask;
    return sign | ((rep_t)exp << significandBits) | sig;
  }

  if (exp <= -((int)significandBits + 1))
    return sign; // underflow to zero

  sig &= significandMask;
  return sign | sig;
}

COMPILER_RT_ABI fp_t __divsf3(fp_t a, fp_t b) {
  const rep_t aRep = toRep(a);
  const rep_t bRep = toRep(b);
  const unsigned int aExp = (aRep >> significandBits) & maxExponent;
  const unsigned int bExp = (bRep >> significandBits) & maxExponent;
  const rep_t sign = (aRep ^ bRep) & signBit;

  rep_t aSig = aRep & significandMask;
  rep_t bSig = bRep & significandMask;
  int scale = 0;

  // Handle special cases.
  if (aExp - 1U >= maxExponent - 1U ||
      bExp - 1U >= maxExponent - 1U) {
    const rep_t aAbs = aRep & absMask;
    const rep_t bAbs = bRep & absMask;

    if (aAbs > infRep)
      return fromRep(aRep | quietBit);
    if (bAbs > infRep)
      return fromRep(bRep | quietBit);

    if (aAbs == infRep) {
      if (bAbs == infRep)
        return fromRep(qnanRep);
      return fromRep(infRep | sign);
    }

    if (bAbs == infRep)
      return fromRep(sign);

    if (!aAbs) {
      if (!bAbs)
        return fromRep(qnanRep);
      return fromRep(sign);
    }

    if (!bAbs)
      return fromRep(infRep | sign);

    if (aAbs < implicitBit)
      scale += normalize(&aSig);
    if (bAbs < implicitBit)
      scale -= normalize(&bSig);
  }

  aSig |= implicitBit;
  bSig |= implicitBit;

  int exp = (int)aExp - (int)bExp + scale + exponentBias;

  // Pre-normalize so the quotient is in [1, 2).
  if (aSig < bSig) {
    aSig <<= 1;
    exp--;
  }

  // Compute quotient with 3 extra bits for rounding using long division.
  const unsigned int shift = significandBits + 3; // 26
  uint32_t num_hi = aSig >> (32 - shift);
  uint32_t num_lo = (uint32_t)aSig << shift;

  uint32_t q = 0;
  uint32_t rem = 0;

  // 50-bit numerator: bits [49:32] in num_hi, [31:0] in num_lo.
  for (int i = 49; i >= 0; --i) {
    const uint32_t bit = (num_hi >> 17) & 1u;
    num_hi = (num_hi << 1) | (num_lo >> 31);
    num_lo <<= 1;

    rem = (rem << 1) | bit;
    if (rem >= bSig) {
      rem -= bSig;
      if (i <= (int)(significandBits + 3))
        q |= (REP_C(1) << i);
    }
  }
  unsigned int sticky = (rem != 0);

  if (exp <= 0) {
    unsigned int shift = (unsigned int)(1 - exp);
    if (shift >= 32)
      return fromRep(sign);
    while (shift--) {
      sticky |= (q & 1u);
      q >>= 1;
    }
    exp = 0;
  }

  rep_t sig = q >> 3;
  const unsigned int guard = (q >> 2) & 1u;
  const unsigned int round = (q >> 1) & 1u;
  const unsigned int sticky_bit = (q & 1u) | sticky;

  if (guard && (round | sticky_bit | (sig & 1u)))
    sig++;

  if (sig & (implicitBit << 1)) {
    sig >>= 1;
    exp++;
  }

  return fromRep(i8085_pack_result(sign, exp, sig));
}
