#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
LLVM_BUILD="${LLVM_BUILD:-$ROOT/llvm-project/build-clang-8085}"
SYSROOT="${SYSROOT:-$ROOT/sysroot}"

RES_DIR="$LLVM_BUILD/lib/clang/20/i8085"
if [[ ! -d "$RES_DIR/lib" ]]; then
  echo "missing resource dir: $RES_DIR/lib" >&2
  exit 1
fi

rm -rf "$SYSROOT/lib" "$SYSROOT/include"
mkdir -p "$SYSROOT/lib" "$SYSROOT/include"
cp -a "$RES_DIR/lib/." "$SYSROOT/lib/"
if [[ -d "$RES_DIR/include" ]]; then
  cp -a "$RES_DIR/include/." "$SYSROOT/include/"
fi

echo "sysroot synced to $SYSROOT"
