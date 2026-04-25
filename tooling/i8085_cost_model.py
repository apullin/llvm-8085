#!/usr/bin/env python3
"""
Small I8085 cost oracle for local MIR allocation experiments.

This is intentionally narrow. It pulls a small instruction timing subset from
the local Intel 8080/8085 assembly manual markdown and uses that to score the
late address-bearing pseudo expansions that matter for the current solver work.

The first version is aimed at:
- LOAD_8/16/32_ADDR_CONTENT
- STORE_8/16_ADDR_CONTENT

The solver still treats spill count separately. This module replaces the old
arbitrary pseudo penalties with explicit byte and T-state estimates for the
different register choices.
"""

from __future__ import annotations

import re
from dataclasses import dataclass
from functools import lru_cache
from pathlib import Path


MANUAL_MD_PATH = (
    Path(__file__).resolve().parent.parent
    / "docs"
    / "8080_8085_Assembly_Language_Programming_Manual_Nov78.pdf.md"
)


@dataclass(frozen=True)
class InstructionCost:
    key: str
    bytes: int
    cycles_8080: int
    tstates_8085: int
    heading: str


@dataclass(frozen=True)
class Cost:
    bytes: int = 0
    tstates: int = 0

    def __add__(self, other: "Cost") -> "Cost":
        return Cost(self.bytes + other.bytes, self.tstates + other.tstates)

    def __bool__(self) -> bool:
        return bool(self.bytes or self.tstates)


def _extract_section(text: str, heading: str) -> str:
    lines = text.splitlines()
    start = None
    prefix = heading.split(" ", 1)[0]
    stop_prefixes = ("## ", "### ", "#### ", "##### ")
    for i, line in enumerate(lines):
        if line.strip() == heading:
            start = i
            break
    if start is None:
        raise ValueError(f"heading {heading!r} not found in manual")

    out: list[str] = []
    for i in range(start, len(lines)):
        line = lines[i]
        if i > start and any(line.startswith(p) for p in stop_prefixes):
            level = len(prefix) - 1
            other_level = len(line.split(" ", 1)[0]) - 1
            # The captured manual often emits a title heading immediately
            # followed by a same-level descriptive heading (for example
            # "### DCX" then "### DECREMENT REGISTER PAIR"). Treat that as
            # part of the same section instead of an early terminator.
            if other_level <= level and i > start + 2:
                break
        out.append(line)
    return "\n".join(out)


def _parse_first_number(text: str) -> int:
    m = re.search(r"(\d+)", text)
    if not m:
        raise ValueError(f"no integer found in {text!r}")
    return int(m.group(1))


def _parse_8085_states(text: str) -> int:
    line = text.replace("|", " ").strip()
    m8085 = re.search(r"\((\d+)(?:\s+or\s+\d+)?\s+on\s+8085\)", line)
    if m8085:
        return int(m8085.group(1))
    return _parse_first_number(line)


def _parse_cycles(text: str) -> int:
    line = text.replace("|", " ").strip()
    return _parse_first_number(line)


def _parse_cost_block(section: str, key: str, bytes_: int, heading: str) -> InstructionCost:
    cycles_line = None
    states_line = None
    for line in section.splitlines():
        if "Cycles:" in line and cycles_line is None:
            cycles_line = line
        if "States:" in line and states_line is None:
            states_line = line
    if cycles_line is None or states_line is None:
        raise ValueError(f"missing timing rows for {heading}")
    return InstructionCost(
        key=key,
        bytes=bytes_,
        cycles_8080=_parse_cycles(cycles_line),
        tstates_8085=_parse_8085_states(states_line),
        heading=heading,
    )


@lru_cache(maxsize=1)
def load_manual_subset() -> dict[str, InstructionCost]:
    text = MANUAL_MD_PATH.read_text()
    sections = {
        "INX_PAIR": ("### INX", 1),
        "DCX_PAIR": ("### DCX", 1),
        "DAD_PAIR": ("### **DAD**", 1),
        "LDAX_INDIRECT": ("### LDAX", 1),
        "LXI_IMM16": ("#### LXI", 3),
        "MOV_REG_REG": ("#### *Move Register to Register*", 1),
        "MOV_TO_MEM": ("#### *Move to Memory*", 1),
        "MOV_FROM_MEM": ("#### *Move from Memory*", 1),
        "PUSH_RP": ("#### *PUSH Register Pair*", 1),
        "POP_RP": ("#### *POP Register Pair*", 1),
        "SPHL": ("### SPHL", 1),
        "STAX_INDIRECT": ("### STAX", 1),
    }
    return {
        key: _parse_cost_block(_extract_section(text, heading), key, bytes_, heading)
        for key, (heading, bytes_) in sections.items()
    }


def instruction_cost(key: str) -> Cost:
    item = load_manual_subset()[key]
    return Cost(item.bytes, item.tstates_8085)


