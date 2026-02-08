# LLVM-8085 Project Journal

## Development Log

## 2026-02-06 DONE Redundant flag-test elimination peephole

**What**: Added peephole pass to remove ORA A when preceded by ANI/ORI/XRI/ANA/ORA/XRA (or memory-indirect variants), since those instructions already set Z/S/P flags identically.

**Where**: `llvm-project/llvm/lib/Target/I8085/I8085Peephole.cpp` (after line 157)

**Why**: Post-pseudo-expansion often emits ORA A to set flags before conditional jumps, but the preceding logical instruction already sets flags correctly. Saves 4 cycles per eliminated ORA A.

## 2026-02-06 DONE MOV_32 batch optimization

**What**: MOV_32 expansion now batch-loads 4 bytes into B/C/D/E then batch-stores when those registers are dead, saving 6 LXI+DAD pairs (~120 cycles per MOV_32). Falls back to byte-by-byte copy through A when any of B/C/D/E are live.

**Where**: `llvm-project/llvm/lib/Target/I8085/I8085ExpandPseudoInsts32.cpp`, `expand<I8085::MOV_32>`

**Why**: MOV_32 copies 4 bytes one at a time with 8 LXI+DAD pairs (~216 cycles). Batching halves the stack access overhead.

**Technical notes**: Uses `computeRegisterLiveness` to check B/C/D/E liveness. Currently not triggering in any benchmark (registers always live at MOV_32 points), but infrastructure is in place for when register pressure improves.

## 2026-02-06 FIX Narrow 32-bit comparison debugging

**What**: Fixed the narrow comparison optimization that wasn't triggering despite correct MIR patterns. Root cause: `preScanKnownZeroBytes` was treating USE operands as DEFs, overwriting valid masks with 0.

**Where**: `I8085ExpandPseudoInsts32.cpp`, `preScanKnownZeroBytes()` and `expand<JMP_32_IF_NOT_EQUAL>`

**Why**: Instructions like JMP_32_IF_NOT_EQUAL and STORE_32_ADDR_CONTENT have GR32 registers as operand(0) but as USES (not defs). Without checking `isDef()`, the pre-scan treated them as definitions and overwrote the known-zero mask to 0.

**Technical notes**: Fix was two-fold: (1) Added `isDef()` check in pre-scan. (2) Added backward walk from JMP instruction to find definitions even after earlier pseudos were expanded and erased. Extracted `computeKnownZeroMask()` as a static helper for reuse. Key gotcha: in Release builds, LLVM_DEBUG is compiled out -- use `errs()` for diagnostics.

## 2026-02-06 DONE Narrow 32-bit comparison optimization

**What**: JMP_32_IF_NOT_EQUAL now skips comparing bytes known to be zero in both operands. For crc32's inner loop (ANDI 1 vs LOAD 0), only byte 0 is compared instead of all 4 bytes.

**Where**: `I8085ExpandPseudoInsts32.cpp` -- `preScanKnownZeroBytes()`, `computeKnownZeroMask()`, `expand<JMP_32_IF_NOT_EQUAL>`

**Why**: Each skipped byte saves ~27 cycles (LXI+DAD+MOV+CPI+JNZ). For crc32 inner loop: 3 bytes skipped x 8 iterations x 32 outer = ~20K cycles saved. Measured: crc32 O2 improved from 274,798 to 266,266 cycles (-3.1%).

## 2026-02-06 DONE Commit all tier-2 optimizations (submodule 815f915)

**What**: Committed flag-test elimination, MOV_32 batch, and narrow comparison optimizations as submodule commit `815f915f7acd`.

**Where**: `llvm-project/` git repo, files: `I8085ExpandPseudoInsts32.cpp`, `I8085Peephole.cpp`

**Why**: All 56/56 benchmarks pass (14 benchmarks x 4 opt levels). Cumulative results: crc32 -3.1% at O2 from narrow comparisons. Other benchmarks unchanged (MOV_32 batch not triggering, flag-test elimination has minimal cycle impact).

## 2026-02-06 DONE Tier-3 optimizations: binOperation batching, 64-bit audit, rotate64 fix

