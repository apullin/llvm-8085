# I8085 ABI (LLVM port)

This document describes the **current** application binary interface as
implemented in `llvm/lib/Target/I8085`. It is meant to be the ground truth for
tests and tooling, not a promise of compatibility with other toolchains.

## Data layout

From `I8085TargetMachine`:

- Endianness: little
- Pointer size: 16-bit
- Alignments: 8-bit for i8/i16/i32/i64/f32/f64 and aggregates

Data layout string:

```
e-p:16:8-i8:8-i16:8-i32:8-i64:8-f32:8-f64:8-n8-a:8
```

## CPU context (8085)

Programmer-visible registers are the 8-bit A, B, C, D, E, H, and L registers.
The pairs BC, DE, and HL are used for 16-bit operations. SP and PC are 16-bit.
Flags include sign, zero, auxiliary carry, parity, and carry (S, Z, AC, P, CY).

## Stack model

- Stack grows downward (8085 `PUSH`/`CALL` semantics).
- Stack alignment: 1 byte.
- Caller allocates outgoing argument space via `CALLSEQ_START` and cleans it
  with `CALLSEQ_END` (caller cleanup).

## Argument passing

All arguments (fixed and varargs) are passed on the stack.

Lowering uses `ArgCC_I8085_Vararg` for all calls and all functions. That CC
assigns stack slots in **argument order** and uses **1-byte alignment** for all
scalar types.

Scalar sizes:

- i8: 1 byte
- i16: 2 bytes
- i32: 4 bytes
- i64: 8 bytes
- f32: 4 bytes
- f64: 8 bytes

The first argument is placed at offset 0 from the adjusted SP, subsequent
arguments follow at increasing offsets.

### Byval

`byval` aggregates are copied by the caller into the outgoing stack area
(`memcpy`), and the callee receives a pointer to that stack copy.

### sret

Aggregate returns are lowered to a hidden `sret` pointer. The pointer is passed
like any other argument (on the stack) and points to caller-allocated storage.

## Return values

Scalar returns use registers:

- i8: `A`
- i16: `BC`
- i32: `BC` (low 16) + `DE` (high 16)
- i64: returned via hidden `sret` pointer (caller-allocated)
- f32: bitcast to i32, returned as `BC`/`DE`
- f64: bitcast to i64, returned as `IAX`/`IBX`

Aggregates larger than 8 bytes are returned via `sret`.

## Register preservation

`CSR_Normal` is empty, so **no GPRs are callee-saved**. Callers must assume
all registers are clobbered across calls. `SP` and `PC` are always reserved by
the compiler. `HL`/`A` can be reserved internally when GR32 pseudos are used,
but that is an internal compiler constraint rather than an ABI rule.

## Comparison to SDCC (Z80 SDCC ABI, version 0)

The closest published ABI in SDCC is the Z80 `__sdcccall(0)` convention. It
passes all parameters on the stack **right-to-left**, returns 8-bit in `L`,
16-bit in `HL`, 24-bit in `EHL`, and 32-bit in `DEHL`, and uses a hidden sret
pointer for larger/aggregate returns. Unless `__z88dk_callee` is used, the
caller cleans the stack.

Our LLVM I8085 ABI differs in **return registers** (`A`/`BC`/`BC+DE` vs
`L`/`HL`/`DEHL`) and in **argument layout** (stack slots assigned in argument
order at increasing offsets). This means **binary compatibility with SDCC
`__sdcccall(0)` is not guaranteed** without a shim or a deliberate ABI change.
