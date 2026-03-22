#!/usr/bin/env python3
"""
Small exact-allocation probe for hot I8085 MIR regions.

This is intentionally not a full allocator. It answers a narrower question:
for a chosen pre-regalloc region, what is the minimum spill cost if we solve
register assignment exactly under a small target-aware model?

The current targets are:
- the bubble_sort inner loop, where LLVM's greedy allocator spills several
  16-bit values in the compare/swap path
- a spill-heavy pjvm_parse block, where the VM cost matters directly
"""

from __future__ import annotations

import argparse
import re
from collections import Counter, defaultdict
from dataclasses import dataclass
from pathlib import Path


BUBBLE_MIR_PATH = Path(".tmp/bubble_sort_pre_ra.mir")
FIB_MIR_PATH = Path(".tmp/fib_pre_ra.mir")
PJVM_MIR_PATH = Path(".tmp/pjvm_pre_ra.mir")


@dataclass(frozen=True)
class RegionModel:
    name: str
    interesting: set[int]
    region_blocks: list[int]
    domains: dict[int, list[str]]


@dataclass(frozen=True)
class ProbeCase:
    title: str
    mir_path: Path
    post_mir_path: Path
    function: str | None
    model: RegionModel
    observed: list[str]


@dataclass(frozen=True)
class SurveyCase:
    name: str
    mir_path: Path
    function: str | None
    top_blocks: int


REG_PAT = re.compile(r"%(\d+):([a-zA-Z0-9_]+)")
USE_PAT = re.compile(r"%(\d+)")
BLOCK_PAT = re.compile(r"  bb\.(\d+)")
SUCC_PAT = re.compile(r"%bb\.(\d+)")
CLASS_PAT = re.compile(r"\{ id: (\d+), class: ([a-zA-Z0-9_]+),")
OPCODE_PAT = re.compile(r"([A-Z][A-Z0-9_]+)")
STACK_SLOT_PAT = re.compile(r"%stack\.(\d+)")
STACK_OBJ_PAT = re.compile(r"- \{ id: (\d+), name: '.*', type: ([a-zA-Z0-9_-]+),")


def parse_cli() -> argparse.Namespace:
    ap = argparse.ArgumentParser(
        description="Probe small exact-allocation regions for the i8085 backend."
    )
    ap.add_argument(
        "--case",
        action="append",
        choices=["bubble", "fib6", "pjvm16", "all"],
        help="Run only selected cases (default: all).",
    )
    ap.add_argument(
        "--top-slots",
        type=int,
        default=6,
        help="Show this many hottest post-greedy stack slots per case.",
    )
    ap.add_argument(
        "--survey",
        action="append",
        choices=["bubble", "pjvm", "json", "fib", "all"],
        help="Rank hot post-greedy blocks by stack traffic (default: none).",
    )
    return ap.parse_args()


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


def parse_stack_object_types(mir_text: str) -> dict[int, str]:
    stack_types: dict[int, str] = {}
    in_stack = False
    for line in mir_text.splitlines():
        if line.strip() == "stack:":
            in_stack = True
            continue
        if not in_stack:
            continue
        if line.startswith("body:"):
            break
        m = STACK_OBJ_PAT.search(line)
        if m:
            stack_types[int(m.group(1))] = m.group(2)
    return stack_types


def extract_function(mir_text: str, name: str) -> str:
    lines = mir_text.splitlines()
    start = None
    end = None
    for i, line in enumerate(lines):
        if line.strip() == f"name:            {name}":
            start = i
            continue
        if start is not None and i > start and line.startswith("---"):
            end = i
            break
    if start is None:
        raise ValueError(f"function {name!r} not found")
    if end is None:
        end = len(lines)
    return "\n".join(lines[start:end])


