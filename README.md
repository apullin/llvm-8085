# llvm-8085

Work‑in‑progress LLVM/Clang toolchain support for the Intel 8085, plus the
surrounding project scaffold (sysroot, tooling, docs, simulator/tracer).

This repository is the **outer project**. The LLVM fork lives in `llvm-project/`
(and is not tracked here yet). The simulator lives in `i8085-trace/` (also not
tracked here yet). We will reconcile the LLVM fork with a proper upstream fork
and submodules later.

## Repo Layout

- `llvm-project/` — LLVM/Clang fork with the i8085 backend (not tracked here yet)
- `sysroot/` — minimal crt + linker scripts + seed headers + libs
- `tooling/` — build helpers, smoke tests, runner scripts
- `docs/` — status and notes
- `sim/` — simulator candidates (clones are intentionally ignored)
- `i8085-trace/` — tracing simulator (kept as its own repo for now)
- `*.pdf`, `*.txt`, `*.md` — reference manuals and extracted text

## Status

See `docs/STATUS.md` for the current checklist, completed milestones, and
remaining work.

## Quick Notes

- This is an embedded‑style toolchain; it relies on a minimal sysroot and
  linker scripts for simple ROM/RAM layouts.
- The manuals are currently committed directly for convenience. We will replace
  these with canonical links once the project structure is finalized.

## Next Steps (High Level)

- Reconcile `llvm-project/` with the proper fork and add as a submodule.
- Expand sysroot (picolibc) and testing coverage.
- Simulator integration and correctness harness expansion.