**What**: Completed tier-3 plan items: (1) binOperation() batched register loading for AND/OR/XOR_32, (2) 64-bit torture test created and passing, (3) 64-bit runtime audit — found and fixed DAD-clobbers-carry bug in int_rotate64.S.

**Where**:
- `I8085ExpandPseudoInsts32.cpp`: binOperation() optimization (submodule 269aa7f)
- `tooling/examples/arith64_torture/arith64_torture.c`: new 64-bit torture test
- `sysroot/libi8085/builtins/int_rotate64.S`: rotate64 DAD-clobbers-carry fix
- `tooling/examples/benchmark.sh`: added arith64_torture (now 15 benchmarks)

**Why**: binOperation saves ~120 cycles when B/C/D/E are dead (currently triggers on bitops_torture: -688cy at O2). The rotate64 bug would produce incorrect results for any non-byte-aligned rotation: `LXI+DAD SP` between `RAL`/`RAR` and the rotation chain destroyed the wrap-around carry bit.

**Technical notes**:
- binOperation batching: loads 4 bytes into B/C/D/E, processes against op2, stores — 3 LXI+DAD instead of 8
- rotate64 fix: replaced `LXI H,0/DAD SP` with 7×DCX (rotl) and 7×INX (rotr) — flag-safe navigation
- arith64_torture: hash-accumulator pattern keeps text small (~15KB), exercises carry propagation, edge cases
- 64-bit audit: __adddi3, __subdi3, __ashldi3, __lshrdi3, __ashrdi3, __udivmoddi4 all clean — no bugs
- C fallback functions (fixsfdi, floatdisf, etc.) exist but no benchmark uses them — deferred
- All 60/60 benchmarks pass (15 benchmarks × 4 opt levels)

## 2026-02-06 DONE Optimize __mul32 with register-resident result

**What**: Restructured `__mul32` to keep the 32-bit result in BC:DE registers instead of on the stack. Also replaced a redundant LXI+DAD with 3×DCX in the bit-test path.

**Where**: `sysroot/libi8085/builtins/int_mul.S`, `__mul32` function

**Why**: Each add-iteration previously did 26 instructions (load result from stack, add, store back). With register-resident result: 17 instructions (add multiplicand from memory directly to BC:DE). Saves ~78 cycles per add-iteration.

**Technical notes**:
- DAD-based 16-bit accumulation was analyzed and rejected: HL is both the only memory pointer AND only DAD accumulator, creating conflicts. Carry save/restore overhead exceeds the savings from DAD.
- Register-resident approach works because BC:DE are free during the multiply loop (multiplicand/multiplier live on stack).
- Measured: bitops_torture -7.4%, arith64_torture -6.2%, opt_sanity -6.2% at O1. 24-byte code size reduction.
- mul_torture unchanged because it tests __mulsi8/__mulsi16 variants, not __mul32 directly.

## 2026-02-06 DONE Optimize __udivmod32 with register-resident quotient

**What**: Rewrote `__udivmod32` to keep the 32-bit quotient register-resident in BC:DE instead of on the stack. Also optimized `__urem32` similarly (BC:DE free since no quotient needed). Both now use contiguous dividend+remainder layout (8 bytes) and trial subtraction (restoring division) instead of MSB-first comparison.

**Where**: `sysroot/libi8085/builtins/int_div.S`, functions `__udivmod32` and `__urem32`

**Why**: The original `__udivmod32` stored quotient on the stack, requiring extra LXI+DAD+MOV sequences every iteration. Register-resident quotient eliminates those stack accesses. Contiguous 8-byte dividend+remainder layout enables a single INX H chain for the shift (no separate LXI+DAD between dividend and remainder). Trial subtraction eliminates the 4-byte comparison before subtraction (subtract first, check carry, restore if needed).

**Technical notes**:
- Quotient shift is pure register ops: 4x (MOV A,r / ADD/ADC A / MOV r,A) = ~56 cycles vs ~107 for stack-based version
- Trial subtraction: subtract divisor from remainder, if carry (borrow), add it back. Avoids separate 4-byte >= comparison
- BC:DE must be saved/restored (PUSH/POP) around the subtraction since they temporarily hold divisor bytes
- INR C sets quotient bit 0 safely because C was just shifted left (bit 0 guaranteed 0)
- Stack layout: [SP+0..3] = dividend, [SP+4..7] = remainder (contiguous for 8-byte shift chain)

