# i8085 Runtime Library Reference

This document catalogs every runtime library routine (builtin) shipped in the
i8085 sysroot, organized by category.  These routines live in
`sysroot/lib/libgcc.a` and are linked automatically by the toolchain.

---

## ABI Quick Reference

### Calling convention

- **Stack-based cdecl.** All arguments are pushed on the stack right-to-left.
  The caller cleans up the stack after the call.
- Each argument occupies its natural size on the stack (i8 = 1 byte,
  i16 = 2 bytes, i32 = 4 bytes, i64 = 8 bytes, f32 = 4 bytes).
- No registers are callee-saved; all GPRs are caller-saved.

### Stack layout on entry

```
[SP+0]  return address (2 bytes, low byte first)
[SP+2]  first argument
[SP+2+sizeof(arg0)]  second argument
...
```

### Return registers

| Return type | Register(s) | Layout |
|-------------|-------------|--------|
| i8          | `A`         | Also mirrored in `C` with `B=0` for i16 compat |
| i16         | `BC`        | `B` = high byte, `C` = low byte |
| i32         | `BC:DE`     | `C` = byte 0 (LSB), `B` = byte 1, `E` = byte 2, `D` = byte 3 (MSB) |
| i64         | via sret    | Hidden sret pointer is the first argument; routine writes 8 bytes to it |
| f32         | `BC:DE`     | Bitcast to i32, same layout as i32 return |

### Byte order

Little-endian throughout.  Multi-byte stack arguments are stored
least-significant byte at the lowest address.

---

## 1. Integer Multiply

All multiply routines are **hand-written i8085 assembly** in
`sysroot/libi8085/builtins/int_mul.S`.
A C reference implementation exists in `sysroot/libi8085/builtins/int_mul.c`
but the assembly version is what ships in `libgcc.a`.

All use the **LSB-first shift-and-add with early termination** algorithm:

```
while (multiplier != 0):
    if bit0(multiplier): result += multiplicand
    multiplicand <<= 1
    multiplier  >>= 1
```

### `__mul8`

| Field | Value |
|-------|-------|
| **Symbol** | `__mul8` |
| **Source** | `int_mul.S` (hand-written assembly) |
| **Signature** | `uint8_t __mul8(uint8_t a, uint8_t b)` |
| **Args** | `[SP+2]` = a (1 byte), `[SP+3]` = b (1 byte) |
| **Return** | Result in `A` (also in `C`; `B` = 0) |
| **Description** | Unsigned 8-bit multiply, returning the low 8 bits of the product. |
| **DAG pattern** | `ISD::MUL` on `MVT::i8` via `RTLIB::MUL_I8 -> "__mul8"` |
| **Notes** | Max 8 loop iterations. Early termination when multiplier is zero. |

### `__mul16`

| Field | Value |
|-------|-------|
| **Symbol** | `__mul16` |
| **Source** | `int_mul.S` (hand-written assembly) |
| **Signature** | `uint16_t __mul16(uint16_t a, uint16_t b)` |
| **Args** | `[SP+2..3]` = a, `[SP+4..5]` = b |
| **Return** | Result in `BC` (`B` = high, `C` = low) |
| **Description** | Unsigned 16-bit multiply, returning the low 16 bits. Uses `HL` as the result accumulator, `DE` as the multiplicand. |
| **DAG pattern** | `ISD::MUL` on `MVT::i16` via `RTLIB::MUL_I16 -> "__mul16"` (fallback when both operands are not sext from i8) |
| **Notes** | Max 16 loop iterations. The 16-bit `DAD D` instruction is used for the accumulation step. |

### `__mul32`

| Field | Value |
|-------|-------|
| **Symbol** | `__mul32` |
| **Source** | `int_mul.S` (hand-written assembly) |
| **Signature** | `uint32_t __mul32(uint32_t a, uint32_t b)` |
| **Args** | `[SP+2..5]` = a (4 bytes), `[SP+6..9]` = b (4 bytes) |
| **Return** | Result in `BC:DE` (`C`=byte0, `B`=byte1, `E`=byte2, `D`=byte3) |
| **Description** | Unsigned 32-bit multiply. Allocates 4 bytes on the stack for the result and modifies both `a` and `b` in-place on the caller's stack frame. |
| **DAG pattern** | `ISD::MUL` on `MVT::i32` via `RTLIB::MUL_I32 -> "__mul32"` (fallback when both operands are not sext from i16) |
| **Notes** | Max 32 loop iterations. Uses byte-by-byte addition with carry for the 32-bit accumulation. |

### `__mulsi8`

