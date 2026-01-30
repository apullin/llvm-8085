# Neutrino 8085 (8085emu/8085emu.github.io)

## Summary
- Type: Static web app (HTML/JS) with assembler + emulator.
- License: GPLv3.
- Core location: JS files under `js/` (microcode tables + per-instruction functions).

## Core/Architecture
- Execution loop in `js/runner/runner.js`.
- Opcode map + instruction lengths in `js/base/microcodes.js`.
- Instruction behavior spread across `js/funcs/*.js` and uses global `dict` mapping.
- UI (buttons/display) is entangled in the runner logic.

## Completeness Notes
- Opcode map has 246 entries (missing 0x08/0x10/0x18/0x28/0x38 and CB/D9/DD/ED/FD — likely NOP/undocumented).
- RIM/SIM appear in the microcode list.
- Interrupt model is unclear from a quick scan; may be UI-driven.

## Extractability / CLI Potential
- Feasible but messy: execution loop depends on DOM objects (buttons, display).
- Would need a refactor to isolate CPU state, memory, and I/O from UI.
- JS/Node CLI is possible if we disentangle globals and DOM.

## Risks / Costs
- GPLv3.
- Non-modular structure increases extraction effort.