## 2026-02-06 FIX Pre-existing division wrapper bugs (8/16/32-bit)

**What**: Fixed 6 wrapper functions that used CALL instead of JMP for tail calls (`__udiv8`, `__udiv16`, `__udiv32`) or failed to re-push arguments (`__urem8`, `__urem16`). Also fixed `__sdiv32` and `__srem32` dividend load offsets (was `lxi h,12`, should be `lxi h,8`).

**Where**: `sysroot/libi8085/builtins/int_div.S`, functions: `__udiv8`, `__urem8`, `__udiv16`, `__urem16`, `__udiv32`, `__sdiv32`, `__srem32`

**Why**: The CALL instruction pushes a 2-byte return address, shifting stack offsets by +2 so `__udivmodN` reads the return address bytes instead of actual arguments. This was never caught because previous tests used compile-time constants that were constant-folded by LLVM. The `__sdiv32`/`__srem32` offset bug caused dividend/divisor confusion (e.g., -1000000/127 computed 127/127=1 then negated to -1 instead of -7874).

**Technical notes**:
- `__udiv8/16/32`: Changed `call __udivmodN` to `jmp __udivmodN` (true tail call, same stack frame)
- `__urem8`: Loads args from stack, packs into BC, pushes, then calls `__udivmod8` (stack offset correct)
- `__urem16`: Loads all 4 arg bytes, pushes divisor then dividend, calls `__udivmod16`, moves DE->BC
- `__sdiv32`/`__srem32`: After push psw (+2) + push d (+2) + push b (+2) = +6, dividend at [SP+8] not [SP+12]
- These bugs are pre-existing (present since the original hand-written assembly), not introduced by the optimization

## 2026-02-06 DISCOVERY Pre-existing 64-bit division bug in int_divdi3.S

**What**: `__udivdi3`/`__udivmoddi4` produces wrong results for non-trivial 64-bit divisions. Example: 0xFFFFFFFFFFFFFFFF / 2 returns 0x005571D09ADE4A18 instead of 0x7FFFFFFFFFFFFFFF.

**Where**: `sysroot/libi8085/builtins/int_divdi3.S`

**Why**: Confirmed pre-existing by testing with unmodified assembly -- same wrong result. Not related to any changes made in this optimization. Deferred: 64-bit division tests in div_torture.c use non-volatile inputs (constant-folded) to work around this.

**Technical notes**: The 64-bit division library functions need a full audit/rewrite, but that's a separate task from the 32-bit optimization work. Simple cases (100/7) also fail, suggesting a fundamental algorithm bug rather than an edge case.

## 2026-02-06 DONE Optimize __adddi3/__subdi3 with two-pass register staging

**What**: Rewrote both functions to eliminate 12 LXI+DAD SP pairs and all 14 PUSH/POP PSW carry-preservation operations. Uses a two-pass approach: stage one operand into registers, then walk the other with INX H through an unbroken carry chain.

**Where**: `sysroot/libi8085/builtins/int_arith64.S`, functions `__adddi3` and `__subdi3`

**Why**: The original approach used 16 LXI+DAD SP pairs (2 per byte × 8 bytes) plus 14 PUSH/POP PSW to preserve carry across DAD SP. Total overhead: ~474 cycles. New approach: 5 LXI+DAD SP pairs, 0 PSW operations.

**Technical notes**:
- Key trick: `MVI A, 0 / ADC A` captures carry as a byte (0 or 1); `ADI 0xFF` restores it (0+0xFF=0xFF carry=0, 1+0xFF=0x00 carry=1)
- `POP B` (non-PSW pop) does NOT affect flags — safe mid-carry-chain for loading staged bytes
- Split into two 4-byte halves with carry bridged via the ADI trick
- Also cleaned up ~450 lines of dead code from abandoned implementation attempts
- arith64_torture: -9,275 clocks (-0.67%), -348 bytes, -1,164 instructions

