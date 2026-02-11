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

## 2026-02-08 DONE LLDB debugging support — GDB stub + ABI plugin + DWARF

**What**: Implemented full LLDB debugging infrastructure for i8085: (1) GDB Remote Serial Protocol server in i8085-trace simulator, (2) LLDB ABI plugin in llvm-project, (3) verified DWARF debug info already works. All 5 end-to-end tests pass.

**Where**:
- `i8085-trace/src/gdb_stub.cpp` (~700 lines, NEW) — GDB RSP server
- `i8085-trace/include/gdb_stub.hpp` (NEW) — header
- `i8085-trace/src/main.cpp` — added `--gdb=PORT` flag
- `i8085-trace/CMakeLists.txt` — added gdb_stub.cpp
- `llvm-project/lldb/source/Plugins/ABI/I8085/` (3 new files) — ABI plugin
- `llvm-project/lldb/source/Plugins/ABI/CMakeLists.txt` — added I8085

**Why**: Needed real debugging infrastructure (breakpoints, single-step, register/memory inspection) to investigate complex runtime issues like the CoreMark bottleneck. Modeled on existing tms9900-trace GDB stub.

**Technical notes**:
- GDB stub serves target XML with 15 registers (B/C/D/E/H/L/M/A/SP/FLAGS/PSW/PC/BC/DE/HL) in DWARF order
- Register serialization: 42 hex chars total (8×2 for 8-bit regs + 5×4 for 16-bit LE regs)
- Protocol: full RSP with ACK, supports ?, g/G, p/P, m/M, s, c, Z0/z0, qSupported, qXfer:features:read, thread info, kill/detach
- Continue loop: fires periodic timers, checks breakpoints after each step, polls for Ctrl-C every 1024 steps
- ABI plugin: `ABISysV_i8085` (RegInfoBasedABI), CFA=SP+2, PC at [CFA-2], all registers volatile
- DWARF already works — .debug_info/.debug_line/.debug_frame sections present with `-g`
- LLDB built at `llvm-project/build-clang-8085/bin/lldb` with `LLVM_ENABLE_PROJECTS="clang;lld;lldb"`
- Usage: `i8085-trace --gdb=1234 program.bin` then `lldb -o "target create program.elf" -o "gdb-remote localhost:1234"`
- Symbol-level debugging works: `breakpoint set -n main`, backtrace shows symbol names, register display shows symbol annotations

## 2026-02-08 FIX HL-clobber bug: live HL destroyed by GR32 pseudo expansions

**What**: Fixed a critical codegen bug where GR32 pseudo instruction expansions (LOAD_32_WITH_ADDR, STORE_32, etc.) clobber HL via implicit-def, destroying live values the register allocator placed in HL. Manifested as CoreMark infinite loop at -O1+ and incorrect 32-bit unsigned comparisons.

**Where**: `I8085ExpandPseudoInsts32.cpp` (expandMI restructured, HLSaveBias added to all SP-relative offsets), `I8085InstrInfo32.td` (removed Defs=[A] from LOAD_32 pseudos)

**Why**: The register allocator assigns HL to hold a live value (e.g., OR mask `2` for `r |= 2`). When a GR32 pseudo with `implicit-def $hl` (NOT dead) appears between the HL assignment and its use, the expansion overwrites HL. The allocator doesn't spill because it believes the pseudo's HL output IS the needed value.