| Field | Value |
|-------|-------|
| **Symbol** | `__mulsi8` |
| **Source** | `int_mul.S` (hand-written assembly) |
| **Signature** | `int16_t __mulsi8(int8_t a, int8_t b)` |
| **Args** | `[SP+2]` = a (1 byte, signed), `[SP+3]` = b (1 byte, signed) |
| **Return** | Signed 16-bit product in `BC` (`B` = high, `C` = low) |
| **Description** | Widening signed 8x8 -> 16-bit multiply. Determines result sign from XOR of input sign bits, negates negative inputs, performs unsigned 8x8->16 shift-and-add, then negates result if needed. |
| **DAG pattern** | `ISD::MUL` on `MVT::i16` when both operands match `(sext i8 to i16)` -- detected by `isMulSExtI8()` in `I8085ISelLowering.cpp`, emitted via `emitMul8LibCall(... "__mulsi8" ...)` |
| **Notes** | Max 8 loop iterations (8-bit multiplier). |

### `__mulsi8_hi8`

| Field | Value |
|-------|-------|
| **Symbol** | `__mulsi8_hi8` |
| **Source** | `int_mul.S` (hand-written assembly) |
| **Signature** | `int8_t __mulsi8_hi8(int8_t a, int8_t b)` |
| **Args** | `[SP+2]` = a (1 byte, signed), `[SP+3]` = b (1 byte, signed) |
| **Return** | Upper 8 bits of the signed 16-bit product in `A` (also `C`; `B`=0) |
| **Description** | Returns bits [15:8] of `(int16_t)a * (int16_t)b`. This is MULHS for i8. |
| **DAG pattern** | `trunc i8 (srl/sra (mul i16 (sext i8, sext i8)), 8)` -- matched by `performTruncMulCombine()` |
| **Notes** | Full signed 8x8->16 multiply is computed, then byte 1 (high byte) is extracted. |

### `__mulsi8_lo8`

| Field | Value |
|-------|-------|
| **Symbol** | `__mulsi8_lo8` |
| **Source** | `int_mul.S` (hand-written assembly) |
| **Signature** | `int8_t __mulsi8_lo8(int8_t a, int8_t b)` |
| **Args** | `[SP+2]` = a (1 byte, signed), `[SP+3]` = b (1 byte, signed) |
| **Return** | Lower 8 bits of signed 16-bit product in `A` (also `C`; `B`=0) |
| **Description** | Returns bits [7:0] of `(int16_t)a * (int16_t)b`. Since the low bits of a signed multiply equal the low bits of unsigned multiply, this is a tail-call to `__mul8`. |
| **DAG pattern** | `trunc i8 (mul i16 (sext i8, sext i8))` -- matched by `performTruncMulCombine()` when the MUL has a single TRUNCATE user |
| **Notes** | Implemented as `jmp __mul8`. Zero overhead. |

### `__mulsi16`

| Field | Value |
|-------|-------|
| **Symbol** | `__mulsi16` |
| **Source** | `int_mul.S` (hand-written assembly) |
| **Signature** | `int32_t __mulsi16(int16_t a, int16_t b)` |
| **Args** | `[SP+2..3]` = a (16-bit signed), `[SP+4..5]` = b (16-bit signed) |
| **Return** | Signed 32-bit product in `BC:DE` |
| **Description** | Widening signed 16x16 -> 32-bit multiply. Negates negative inputs, performs unsigned multiply with 4-byte multiplicand (zero-extended) and 16-bit multiplier in `DE`, then applies sign. |
| **DAG pattern** | `ISD::MUL` on `MVT::i32` when both operands match `(sext i16 to i32)` -- detected by `isSExtFromI16()`, emitted via `emitMul16LibCall(... "__mulsi16" ...)` |
| **Notes** | Allocates 8 bytes on the stack (4 for multiplicand, 4 for result). Max 16 loop iterations. |

### `__mulsi16_shr8`

| Field | Value |
|-------|-------|
| **Symbol** | `__mulsi16_shr8` |
| **Source** | `int_mul.S` (hand-written assembly) |
| **Signature** | `int16_t __mulsi16_shr8(int16_t a, int16_t b)` |
| **Args** | `[SP+2..3]` = a, `[SP+4..5]` = b |
| **Return** | Result in `BC` |
| **Description** | Returns `(int16_t)(((int32_t)a * (int32_t)b) >> 8)`, i.e. bytes [1..2] of the 32-bit product. Optimized: accumulates only a 3-byte partial result (bytes 0, 1, 2) and returns bytes 1-2. |
| **DAG pattern** | `trunc i16 (srl/sra (mul i32 (sext i16, sext i16)), 8)` -- matched by `performTruncMulCombine()` |
| **Notes** | Useful for Q8.8 fixed-point multiplication. Same sign-handling as `__mulsi16`. |

### `__mulsi16_hi16`