## 2026-02-06 DONE Optimize __mulsi16 variants with register-resident result + XTHL

**What**: Optimized 4 multiply functions (__mulsi16, __mulsi16_shr8, __mulsi16_hi16, __mului16) to keep 32-bit result in BC:DE and use XTHL for flag-safe multiplier access.

**Where**: `sysroot/libi8085/builtins/int_mul.S`

**Why**: Previous versions allocated 4 bytes on stack for result, requiring LXI+DAD SP per iteration. Register-resident result + direct ADD M/ADC M saves ~124 cycles per add iteration.

**Technical notes**:
- `XTHL` (opcode 0xE3): exchanges HL with [SP] in 16 cycles, does NOT affect flags. Perfect for swapping multiplier on/off stack without losing carry from the shift-right.
- Multiplier shifted right in HL, carry captured, XTHL saves it back, JNC uses the preserved carry
- mul_torture: -7,248 clocks (-2.6%), -76 bytes

## 2026-02-06 DONE GR32 conditional scratch allocation (4 vs 8 bytes)

**What**: Compiler backend now allocates 4 bytes instead of 8 when only one of IAX/IBX is used. When only IBX is used, it's remapped to offset 0.

**Where**: `llvm-project/.../I8085FrameLowering.cpp`, `I8085MachineFunctionInfo.h`, `I8085ExpandPseudoInsts32.cpp`

**Why**: Most functions only use one GR32 register. Saves 4 bytes of stack frame per such function.

**Technical notes**: Added `IBXRemappedToZero` flag to MachineFunctionInfo. getScratchOffset() checks the flag to remap IBX from base+4 to base+0. libgcc.a -12 bytes.

## 2026-02-07 DONE i8085-trace periodic timer support

**What**: Added `--timer=CODE:PERIOD` CLI option to the i8085-trace simulator. Triggers an RST interrupt every PERIOD T-states. Supports multiple timers, text names (e.g. `--timer=rst6.5:30720`), and HLT wake-on-interrupt.

**Where**: `i8085-trace/src/main.cpp` — PeriodicTimer struct, CLI parsing, execution loop check

**Why**: FreeRTOS needs a periodic tick interrupt to drive its scheduler. RST 6.5 at 30720 T-states = 100 Hz at 3.072 MHz.

## 2026-02-07 DONE FreeRTOS port for Intel 8085

**What**: Complete FreeRTOS port with preemptive scheduling, static allocation, two-task demo proving time-sliced execution. Both tasks increment memory-mapped markers equally (90/90 after 500K steps).

**Where**:
- `FreeRTOS-Kernel/portable/GCC/I8085/` — portmacro.h, port.c, portasm.S
- `FreeRTOS/` — FreeRTOSConfig.h, startup.S, linker.ld, demo_basic.c, Makefile

**Why**: Demonstrates the i8085 backend can run a real RTOS with preemptive context switching.

**Technical notes**:
- StackType_t = uint8_t, BaseType_t = int16_t, TickType_t = uint16_t
- Context frame (14 bytes): pvParams + dummy_ret + PC + PSW + BC + DE + HL
- RST 6.5 at vector 0x0034 for tick ISR; SIM mask 0x0D unmasks RST 6.5
- ISR stack at 0xFD00 (64 bytes); main stack at 0xFE00 (256 bytes)
- Binary: 11.5KB with tasks.c at -O0, rest at -Oz (gc-sections enabled)

## 2026-02-07 DISCOVERY Backend codegen bug at -O1+ (pointer-to-struct-member comparison)

**What**: FreeRTOS `vTaskSwitchContext()` crashes at -O1 and above. The compiled code computes the wrong address for the list sentinel comparison in `listGET_OWNER_OF_NEXT_ENTRY()` — uses a stack address (0xFCFF) instead of the actual BSS sentinel address (0x2196). This causes `pvOwner` to be read from uninitialized memory, setting `pxCurrentTCB = NULL` and crashing the scheduler.

**Where**: Compiled output of FreeRTOS `tasks.c`, function `vTaskSwitchContext` at ~0x1114-0x118D. The comparison at 0x1176 (CMP H) compares against a stack-frame address instead of &pxReadyTasksLists[1].xListEnd.

