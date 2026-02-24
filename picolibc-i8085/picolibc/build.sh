#!/usr/bin/env bash
set -euo pipefail

ROOT=$(cd "$(dirname "$0")/../.." && pwd)
SRC_DIR="$ROOT/picolibc-i8085/picolibc-src"
BUILD_DIR="$ROOT/picolibc-i8085/build"
SYSROOT="$ROOT/sysroot"
TOOLCHAIN_BIN="$ROOT/llvm-project/build-clang-8085/bin"
CROSS_TEMPLATE="$ROOT/picolibc-i8085/picolibc/cross/i8085-unknown-elf.txt"

if [[ ! -d "$SRC_DIR" ]]; then
  echo "picolibc source not found. Run: git submodule update --init picolibc-i8085/picolibc-src" >&2
  exit 1
fi

if [[ ! -x "$TOOLCHAIN_BIN/clang" ]]; then
  echo "Toolchain not found at $TOOLCHAIN_BIN" >&2
  exit 1
fi

# Generate cross file from template with resolved paths
CROSS_FILE="$BUILD_DIR/i8085-unknown-elf.txt"
mkdir -p "$BUILD_DIR"
sed "s|@TOOLCHAIN_BIN@|$TOOLCHAIN_BIN|g" "$CROSS_TEMPLATE" > "$CROSS_FILE"

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

echo "picolibc installed to $SYSROOT"