| Field | Value |
|-------|-------|
| **Symbol** | `__mulsi16_hi16` |
| **Source** | `int_mul.S` (hand-written assembly) |
| **Signature** | `int16_t __mulsi16_hi16(int16_t a, int16_t b)` |
| **Args** | `[SP+2..3]` = a, `[SP+4..5]` = b |
| **Return** | Upper 16 bits of signed 32-bit product in `BC` |
| **Description** | Returns bits [31:16] of `(int32_t)a * (int32_t)b`. Computes the full 4-byte product for carry correctness and extracts bytes 2-3. |
| **DAG pattern** | `trunc i16 (srl/sra (mul i32 (sext i16, sext i16)), 16)` -- matched by `performTruncMulCombine()` |
| **Notes** | This is MULHS for i16. |

### `__mulsi16_lo16`

| Field | Value |
|-------|-------|
| **Symbol** | `__mulsi16_lo16` |
| **Source** | `int_mul.S` (hand-written assembly) |
| **Signature** | `int16_t __mulsi16_lo16(int16_t a, int16_t b)` |
| **Args** | `[SP+2..3]` = a, `[SP+4..5]` = b |
| **Return** | Lower 16 bits of signed 32-bit product in `BC` |
| **Description** | Returns bits [15:0] of `(int32_t)a * (int32_t)b`. Since the lower 16 bits of a signed multiply equal those of the unsigned multiply, no sign handling is needed. Functionally identical to `__mul16`. |
| **DAG pattern** | `trunc i16 (mul i32 (sext i16, sext i16))` -- matched by `performTruncMulCombine()` when the MUL has a single TRUNCATE user |
| **Notes** | Standalone copy of the `__mul16` algorithm (not a tail-call, unlike `__mulsi8_lo8`). |

### `__mulsi32`

| Field | Value |
|-------|-------|
| **Symbol** | `__mulsi32` |
| **Source** | `int_mul.S` (hand-written assembly) |
| **Signature** | `void __mulsi32(int64_t *sret, int32_t a, int32_t b)` |
| **Args** | `[SP+2..3]` = sret pointer, `[SP+4..7]` = a, `[SP+8..11]` = b |
| **Return** | Writes 8-byte signed 64-bit product to the sret pointer |
| **Description** | Widening signed 32x32 -> 64-bit multiply. Uses sret convention since i64 does not fit in return registers. Negates negative inputs, performs unsigned 32x32->64 shift-and-add with 8-byte result and 8-byte multiplicand on the stack, then applies sign. |
| **DAG pattern** | `ISD::MUL` on `MVT::i64` when both operands match `(sext i32 to i64)` -- detected by `isSExtFromI32()` / `ComputeNumSignBits >= 33`, emitted via `emitMul32I64LibCall(... "__mulsi32" ...)` |
| **Notes** | Allocates 18 bytes on the stack (8 result + 8 multiplicand + 2 sign flag). Max 32 loop iterations. Modifies `a` and `b` in-place on the caller's stack. |

### `__mulsi32_shr16`

| Field | Value |
|-------|-------|
| **Symbol** | `__mulsi32_shr16` |
| **Source** | `int_mul.S` (hand-written assembly) |
| **Signature** | `int32_t __mulsi32_shr16(int32_t a, int32_t b)` |
| **Args** | `[SP+2..5]` = a, `[SP+6..9]` = b |
| **Return** | Result in `BC:DE` |
| **Description** | Returns `(int32_t)(((int64_t)a * (int64_t)b) >> 16)`, i.e. bytes [2..5] of the 64-bit product. Optimized: accumulates a 6-byte partial result and returns bytes 2-5. |
| **DAG pattern** | `trunc i32 (srl/sra (mul i64 (sext i32, sext i32)), 16)` -- matched by `performTruncMulCombine()` |
| **Notes** | This is Q15.16 fixed-point multiply. Allocates 18 bytes on the stack. Same sign-handling approach as `__mulsi32`. |

### `__mulsi32_hi32`

| Field | Value |
|-------|-------|
| **Symbol** | `__mulsi32_hi32` |
| **Source** | `int_mul.S` (hand-written assembly) |
| **Signature** | `int32_t __mulsi32_hi32(int32_t a, int32_t b)` |
| **Args** | `[SP+2..5]` = a, `[SP+6..9]` = b |
| **Return** | Upper 32 bits of signed 64-bit product in `BC:DE` |
| **Description** | Returns bits [63:32] of `(int64_t)a * (int64_t)b`. Computes the full 8-byte product for carry accuracy and extracts bytes 4-7. |
| **DAG pattern** | `trunc i32 (srl/sra (mul i64 (sext i32, sext i32)), 32)` -- matched by `performTruncMulCombine()` |
| **Notes** | This is MULHS for i32. Allocates 18 bytes on the stack. |

### `__muldi3`