**Why**: The pattern involves taking the address of a struct member (`&pxList->xListEnd`) and comparing it to a pointer obtained by traversing the list. At -O0 this works correctly; at -O1+ the address computation is wrong.

**Technical notes**:
- Symptom: system resets every ~20K steps, pxCurrentTCB at 0x216E overwritten to 0x0000
- Root cause: address of `xListEnd` sentinel computed as stack_addr + offset instead of pxList + offset
- Tested: -O0 works (markers 51/52), -O1/-O2/-Os/-Oz all fail (markers 0/0)
- Workaround: compile tasks.c at -O0 (Makefile has special rule)
- Likely a register allocation or stack-slot computation bug in the i8085 ISel/RA

## 2026-02-07 FIX Machine Copy Propagation kills live HL value (Defs=[HL] on pseudos)

**What**: Fixed the -O1+ codegen bug that crashed FreeRTOS. Root cause: 6 pseudo instructions (STORE_16, STORE_8, LOAD_16_WITH_ADDR, LOAD_8_WITH_ADDR, STORE_8_AT_OFFSET_WITH_SP, STORE_16_AT_OFFSET_WITH_SP) declared `Defs=[HL]` in their TableGen definitions, but they only conditionally use HL as scratch (preserving it via PUSH/POP when `isHLOrSubRegLive()` returns true). Machine Copy Propagation saw `implicit-def $hl` and deleted a `$hl = COPY $bc` that was needed, leaving HL with a stale stack address instead of the list base pointer.

**Where**: `llvm-project/llvm/lib/Target/I8085/I8085InstrInfo.td` (removed HL from Defs of 6 pseudos), `I8085ExpandPseudoInsts.cpp` (updated comments), `I8085InstrInfo.cpp` (updated comments)

**Why**: The `Defs=[HL]` declaration told all post-RA passes that HL was unconditionally clobbered by these pseudos. But the expansion code only uses HL as scratch and PUSH/POP's it when live. Machine Copy Propagation used this false clobber to eliminate a COPY that the `ADD_16` instruction needed to compute `&pxList->xListEnd`.

**Technical notes**:
- The bug was hard to reproduce in minimal test cases because it required sufficient register pressure for the reg allocator to spill to stack and insert the COPY that would later be deleted
- FreeRTOS now compiles at -Oz for ALL kernel files (no more -O0 workaround)
- Binary: 8.6KB (down from 11.5KB with -O0) — 25% reduction
- All 55/56 benchmarks pass (1 pre-existing div_torture O0 link error)
- FreeRTOS verified at -O0, -O1, -O2, -Oz

## 2026-02-07 DONE FreeRTOS queue demo + stack sizing fix

**What**: Created queue-based inter-task communication demo (producer/consumer). Fixed stack overflow crash at -O0 by increasing configMINIMAL_STACK_SIZE from 64 to 256 bytes, ISR stack from 64 to 256 bytes. Added `configSTACK_DEPTH_TYPE = unsigned int` to avoid uint8_t truncation.

**Where**: `FreeRTOS/demo_queue.c`, `FreeRTOS/FreeRTOSConfig.h`, `FreeRTOS/linker.ld`, `FreeRTOS/Makefile`

**Why**: 64-byte task stacks were insufficient for queue functions at -O0 (huge stack frames without optimization). The queue demo exercises xQueueSend/xQueueReceive, providing more comprehensive validation of the FreeRTOS port.

**Technical notes**:
- Producer at prio 1, consumer at prio 2 (higher). SENT/RECV markers off by 1 due to preemption race — expected behavior.
- Sum verification: RECV=78, SUM=3081 = 78×79/2 — mathematically correct
- ISR stack: 0xFB00-0xFC00 (256B), main stack: 0xFC00-0xFE00 (512B), markers: 0xFE00+

## 2026-02-07 FIX Two off-by-one/two bugs in 64-bit division wrapper functions

**What**: Fixed two stack offset bugs in `__udivdi3`, `__divdi3`, and `__moddi3` wrappers in `int_divdi3.S`. The core `__udivmoddi4` algorithm was correct; only the wrapper argument-passing was broken.

