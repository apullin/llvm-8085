#!/usr/bin/env python3
"""
Solve small I8085 register-allocation regions with Z3 Optimize.

This is a real solver-backed version of the existing exact-allocation probe.
It uses the same local legality/cost model, but expresses the assignment and
objective as constraints instead of brute-force search.
"""

from __future__ import annotations

import argparse
import importlib.util
import sys
from dataclasses import dataclass
from pathlib import Path

from z3 import If, Int, Optimize, Or, sat

ROOT = Path(__file__).resolve().parent
PROBE_PATH = ROOT / "exact_alloc_probe.py"
SPEC = importlib.util.spec_from_file_location("exact_alloc_probe", PROBE_PATH)
if SPEC is None or SPEC.loader is None:
    raise RuntimeError(f"unable to load {PROBE_PATH}")
probe = importlib.util.module_from_spec(SPEC)
sys.modules[SPEC.name] = probe
SPEC.loader.exec_module(probe)


PHYSREGS = ("BC", "DE", "HL")
SPILL = "SPILL"
ALL_CHOICES = PHYSREGS + (SPILL,)
CHOICE_INDEX = {name: idx for idx, name in enumerate(ALL_CHOICES)}


@dataclass(frozen=True)
class SolveCase:
    key: str
    title: str
    mir_path: Path
    function: str | None
    model: probe.RegionModel


def parse_cli() -> argparse.Namespace:
    ap = argparse.ArgumentParser(description="Z3-based local allocation probe for I8085.")
    ap.add_argument(
        "--case",
        action="append",
        choices=["bubble", "fib6", "pjvm16", "pjvm8all", "pjvm16all", "all"],
        help="Cases to solve (default: all).",
    )
    return ap.parse_args()


def build_block_case(
    key: str,
    title: str,
    mir_path: Path,
    function: str,
    block: int,
) -> SolveCase:
    mir_text = probe.extract_function(mir_path.read_text(), function)
    classes = probe.parse_classes(mir_text)
    blocks = probe.parse_blocks(mir_text)
    interesting: set[int] = set()
    for inst in blocks[block]["instrs"]:  # type: ignore[index]
        rhs = inst.split("=", 1)[1] if "=" in inst else inst
        if "=" in inst:
            lhs = inst.split("=", 1)[0]
            for m in probe.REG_PAT.finditer(lhs):
                rid = int(m.group(1))
                if classes.get(rid, "").startswith("gr16"):
                    interesting.add(rid)
        for u in probe.USE_PAT.findall(rhs):
            rid = int(u)
            if classes.get(rid, "").startswith("gr16"):
                interesting.add(rid)
    domains = {
        n: (
            ["BC", "DE"]
            if classes[n] in ("gr16bd", "gr16bdsp")
            else ["BC", "DE", "HL"]
        )
        for n in sorted(interesting)
    }
    return SolveCase(
        key,
        title,
        mir_path,
        function,
        probe.RegionModel("i8085-cost-oracle", interesting, [block], domains),
    )


def build_cases() -> list[SolveCase]:
    bubble_text = probe.BUBBLE_MIR_PATH.read_text()
    bubble_classes = probe.parse_classes(bubble_text)
    bubble_interesting = {17, 29, 7, 32, 9, 36, 35}
    bubble_domains = {
        n: (
            ["BC", "DE"]
            if bubble_classes[n] in ("gr16bd", "gr16bdsp")
            else ["BC", "DE", "HL"]
        )
        for n in bubble_interesting
    }

    fib_text = probe.extract_function(probe.FIB_MIR_PATH.read_text(), "main")
    fib_classes = probe.parse_classes(fib_text)
    fib_interesting = {12, 14, 16, 17, 19, 21, 29}
    fib_domains = {
        n: (
            ["BC", "DE"]
            if fib_classes[n] in ("gr16bd", "gr16bdsp")
            else ["BC", "DE", "HL"]
        )
        for n in fib_interesting
    }

    pjvm_text = probe.extract_function(probe.PJVM_MIR_PATH.read_text(), "pjvm_parse")
    pjvm_classes = probe.parse_classes(pjvm_text)
    pjvm_interesting = {
        152,
        230,
        113,
        115,
        118,
        120,
        123,
        127,
        131,
        133,
        137,
        139,
        142,
        144,
        147,
        149,
    }
    pjvm_domains = {
        n: (
            ["BC", "DE"]
            if pjvm_classes[n] in ("gr16bd", "gr16bdsp")
            else ["BC", "DE", "HL"]
        )
        for n in pjvm_interesting
    }

    return [
        SolveCase(
            "bubble",
            "bubble_sort compare/swap core (bb.11..bb.20)",
            probe.BUBBLE_MIR_PATH,
            None,
            probe.RegionModel(
                "i8085-cost-oracle",
                bubble_interesting,
                [11, 12, 13, 14, 15, 16, 17, 18, 19, 20],
                bubble_domains,
            ),
        ),
        SolveCase(
            "fib6",
            "fib main recurrence/store block (bb.6)",
            probe.FIB_MIR_PATH,
            "main",
            probe.RegionModel(
                "i8085-cost-oracle",
                fib_interesting,
                [6],
                fib_domains,
            ),
        ),
        SolveCase(
            "pjvm16",
            "pjvm_parse fanout/copy block (bb.16)",
            probe.PJVM_MIR_PATH,
            "pjvm_parse",
            probe.RegionModel(
                "i8085-cost-oracle",
                pjvm_interesting,
                [16],
                pjvm_domains,
            ),
        ),
        build_block_case(
            "pjvm8all",
            "pjvm_parse full 16-bit block model (bb.8)",
            probe.PJVM_MIR_PATH,
            "pjvm_parse",
            8,
        ),
        build_block_case(
            "pjvm16all",
            "pjvm_parse full 16-bit block model (bb.16)",
            probe.PJVM_MIR_PATH,
            "pjvm_parse",
            16,
        ),
    ]


