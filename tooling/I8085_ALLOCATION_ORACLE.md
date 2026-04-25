# I8085 Allocation Oracle Notes

This directory contains offline research probes for i8085 register allocation.
They are not part of the normal LLVM code-generation pipeline.

The production backend work that came out of this line of investigation lives
in the LLVM target, mainly the address-register hint pass. These scripts are
kept so the experiments can be reproduced and refined later.

## Tools

- `exact_alloc_probe.py`
  - brute-force exact assignment for selected hot MIR regions
  - includes post-greedy stack-slot surveys

- `z3_alloc_probe.py`
  - Z3 `Optimize` version of the same local assignment problem
  - useful for checking that larger picoJVM regions are solver-sized

- `i8085_cost_model.py`
  - narrow byte/T-state cost oracle for address-bearing pseudo choices
  - extracts instruction timing data from the local Intel 8080/8085 manual
  - also estimates the dominant 16-bit SP-relative spill/reload path

## Inputs

The probes expect MIR dumps under `.tmp/`, for example:

```bash
llvm-project/build-clang-8085/bin/clang \
  --target=i8085-unknown-elf -Oz -ffreestanding -fno-builtin \
  -S -emit-llvm tooling/examples/fib/fib.c -o .tmp/fib.ll

llvm-project/build-clang-8085/bin/llc \
  -mtriple=i8085-unknown-elf -stop-before=greedy \
  .tmp/fib.ll -o .tmp/fib_pre_ra.mir

llvm-project/build-clang-8085/bin/llc \
  -mtriple=i8085-unknown-elf -stop-after=greedy \
  .tmp/fib.ll -o .tmp/fib_post_ra_like.mir
```

Similar dumps were used for `bubble_sort` and `pjvm_parse`.

## Typical Runs

```bash
python3 tooling/exact_alloc_probe.py --case fib6 --case pjvm16 --top-slots 4
python3 tooling/exact_alloc_probe.py --survey pjvm --survey fib --top-slots 4
python3 tooling/z3_alloc_probe.py --case fib6 --case pjvm16 --case pjvm16all
```

`z3_alloc_probe.py` requires the Python `z3` package. The brute-force probe and
the cost model do not.

## Current Status

The scripts model only a small part of the real backend cost surface:

- address-bearing `LOAD_*_ADDR_CONTENT` and `STORE_*_ADDR_CONTENT` pseudos
- 16-bit spill/reload costs through the common SP-relative path
- rough byte and 8085 T-state costs from the local Intel manual

They intentionally omit several nonlocal effects:

- PSW/flag preservation
- call-adjacent register pressure
- whole-function allocation interactions
- late combine opportunities

That is why the solver output should be treated as an oracle for experiments,
not as a production allocator.