**Where**: `sysroot/libi8085/builtins/int_divdi3.S` -- `__udivdi3` (line ~629), `__divdi3` (line ~966), `__moddi3` (line ~1214) for bug 1; `__divdi3` (line ~1022), `__moddi3` (line ~1271) for bug 2. Also updated `tooling/examples/div_torture/div_torture.c` to use volatile i64 inputs.

**Why**: 64-bit division produced wrong results for values with non-zero MSB byte. Example: `0xFFFFFFFFFFFFFFFF / 2` returned `0x005571D09ADE4A18` instead of `0x7FFFFFFFFFFFFFFF`.

**Technical notes**:
- Bug 1 (off-by-one): Sliding-window push pattern `lxi h, 20; dad sp; mov d, m; dcx h; mov e, m; push d` reads bytes at SP+20 and SP+19, but the target pair bytes are at SP+20 and SP+21. Fixed by changing `lxi h, 20` to `lxi h, 21` in all b/a pair pushes across `__udivdi3`, `__divdi3`, `__moddi3` (24 instances). `__umoddi3` was NOT affected (uses offset 29, which is correct for its frame).
- Bug 2 (off-by-two): In `__divdi3` and `__moddi3`, the sret pointer push used `lxi h, 22` but sret is at SP+20 after push psw(+2) + push b(+8) + push a(+8) = 18 bytes from original SP where sret was at SP+2. So 2+18=20, not 22.
- Comprehensive test: 17 test cases covering unsigned div, signed div (positive/negative/both), unsigned mod, signed mod, edge cases (MAX/2, equal, 0/x, MAX/1, MIN/-1). All pass.
- div_torture.c: i64 inputs changed to volatile (were non-volatile to work around the bug). Still guarded by `__OPTIMIZE__` at -O0 due to ROM overflow (2839 bytes over 48K limit).
- Full benchmark suite: 56/56 HALTED, no regressions.

## 2026-02-07 DONE LOAD/STORE 32-bit coalescing and cross-operation register forwarding

**What**: Implemented two backend optimizations in I8085ExpandPseudoInsts32.cpp: (1) batch-into-B/C/D/E coalescing for 5 LOAD/STORE 32-bit pseudo expansions, and (2) cross-operation register forwarding that skips unnecessary scratch store+load round-trips between consecutive batch-mode 32-bit operations.

**Where**: `llvm-project/llvm/lib/Target/I8085/I8085ExpandPseudoInsts32.cpp` — 326 insertions, 41 deletions. Submodule commit `e1711ca994bd`.

**Why**: The 8085 has no indexed addressing, so every 32-bit stack access costs 4 × (LXI+DAD+MOV) = 20 bytes. Coalescing reduces this to 2 × (LXI+DAD) + 6 × INX = 12 bytes by batch-loading into B/C/D/E when they're dead after the instruction.

**Technical notes**:
- Workstream 1 (coalescing): Applied to LOAD_32_WITH_ADDR, LOAD_32_OFFSET_WITH_SP, STORE_32_AT_OFFSET_WITH_SP, STORE_32 (non-HL), LOAD_32_WITH_IMM_ADDR. Uses `computeRegisterLiveness` with 20-insn lookahead to check B/C/D/E dead.
- Workstream 2 (forwarding): `tryForwardBCDE()` peeks at the next GR32 pseudo. If it's a binOperation (XOR_32/OR_32/AND_32) consuming the same register, skips Phase 3 store in producer and Phase 1 load in consumer. `BCDEForwarded` DenseMap tracks state across expansion loop restarts.
- O2 code size gains: fp_bench -19.1% (17885->14463), arith64 -20.1% (14871->11883), div_torture -16.4% (23281->19471), bitops -14.8% (15205->12960), float_torture -12.1% (10890->9575), bubble_sort -7.1% (675->627), fib -4.7% (277->264), crc32 -2.0% (656->643). O0 gains even larger (arith64 -27%, fp_bench -22%).
- Forwarding provides small additional gain (~0.2% on arith64) beyond coalescing.
- All 56 benchmarks (O0/O1/O2/Os) HALTED. All 15 benchmarks at -Oz HALTED. FreeRTOS demos verified at -O0, -O1, -O2, -Oz.

