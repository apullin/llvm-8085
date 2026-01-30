# 8085 Simulator Candidates — Comparison

| Candidate | License | Core language | Completeness notes | Extractability / CLI effort |
| --- | --- | --- | --- | --- |
| sim8085 | BSD-3 | C (Emscripten) | Full opcode switch, RIM/SIM implemented, timing + interrupt masks | **Low**: core already isolated in `src/core/8085.c` (just de-emscripten for native)
| GNUSim8085 | GPLv2+ | C (GTK/GLib) | RIM/SIM not implemented; GUI-focused | **High**: GUI + GLib coupling
| 8085emu.github.io (Neutrino) | GPLv3 | JS | Opcode map missing some undocumented/NOP slots; RIM/SIM present; UI-driven | **Medium–High**: DOM/global coupling, refactor needed for Node CLI
| hotkeysoft/emulators | MIT | C++ | 8085-specific instructions (RIM/SIM) stubbed; likely 8080-complete | **Medium**: CPU class is isolated but needs 8085 fixes + portable build

## Recommendation (first pass)
1) **sim8085** — best mix of completeness + permissive license + easiest core extraction.
2) **hotkeysoft/emulators** — permissive license but needs 8085 work; good fallback if we want a C++ core.
3) **Neutrino (8085emu.github.io)** — functional, but GPLv3 and UI coupling make CLI extraction heavier.
4) **GNUSim8085** — full GUI app, GPL, and missing RIM/SIM; not ideal for a reusable core.

Next step if you want: I can prototype a minimal CLI harness around sim8085’s C core (load binary, step/run, dump regs/flags), and keep it in `sim/`.