def load_case_region(case: SolveCase):
    mir_text = case.mir_path.read_text()
    if case.function is not None:
        mir_text = probe.extract_function(mir_text, case.function)
    blocks = probe.parse_blocks(mir_text)
    parsed = probe.parse_region(blocks, case.model)
    _live_in, live_out = probe.liveness(blocks, parsed, case.model.region_blocks)
    _live_before, live_after = probe.inst_liveness(parsed, live_out)
    graph, use_count = probe.interference(parsed, live_out, case.model.interesting)
    return parsed, live_after, graph, use_count


def solve_case(case: SolveCase) -> None:
    parsed, live_after, graph, use_count = load_case_region(case)
    opt = Optimize()
    vars_by_reg = {
        reg: Int(f"v{reg}") for reg in sorted(case.model.interesting)
    }

    # Legal domains.
    for reg, var in vars_by_reg.items():
        allowed = [CHOICE_INDEX[r] for r in case.model.domains[reg]] + [CHOICE_INDEX[SPILL]]
        opt.add(Or(*[var == idx for idx in allowed]))

    # Interference.
    for reg in sorted(graph):
        for other in sorted(graph[reg]):
            if reg >= other:
                continue
            opt.add(
                Or(
                    vars_by_reg[reg] == CHOICE_INDEX[SPILL],
                    vars_by_reg[other] == CHOICE_INDEX[SPILL],
                    vars_by_reg[reg] != vars_by_reg[other],
                )
            )

    total_terms = []

    # Spill cost.
    for reg, var in vars_by_reg.items():
        total_terms.append(
            If(var == CHOICE_INDEX[SPILL], use_count[reg], 0)
        )

    # Pseudo-expansion penalties and illegal combinations.
    for b, ins in parsed.items():
        for i, item in enumerate(ins):
            opcode = item["opcode"]  # type: ignore[assignment]
            uses = item["uses"]  # type: ignore[assignment]
            use_positions = item["use_positions"]  # type: ignore[assignment]
            for u in uses:
                has_later_use = u in live_after[(b, i)]
                var = vars_by_reg[u]
                for physreg in PHYSREGS:
                    cost = probe.pseudo_cost(opcode, use_positions[u], physreg, has_later_use)
                    idx = CHOICE_INDEX[physreg]
                    if cost is None:
                        opt.add(var != idx)
                    elif cost:
                        total_terms.append(If(var == idx, cost, 0))

    total = sum(total_terms)
    h = opt.minimize(total)
    if opt.check() != sat:
        raise RuntimeError(f"unsat: {case.key}")

    model = opt.model()
    assignment = {
        reg: ALL_CHOICES[model.eval(var).as_long()]
        for reg, var in vars_by_reg.items()
    }
    best_cost = opt.lower(h)
    breakdown = probe.oracle_breakdown(parsed, live_after, assignment)
    print(f"Case: {case.key} - {case.title}")
    print(f"  region vars: {len(case.model.interesting)}")
    print(f"  z3 total cost: {best_cost}")
    print(f"  z3 assignment: {assignment}")
    if breakdown:
        print("  z3 oracle penalties:")
        for b, opcode, u, physreg, cost, later in breakdown:
            print(
                f"    bb.{b} {opcode} uses %{u} in {physreg}: +{cost} "
                f"(live-after={later})"
            )
    if len(case.model.interesting) <= 18:
        brute_order, brute_cost, brute_assign = probe.exact_color(
            graph, use_count, case.model.domains, parsed, live_after
        )
        print(f"  brute-force total cost: {brute_cost}")
        print(f"  brute-force assignment: {brute_assign}")
        print(f"  brute-force order: {brute_order}")
        same_cost = int(str(best_cost)) == brute_cost
        same_assignment = assignment == brute_assign
        print(f"  parity(cost): {same_cost}")
        print(f"  parity(assignment): {same_assignment}")
    else:
        print("  brute-force cross-check: skipped (region too large)")
    print()


def main() -> None:
    args = parse_cli()
    cases = build_cases()
    wanted = set(args.case or ["all"])
    if "all" in wanted:
        selected = cases
    else:
        selected = [case for case in cases if case.key in wanted]

    for case in selected:
        solve_case(case)


if __name__ == "__main__":
    main()