| Field | Value |
|-------|-------|
| **Symbol** | `__muldi3` |
| **Source** | compiler-rt C (`llvm-project/compiler-rt/lib/builtins/muldi3.c`), compiled at `-O0` |
| **Signature** | `int64_t __muldi3(int64_t a, int64_t b)` (sret convention on i8085) |
| **Args** | `[SP+2..3]` = sret pointer, `[SP+4..11]` = a, `[SP+12..19]` = b |
| **Return** | Writes 8-byte result to sret pointer |
| **Description** | General 64x64 -> 64-bit multiply (standard compiler-rt implementation). |
| **DAG pattern** | `ISD::MUL` on `MVT::i64` via `RTLIB::MUL_I64 -> "__muldi3"` (fallback when operands are not sext from i32) |
| **Notes** | Very expensive on i8085. Prefer `__mulsi32` when operands are known to be sign-extended 32-bit values. |

---

## 2. Integer Division & Remainder

### Unsigned (8/16/32-bit)

Source: `sysroot/libi8085/builtins/int_udiv.c` (target-specific C, compiled at `-O0`).

All routines share a common `i8085_udivmod32()` helper that implements
restoring long division over 32 iterations, working on 32-bit values
regardless of the operand width.

| Symbol | Signature | Description | DAG pattern |
|--------|-----------|-------------|-------------|
| `__udiv8` | `uint8_t __udiv8(uint8_t a, uint8_t b)` | Unsigned 8-bit divide | `ISD::UDIV` `MVT::i8` via `RTLIB::UDIV_I8` |
| `__udiv16` | `uint16_t __udiv16(uint16_t a, uint16_t b)` | Unsigned 16-bit divide | `ISD::UDIV` `MVT::i16` via `RTLIB::UDIV_I16` |
| `__udiv32` | `uint32_t __udiv32(uint32_t a, uint32_t b)` | Unsigned 32-bit divide | `ISD::UDIV` `MVT::i32` via `RTLIB::UDIV_I32` |
| `__urem8` | `uint8_t __urem8(uint8_t a, uint8_t b)` | Unsigned 8-bit remainder | `ISD::UREM` `MVT::i8` via `RTLIB::UREM_I8` |
| `__urem16` | `uint16_t __urem16(uint16_t a, uint16_t b)` | Unsigned 16-bit remainder | `ISD::UREM` `MVT::i16` via `RTLIB::UREM_I16` |
| `__urem32` | `uint32_t __urem32(uint32_t a, uint32_t b)` | Unsigned 32-bit remainder | `ISD::UREM` `MVT::i32` via `RTLIB::UREM_I32` |

**Args:** `[SP+2]` = a (natural size), `[SP+2+sizeof(a)]` = b (natural size).
**Return:** Result in `A` (i8), `BC` (i16), or `BC:DE` (i32).
**Notes:** Division by zero returns 0.  DAG combines replace power-of-two
`UDIV` with `SRL` and power-of-two `UREM` with `AND`, so these are only
called for non-constant or non-power-of-two divisors.

### Signed (8/16/32-bit)

Source: `sysroot/libi8085/builtins/int_sdiv.c` (target-specific C, compiled at `-O0`).

Uses `i8085_abs_u32()` to convert to unsigned magnitudes, calls
`i8085_udivmod32()`, then applies the sign.

| Symbol | Signature | Description | DAG pattern |
|--------|-----------|-------------|-------------|
| `__sdiv8` | `int8_t __sdiv8(int8_t a, int8_t b)` | Signed 8-bit divide | `ISD::SDIV` `MVT::i8` via `RTLIB::SDIV_I8` |
| `__sdiv16` | `int16_t __sdiv16(int16_t a, int16_t b)` | Signed 16-bit divide | `ISD::SDIV` `MVT::i16` via `RTLIB::SDIV_I16` |
| `__sdiv32` | `int32_t __sdiv32(int32_t a, int32_t b)` | Signed 32-bit divide | `ISD::SDIV` `MVT::i32` via `RTLIB::SDIV_I32` |
| `__srem8` | `int8_t __srem8(int8_t a, int8_t b)` | Signed 8-bit remainder | `ISD::SREM` `MVT::i8` via `RTLIB::SREM_I8` |
| `__srem16` | `int16_t __srem16(int16_t a, int16_t b)` | Signed 16-bit remainder | `ISD::SREM` `MVT::i16` via `RTLIB::SREM_I16` |
| `__srem32` | `int32_t __srem32(int32_t a, int32_t b)` | Signed 32-bit remainder | `ISD::SREM` `MVT::i32` via `RTLIB::SREM_I32` |

**Args/Return:** Same layout as unsigned counterparts.
**Notes:** Remainder sign follows the dividend (C semantics). Division by
zero returns 0.

### 64-bit Division & Remainder

Source: `sysroot/libi8085/builtins/int_divdi3.c` (target-specific C, compiled at `-O0`).