def parse_region(
    blocks: dict[int, dict[str, object]],
    model: RegionModel,
) -> dict[int, list[dict[str, object]]]:
    parsed: dict[int, list[dict[str, object]]] = {}
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
            opm = OPCODE_PAT.search(rhs.strip())
            opcode = opm.group(1) if opm else ""
            use_regs = [int(u) for u in USE_PAT.findall(rhs)]
            use_positions = {
                rid: idx for idx, rid in enumerate(use_regs) if rid in model.interesting
            }
            ins.append(
                {
                    "text": inst,
                    "defs": defs,
                    "uses": uses,
                    "opcode": opcode,
                    "use_positions": use_positions,
                }
            )
        parsed[b] = ins
    return parsed


def parse_defs_uses_all(
    blocks: dict[int, dict[str, object]],
    region_blocks: list[int],
) -> dict[int, list[dict[str, object]]]:
    parsed: dict[int, list[dict[str, object]]] = {}
    for b in region_blocks:
        ins = []
        for inst in blocks[b]["instrs"]:  # type: ignore[index]
            defs: list[int] = []
            if "=" in inst:
                lhs, _rhs = inst.split("=", 1)
                defs = [int(m.group(1)) for m in REG_PAT.finditer(lhs)]
            rhs = inst.split("=", 1)[1] if "=" in inst else inst
            uses = [int(u) for u in USE_PAT.findall(rhs)]
            opm = OPCODE_PAT.search(rhs.strip())
            opcode = opm.group(1) if opm else ""
            slotm = STACK_SLOT_PAT.search(inst)
            slot = int(slotm.group(1)) if slotm else None
            ins.append(
                {
                    "text": inst,
                    "defs": defs,
                    "uses": uses,
                    "opcode": opcode,
                    "slot": slot,
                }
            )
        parsed[b] = ins
    return parsed


def block_use_def(parsed: dict[int, list[dict[str, object]]]):
    bdef: dict[int, set[int]] = {}
    buse: dict[int, set[int]] = {}
    for b, ins in parsed.items():
        seen: set[int] = set()
        defs: set[int] = set()
        uses: set[int] = set()
        for item in ins:
            inst_defs = item["defs"]  # type: ignore[assignment]
            inst_uses = item["uses"]  # type: ignore[assignment]
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
    parsed: dict[int, list[dict[str, object]]],
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


def inst_liveness(
    parsed: dict[int, list[dict[str, object]]],
    live_out: dict[int, set[int]],
):
    live_before: dict[tuple[int, int], set[int]] = {}
    live_after: dict[tuple[int, int], set[int]] = {}
    for b, ins in parsed.items():
        live = set(live_out[b])
        for i in reversed(range(len(ins))):
            item = ins[i]
            live_after[(b, i)] = set(live)
            defs = item["defs"]  # type: ignore[assignment]
            uses = item["uses"]  # type: ignore[assignment]
            for d in defs:
                live.discard(d)
            for u in uses:
                live.add(u)
            live_before[(b, i)] = set(live)
    return live_before, live_after


def interference(
    parsed: dict[int, list[dict[str, object]]],
    live_out: dict[int, set[int]],
    interesting: set[int],
):
    graph = {n: set() for n in interesting}
    use_count = {n: 0 for n in interesting}
    for b, ins in parsed.items():
        live = set(live_out[b])
        for item in reversed(ins):
            defs = item["defs"]  # type: ignore[assignment]
            uses = item["uses"]  # type: ignore[assignment]
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


def pseudo_cost(
    opcode: str,
    use_pos: int,
    physreg: str,
    has_later_use: bool,
) -> int | None:
    # None means "illegal in this simplified local model".
    if opcode == "LOAD_8_ADDR_CONTENT" and use_pos == 0:
        if physreg == "HL":
            return 0
        if physreg in ("BC", "DE"):
            return 1
    if opcode == "LOAD_16_ADDR_CONTENT" and use_pos == 0:
        if physreg == "HL":
            return 0
        if physreg in ("BC", "DE"):
            return 1
    if opcode == "STORE_8_ADDR_CONTENT" and use_pos == 0:
        if physreg == "HL":
            return 0
        if physreg in ("BC", "DE"):
            return 1
    if opcode == "STORE_16_ADDR_CONTENT" and use_pos == 0:
        if physreg == "HL":
            return None if has_later_use else 0
        if physreg in ("BC", "DE"):
            return 1
    if opcode == "LOAD_32_ADDR_CONTENT" and use_pos == 0:
        if physreg == "HL":
            return None if has_later_use else 2
        if physreg in ("BC", "DE"):
            return 0
    return 0


