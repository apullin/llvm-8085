# LLVM Backend for Intel 8085

A production-quality LLVM backend for the [Intel 8085](https://en.wikipedia.org/wiki/Intel_8085) — the 8-bit microprocessor whose instruction set is the direct ancestor of the x86 architecture that runs most of the world's computers today. This project brings a modern optimizing compiler back to where it all started. Compiles C, C++ (with STL), and Rust 🦀 to native 8085 machine code.

## Highlights

- **Full LLVM 20 toolchain** — clang, opt, lld, llvm-objcopy, assembler, and disassembler. No external tools required.
- **Heavily stress-tested** — 15 benchmarks (75/75 pass across -O0, -O1, -O2, -Os, -Oz), Csmith random programs, MiniLZO compression, picolibc sprintf suite.
- **C++ with STL** — freestanding C++20 with a large, verified subset of libc++ containers, utilities, and algorithms. See [C++ with STL](#c-with-stl).
- **Rust `#![no_std]`** 🦀 — cross-compilation via a custom Rust build linked against our LLVM fork. See [Rust Support](#rust-support).
- **FreeRTOS port** — preemptive multitasking with three verified demos: basic task switching, producer/consumer queues, and dynamic heap allocation with `heap_4`. See [FreeRTOS](#freertos).
- **Hand-tuned runtime library** — 2,598 bytes of assembly-optimized IEEE 754 soft-float (24/24 correctness tests), 32/64-bit multiply/divide/shift, `memcpy`/`memset`/`memmove`/`memchr`, and string operations.
- **GDB/LLDB debugging** — the [i8085-trace](https://github.com/apullin/i8085-trace) emulator includes a GDB Remote Serial Protocol stub. Full DWARF support with `-g`.

## The CPU

The [Intel 8085](https://en.wikipedia.org/wiki/Intel_8085) (1976) was the last and most refined member of Intel's 8-bit processor family. It has seven 8-bit registers (A, B, C, D, E, H, L) that can be used as three 16-bit pairs (BC, DE, HL), a 16-bit stack pointer and program counter, and a flag register with sign, zero, auxiliary carry, parity, and carry bits.

What it *doesn't* have is almost more interesting from a compiler perspective: no multiply or divide instructions, no indexed addressing modes, no barrel shifter, and only one register (HL) that can address memory. Every stack variable access requires a 3-instruction sequence (`LXI H, offset; DAD SP; MOV r, M`). This makes the 8085 one of the most challenging targets for a modern optimizing compiler — and one of the most rewarding when it works.

The 8085's instruction set evolved into the 8086 (1978), which became the x86 architecture. The register names, flags, stack model, and little-endian byte order are a direct inheritance. Writing an LLVM backend for the 8085 is, in a sense, going back to the roots of the most commercially successful processor architecture in history.

For full details, see the [MCS-80/85 Family User's Manual](https://archive.org/details/Intel8085) (Intel, 1977).

## Quick Start

### Build the toolchain

```bash
git clone git@github.com:apullin/llvm-8085.git
cd llvm-8085
git submodule update --init

cd llvm-project
mkdir build-clang-8085 && cd build-clang-8085
cmake -G Ninja \
  -DLLVM_TARGETS_TO_BUILD="I8085" \
  -DLLVM_ENABLE_PROJECTS="clang;lld" \
  -DCMAKE_BUILD_TYPE=Release \
  ../llvm
ninja
```

### Compile and link

```bash
# Compile C to object file
clang --target=i8085-unknown-elf -Os -ffreestanding -fno-builtin -c main.c -o main.o

# Assemble startup code
clang --target=i8085-unknown-elf -c crt0.S -o crt0.o

# Link with LLD
ld.lld -T linker.ld crt0.o main.o -o program.elf libgcc.a libc.a libgcc.a

# Extract raw binary
llvm-objcopy -O binary program.elf program.bin
```

### Run on emulator

```bash
i8085-trace -S -n 1000000 -d 0x0200:16 program.bin
```

## Repository Structure

```
llvm-8085/
├── llvm-project/               # LLVM 20 fork (submodule, branch: i8085)
├── rust-i8085/                 # Rust 1.88.0 fork (submodule, branch: i8085)
├── FreeRTOS-Kernel/            # FreeRTOS kernel (submodule, branch: i8085)
├── gcc-torture-tests/          # GCC mirror (submodule, sparse checkout)
├── FreeRTOS/                   # FreeRTOS demos and port files
├── sysroot/
│   ├── crt/                    # CRT0 startup code
│   ├── ldscripts/              # Linker scripts (32K, 48K, 64K layouts)
│   ├── include/                # Headers (picolibc + libc++)
│   ├── lib/                    # Libraries (libgcc.a, libc.a)
│   └── libi8085/builtins/      # Hand-written runtime assembly
├── tooling/
│   └── examples/               # 15 benchmarks + C++/Rust/serde tests
├── docs/                       # ABI, status, runtime library docs
└── PROJECT_JOURNAL.md          # Detailed development log
```

## Toolchain

The backend produces ELF object files directly — no external assembler needed:

```
 .c / .S  ──▶  clang -c  ──▶  .o (ELF)  ──▶  ld.lld  ──▶  llvm-objcopy  ──▶  .bin
```

All standard LLVM tools work: `opt` for IR optimization, `llc` for code generation, `llvm-objdump` for disassembly, `llvm-size` for section sizes.

## Language Support

### C

Full C11 support in freestanding mode. All integer sizes through 64-bit, IEEE 754 single-precision soft-float, `switch` statements, inline assembly, varargs, struct passing/returning (byval/sret).

### C++ with STL

Freestanding C++ with header-only libc++ from the LLVM tree:

```bash
clang --target=i8085-unknown-elf -O2 -ffreestanding -fno-builtin \
  -fno-exceptions -fno-rtti -fno-threadsafe-statics -nostdinc++ \
  -isystem sysroot/include/c++/v1 -isystem sysroot/include \
  -std=c++20 -c program.cpp -o program.o
```

**Working STL containers and utilities**: `vector`, `string`, `pair`, `tuple`, `optional`, `string_view`, `unique_ptr`, `initializer_list`, `bitset`, `numeric_limits`, `array`, and algorithms (`find`, `count`, `reverse`, `min`, `max`, `sort`).

**Working language features**: lambdas (value, reference, mutable, as template arguments), multiple inheritance with virtual dispatch and pointer adjustment, move semantics (construct + assign), variadic templates, structured bindings (pair, tuple, struct), deep virtual inheritance chains, type-erased callables (manual `std::function`), lambda comparators.

A minimal C++ runtime provides `operator new`/`delete` (wrapping `malloc`/`free`), `__cxa_pure_virtual`, `__cxa_atexit`, and `__dso_handle`. Global constructors run via `.init_array` iteration in CRT0.

### Rust Support

Rust `#![no_std]` cross-compilation via a custom Rust 1.88.0 build linked against our LLVM fork.

**Setup** (one-time):

1. The Rust fork is tracked as the `rust-i8085` submodule
2. Build stage 1: `cd rust-i8085 && python3 x.py build --stage 1 library`
3. Register toolchain: `rustup toolchain link i8085 rust-i8085/build/<host>/stage1`

**Build a Rust program**:

```bash
CARGO_FEATURE_NO_F16_F128=1 cargo +i8085 build \
  -Z build-std=core --target i8085-unknown-none.json
```

See `tooling/examples/rust_test/` for a basic arithmetic/control-flow test and `tooling/examples/rust_serde/` for a serde-json serialization/deserialization demo.

**Serde JSON on an 8085**: The `rust_serde` example serializes and deserializes `Point{x: i16, y: i16, z: i16}` to/from JSON using `serde` + `serde-json-core`. Produces `{"x":42,"y":-7,"z":300}`, round-trips correctly, 11.5KB release binary with LTO.

## FreeRTOS

The `FreeRTOS/` directory contains a full FreeRTOS port for the Intel 8085.

### Demo programs

**Basic** (`demo_basic.c`): Two equal-priority tasks increment separate counters under preemptive time-slicing. Verifies scheduler alternation.

**Queue** (`demo_queue.c`): Producer/consumer pattern with `xQueueSend`/`xQueueReceive`, event groups, and tick-based delays.

**Heap** (`demo_heap.c`): Dynamic allocation stress test using `heap_4`. Exercises `pvPortMalloc`/`vPortFree`, block coalescing, and dynamic task/queue create+delete.

```bash
cd FreeRTOS && make run          # basic two-task test
cd FreeRTOS && make run-queue    # producer/consumer
cd FreeRTOS && make run-heap     # heap stress test
```

Timer interrupt: `i8085-trace --timer=65:30720` (RST 6.5 at 100 Hz for 3.072 MHz clock).

## Debugging and Emulation

Development and testing uses [i8085-trace](https://github.com/apullin/i8085-trace), a standalone Intel 8085 CPU emulator with cycle-accurate execution traces, memory dumps, and JSON output.

### GDB/LLDB remote debugging

The emulator includes a full GDB Remote Serial Protocol stub. Connect with LLDB for interactive debugging:

```bash
# Start emulator in GDB server mode
i8085-trace --gdb=1234 program.bin

# In another terminal
lldb -o "target create program.elf" -o "gdb-remote localhost:1234"
(lldb) breakpoint set -n main
(lldb) continue
(lldb) register read
```

Full DWARF debug info works with the `-g` flag. Supports breakpoints, single-stepping, register and memory inspection, and async break (Ctrl-C). An LLDB ABI plugin (`ABISysV_i8085`) is included in the LLVM build.

## Runtime Libraries

### Compiler Builtins (`sysroot/libi8085/builtins/`)

Hand-coded 8085 assembly for all performance-critical operations:

| Category | Functions | Notes |
|----------|-----------|-------|
| Soft-float (IEEE 754) | `__addsf3`, `__subsf3`, `__mulsf3`, `__divsf3`, comparisons, conversions | 2,598 bytes, 24/24 correctness tests |
| 32-bit integer | `__mulsi3`, `__divsi3`, `__udivsi3`, `__modsi3`, `__umodsi3` | Shift-and-add multiply, restoring division |
| 32-bit shifts | `__ashlsi3`, `__lshrsi3`, `__ashrsi3` | Byte-shuffle for constant shifts (ISel) |
| 64-bit integer | `__muldi3`, `__divdi3`, `__udivdi3`, `__moddi3`, `__umoddi3`, `__udivmoddi4` | Full 64-bit arithmetic |
| 64-bit shifts | `__ashldi3`, `__lshrdi3`, `__ashrdi3` | |
| Memory | `memcpy`, `memset`, `memmove`, `memcmp`, `memchr` | Hand-written byte loops |
| String | `strlen`, `strcpy`, `strncpy`, `strcmp`, `strncmp`, `strchr`, `strrchr` | |
| Heap | `malloc`, `free` | Bump allocator (replaces picolibc malloc) |

### C library

[picolibc](https://github.com/picolibc/picolibc) built for i8085 at `-Oz`, providing `printf`, `sprintf`, `strtol`, `qsort`, and other standard C library functions.

## Calling Convention

| Item | Convention |
|------|-----------|
| Arguments | All on stack (no register args), caller cleanup |
| Return i8 | `A` |
| Return i16 | `BC` |
| Return i32 | `BC` (low) + `DE` (high) |
| Return i64 | Hidden `sret` pointer |
| Return float | Bitcast to i32, returned as BC/DE |
| Callee-saved | None (all registers caller-saved) |
| Stack growth | Downward |
| Alignment | 1 byte |

Aggregates larger than 4 bytes are returned via hidden `sret` pointer. `byval` aggregates are copied to the stack by the caller. See [docs/ABI.md](docs/ABI.md) for full details.

## Testing

### Benchmark Suite

15 programs, all passing at every optimization level (75/75 HALTED):

| Benchmark | Description | -O0 | -O1 | -O2 | -Os |
|-----------|-------------|----:|----:|----:|----:|
| fib | Recursive Fibonacci | 429 | 300 | 300 | 356 |
| deep_recursion | Deep call chain | 366 | 215 | 2,707 | 2,699 |
| bubble_sort | Array sort (16-bit) | 1,660 | 665 | 665 | 641 |
| crc32 | CRC-32 (bit-by-bit) | 1,396 | 691 | 691 | 683 |
| crc32_lut | CRC-32 (lookup table) | 881 | 1,155 | 1,155 | 948 |
| string_torture | String operations | 2,181 | 1,493 | 1,493 | 1,509 |
| q7_8_matmul | Fixed-point matrix multiply | 3,223 | 2,987 | 2,987 | 2,987 |
| json_parse | JSON tokenizer | 8,923 | 8,019 | 8,019 | 8,015 |
| mul_torture | 16/32-bit multiply | 8,679 | 7,027 | 7,027 | 6,955 |
| opt_sanity | Constant folding stress | 10,386 | 327 | 327 | 327 |
| float_torture | IEEE 754 correctness (24 tests) | 11,679 | 9,619 | 9,619 | 9,635 |
| bitops_torture | 32-bit bitwise ops | 13,024 | 13,212 | 13,212 | 13,236 |
| fp_bench | Floating-point workload | 19,967 | 14,559 | 14,559 | 14,595 |
| arith64_torture | 64-bit arithmetic | 15,671 | 13,215 | 13,215 | 13,191 |
| div_torture | 16/32/64-bit division | 30,476 | 17,979 | 17,979 | 17,775 |

Code sizes are `.text` section bytes. All cycle counts from [i8085-trace](https://github.com/apullin/i8085-trace) emulator.

### Stress Tests

| Test | Result | Notes |
|------|--------|-------|
| **Csmith** (random programs) | Self-consistent | O0=O1=O2 where all complete |
| **MiniLZO** | O1-Os | Lossless compression + decompression round-trip |
| **picolibc sprintf** (35 tests) | 35/35 at O1 | Format strings, variadics |
| **C++ stress test** (40 bytes) | O0-Os | Lambdas, MI, move, variadic, structured bindings, vtables, type erasure |
| **STL test** (33 bytes) | O0-Os | vector, string, pair, tuple, optional, sort, bitset, unique_ptr |

### LLVM Lit Tests

101 tests (85 CodeGen + 16 MC), all passing.

### GCC C Torture Tests

The [GCC C torture test suite](https://gcc.gnu.org/git/?p=gcc.git;a=tree;f=gcc/testsuite/gcc.c-torture/execute) validates compiler correctness across ~1300 standalone C programs. Results at `-Os`:

- **1167/1167 compiled tests pass** (100%)
- 90% compile rate (120 skip due to missing GCC extensions, nested functions, etc.)
- Skip list covers expected platform differences: `sizeof(int)==2`, no `double`, no SIMD

```bash
# Run the full suite (~10 min)
bash tooling/gcc-torture/run-torture.sh

# Run a specific test
bash tooling/gcc-torture/run-torture.sh --filter="pr22141-1.c"

# Run at a different optimization level
bash tooling/gcc-torture/run-torture.sh --opt=O0
```

The skip list with categorized platform differences is at `tooling/gcc-torture/skip-list.txt`.

## Backend Optimizations

The 8085's lack of indexed addressing means every stack access is 5 bytes (`LXI H, offset; DAD SP; MOV M, reg`). Several custom optimization passes reduce this overhead:

- **HL tracking pass** — tracks `HL = SP + offset` across instructions, replaces `LXI + DAD` with `INX`/`DCX` for small deltas
- **32-bit load/store coalescing** — batches register loads into `B/C/D/E` when destination registers are dead
- **Cross-operation forwarding** — skips redundant store/load between consecutive 32-bit operations
- **Byte-shuffle shifts** — constant shifts by 8/16/24 use register moves instead of shift loops
- **Inline threshold tuning** — `-Os` uses threshold=0 to prevent code-expanding inlines (1937B vs 3445B on PID benchmark)
- **PSW-preserving pseudo expansion** — detects live flags across pseudo instruction expansion to prevent `DAD` from clobbering carry

## Resources

- [MCS-80/85 Family User's Manual](https://archive.org/details/Intel8085) (Intel, 1977)
- [i8085-trace](https://github.com/apullin/i8085-trace) — Intel 8085 CPU emulator with GDB stub
- [docs/ABI.md](docs/ABI.md) — Calling convention and ABI documentation
- [docs/STATUS.md](docs/STATUS.md) — Project status and checklist
- [PROJECT_JOURNAL.md](PROJECT_JOURNAL.md) — Detailed development log

## License

Apache 2.0 with LLVM Exceptions (follows upstream LLVM licensing).