Uses `i8085_udivmod64()` with restoring long division over 64 iterations.

| Symbol | Signature | Description | DAG pattern |
|--------|-----------|-------------|-------------|
| `__udivdi3` | `uint64_t __udivdi3(uint64_t a, uint64_t b)` | Unsigned 64-bit divide | `ISD::UDIV` `MVT::i64` via `RTLIB::UDIV_I64` |
| `__umoddi3` | `uint64_t __umoddi3(uint64_t a, uint64_t b)` | Unsigned 64-bit remainder | `ISD::UREM` `MVT::i64` via `RTLIB::UREM_I64` |
| `__udivmoddi4` | `uint64_t __udivmoddi4(uint64_t a, uint64_t b, uint64_t *rem)` | Combined unsigned 64-bit divmod | Not directly triggered by DAG; called internally |
| `__divdi3` | `int64_t __divdi3(int64_t a, int64_t b)` | Signed 64-bit divide | `ISD::SDIV` `MVT::i64` via `RTLIB::SDIV_I64` |
| `__moddi3` | `int64_t __moddi3(int64_t a, int64_t b)` | Signed 64-bit remainder | `ISD::SREM` `MVT::i64` via `RTLIB::SREM_I64` |

**Args:** All i64 arguments and returns use sret convention (hidden pointer
as first stack argument).
**Notes:** Extremely expensive on i8085 (64 loop iterations through 64-bit
shifts and compares). Division by zero returns 0.

---

## 3. Integer Shifts (64-bit)

Source: compiler-rt C (`llvm-project/compiler-rt/lib/builtins/`), compiled at `-Os`.

8/16/32-bit shifts are handled by native instruction sequences; only 64-bit
shifts require library calls. The backend first attempts to inline-expand
i64 shifts using two i32 halves (see `LowerShiftI64()` in
`I8085ISelLowering.cpp`), but the resulting i32 shifts may themselves require
these routines when the shift amount is not constant.

| Symbol | Source | Signature | Description | DAG pattern |
|--------|--------|-----------|-------------|-------------|
| `__ashldi3` | compiler-rt `ashldi3.c` | `int64_t __ashldi3(int64_t a, int32_t b)` | Left shift i64 by i32 amount | `RTLIB::SHL_I64` |
| `__lshrdi3` | compiler-rt `lshrdi3.c` | `uint64_t __lshrdi3(uint64_t a, int32_t b)` | Logical right shift i64 | `RTLIB::SRL_I64` |
| `__ashrdi3` | compiler-rt `ashrdi3.c` | `int64_t __ashrdi3(int64_t a, int32_t b)` | Arithmetic right shift i64 | `RTLIB::SRA_I64` |

**Notes:** The i64 value is passed via sret on i8085. The shift amount is
passed as an i32 on the stack.

---

## 4. Integer Comparison (64-bit)

Source: compiler-rt C (`llvm-project/compiler-rt/lib/builtins/`), compiled at `-Os`.

| Symbol | Source | Signature | Description | DAG pattern |
|--------|--------|-----------|-------------|-------------|
| `__cmpdi2` | compiler-rt `cmpdi2.c` | `si_int __cmpdi2(int64_t a, int64_t b)` | Signed 64-bit compare: returns 0 if a<b, 1 if a==b, 2 if a>b | Used by comparison lowering for i64 |
| `__ucmpdi2` | compiler-rt `ucmpdi2.c` | `si_int __ucmpdi2(uint64_t a, uint64_t b)` | Unsigned 64-bit compare: same return convention | Used by comparison lowering for i64 |

**Notes:** These return an `si_int` (i32 on i8085) in `BC:DE`.

---

## 5. Integer Miscellaneous

### `__negdi2`

| Field | Value |
|-------|-------|
| **Symbol** | `__negdi2` |
| **Source** | compiler-rt C (`negdi2.c`), compiled at `-Os` |
| **Signature** | `int64_t __negdi2(int64_t a)` |
| **Description** | Negate a 64-bit integer. |
| **Notes** | Used when the backend needs to negate an i64 value. |

### `__clzsi2`

| Field | Value |
|-------|-------|
| **Symbol** | `__clzsi2` |
| **Source** | `sysroot/libi8085/builtins/clzsi2.S` (hand-written assembly) |
| **Signature** | `int __clzsi2(uint32_t a)` |
| **Args** | `[SP+2..5]` = a (4 bytes) |
| **Return** | Count in `BC:DE` (i32, but only low byte is meaningful: 0--32) |
| **Description** | Count leading zeros of a 32-bit value. Scans from the MSB (byte 3) down to the LSB (byte 0). Uses an inner `clz8` subroutine for each byte. Returns 32 if input is zero. |
| **DAG pattern** | `ISD::CTLZ` / `ISD::CTLZ_ZERO_UNDEF` on `MVT::i32` via `RTLIB::CTLZ_I32 -> "__clzsi2"` |
| **Notes** | The inner `clz8` loop shifts left and counts until the MSB is set. |