def oracle_penalty(
    parsed: dict[int, list[dict[str, object]]],
    live_after: dict[tuple[int, int], set[int]],
    assign: dict[int, str],
) -> int | None:
    total = 0
    for b, ins in parsed.items():
        for i, item in enumerate(ins):
            opcode = item["opcode"]  # type: ignore[assignment]
            uses = item["uses"]  # type: ignore[assignment]
            use_positions = item["use_positions"]  # type: ignore[assignment]
            for u in uses:
                physreg = assign.get(u)
                if physreg is None or physreg == "SPILL":
                    continue
                has_later_use = u in live_after[(b, i)]
                cost = pseudo_cost(opcode, use_positions[u], physreg, has_later_use)
                if cost is None:
                    return None
                total += cost
    return total


def oracle_breakdown(
    parsed: dict[int, list[dict[str, object]]],
    live_after: dict[tuple[int, int], set[int]],
    assign: dict[int, str],
) -> list[tuple[int, str, int, str, int, bool]]:
    rows: list[tuple[int, str, int, str, int, bool]] = []
    for b, ins in parsed.items():
        for i, item in enumerate(ins):
            opcode = item["opcode"]  # type: ignore[assignment]
            uses = item["uses"]  # type: ignore[assignment]
            use_positions = item["use_positions"]  # type: ignore[assignment]
            for u in uses:
                physreg = assign.get(u)
                if physreg is None or physreg == "SPILL":
                    continue
                has_later_use = u in live_after[(b, i)]
                cost = pseudo_cost(opcode, use_positions[u], physreg, has_later_use)
                if cost:
                    rows.append((b, opcode, u, physreg, cost, has_later_use))
    return rows


def collect_hint_votes(
    parsed: dict[int, list[dict[str, object]]],
    assign: dict[int, str],
) -> dict[tuple[str, int], Counter[str]]:
    votes: dict[tuple[str, int], Counter[str]] = defaultdict(Counter)
    for ins in parsed.values():
        for item in ins:
            opcode = item["opcode"]  # type: ignore[assignment]
            uses = item["uses"]  # type: ignore[assignment]
            use_positions = item["use_positions"]  # type: ignore[assignment]
            for u in uses:
                physreg = assign.get(u)
                if physreg is None or physreg == "SPILL":
                    continue
                votes[(opcode, use_positions[u])][physreg] += 1
    return votes


def post_stack_traffic(
    parsed: dict[int, list[dict[str, object]]],
) -> dict[int, Counter[str]]:
    traffic: dict[int, Counter[str]] = defaultdict(Counter)
    for ins in parsed.values():
        for item in ins:
            slot = item["slot"]  # type: ignore[assignment]
            if slot is None:
                continue
            opcode = item["opcode"]  # type: ignore[assignment]
            if opcode.startswith("LOAD_"):
                traffic[slot]["loads"] += 1
                traffic[slot][opcode] += 1
            elif opcode.startswith("STORE_"):
                traffic[slot]["stores"] += 1
                traffic[slot][opcode] += 1
    return traffic


