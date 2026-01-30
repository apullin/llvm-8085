# I8085 LLVM Port Status

Last updated: 2026-01-30

## Baseline
- llvm-project fork: `llvm-project/`
- Sysroot seed: `sysroot/` (crt0 + linker scripts)
- Simulator/tracer: `i8085-trace/`
- Tests: 151 in `llvm-project/llvm/test/CodeGen/I8085`

## Definition of "First-Class" (Embedded Target)
### ISA / MC / Asm / Disasm
- [x] Asm parser + printer present
- [x] Disassembler present
- [x] MC tests (opcode roundtrip + opcode map baseline)
- [ ] Full opcode encoding/decoding audit vs manual (sim-assisted)
- [x] Baseline asm diagnostics coverage for invalid forms

### CodeGen / ABI
- [x] Core instruction selection and lowering
- [x] Stack realignment + base pointer for var-sized allocas
- [x] Core i32/i64 lowering (arith, shifts, compares, calls)
- [x] Varargs lowering coverage
- [x] Byval/sret coverage at O0..O2
- [x] Legalization/expand for uncommon ops (mul/div/rem, shifts)
- [x] Inline asm constraints and diagnostics coverage
- [x] Explicit errors for unsupported features (atomics/TLS/EH)

### Object / ELF / LLD
- [x] Minimal LLD ELF link smoke test
- [x] MC relocation tests (basic)
- [x] Relocation coverage for i8085 code model
- [x] Object writer/readobj coverage for reloc types

### Clang Driver
- [x] Clang target wiring for `i8085-unknown-elf` (triple, defaults, -mcpu/-mattr)
- [x] Default sysroot/multilib layout for i8085
- [x] Clang driver tests for i8085 triple

### Runtime / Sysroot
- [x] Minimal crt0 (stack, data/bss init scaffolding)
- [x] Data/bss init verified for ROM/RAM layout
- [x] libgcc/libc integration (mul/div/rem, 32/64-bit helpers)
- [x] Minimal sysroot headers (picolibc later)
- [x] Sysroot header sanity test (clang -fsyntax-only)

### Debug / Unwind
- [x] CFI generation for standard frames
- [x] Minimal DWARF line table sanity
- [x] Unwind policy documented (no EH vs minimal)

### Optimization Pipeline
- [x] O0/O1/O2/Os stability (ABI + correctness tests)
- [x] TTI hooks present (minimal)
- [x] Peephole coverage for common idioms

### Tests (Correctness)
- [x] Basic ALU/logic/compare codegen tests
- [x] Roundtrip opcode tests
- [x] Branch/flag edge cases (carry/aux/parity)
- [x] Memory modes + unaligned access tests
- [x] Call/return/stack-frame stress tests
- [x] Varargs/byval/sret/i64 tests
- [x] End-to-end clang -> asm -> link smoke tests
- [x] Port I/O + interrupt/RETI + HLT edge cases
- [x] ABI stress expansions (varargs + sret + byval under O2)

### Tooling
- [x] llvm-objdump/readobj coverage for i8085
- [x] llvm-mc regression tests for edge cases
- [ ] Coverage/export support from simulator (future)

## Current Tranche (2026-01-30)
- [x] Correctness: port I/O + interrupt/RETI + HLT edge cases
- [x] ABI: varargs + sret + byval stress under -O2
- [x] Inline asm/TLS/EH/atomics diagnostics + tests
- [x] Opcode map baseline generation + test

## Next Priorities (after tranche)
1. Picolibc sysroot build + headers/libm integration.
2. Simulator coverage export (stretch).
3. Full opcode audit vs manual (image-based, table-by-table).
