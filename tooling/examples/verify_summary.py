#!/usr/bin/env python3
"""Verify i8085-trace summary JSON output."""

import argparse
import json
import sys


def main() -> int:
    parser = argparse.ArgumentParser(description="Verify i8085-trace summary JSON")
    parser.add_argument("--summary", required=True, help="path to summary JSON output")
    parser.add_argument("--expect-halt", required=True, help="expected halt reason")
    args = parser.parse_args()

    with open(args.summary, "r", encoding="utf-8") as f:
        data = f.read().strip()

    if not data:
        print("error: empty summary output", file=sys.stderr)
        return 1

    try:
        obj = json.loads(data)
    except json.JSONDecodeError as exc:
        print(f"error: invalid JSON: {exc}", file=sys.stderr)
        return 1

    halt = obj.get("halt")
    if halt != args.expect_halt:
        print("error: halt mismatch", file=sys.stderr)
        print(f"  expected: {args.expect_halt}", file=sys.stderr)
        print(f"  actual:   {halt}", file=sys.stderr)
        return 1

    print("ok: halt matched")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