def block_stack_traffic(
    parsed: dict[int, list[dict[str, object]]],
    stack_types: dict[int, str],
) -> dict[int, dict[str, object]]:
    per_block: dict[int, dict[str, object]] = {}
    for b, ins in parsed.items():
        totals = Counter[str]()
        slots: dict[int, dict[str, object]] = {}
        for item in ins:
            slot = item["slot"]  # type: ignore[assignment]
            if slot is None:
                continue
            opcode = item["opcode"]  # type: ignore[assignment]
            kind = stack_types.get(slot, "unknown")
            totals["total"] += 1
            totals[kind] += 1
            slot_info = slots.setdefault(
                slot,
                {"kind": kind, "counts": Counter[str]()},
            )
            ctr = slot_info["counts"]  # type: ignore[assignment]
            if opcode.startswith("LOAD_"):
                totals["loads"] += 1
                totals[f"{kind}-loads"] += 1
                ctr["loads"] += 1
            elif opcode.startswith("STORE_"):
                totals["stores"] += 1
                totals[f"{kind}-stores"] += 1
                ctr["stores"] += 1
            ctr["total"] += 1
            ctr[opcode] += 1
        per_block[b] = {"totals": totals, "slots": slots}
    return per_block


def print_block_survey(
    name: str,
    mir_path: Path,
    function: str | None,
    top_blocks: int,
    top_slots: int,
) -> None:
    mir_text = mir_path.read_text()
    if function is not None:
        mir_text = extract_function(mir_text, function)
    blocks = parse_blocks(mir_text)
    parsed = parse_defs_uses_all(blocks, sorted(blocks))
    stack_types = parse_stack_object_types(mir_text)
    per_block = block_stack_traffic(parsed, stack_types)
    ranked = sorted(
        (
            (b, info["totals"], info["slots"])
            for b, info in per_block.items()
            if info["totals"]["total"]  # type: ignore[index]
        ),
        key=lambda row: (
            row[1]["spill-slot"],
            row[1]["total"],
            row[1]["loads"],
            -row[0],
        ),
        reverse=True,
    )

    print(f"Survey: {name}")
    print(f"  MIR: {mir_path}")
    if function is not None:
        print(f"  Function: {function}")
    for b, totals, slots in ranked[:top_blocks]:
        print(
            "  bb.%d total=%d spill=%d default=%d loads=%d stores=%d"
            % (
                b,
                totals["total"],
                totals["spill-slot"],
                totals["default"],
                totals["loads"],
                totals["stores"],
            )
        )
        hot_slots = sorted(
            slots.items(),
            key=lambda kv: (
                kv[1]["counts"]["total"],  # type: ignore[index]
                kv[1]["counts"]["loads"],  # type: ignore[index]
                kv[1]["counts"]["stores"],  # type: ignore[index]
                -kv[0],
            ),
            reverse=True,
        )
        for slot, slot_info in hot_slots[:top_slots]:
            kind = slot_info["kind"]  # type: ignore[assignment]
            ctr = slot_info["counts"]  # type: ignore[assignment]
            kinds = ", ".join(
                f"{name}:{count}"
                for name, count in sorted(
                    (
                        (k, v)
                        for k, v in ctr.items()
                        if k not in ("total", "loads", "stores", "kind")
                    ),
                    key=lambda kv: (-kv[1], kv[0]),
                )
            )
            print(
                "    %%stack.%d (%s): total=%d loads=%d stores=%d [%s]"
                % (
                    slot,
                    kind,
                    ctr["total"],
                    ctr["loads"],
                    ctr["stores"],
                    kinds,
                )
            )
    print()


def print_hint_votes(votes: dict[tuple[str, int], Counter[str]]) -> None:
    if not votes:
        return
    print("  derived hint votes:")
    for (opcode, use_pos), ctr in sorted(votes.items()):
        if opcode.endswith("_ADDR_CONTENT") or opcode.startswith("STORE_"):
            ordered = ", ".join(f"{reg}:{count}" for reg, count in ctr.most_common())
            print(f"    {opcode} use#{use_pos}: {ordered}")