def sequence_cost(keys: list[str]) -> Cost:
    total = Cost()
    for key in keys:
        total += instruction_cost(key)
    return total


def pseudo_choice_cost(
    opcode: str,
    use_pos: int,
    physreg: str,
    has_later_use: bool,
) -> Cost | None:
    """
    Return an estimated late-expansion cost for one pseudo operand choice.

    This is deliberately local:
    - it only models the address-bearing operand use we are assigning
    - it ignores PSW save/restore and other nonlocal liveness effects for now
    - it assumes generic non-A/non-HL source/dest values where that matters
    """

    if use_pos != 0:
        return Cost()

    if opcode == "LOAD_8_ADDR_CONTENT":
        if physreg == "HL":
            return sequence_cost(["MOV_FROM_MEM"])
        if physreg in ("BC", "DE"):
            return sequence_cost(["LDAX_INDIRECT", "MOV_REG_REG"])
        return Cost()

    if opcode == "LOAD_16_ADDR_CONTENT":
        if physreg == "HL":
            seq = ["MOV_FROM_MEM", "INX_PAIR", "MOV_FROM_MEM"]
            if has_later_use:
                seq.append("DCX_PAIR")
            return sequence_cost(seq)
        if physreg in ("BC", "DE"):
            return sequence_cost(
                [
                    "LDAX_INDIRECT",
                    "MOV_REG_REG",
                    "INX_PAIR",
                    "LDAX_INDIRECT",
                    "MOV_REG_REG",
                    "DCX_PAIR",
                ]
            )
        return Cost()

    if opcode == "STORE_8_ADDR_CONTENT":
        if physreg == "HL":
            return sequence_cost(["MOV_TO_MEM"])
        if physreg in ("BC", "DE"):
            return sequence_cost(["MOV_REG_REG", "STAX_INDIRECT"])
        return Cost()

    if opcode == "STORE_16_ADDR_CONTENT":
        if physreg == "HL":
            seq = ["MOV_TO_MEM", "INX_PAIR", "MOV_TO_MEM"]
            if has_later_use:
                seq.append("DCX_PAIR")
            return sequence_cost(seq)
        if physreg in ("BC", "DE"):
            return sequence_cost(
                [
                    "MOV_REG_REG",
                    "STAX_INDIRECT",
                    "INX_PAIR",
                    "MOV_REG_REG",
                    "STAX_INDIRECT",
                    "DCX_PAIR",
                ]
            )
        return Cost()

    if opcode == "LOAD_32_ADDR_CONTENT":
        if physreg == "HL":
            # HL source gets copied to BC once because scratch stores clobber HL.
            # After that, the loop uses the same per-byte address setup as BC/DE.
            return sequence_cost(
                ["MOV_REG_REG", "MOV_REG_REG"]
                + ["MOV_REG_REG"] * 8
                + ["INX_PAIR"] * 6
                + ["MOV_FROM_MEM"] * 4
            )
        if physreg in ("BC", "DE"):
            return sequence_cost(
                ["MOV_REG_REG"] * 8
                + ["INX_PAIR"] * 6
                + ["MOV_FROM_MEM"] * 4
            )
        return Cost()

    if opcode == "STORE_32_ADDR_CONTENT":
        if physreg == "HL":
            return sequence_cost(
                ["PUSH_RP"]
                + ["POP_RP"] * 4
                + ["MOV_TO_MEM"] * 4
                + ["INX_PAIR"] * 3
                + ["PUSH_RP"] * 3
            )
        if physreg in ("BC", "DE"):
            return sequence_cost(
                ["MOV_REG_REG"] * 8
                + ["INX_PAIR"] * 6
                + ["MOV_TO_MEM"] * 4
            )
        return Cost()

    return Cost()


def estimated_spill_cost_16() -> Cost:
    """
    Approximate cost of one 16-bit spill/reload through the common SP-relative
    stack-slot path seen in current hot MIR.

    This intentionally models the dominant simple path:
    - STORE_16 %stack.N, 0, reg, implicit $sp
    - LOAD_16_WITH_ADDR %stack.N, 0, implicit $sp

    Using the late expander, both commonly become:
    - LXI H, off
    - DAD SP
    - MOV M, low / MOV r,M
    - INX H
    - MOV M, high / MOV r,M

    This omits rarer PSW/HL-preserve paths and adjacent-pair combines, but it
    is already much closer to the real backend cost surface than treating a
    spill as abstract "1".
    """
    return sequence_cost(["LXI_IMM16", "DAD_PAIR", "MOV_TO_MEM", "INX_PAIR", "MOV_TO_MEM"])


def estimated_reload_cost_16() -> Cost:
    return sequence_cost(["LXI_IMM16", "DAD_PAIR", "MOV_FROM_MEM", "INX_PAIR", "MOV_FROM_MEM"])
