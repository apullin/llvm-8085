# GNUSim8085 (GNUSim8085/GNUSim8085)

## Summary
- Type: Full GUI simulator + assembler + debugger (GTK).
- License: GPL (source headers indicate GPLv2+).
- Core location: `src/8085.c` + `src/8085-instructions.c`.

## Core/Architecture
- C core tightly integrated with GUI and GLib helpers.
- Uses global `EefSystem` state and callbacks for UI hooks.
- Build uses Meson or Autotools; heavy GTK dependency.

## Completeness Notes
- `RIM`/`SIM` are present but explicitly **not implemented** (`g_warning`).
- Instruction file shows TODOs for 8085-specific behavior.
- Good for interactive debugging, but core completeness is questionable.

## Extractability / CLI Potential
- Possible but not ideal: UI coupling + GLib/GTK dependency.
- Would need to carve out CPU core into a standalone library and strip GUI hooks.

## Risks / Costs
- GPL license may be incompatible with desired integration.
- Heavier build dependencies and refactor effort.
