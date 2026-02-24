# Building picolibc for i8085

The `sysroot/` directory ships with pre-built picolibc headers and libraries. You only need to rebuild if you want to modify libc itself.

## Prerequisites

- The LLVM i8085 toolchain must be built first (see top-level README)
- [Meson](https://mesonbuild.com/) build system (`pip install meson`)
- [Ninja](https://ninja-build.org/) (`pip install ninja` or `brew install ninja`)

## Source

picolibc is included as a git submodule at `picolibc-src/`:

```bash
git submodule update --init picolibc-i8085/picolibc-src
```

## Build

```bash
bash picolibc-i8085/picolibc/build.sh
```

This will:
1. Generate a Meson cross file from the template (`picolibc/cross/i8085-unknown-elf.txt`)
2. Configure and build picolibc with `--target=i8085-unknown-elf`
3. Install headers to `sysroot/include/` and libraries to `sysroot/lib/`

## What it produces

- `sysroot/lib/libc.a` — C standard library (printf, sprintf, strtol, qsort, etc.)
- `sysroot/lib/libm.a` — Math library
- `sysroot/lib/libg.a`, `libnosys.a`, `libsemihost.a` — Support libraries
- `sysroot/lib/picolibc.ld` — Picolibc linker script fragments
- `sysroot/include/` — C standard headers (stdio.h, stdlib.h, string.h, etc.)
- `sysroot/lib/crt*.o` — C runtime startup objects

## Configuration

Key Meson options used:
- `multilib=false` — single library variant (no per-CPU-model multilib)
- `picocrt=false` — we use our own CRT0 (`sysroot/crt/crt0.S`)
- `thread-local-storage=false` — no TLS on 8085
- `newlib-global-errno=true` — single global errno (no thread safety needed)

The cross file template is at `picolibc/cross/i8085-unknown-elf.txt`. The build script substitutes `@TOOLCHAIN_BIN@` with the actual toolchain path at build time.
