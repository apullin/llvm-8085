// i8085 soft-float single-precision multiply (local copy of compiler-rt).

#define SINGLE_PRECISION

#include "fp_lib.h"
#include "fp_mul_impl.inc"

COMPILER_RT_ABI fp_t __mulsf3(fp_t a, fp_t b) { return __mulXf3__(a, b); }