def print_stack_traffic(
    traffic: dict[int, Counter[str]],
    top_slots: int,
) -> None:
    if not traffic:
        return
    print("  post-greedy stack traffic:")
    ranked = sorted(
        traffic.items(),
        key=lambda kv: (
            kv[1]["loads"] + kv[1]["stores"],
            kv[1]["loads"],
            kv[1]["stores"],
            -kv[0],
        ),
        reverse=True,
    )
    for slot, ctr in ranked[:top_slots]:
        total = ctr["loads"] + ctr["stores"]
        kinds = ", ".join(
            f"{name}:{count}"
            for name, count in sorted(
                ((k, v) for k, v in ctr.items() if k not in ("loads", "stores")),
                key=lambda kv: (-kv[1], kv[0]),
            )
        )
        print(
            f"    %stack.{slot}: total={total} loads={ctr['loads']} stores={ctr['stores']}"
            f" [{kinds}]"
        )


def exact_color(
    graph: dict[int, set[int]],
    use_count: dict[int, int],
    domains: dict[int, list[str]],
    parsed: dict[int, list[dict[str, object]]],
    live_after: dict[tuple[int, int], set[int]],
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
            penalty = oracle_penalty(parsed, live_after, assign)
            if penalty is None:
                return
            total = cost + penalty
            if total < best_cost:
                best_cost = total
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


def run_case(case: ProbeCase, top_slots: int) -> None:
    mir_text = case.mir_path.read_text()
    if case.function is not None:
        mir_text = extract_function(mir_text, case.function)
    blocks = parse_blocks(mir_text)
    parsed = parse_region(blocks, case.model)
    live_in, live_out = liveness(blocks, parsed, case.model.region_blocks)
    _live_before, live_after = inst_liveness(parsed, live_out)
    graph, use_count = interference(parsed, live_out, case.model.interesting)
    order, best_cost, best_assign = exact_color(
        graph, use_count, case.model.domains, parsed, live_after
    )
    breakdown = oracle_breakdown(parsed, live_after, best_assign or {})
    hint_votes = collect_hint_votes(parsed, best_assign or {})

    post_text = case.post_mir_path.read_text()
    if case.function is not None:
        post_text = extract_function(post_text, case.function)
    post_blocks = parse_blocks(post_text)
    post_parsed = parse_defs_uses_all(post_blocks, case.model.region_blocks)
    traffic = post_stack_traffic(post_parsed)

    print(f"MIR: {case.mir_path}")
    print(f"Post-greedy MIR: {case.post_mir_path}")
    if case.function is not None:
        print(f"Function: {case.function}")
    print(f"Region: {case.title}")
    print("Interesting vregs:", sorted(case.model.interesting))
    print(f"Model: {case.model.name}")
    print("  live-in:")
    for b in case.model.region_blocks:
        if live_in[b]:
            print(f"    bb.{b}: {sorted(live_in[b])}")
    print("  interference:")
    for n in sorted(graph):
        print(f"    %{n}: {sorted(graph[n])}")
    print(f"  use_count: {use_count}")
    print(f"  search order: {order}")
    print(f"  best total cost: {best_cost}")
    print(f"  best assignment: {best_assign}")
    if breakdown:
        print("  oracle penalties:")
        for b, opcode, u, physreg, cost, later in breakdown:
            print(
                f"    bb.{b} {opcode} uses %{u} in {physreg}: +{cost}"
                f" (live-after={later})"
            )
    print_hint_votes(hint_votes)
    print_stack_traffic(traffic, top_slots)
    print("Observed greedy post-RA behavior in the same region:")
    for line in case.observed:
        print(f"  {line}")
    print()


def main() -> None:
    args = parse_cli()
    bubble_text = BUBBLE_MIR_PATH.read_text()
    bubble_classes = parse_classes(bubble_text)
    bubble_interesting = {17, 29, 7, 32, 9, 36, 35}
    bubble_domains = {
        n: (
            ["BC", "DE"]
            if bubble_classes[n] in ("gr16bd", "gr16bdsp")
            else ["BC", "DE", "HL"]
        )
        for n in bubble_interesting
    }

    pjvm_text = extract_function(PJVM_MIR_PATH.read_text(), "pjvm_parse")
    pjvm_classes = parse_classes(pjvm_text)
    fib_text = extract_function(FIB_MIR_PATH.read_text(), "main")
    fib_classes = parse_classes(fib_text)
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
    fib_interesting = {12, 14, 16, 17, 19, 21, 29}
    fib_domains = {
        n: (
            ["BC", "DE"]
            if fib_classes[n] in ("gr16bd", "gr16bdsp")
            else ["BC", "DE", "HL"]
        )
        for n in fib_interesting
    }

    cases = [
        ProbeCase(
            title="bubble_sort compare/swap core (bb.11..bb.20)",
            mir_path=BUBBLE_MIR_PATH,
            post_mir_path=Path(".tmp/bubble_sort_post_ra_like.mir"),
            function=None,
            model=RegionModel(
                "i8085-cost-oracle",
                bubble_interesting,
                [11, 12, 13, 14, 15, 16, 17, 18, 19, 20],
                bubble_domains,
            ),
            observed=[
                "spills loop index %95 to %stack.6",
                "spills address values derived from %7/%9 to %stack.8 and %stack.9",
                "spills loaded compare value %98/%36 to %stack.7",
                "re-loads those values in bb.16, bb.20, and bb.21",
            ],
        ),
        ProbeCase(
            title="fib main recurrence/store block (bb.6)",
            mir_path=FIB_MIR_PATH,
            post_mir_path=Path(".tmp/fib_post_ra_like.mir"),
            function="main",
            model=RegionModel(
                "i8085-cost-oracle",
                fib_interesting,
                [6],
                fib_domains,
            ),
            observed=[
                "spills the doubled index family to %stack.2 around the two recurrence loads",
                "spills the loop index family to %stack.0 before the volatile store",
                "spills the first loaded Fibonacci value to %stack.3 before the second load",
            ],
        ),
        ProbeCase(
            title="pjvm_parse fanout/copy block (bb.16)",
            mir_path=PJVM_MIR_PATH,
            post_mir_path=Path(".tmp/pjvm_post_ra_like.mir"),
            function="pjvm_parse",
            model=RegionModel(
                "i8085-cost-oracle",
                pjvm_interesting,
                [16],
                pjvm_domains,
            ),
            observed=[
                "spills the %230 base family to %stack.5 and reloads it before each offset use",
                "spills the %152 base family to %stack.15 and reloads it before each destination-address use",
                "also spills the computed m_co-derived destination to %stack.21 before STORE_32_ADDR_CONTENT",
            ],
        ),
    ]

    surveys = [
        SurveyCase("bubble_sort post-greedy", Path(".tmp/bubble_sort_post_ra_like.mir"), None, 6),
        SurveyCase("pjvm_parse post-greedy", Path(".tmp/pjvm_post_ra_like.mir"), "pjvm_parse", 8),
        SurveyCase("json_tokenize post-greedy", Path(".tmp/json_parse_post_ra_like.mir"), "json_tokenize", 8),
        SurveyCase("fib main post-greedy", Path(".tmp/fib_post_ra_like.mir"), "main", 6),
    ]

    if args.case is None:
        selected = [] if args.survey else cases
    else:
        wanted = set(args.case)
        if "all" in wanted:
            selected = cases
        else:
            mapping = {"bubble": 0, "fib6": 1, "pjvm16": 2}
            selected = [cases[mapping[name]] for name in wanted]

    for case in selected:
        run_case(case, args.top_slots)

    wanted_surveys = set(args.survey or [])
    if "all" in wanted_surveys:
        selected_surveys = surveys
    else:
        survey_map = {"bubble": 0, "pjvm": 1, "json": 2, "fib": 3}
        selected_surveys = [surveys[survey_map[name]] for name in wanted_surveys]

    for survey in selected_surveys:
        print_block_survey(
            survey.name,
            survey.mir_path,
            survey.function,
            survey.top_blocks,
            args.top_slots,
        )


if __name__ == "__main__":
    main()