## 2026-02-07 DONE FreeRTOS heap_4 dynamic allocation demo

**What**: Implemented and verified a FreeRTOS heap_4 demo that exercises pvPortMalloc/vPortFree with block coalescing, dynamic task creation/deletion, and dynamic queue creation/deletion.

**Where**: `FreeRTOS/demo_heap.c` (new), `FreeRTOS/Makefile` (heap targets + -O1 workaround), `FreeRTOS/FreeRTOSConfig.h` (enabled dynamic allocation)

**Why**: Stress-test the backend with a real-world dynamic memory allocator. heap_4 exercises pointer arithmetic, linked-list manipulation, and block coalescing — all patterns that push the i8085 codegen hard.

**Technical notes**:
- 4 test patterns: simple alloc/free (32B), coalescing (3×64B free → 180B realloc), dynamic queue create/send/receive/delete, dynamic task create + self-delete via vTaskDelete.
- configTOTAL_HEAP_SIZE=4096, all markers stable after 37+ rounds (ALLOCS=FREES=185, FREE_HEAP=3784).
- Found two new -O2/-Oz codegen bugs: (1) tasks.c scheduler startup fails when configSUPPORT_DYNAMIC_ALLOCATION=1 (additional code paths trigger miscompilation in prvCreateIdleTasks), (2) heap_4.c pvPortMalloc(64) returns NULL while pvPortMalloc(32) works (block-finding logic miscompiles).
- Workaround: tasks.c and heap_4.c compiled at -O1 (max safe level), everything else at -Oz.
- Works at -O0 and -O1 across the board. All three FreeRTOS demos (basic, queue, heap) verified working.

## 2026-02-07 FIX LOAD_16_WITH_ADDR false Defs=[A] causing MCP miscompilation at -O2/-Oz

**What**: Removed incorrect `Defs=[A]` from `LOAD_16_WITH_ADDR` pseudo instruction. This false def caused Machine Copy Propagation (MCP) to incorrectly remove `$a = COPY $b` as dead, breaking tasks.c at -O2+ (scheduler startup failed when `configSUPPORT_DYNAMIC_ALLOCATION=1`).

**Where**: `llvm-project/llvm/lib/Target/I8085/I8085InstrInfo.td` (LOAD_16_WITH_ADDR definition), `FreeRTOS/Makefile` (removed -O1 workarounds for tasks.c and heap_4.c)

**Why**: `LOAD_16_WITH_ADDR` only clobbers A when dest=HL (uses `MOV A,M` + `MOV L,A`). When dest=BC/DE, expansion uses `MOV_FROM_M` or `LOAD_8_WITH_ADDR` which do NOT touch A. The `shouldPreservePSW` mechanism in `I8085ExpandPseudoInsts.cpp` already handles saving/restoring PSW when dest=HL and A is live. Declaring `Defs=[A]` globally made MCP think every LOAD_16_WITH_ADDR redefines A, so it deleted preceding `$a = COPY` instructions as dead.

**Technical notes**:
- Found via `opt-bisect-limit` binary search: pass 3659 = "Machine Copy Propagation Pass on function (xTaskCreateStatic)" was the culprit.
- MIR showed: `$de = LOAD_16_WITH_ADDR %stack.2, implicit-def $a` / `$a = COPY killed $b` / `$bc = LOAD_16_WITH_ADDR %stack.3, implicit-def $a` / `JMP_8_IF killed $a`. MCP removed the COPY because it saw $a redefined by the second LOAD_16_WITH_ADDR.
- Same bug class as the earlier `Defs=[HL]` fix on STORE/LOAD pseudos.
- heap_4.c at -O2 was NOT actually broken (tested fine with current compiler). The -O1 workaround there was unnecessary.
- Verification: all 14 benchmarks x 5 opt levels (O0/O1/O2/Os/Oz) = 70/70 HALTED. All 3 FreeRTOS demos pass at -Oz with no workarounds. libgcc rebuilt with fixed compiler.

---
*Last Updated: 2026-02-07*