### `__clzdi2`

| Field | Value |
|-------|-------|
| **Symbol** | `__clzdi2` |
| **Source** | compiler-rt C (`clzdi2.c`), compiled at `-Os` |
| **Signature** | `int __clzdi2(uint64_t a)` |
| **Description** | Count leading zeros of a 64-bit value. |
| **DAG pattern** | `ISD::CTLZ` / `ISD::CTLZ_ZERO_UNDEF` on `MVT::i64` via `RTLIB::CTLZ_I64 -> "__clzdi2"` |

### `__ctzsi2`

| Field | Value |
|-------|-------|
| **Symbol** | `__ctzsi2` |
| **Source** | compiler-rt C (`ctzsi2.c`), compiled at `-Os` |
| **Signature** | `int __ctzsi2(uint32_t a)` |
| **Description** | Count trailing zeros of a 32-bit value. |
| **DAG pattern** | `ISD::CTTZ` / `ISD::CTTZ_ZERO_UNDEF` on `MVT::i32` via `RTLIB::CTTZ_I32 -> "__ctzsi2"` |

### `__ctzdi2`

| Field | Value |
|-------|-------|
| **Symbol** | `__ctzdi2` |
| **Source** | compiler-rt C (`ctzdi2.c`), compiled at `-Os` |
| **Signature** | `int __ctzdi2(uint64_t a)` |
| **Description** | Count trailing zeros of a 64-bit value. |
| **DAG pattern** | `ISD::CTTZ` / `ISD::CTTZ_ZERO_UNDEF` on `MVT::i64` via `RTLIB::CTTZ_I64 -> "__ctzdi2"` |

---

## 6. Floating Point

All floating-point routines operate on IEEE 754 single-precision (`float`,
32-bit) values.  On i8085, `float` is bitcast to/from `i32` and passed/returned
in `BC:DE`.

### Arithmetic

| Symbol | Source | Description | DAG pattern |
|--------|--------|-------------|-------------|
| `__addsf3` | `sysroot/libi8085/builtins/addsf3.c` (local copy of compiler-rt `fp_add_impl.inc`) | `float + float` | `ISD::FADD` `MVT::f32` via `RTLIB::ADD_F32` |
| `__subsf3` | `sysroot/libi8085/builtins/subsf3.c` (local copy; flips sign bit and calls `__addsf3`) | `float - float` | `ISD::FSUB` `MVT::f32` via `RTLIB::SUB_F32` |
| `__mulsf3` | `sysroot/libi8085/builtins/mulsf3.c` (local copy of compiler-rt `fp_mul_impl.inc`) | `float * float` | `ISD::FMUL` `MVT::f32` via `RTLIB::MUL_F32` |
| `__divsf3` | `sysroot/libi8085/builtins/divsf3.c` (standalone implementation with long-division quotient) | `float / float` | `ISD::FDIV` `MVT::f32` via `RTLIB::DIV_F32` |
| `__negsf2` | `sysroot/libi8085/builtins/negsf2.c` (local copy; XORs sign bit) | `-float` | `ISD::FNEG` `MVT::f32` (expanded) |