**Technical notes**:
- Found via `opt-bisect-limit` binary search: pass 225 (InstCombine) transforms `icmp sle %x, 1` → `icmp ult %x, 2`, exposing the bug (signed comparisons used different codegen that avoided the HL conflict)
- Fix: detect `implicit-def $hl` NOT dead on non-branch pseudos, wrap expansion in PUSH H / POP H, add HLSaveBias (+2) to all SP-relative offsets
- Also removed Defs=[A] from LOAD_32 pseudos (batch path doesn't touch A; fallback path now preserves A with PUSH PSW)
- Also eliminated DiffMBB indirection in JMP_32_IF_NOT_EQUAL, using direct jumps with explicit JNZ+JMP in every comparison block (fixes analyzeBranch)
- All 15 benchmarks pass at O0/O1/O2/Os (60/60 HALTED), submodule commit d3493829

## 2026-02-08 DONE C++ support: global constructors, virtual dispatch, new/delete

**What**: Added full C++ support to the i8085 toolchain. Classes, templates, virtual dispatch, global constructors, and dynamic allocation (new/delete) all work at O0/O1/O2/Os.

**Where**: `I8085AsmPrinter.cpp` (emitXXStructor fix), `sysroot/crt/crt0.S` (.init_array iteration + _exit stub), `sysroot/ldscripts/*.ld` (.init_array/.fini_array + heap symbols), `sysroot/libi8085/builtins/malloc.S` (bump allocator), `tooling/examples/cpp_test/cpp_test.cpp`

**Why**: C++ is the final language feature needed for a complete toolchain. The port now supports C and C++ with full optimization.

**Technical notes**:
- Root cause of empty .init_array: `I8085AsmPrinter::emitXXStructor` overrode the base class with an empty body, silently discarding all `@llvm.global_ctors` entries. One-line fix: call `AsmPrinter::emitXXStructor(DL, CV)`.
- picolibc's malloc (compiled by our backend) has a codegen bug: dereferences NULL free list pointer instead of checking it. Workaround: hand-written bump allocator (malloc.S) linked ahead of libc.a. free() is a no-op.
- CRT0 uses PCHL trampoline for indirect calls to .init_array function pointers (8085 has no indirect CALL instruction).
- `wchar_t` typedef in stddef.h guarded with `#ifndef __cplusplus` (it's a built-in keyword in C++).
- Compiler flags: `-fno-exceptions -fno-rtti -fno-threadsafe-statics` (unwinding tables and RTTI would blow ROM budget).

## 2026-02-09 FIX Branchless sort codegen - STORE_8 SrcIsHL and LOAD_8_WITH_ADDR LIFO order

**What**: Fixed two pseudo expansion bugs that caused branchless sorting networks (libc++ `__cond_swap` / `__partially_sorted_swap`) to produce wrong results. Removed the `&& false` workaround from `sort.h` that disabled branchless sort on i8085.

**Where**: `llvm-project/llvm/lib/Target/I8085/I8085ExpandPseudoInsts.cpp` (STORE_8 expansion ~line 577-668, LOAD_8_WITH_ADDR expansion ~line 1003-1068), `sysroot/include/c++/v1/__algorithm/sort.h` (removed `#ifdef __i8085__ && false #endif` at line 151)

**Why**: Register allocator can assign H or L to 8-bit operands (GR8 class includes H/L). Pseudo expansion uses LXI+DAD to compute stack addresses, which clobbers HL. Two distinct failure modes:

**Technical notes**:
- **Bug 1 (STORE_8 SrcIsHL, O2/Os)**: When srcReg is H or L and baseReg != HL, `LXI H,offset / DAD SP` destroys the source value before `MOV M,src`. Fix: detect `SrcIsHL`, emit `MOV A,src` before LXI, then use `MOV M,A`. Need PSW save if A was already live.
- **Bug 2 (LOAD_8_WITH_ADDR OtherSubLive, O1)**: When destReg is H (or L) and the other sub-register L (or H) is live, expansion uses A as temp: PUSH H, PUSH PSW, LXI+DAD, MOV A,M, POP H, MOV dest,A, POP PSW. But PUSH H first / PUSH PSW second means POP H actually pops PSW (LIFO!), putting flags byte 0x46 into L. Fix: when OtherSubLive, swap push order to PUSH PSW first, PUSH H second.
- Trace forensics: L=0x46 came from POP H at step 724, which popped PSW value (A=0x14, flags=0x46) instead of saved HL. The 0x46 flags byte propagated through the sort network to the final output.
- Lesson: when pseudo expansion has multiple PUSH/POP pairs with different pop orders in different code paths, each path needs its own push order to maintain LIFO correctness.
- Verification: sort5 branchless test correct at O0/O1/O2/Os/Oz. stl_test2 33/33 bytes at all opt levels. 60/60 benchmarks HALTED.

## 2026-02-09 DONE Compiler stress testing — Csmith, LZ77, picolibc

**What**: Ran 3 parallel stress test campaigns: (1) Csmith-style random program differential testing (500 programs, O0 vs O1/O2/Os/Oz), (2) LZ77 compression/decompression (pointer arithmetic, hash tables, sliding window), (3) picolibc sprintf/libc function testing (va_args, string ops, integer formatting).

**Where**: Test infrastructure at `/tmp/gen_random_test.py`, `/tmp/difftest.sh`, `/tmp/sprintf_test.c`, `/tmp/minilzo_test.c`, `/tmp/lz_stress2.c`. Full report: `/tmp/stress_test_bugs.md`.

**Why**: After STL bring-up exposed several backend bugs, wanted broad coverage to find more latent codegen issues before they bite real users.

**Technical notes**:
- **Csmith**: 500 UB-safe comparisons, 0 real miscompiles. 6 false positives were signed overflow UB (`int` is 16-bit). Confirmed with `-fwrapv`.
- **LZ77**: Found NEW O1 bug — `switch i32` phi initialization clobbers HL holding loop index phi. While loops with 3+ exit paths at -O1 generate broken code (loop never executes). Minimal repro: `/tmp/switch_loop_extern.c`. opt-bisect-limit=0 still fails → backend codegen, not optimizer.
- **picolibc**: Custom my_snprintf 37/37 pass at all opt levels. Core string/memory functions 32/32 pass (hand-written ASM in libgcc.a). Found 3 bugs in pre-compiled libc.a: strncpy infinite loop, strtol/atoi wrong results, strrchr infinite loop. All 3 are in picolibc's compiled C objects — likely same backend codegen bugs from pre-fix compiler. Recompiling libc.a with current compiler may resolve them.
- picolibc snprintf blocked — pulls 78KB double-precision Ryu engine, overflows 64K. Would need `__IO_DEFAULT='i'` rebuild.

## 2026-02-09 DONE Recompiled picolibc libc.a — all 3 libc bugs resolved

**What**: Rebuilt picolibc libc.a (895 objects at -Oz) with the fixed compiler. All 3 previously broken functions now work: strncpy (was infinite loop), strtol/atoi (was returning -4 for "42"), strrchr (was infinite loop).

**Where**: `libi8085/picolibc/build.sh` → `sysroot/lib/libc.a`. Verified with test programs for strncpy, strtol, strrchr.

**Why**: The old libc.a was compiled by a pre-fix compiler that had DAD-clobbers-carry, STORE_8 SrcIsHL, and LOAD_8 LIFO bugs. Recompiling with the current compiler resolved all three.

**Technical notes**: 60/60 benchmarks pass. stl_test2 33/33 at O0/O1/O2/Os. Key insight: all "working" libc functions were hand-written ASM in libgcc.a (memset, memcpy, strlen, strcmp, memchr); all "broken" ones came from picolibc's compiled C in libc.a.

## 2026-02-09 FIX O1 Machine LICM stale dead flag on implicit-def $hl

**What**: Fixed O1-only bug where Machine LICM hoists GR32 pseudo instructions to loop preheaders, preserving a stale `dead` flag on `implicit-def $hl`. The expand-pseudo pass then skips PUSH H/POP H preservation, and the expansion clobbers HL (which now holds a live loop index phi).

**Where**: `llvm-project/llvm/lib/Target/I8085/I8085ExpandPseudoInsts32.cpp` (lines 2344-2359). Submodule commit bdef3938ea51.

**Why**: While loops with 3+ exit paths generate `switch i32` at O1, with an i32 phi whose LOAD_32 initialization gets hoisted by LICM. At the original position HL wasn't live, but at the hoisted position it IS live-out to the loop header.

**Technical notes**:
- Fix: defensive LivePhysRegs recomputation when `implicit-def $hl` is marked dead — recomputes actual liveness from block live-outs by stepping backward. If HL is live despite dead flag, forces NeedHLSave=true.
- Minimal repro `/tmp/switch_loop_extern.c` now correct at O0/O1/O2/Os/Oz.
- lz_stress2 LZ77 stress test still fails at O1 (1/7) — this is a DIFFERENT O1 bug (no PUSH H emitted at all, so the LICM stale-flag fix doesn't trigger). Needs separate investigation.

## 2026-02-10 DONE Rust compiles and runs on i8085

**What**: Brought up Rust (#![no_std]) targeting the Intel 8085. Built rustc 1.88.0 from source linked against our custom LLVM 20 fork. Compiled Rust's `core` library and `compiler_builtins` for i8085. A minimal test crate (arithmetic, control flow, loops) runs correctly on the i8085-trace simulator — all 4 tests pass.

**Where**:
- `rust-i8085/` — Rust 1.88.0 source, custom bootstrap.toml
- `rust-i8085/compiler/rustc_target/src/callconv/i8085.rs` — Rust ABI for i8085
- `rust-i8085/compiler/rustc_codegen_llvm/src/llvm_util.rs` — f16/f128 disabled for i8085
- `tooling/examples/rust_test/` — test crate, target spec JSON, Cargo.toml
- `llvm-project/llvm/lib/Target/I8085/I8085ISelLowering.cpp` — ISel fixes for Rust

**Why**: Extending the i8085 toolchain beyond C/C++ to support Rust's `#![no_std]` ecosystem.

**Technical notes**:
- Key ISel fixes: (1) Changed all Custom i64 ops to Expand (avoids BUILD_PAIR i64 surviving to ISel), (2) CanLowerReturn limited to 4 bytes (forces sret for i64 returns), (3) UMULO/SMULO i32 rewritten to use `emitMul32LoHi` helper (calls __mului32/__mulsi32 via sret, loads lo/hi as separate i32 — avoids creating i64 DAG nodes during operation legalization), (4) Removed TRUNCATE i32 Custom handler.
- `CARGO_FEATURE_NO_F16_F128=1` required to disable f128/f16 builtins (overflow 16-bit address space).
- Build: `python3 x.py build --stage 1 library` (~40s), then `cargo +i8085 build -Z build-std=core --target i8085-unknown-none.json` (~43s).
- Target spec includes pre-link-args for CRT0, linker script, --gc-sections, and post-link-args for libgcc.a + libc.a.
- Test output: `0200: 2A 9B 01 37` — 42 (i16 add), 155 (u8 add), 1 (branch), 55 (loop sum). 44 steps, 278 cycles.
- All 56 C benchmarks (14 × 4 opt levels) still HALTED after ISel changes.

## 2026-02-10 DONE SIGN_EXTEND_16 pseudo for ashr i16 x, 15

**What**: Added new SIGN_EXTEND_16 pseudo instruction that replaces `ashr i16 x, 15` (sign bit extraction) with 5 instructions instead of ~61 (ASR_16_BY_8 + 7 rotates).

**Where**: `I8085InstrInfo.td` (pseudo def), `I8085ExpandPseudoInsts.cpp` (expansion), `I8085ISelDAGToDAG.cpp` (ISel special case for ShiftAmt==15)

**Why**: The `int / 256` pattern in lpf_step generates `ashr i16 x, 15` for sign extraction (rounding correction). This expanded to ASR_16_BY_8 + 7×ASR_16 = ~61 machine instructions. With inlining duplicating this 4×, it accounted for the majority of rotate bloat (153 RAR/RAL instructions at -Os).

**Technical notes**: Expansion: `MOV A,H; ADI 128; SBB A; MOV L,A; MOV H,A`. The ADI 128 trick converts bit7 to carry, then SBB A produces 0xFF (negative) or 0x00 (positive). Results: -Os 3613→3445B (-168B), -Os-noinline 2178→2122B (-56B), rotates 153→90 (-41%).

## 2026-02-10 DISCOVERY LLVM inliner cost model is TTI-independent

**What**: Investigated why pid_step (cost=5, threshold=45) is inlined at -Os despite TTI cost increases. Found that the LLVM inliner uses its own hardcoded cost model (`InstrCost=5` per IR instruction), completely independent of TTI's `getArithmeticInstrCost`.

**Where**: `llvm/lib/Analysis/InlineCost.cpp` (InlineCostCallAnalyzer), `llvm/include/llvm/Analysis/TargetTransformInfo.h` (hook signatures)

**Why**: TTI cost changes (i16 type scale 2→4, shift costs 2→8 multiplier) only affect loop unrolling/vectorization, NOT inlining. No standard TTI hook can reduce the -Os inline threshold: `adjustInliningThreshold` returns unsigned (can only increase), `getInliningThresholdMultiplier` returns unsigned (minimum 1), `getInlinerVectorBonusPercent` has `assert(VectorBonus >= 0)`.

**Technical notes**: pid_step's inline cost = 5 because: ~15 non-free IR instructions × 5 = 75, minus callsite savings (8 args × 5 + 5 call + penalty ≈ 70) = net 5. The 906B gap between -Os (3445B) and -Oz (2539B) comes entirely from `minsize` backend attribute — both make identical inlining decisions. Workarounds: `-Oz`, `-fno-inline`, `__attribute__((noinline))`, or `-mllvm -inline-threshold=N`.

## 2026-02-10 FIX -Os inlining bug — function-inline-threshold override

**What**: Fixed LLVM's -Os inliner making code LARGER on i8085 by injecting `function-inline-threshold="0"` attribute on all optsize/minsize functions. This limits inlining to only net-beneficial cases (cost ≤ 0), preventing the inliner from duplicating function bodies that expand to hundreds of bytes of machine code when a CALL is only 3 bytes.

**Where**: `I8085TargetMachine.h` (registerPassBuilderCallbacks declaration), `I8085TargetMachine.cpp` (I8085AnnotateInlineThresholdPass struct + registerPassBuilderCallbacks implementation)

**Why**: The LLVM inliner's IR-level cost model (InstrCost=5) underestimates machine code expansion on 8-bit i8085 where each IR instruction expands to 4-10 machine instructions. pid_step (cost=5, threshold=45) and lpf_step (cost=15, threshold=45) were both inlined at -Os, duplicating ~300B+ of machine code each. A 1980 non-optimizing Small-C compiler produced 2x smaller code by simply using subroutine calls.

**Technical notes**: Uses `registerPassBuilderCallbacks` to inject a module pass via `PipelineEarlySimplificationEPCallback` (runs BEFORE inliner). The `function-inline-threshold` attribute completely replaces all prior threshold computations at `InlineCost.cpp:finalizeAnalysis()` line 1019. With threshold=0, effective comparison is `cost < max(1, 0)` → `cost < 1`. Only cost ≤ 0 inlines (like readADC/writePWM wrappers) proceed. Results: -Os obj text 1937B (was 3445B linked), now SMALLER than -Os -fno-inline (2122B) because beneficial wrapper inlines still occur. -Oz obj text 1974B (was 2539B). 75/75 benchmarks pass, zero regressions.

---
*Last Updated: 2026-02-10*
