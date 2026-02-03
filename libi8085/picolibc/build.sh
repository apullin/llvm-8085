#!/usr/bin/env bash
set -euo pipefail

ROOT=$(cd "$(dirname "$0")/../.." && pwd)
SRC_DIR="$ROOT/libi8085/third_party/picolibc"
BUILD_DIR="$ROOT/libi8085/build/picolibc-i8085-build"
SYSROOT="$ROOT/sysroot"
TOOLCHAIN_BIN="$ROOT/tooling/build/build-clang-8085/bin"
CROSS_FILE="$ROOT/libi8085/picolibc/cross/i8085-unknown-elf.txt"

export PATH="$TOOLCHAIN_BIN:$PATH"

meson setup "$BUILD_DIR" "$SRC_DIR" \
  --cross-file "$CROSS_FILE" \
  --prefix "$SYSROOT" \
  --libdir lib \
  --includedir include \
  -Dmultilib=false \
  -Dtests=false \
  -Dpicocrt=false \
  -Dpicocrt-lib=false \
  -Dthread-local-storage=false \
  -Dnewlib-global-errno=true \
  -Dspecsdir=none

ninja -C "$BUILD_DIR"
ninja -C "$BUILD_DIR" install