**Notes on `__divsf3`:** This is a custom standalone implementation (not
compiler-rt's `divsf3.c`) that uses a bit-at-a-time long division loop over
50 iterations to compute the quotient with 3 extra guard/round/sticky bits
for IEEE-correct rounding. It handles all special cases (NaN, Inf, zero,
denormals).

**Notes on source origin:** The `addsf3.c`, `subsf3.c`, `mulsf3.c`, and
`negsf2.c` files are local copies of compiler-rt sources placed in
`sysroot/libi8085/builtins/` rather than being compiled directly from
`llvm-project/compiler-rt/`. This avoids include-path issues and lets the
build script use target-specific include directories.  They are compiled at
`-O0`.

### Comparison

Source: `sysroot/libi8085/builtins/comparesf2.c` (local copy of compiler-rt,
compiled at `-O0`).

All comparison routines are defined in a single object file.  Several
symbols are aliases.

| Symbol | Description | Return value | DAG pattern |
|--------|-------------|--------------|-------------|
| `__lesf2` | LE/LT/EQ/NE comparison | -1 if a<b, 0 if a==b, 1 if a>b, **1 if NaN** | `RTLIB::OLE_F32` |
| `__eqsf2` | Alias of `__lesf2` | Same | `RTLIB::OEQ_F32` |
| `__ltsf2` | Alias of `__lesf2` | Same | `RTLIB::OLT_F32` |
| `__nesf2` | Alias of `__lesf2` | Same | `RTLIB::UNE_F32` |
| `__cmpsf2` | Alias of `__lesf2` (ELF only) | Same | -- |
| `__gesf2` | GE/GT comparison | -1 if a<b, 0 if a==b, 1 if a>b, **-1 if NaN** | `RTLIB::OGE_F32` |
| `__gtsf2` | Alias of `__gesf2` | Same | `RTLIB::OGT_F32` |
| `__unordsf2` | Unordered check | 0 if both are numbers, 1 if either is NaN | `RTLIB::UO_F32` |

**Notes:** The difference between `__lesf2` and `__gesf2` is solely in the
NaN return value.  Callers use the sign of the return value to determine
the comparison result.

### `__fe_getround` / `__fe_raise_inexact`

| Field | Value |
|-------|-------|
| **Source** | compiler-rt C (`fp_mode.c`), compiled at `-Os` |
| **Description** | Floating-point environment support stubs. `__fe_getround` returns the current rounding mode (always round-to-nearest on i8085). `__fe_raise_inexact` is a no-op. |
| **Notes** | Required by certain compiler-rt FP routines. Not directly called by user code. |

---

## 7. Float-Int Conversion

### Float to Integer

| Symbol | Source | Signature | Description | DAG pattern |
|--------|--------|-----------|-------------|-------------|
| `__fixsfsi` | compiler-rt `fixsfsi.c` (`-O0`) | `int32_t __fixsfsi(float a)` | `float` -> signed `i32` (truncates toward zero) | `ISD::FP_TO_SINT` `MVT::i32` via `RTLIB::FPTOSINT_F32_I32` |
| `__fixunssfsi` | compiler-rt `fixunssfsi.c` (`-O0`) | `uint32_t __fixunssfsi(float a)` | `float` -> unsigned `i32` | `ISD::FP_TO_UINT` `MVT::i32` via `RTLIB::FPTOUINT_F32_I32` |
| `__fixsfdi` | compiler-rt `fixsfdi.c` (`-O0`, `-D__SOFTFP__`) | `int64_t __fixsfdi(float a)` | `float` -> signed `i64` (sret) | Default LLVM expansion for `FP_TO_SINT` `MVT::i64` |
| `__fixunssfdi` | compiler-rt `fixunssfdi.c` (`-O0`, `-D__SOFTFP__`) | `uint64_t __fixunssfdi(float a)` | `float` -> unsigned `i64` (sret) | Default LLVM expansion for `FP_TO_UINT` `MVT::i64` |

### Integer to Float

| Symbol | Source | Signature | Description | DAG pattern |
|--------|--------|-----------|-------------|-------------|
| `__floatsisf` | compiler-rt `floatsisf.c` (`-O0`) | `float __floatsisf(int32_t a)` | signed `i32` -> `float` | `ISD::SINT_TO_FP` `MVT::i32` via `RTLIB::SINTTOFP_I32_F32` |
| `__floatunsisf` | compiler-rt `floatunsisf.c` (`-O0`) | `float __floatunsisf(uint32_t a)` | unsigned `i32` -> `float` | `ISD::UINT_TO_FP` `MVT::i32` via `RTLIB::UINTTOFP_I32_F32` |
| `__floatdisf` | `sysroot/libi8085/builtins/floatdisf.c` (custom) | `float __floatdisf(int64_t a)` | signed `i64` -> `float` (sret for arg) | Default LLVM expansion for `SINT_TO_FP` `MVT::i64` |
| `__floatundisf` | `sysroot/libi8085/builtins/floatundisf.c` (custom) | `float __floatundisf(uint64_t a)` | unsigned `i64` -> `float` (sret for arg) | Default LLVM expansion for `UINT_TO_FP` `MVT::i64` |

**Notes on `__floatdisf` / `__floatundisf`:** These are custom
implementations (not from compiler-rt) that manually construct the IEEE 754
bit pattern using integer arithmetic: compute leading zeros with
`__builtin_clzll`, normalize the mantissa, and apply round-to-nearest-even.

---

## 8. Memory Operations

Source: `sysroot/libi8085/builtins/memops.S` (hand-written assembly).

These are used by the compiler for aggregate copies, structure
initialization, and `memcpy`/`memset`/`memmove` calls.  The backend sets
`MaxStoresPerMemcpy = 0` etc. to force library calls for all memory
operations.

### `memcpy`

| Field | Value |
|-------|-------|
| **Symbol** | `memcpy` |
| **Source** | `memops.S` (hand-written assembly) |
| **Signature** | `void *memcpy(void *dst, const void *src, size_t n)` |
| **Args** | `[SP+2..3]` = dst, `[SP+4..5]` = src, `[SP+6..7]` = n |
| **Return** | Original `dst` in `BC` |
| **Description** | Forward byte-by-byte copy from `src` to `dst`. Uses `HL` as source pointer, `BC` as destination pointer, `DE` as counter. Loop: `MOV A,M / STAX B / INX / DCX` until `DE` == 0. |
| **Notes** | No overlap handling (use `memmove` for overlapping regions). |

### `memset`

| Field | Value |
|-------|-------|
| **Symbol** | `memset` |
| **Source** | `memops.S` (hand-written assembly) |
| **Signature** | `void *memset(void *dst, int c, size_t n)` |
| **Args** | `[SP+2..3]` = dst, `[SP+4]` = c (only low byte used), `[SP+6..7]` = n |
| **Return** | Original `dst` in `BC` |
| **Description** | Fills `n` bytes at `dst` with the byte value `c`. Uses `HL` as destination pointer, `DE` as counter. |
| **Notes** | The `c` argument occupies 2 bytes on the stack (i16 ABI) but only the low byte is used. |

### `memmove`

| Field | Value |
|-------|-------|
| **Symbol** | `memmove` |
| **Source** | `memops.S` (hand-written assembly) |
| **Signature** | `void *memmove(void *dst, const void *src, size_t n)` |
| **Args** | `[SP+2..3]` = dst, `[SP+4..5]` = src, `[SP+6..7]` = n |
| **Return** | Original `dst` in `BC` |
| **Description** | Overlap-safe memory copy. Compares `dst` vs `src`: if `dst < src`, copies forward (same loop as `memcpy`); otherwise copies backward from `src+n-1` to `dst+n-1`. |
| **Notes** | Returns immediately if `n == 0`. |

---

## Build System

All routines are compiled by `tooling/build-libgcc.sh` and archived into
`sysroot/lib/libgcc.a`.

### Source categories

| Category | Source directory | Compilation |
|----------|----------------|-------------|
| Hand-written assembly (multiply, memops, clzsi2) | `sysroot/libi8085/builtins/*.S` | `clang -target i8085-unknown-elf -c` |
| Target-specific C (division, FP wrappers, conversions) | `sysroot/libi8085/builtins/*.c` | `clang -O0 ... -emit-llvm` then `llc -filetype=obj` |
| compiler-rt C (shifts, comparisons, conversions, fp_mode) | `llvm-project/compiler-rt/lib/builtins/*.c` | `clang -Os/-O0 ... -emit-llvm` then `llc -filetype=obj` |

### Optimization levels

| Level | Used for |
|-------|----------|
| `-O0` | All multiply, division, floating-point arithmetic/conversion routines (avoids complex codegen) |
| `-Os` | `ashldi3`, `ashrdi3`, `lshrdi3`, `clzdi2`, `cmpdi2`, `ctzdi2`, `ctzsi2`, `negdi2`, `ucmpdi2`, `fp_mode` |

### Complete file inventory

```
sysroot/lib/builtins/
  int_mul.o         - __mul8, __mul16, __mul32, __mulsi8, __mulsi8_hi8,
                      __mulsi8_lo8, __mulsi16, __mulsi16_shr8,
                      __mulsi16_hi16, __mulsi16_lo16, __mulsi32,
                      __mulsi32_shr16, __mulsi32_hi32
  int_udiv.o        - __udiv8, __udiv16, __udiv32, __urem8, __urem16, __urem32
  int_sdiv.o        - __sdiv8, __sdiv16, __sdiv32, __srem8, __srem16, __srem32
  int_divdi3.o      - __udivdi3, __umoddi3, __udivmoddi4, __divdi3, __moddi3
  muldi3.o          - __muldi3
  ashldi3.o         - __ashldi3
  ashrdi3.o         - __ashrdi3
  lshrdi3.o         - __lshrdi3
  negdi2.o          - __negdi2
  clzsi2.o          - __clzsi2
  clzdi2.o          - __clzdi2
  ctzsi2.o          - __ctzsi2
  ctzdi2.o          - __ctzdi2
  cmpdi2.o          - __cmpdi2
  ucmpdi2.o         - __ucmpdi2
  addsf3.o          - __addsf3
  subsf3.o          - __subsf3
  mulsf3.o          - __mulsf3
  divsf3.o          - __divsf3
  negsf2.o          - __negsf2
  comparesf2.o      - __lesf2, __eqsf2, __ltsf2, __nesf2, __cmpsf2,
                      __gesf2, __gtsf2, __unordsf2
  fp_mode.o         - __fe_getround, __fe_raise_inexact
  fixsfsi.o         - __fixsfsi
  fixunssfsi.o      - __fixunssfsi
  fixsfdi.o         - __fixsfdi
  fixunssfdi.o      - __fixunssfdi
  floatsisf.o       - __floatsisf
  floatunsisf.o     - __floatunsisf
  floatdisf.o       - __floatdisf
  floatundisf.o     - __floatundisf
  memops.o          - memcpy, memset, memmove
```
