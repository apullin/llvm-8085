#!/usr/bin/env python3
"""
Small exact-allocation probe for one hot I8085 MIR region.

This is intentionally not a full allocator. It answers a narrower question:
for a chosen pre-regalloc region, what is the minimum spill cost if we solve
register assignment exactly under a small target-aware model?

The first target is the bubble_sort inner loop, where LLVM's greedy allocator
spills several 16-bit values in the compare/swap path.
"""

from __future__ import annotations

import re
from dataclasses import dataclass
from pathlib import Path


MIR_PATH = Path(".tmp/bubble_sort_pre_ra.mir")


@dataclass(frozen=True)
class RegionModel:
    name: str
    interesting: set[int]
    region_blocks: list[int]
    domains: dict[int, list[str]]


REG_PAT = re.compile(r"%(\d+):([a-zA-Z0-9_]+)")
USE_PAT = re.compile(r"%(\d+)")
BLOCK_PAT = re.compile(r"  bb\.(\d+)")
SUCC_PAT = re.compile(r"%bb\.(\d+)")
CLASS_PAT = re.compile(r"\{ id: (\d+), class: ([a-zA-Z0-9_]+),")


def parse_blocks(mir_text: str) -> dict[int, dict[str, object]]:
    blocks: dict[int, dict[str, object]] = {}
    cur: int | None = None
    for line in mir_text.splitlines():
        m = BLOCK_PAT.match(line)
        if m:
            cur = int(m.group(1))
            blocks[cur] = {"succ": [], "instrs": []}
            continue
        if cur is None:
            continue
        if line.lstrip().startswith("successors:"):
            blocks[cur]["succ"] = [int(x) for x in SUCC_PAT.findall(line)]
            continue
        if line.startswith("    "):
            s = line.strip()
            if s:
                blocks[cur]["instrs"].append(s)
    return blocks


def parse_classes(mir_text: str) -> dict[int, str]:
    return {int(r): c for r, c in CLASS_PAT.findall(mir_text)}


def parse_region(
    blocks: dict[int, dict[str, object]],
    model: RegionModel,
) -> dict[int, list[tuple[str, list[int], list[int]]]]:
    parsed: dict[int, list[tuple[str, list[int], list[int]]]] = {}
    for b in model.region_blocks:
        ins = []
        for inst in blocks[b]["instrs"]:  # type: ignore[index]
            defs: list[int] = []
            if "=" in inst:
                lhs, _rhs = inst.split("=", 1)
                for m in REG_PAT.finditer(lhs):
                    rid = int(m.group(1))
                    if rid in model.interesting:
                        defs.append(rid)
            rhs = inst.split("=", 1)[1] if "=" in inst else inst
            uses = [int(u) for u in USE_PAT.findall(rhs) if int(u) in model.interesting]
            ins.append((inst, defs, uses))
        parsed[b] = ins
    return parsed


def block_use_def(parsed: dict[int, list[tuple[str, list[int], list[int]]]]):
    bdef: dict[int, set[int]] = {}
    buse: dict[int, set[int]] = {}
    for b, ins in parsed.items():
        seen: set[int] = set()
        defs: set[int] = set()
        uses: set[int] = set()
        for _inst, inst_defs, inst_uses in ins:
            for u in inst_uses:
                if u not in seen:
                    uses.add(u)
            for d in inst_defs:
                seen.add(d)
                defs.add(d)
        bdef[b] = defs
        buse[b] = uses
    return bdef, buse


def liveness(
    blocks: dict[int, dict[str, object]],
    parsed: dict[int, list[tuple[str, list[int], list[int]]]],
    region_blocks: list[int],
):
    bdef, buse = block_use_def(parsed)
    live_in = {b: set() for b in region_blocks}
    live_out = {b: set() for b in region_blocks}
    changed = True
    while changed:
        changed = False
        for b in reversed(region_blocks):
            out: set[int] = set()
            for succ in blocks[b]["succ"]:  # type: ignore[index]
                if succ in live_in:
                    out |= live_in[succ]
            inn = buse[b] | (out - bdef[b])
            if out != live_out[b] or inn != live_in[b]:
                live_out[b] = out
                live_in[b] = inn
                changed = True
    return live_in, live_out


