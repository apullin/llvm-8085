# hotkeysoft/emulators (8080/8085 core)

## Summary
- Type: Multi-CPU emulator collection (C++/VS solutions).
- License: MIT.
- 8080 core location: `8080/CPU8080.*`.

## Core/Architecture
- C++ CPU class with opcode table, interrupts, and peripherals.
- Project is Windows/Visual Studio-centric; console-based wrappers included.

## Completeness Notes
- README claims 8080/8085, but code has TODOs for 8085-only instructions:
  - `RIM`/`SIM` are currently stubbed as NOPs.
- Likely 8080-complete, 8085-incomplete.

## Extractability / CLI Potential
- Reasonable: core is a distinct C++ class, easier to wrap in a CLI.
- Would require adding missing 8085 instructions and interrupt semantics.

## Risks / Costs
- 8085 completeness is not there yet.
- Windows-oriented build files; needs CMake or Make for portability.
