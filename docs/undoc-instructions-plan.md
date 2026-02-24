# Plan: Add Undocumented 8085 Instructions

## Context

The 8085 has 10 undocumented instructions baked into silicon that Intel never officially documented. Several are transformative for code quality — particularly LDSI/LHLX/SHLX which eliminate the carry-clobbering `LXI H,offset; DAD SP; MOV r,M` stack access pattern (our #1 code size bottleneck), and DSUB/ARHL which replace 6-7 byte sequences with single bytes.

Goal: define all 10 in the assembler/disassembler, use the high-impact ones in codegen behind a `-mattr=+undoc` feature flag, update the simulator to execute them, and verify all 75 benchmarks (15 x 5 opt levels) HALT.

## The Instructions

| Mnemonic | Opcode | Bytes | What it does | Codegen value |
|----------|--------|-------|-------------|---------------|
| DSUB | 08 | 1 | HL = HL - BC, all flags | HIGH — replaces 6-byte SUB_16 |
| ARHL | 10 | 1 | Arithmetic right shift HL, CY=L[0] | HIGH — replaces 7-byte SRA_16 |
| RDEL | 18 | 1 | Rotate DE left through carry | MEDIUM — replaces 6-byte rotate |
| LDHI imm8 | 28 | 2 | DE = HL + imm8, no flags | LOW — struct offset |
| LDSI imm8 | 38 | 2 | DE = SP + imm8, no flags | **CRITICAL** — stack access |
| SHLX | D9 | 1 | [DE] = HL (16-bit store) | **CRITICAL** — paired with LDSI |
| LHLX | ED | 1 | HL = [DE] (16-bit load) | **CRITICAL** — paired with LDSI |
| RSTV | CB | 1 | RST 0x40 if V flag set | NONE — asm only |
| JNX5 addr16 | DD | 3 | Jump if not X5 | NONE — asm only |
| JX5 addr16 | FD | 3 | Jump if X5 | NONE — asm only |

## Key Savings

**Stack access (LDSI+LHLX):** 3 bytes vs 5-8 bytes, NO carry clobber (eliminates PUSH PSW/POP PSW overhead when flags live). This is the single most common pattern in compiler output.

**16-bit subtract (DSUB):** 1 byte vs 6 bytes, only when regs are already HL and BC.

**16-bit arithmetic right shift (ARHL):** 1 byte vs 7 bytes, only when operating on HL.

**Rotate DE left (RDEL):** 1 byte vs 6 bytes, only when operating on DE.

---

## Phase 1: Simulator Support (prerequisite — can't test without it)

All 10 opcodes currently call `InvalidInstruction()` in the simulator.

### Files
- `i8085-trace/include/i8085_cpu.h` — Add `v` and `x5` fields to Flags struct (currently in `pad:3`)
- `i8085-trace/src/i8085_exec.c` — Implement all 10 opcodes in the switch statement
- `i8085-trace/src/i8085_disasm.c` — Correct mnemonics (currently shows "NOP" or wrong insn)

### Implementation
- DSUB: `uint32_t result = (state->h<<8|state->l) - (state->b<<8|state->c);` set Z,S,P,CY,AC,V,X5
- ARHL: `cy = state->l & 1; hl = (int16_t)hl >> 1;` set CY only
- RDEL: rotate DE left through carry, set CY,V
- LDHI: `DE = HL + opcode[1]; pc+=1;` no flags
- LDSI: `DE = SP + opcode[1]; pc+=1;` no flags
- SHLX: `mem[DE] = L; mem[DE+1] = H;` no flags
- LHLX: `L = mem[DE]; H = mem[DE+1];` no flags
- RSTV: if V flag, push PC, jump 0x40; else NOP
- JNX5/JX5: conditional jump on X5 flag, `pc+=2`

### Disassembler fixes
- 0x08→"DSUB", 0x10→"ARHL", 0x18→"RDEL" (currently "NOP")
- 0x28→"LDHI $xx" (2-byte), 0x38→"LDSI $xx" (2-byte)
- 0xCB→"RSTV" (1-byte, currently "JMP" 3-byte — wrong size!)
- 0xD9→"SHLX" (currently "RETI")
- 0xDD→"JNX5 $xxxx" (3-byte), 0xED→"LHLX" (1-byte, currently "CALL" 3-byte — wrong size!)
- 0xFD→"JX5 $xxxx" (3-byte)

### Build & test
- `cd i8085-trace && mkdir -p build && cd build && cmake .. && make`
- Write a small test .S that exercises each instruction, verify on simulator

## Phase 2: Feature Flag + Instruction Definitions (LLVM backend)

### 2A: Subtarget feature

**`I8085.td`** — Add:
```tablegen
def FeatureUndocumented : SubtargetFeature<"undoc", "HasUndocumented", "true",
    "Enable undocumented 8085 instructions">;
def : Proc<"i8085-undoc", [FeatureUndocumented]>;
```

**`I8085Subtarget.h`** — Add member + accessor:
```cpp
bool HasUndocumented = false;
bool hasUndocumented() const { return HasUndocumented; }
```

Usage: `-mattr=+undoc` or `-mcpu=i8085-undoc`

### 2B: Instruction definitions

**`I8085InstrInfo.td`** — Define all 10 using existing format classes:

| Instruction | Format | Key attributes |
|---|---|---|
| DSUB | F1 (1-byte, opcode 0x08) | Defs=[HL,SREG], Uses=[HL,BC] |
| ARHL | F1 (1-byte, opcode 0x10) | Defs=[HL,SREG], Uses=[HL] |
| RDEL | F1 (1-byte, opcode 0x18) | Defs=[DE,SREG], Uses=[DE,SREG] |
| LDHI | F5 (2-byte, opcode 0x28) | Defs=[DE], Uses=[HL] |
| LDSI | F5 (2-byte, opcode 0x38) | Defs=[DE], Uses=[SP] |
| SHLX | F1 (1-byte, opcode 0xD9) | Uses=[DE,HL], mayStore=1 |
| LHLX | F1 (1-byte, opcode 0xED) | Defs=[HL], Uses=[DE], mayLoad=1 |
| RSTV | F1 (1-byte, opcode 0xCB) | isCall=1, hasSideEffects=1 |
| JNX5 | F6 (3-byte, opcode 0xDD) | isBranch=1, isTerminator=1 |
| JX5 | F6 (3-byte, opcode 0xFD) | isBranch=1, isTerminator=1 |

All gated by `let Predicates = [HasUndocumented]`. No ISel DAG patterns (codegen goes through pseudo expansion).

### 2C: Disassembler update

**`I8085Disassembler.cpp`** — Add to `getInstructionSize()`:
- 0x28, 0x38 → return 2
- 0xDD, 0xFD → return 3
- (0x08, 0x10, 0x18, 0xCB, 0xD9, 0xED already fall through to return 1)

### 2D: MC tests

Add `test/MC/I8085/undoc.s` — assemble all 10 instructions, verify encoding bytes.

### Verify: `ninja clang llc lld` builds, MC tests pass.

## Phase 3: Codegen — Pseudo Expansion

Thread `bool HasUndoc = STI.hasUndocumented()` through both expansion files. Then modify specific expansions:

### 3A: LDSI + LHLX for 16-bit stack loads (HIGHEST IMPACT)

**File:** `I8085ExpandPseudoInsts.cpp`, `expand<LOAD_16_WITH_ADDR>`

When `baseReg==SP && destReg==HL && offset in [0,255] && DE is dead && HasUndoc`:
```
LDSI offset    ; 2 bytes (DE = SP + offset, NO flag clobber)
LHLX           ; 1 byte  (HL = [DE])
```
= 3 bytes vs 7+ bytes. Eliminates carry clobber entirely.

**DE liveness check:** Use existing `isPhysRegLive()` pattern. If DE is live, fall back to existing LXI+DAD path.

### 3B: LDSI + LDAX D for 8-bit stack loads

When `baseReg==SP && offset in [0,255] && DE is dead && HasUndoc`:
```
LDSI offset    ; 2 bytes
LDAX D         ; 1 byte (A = [DE])
MOV r, A       ; 1 byte (if r != A)
```
= 3-4 bytes vs 5 bytes. No carry clobber.

### 3C: LDSI + SHLX for 16-bit stack stores

When `baseReg==SP && srcReg==HL && offset in [0,255] && DE is dead && HasUndoc`:
```
LDSI offset    ; 2 bytes
SHLX           ; 1 byte ([DE] = HL)
```
= 3 bytes vs 7 bytes. No carry clobber.

For src==BC/DE: copy to HL first, then LDSI+SHLX. Still saves bytes.

### 3D: LDSI + STAX D for 8-bit stack stores

When `baseReg==SP && offset in [0,255] && DE is dead && HasUndoc`:
```
MOV A, srcReg  ; 1 byte (if src != A)
LDSI offset    ; 2 bytes
STAX D         ; 1 byte ([DE] = A)
```
= 3-4 bytes vs 5 bytes. No carry clobber.

### 3E: DSUB for SUB_16

**File:** `I8085ExpandPseudoInsts.cpp`, `expand<SUB_16>`

When `destReg==HL && operandTwo==BC && HasUndoc`:
```
DSUB           ; 1 byte
```
= 1 byte vs 6 bytes.

For other register combos: copy to HL/BC first if cheaper. E.g., dest=HL, op2=DE → MOV B,D / MOV C,E / DSUB = 3 bytes vs 6 bytes.

### 3F: ARHL for SRA_16 by 1

**File:** `I8085ExpandPseudoInsts.cpp`, `expand<SRA_16>`

When `destReg==HL && shift_amount==1 && HasUndoc`:
```
ARHL           ; 1 byte
```
= 1 byte vs ~7 bytes.

### 3G: RDEL for rotate-DE-left

**File:** `I8085ExpandPseudoInsts.cpp`, `expand<RL_16>`

When `destReg==DE && HasUndoc`:
```
RDEL           ; 1 byte
```
= 1 byte vs ~6 bytes. RDEL rotates through carry (same as existing RL_16 semantics).

### 3H: LDSI + LHLX/SHLX in 32-bit expansions

**File:** `I8085ExpandPseudoInsts32.cpp`

Same LDSI+LHLX/SHLX pattern for `LOAD_32_OFFSET_WITH_SP` and `STORE_32_AT_OFFSET_WITH_SP`. Each 32-bit stack access has two 16-bit word accesses — both can use LDSI+LHLX/SHLX.

### Verify: Rebuild LLVM. Run all 75 benchmarks with `-mattr=+undoc`. All must HALT.

## Phase 4: Runtime Library

### 4A: Update hand-written assembly

Use conditional assembly in `sysroot/libi8085/builtins/*.S`:

Priority by `dad sp` count:
1. softfp.S (~210 instances)
2. int_arith64.S (~112)
3. int_mul.S (~111)
4. int_divdi3.S (~96)
5. int_div.S (~53)
6. int_shift64.S (~50)

Replace `lxi h, N; dad sp` → `ldsi N` + `lhlx`/`shlx`/`ldax d`/`stax d` where applicable.

Also replace manual 16-bit subtract sequences with `dsub`, and manual HL arithmetic right shifts with `arhl`.

### 4B: Build libgcc-undoc.a

Modify `tooling/build-libgcc.sh` to accept a flag and pass `-mattr=+undoc` (or `-Wa,-mattr=+undoc` for .S files). Build a separate `libgcc-undoc.a`.

### Verify: Run all 75 benchmarks with `-mattr=+undoc` and `LIBGCC=libgcc-undoc.a`. All must HALT.

## Phase 5: Testing & Measurement

1. **Simulator unit test**: standalone binary exercising all 10 instructions with edge cases
2. **MC round-trip test**: assemble + disassemble all 10 instructions
3. **75 benchmark regression**: `-mattr=+undoc` at O0/O1/O2/Os/Oz — all HALT
4. **75 benchmark baseline**: no flag (existing behavior) — verify no regressions
5. **Code size comparison**: measure `.text` delta per benchmark at each opt level

## Bug Risk

| Risk | Mitigation |
|---|---|
| LDSI clobbers DE — miscompile if DE is live | Use `isPhysRegLive()` check, same pattern as existing HL-clobber detection |
| DSUB flag semantics differ from SUB_16 expansion | DSUB sets all flags including V/X5; existing code models SREG as one register, so this is safe |
| Disassembler wrong instruction size for 0xCB/0xED | Currently disassembled as 3-byte JMP/CALL — must fix to 1-byte |
| LDSI unsigned 8-bit immediate — offsets >255 | Fall back to existing LXI+DAD pattern for large offsets |
| Simulator flags byte layout for PUSH PSW | Must verify V(bit1) and X5(bit5) roundtrip correctly through PUSH/POP PSW |