def interference(
    parsed: dict[int, list[tuple[str, list[int], list[int]]]],
    live_out: dict[int, set[int]],
    interesting: set[int],
):
    graph = {n: set() for n in interesting}
    use_count = {n: 0 for n in interesting}
    for b, ins in parsed.items():
        live = set(live_out[b])
        for _inst, defs, uses in reversed(ins):
            for x in defs + uses:
                use_count[x] += 1
            for d in defs:
                for l in live:
                    if l != d:
                        graph[d].add(l)
                        graph[l].add(d)
                live.discard(d)
            for u in uses:
                live.add(u)
    return graph, use_count


def exact_color(
    graph: dict[int, set[int]],
    use_count: dict[int, int],
    domains: dict[int, list[str]],
):
    order = sorted(
        graph,
        key=lambda n: (len(domains[n]), len(graph[n]), use_count[n]),
        reverse=True,
    )
    best_cost = 10**9
    best_assign: dict[int, str] | None = None

    def search(i: int, assign: dict[int, str], cost: int):
        nonlocal best_cost, best_assign
        if cost >= best_cost:
            return
        if i == len(order):
            best_cost = cost
            best_assign = assign.copy()
            return
        node = order[i]
        used = {
            assign[other]
            for other in graph[node]
            if other in assign and assign[other] != "SPILL"
        }
        for reg in domains[node]:
            if reg not in used:
                assign[node] = reg
                search(i + 1, assign, cost)
                del assign[node]
        assign[node] = "SPILL"
        search(i + 1, assign, cost + use_count[node])
        del assign[node]

    search(0, {}, 0)
    return order, best_cost, best_assign


def main() -> None:
    mir_text = MIR_PATH.read_text()
    blocks = parse_blocks(mir_text)
    classes = parse_classes(mir_text)

    # Hot compare/swap region in bubble_sort main.
    interesting = {17, 29, 7, 32, 9, 36, 35}
    region_blocks = [11, 12, 13, 14, 15, 16, 17, 18, 19, 20]

    generic_domains = {
        n: (["BC", "DE"] if classes[n] in ("gr16bd", "gr16bdsp") else ["BC", "DE", "HL"])
        for n in interesting
    }
    # Address values %7/%9 feed *_ADDR_CONTENT pseudos which clobber HL during
    # expansion, so a target-aware model should not let them live in HL.
    target_domains = {n: list(v) for n, v in generic_domains.items()}
    target_domains[7] = ["BC", "DE"]
    target_domains[9] = ["BC", "DE"]

    models = [
        RegionModel("generic", interesting, region_blocks, generic_domains),
        RegionModel("i8085-aware", interesting, region_blocks, target_domains),
    ]

    print(f"MIR: {MIR_PATH}")
    print("Region: bubble_sort compare/swap core (bb.11..bb.20)")
    print("Interesting vregs:", sorted(interesting))

    for model in models:
        parsed = parse_region(blocks, model)
        live_in, live_out = liveness(blocks, parsed, model.region_blocks)
        graph, use_count = interference(parsed, live_out, model.interesting)
        order, best_cost, best_assign = exact_color(graph, use_count, model.domains)

        print()
        print(f"Model: {model.name}")
        print("  live-in:")
        for b in model.region_blocks:
            if live_in[b]:
                print(f"    bb.{b}: {sorted(live_in[b])}")
        print("  interference:")
        for n in sorted(graph):
            print(f"    %{n}: {sorted(graph[n])}")
        print(f"  search order: {order}")
        print(f"  best spill cost: {best_cost}")
        print(f"  best assignment: {best_assign}")

    print()
    print("Observed greedy post-RA behavior in the same region:")
    print("  spills loop index %95 to %stack.6")
    print("  spills address values derived from %7/%9 to %stack.8 and %stack.9")
    print("  spills loaded compare value %98/%36 to %stack.7")
    print("  re-loads those values in bb.16, bb.20, and bb.21")


if __name__ == "__main__":
    main()
