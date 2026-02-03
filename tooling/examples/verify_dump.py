#!/usr/bin/env python3
"""Verify i8085-trace memory dump output against expected hex bytes."""

import argparse
import re
import sys


def parse_expected(path: str) -> list[int]:
    out: list[int] = []
    with open(path, "r", encoding="utf-8") as f:
        for line in f:
            line = line.split("#", 1)[0].strip()
            if not line:
                continue
            for tok in line.split():
                out.append(int(tok, 16))
    return out


def parse_dump(path: str) -> list[int]:
    out: list[int] = []
    with open(path, "r", encoding="utf-8") as f:
        for line in f:
            m = re.match(r"^\s*[0-9A-Fa-f]{4}:", line)
            if not m:
                continue
            _, rest = line.split(":", 1)
            data = rest.split("|")[0].strip()
            if not data:
                continue
            for tok in data.split():
                out.append(int(tok, 16))
    return out


def main() -> int:
    parser = argparse.ArgumentParser(description="Verify i8085-trace memory dump")
    parser.add_argument("--dump", required=True, help="path to i8085-trace stderr output")
    parser.add_argument("--expected", required=True, help="path to expected hex bytes")
    args = parser.parse_args()

    expected = parse_expected(args.expected)
    actual = parse_dump(args.dump)

    if len(actual) < len(expected):
        print("error: dump shorter than expected", file=sys.stderr)
        print(f"  expected bytes: {len(expected)}", file=sys.stderr)
        print(f"  actual bytes:   {len(actual)}", file=sys.stderr)
        return 1

    for i, exp in enumerate(expected):
        act = actual[i]
        if act != exp:
            print("error: mismatch at byte index %d" % i, file=sys.stderr)
            print(f"  expected: 0x{exp:02X}", file=sys.stderr)
            print(f"  actual:   0x{act:02X}", file=sys.stderr)
            return 1

    print(f"ok: matched {len(expected)} bytes")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
