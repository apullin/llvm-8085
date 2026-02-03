#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
TOOLBIN="${TOOLBIN:-$ROOT/tooling/build/build-clang-8085/bin}"
SYSROOT="${SYSROOT:-$ROOT/sysroot}"

CLANG="${TOOLBIN}/clang"
LLC="${TOOLBIN}/llc"
AR="${TOOLBIN}/llvm-ar"

BUILTINS_DIR="${ROOT}/llvm-project/compiler-rt/lib/builtins"
LIBI8085_DIR="${SYSROOT}/libi8085"
LIBI8085_BUILTINS_DIR="${LIBI8085_DIR}/builtins"
OUT_DIR="${SYSROOT}/lib/builtins"

if [[ ! -x "${CLANG}" || ! -x "${LLC}" || ! -x "${AR}" ]]; then
  echo "missing toolchain in ${TOOLBIN}" >&2
  exit 1
fi

mkdir -p "${OUT_DIR}"
rm -f "${OUT_DIR}"/*.o

tmpdir="$(mktemp -d)"
trap 'rm -rf "${tmpdir}"' EXIT

float_sources_o0=(
  fixsfsi
  fixunssfsi
  floatsisf
  floatunsisf
)

float_sources_o0_softfp=(
  fixsfdi
  fixunssfdi
)

float_sources_o1=(
)
float_sources_os=(
  fp_mode
)

int_sources_o0=(
  muldi3
)

int_sources_os=(
  ashldi3
  ashrdi3
  clzdi2
  cmpdi2
  ctzdi2
  ctzsi2
  lshrdi3
  negdi2
  ucmpdi2
)

for base in "${float_sources_o0[@]}"; do
  src="${BUILTINS_DIR}/${base}.c"
  if [[ ! -f "${src}" ]]; then
    echo "missing source: ${src}" >&2
    exit 1
  fi
  "${CLANG}" -target i8085-unknown-elf -O0 -ffreestanding -fno-builtin \
    -nostdlib -I "${LIBI8085_DIR}/include" -I "${BUILTINS_DIR}" \
    -emit-llvm -c "${src}" -o "${tmpdir}/${base}.bc"
  "${LLC}" -mtriple=i8085-unknown-elf -filetype=obj \
    "${tmpdir}/${base}.bc" -o "${OUT_DIR}/${base}.o"
done

for base in "${float_sources_o0_softfp[@]}"; do
  src="${BUILTINS_DIR}/${base}.c"
  if [[ ! -f "${src}" ]]; then
    echo "missing source: ${src}" >&2
    exit 1
  fi
  "${CLANG}" -target i8085-unknown-elf -O0 -ffreestanding -fno-builtin \
    -D__SOFTFP__ -nostdlib -I "${LIBI8085_DIR}/include" -I "${BUILTINS_DIR}" \
    -emit-llvm -c "${src}" -o "${tmpdir}/${base}.bc"
  "${LLC}" -mtriple=i8085-unknown-elf -filetype=obj \
    "${tmpdir}/${base}.bc" -o "${OUT_DIR}/${base}.o"
done

for base in "${float_sources_o1[@]}"; do
  src="${BUILTINS_DIR}/${base}.c"
  if [[ ! -f "${src}" ]]; then
    echo "missing source: ${src}" >&2
    exit 1
  fi
  "${CLANG}" -target i8085-unknown-elf -O0 -ffreestanding -fno-builtin \
    -nostdlib -I "${LIBI8085_DIR}/include" -I "${BUILTINS_DIR}" \
    -emit-llvm -c "${src}" -o "${tmpdir}/${base}.bc"
  "${LLC}" -mtriple=i8085-unknown-elf -filetype=obj \
    "${tmpdir}/${base}.bc" -o "${OUT_DIR}/${base}.o"
done

for base in "${float_sources_os[@]}"; do
  src="${BUILTINS_DIR}/${base}.c"
  if [[ ! -f "${src}" ]]; then
    echo "missing source: ${src}" >&2
    exit 1
  fi
  "${CLANG}" -target i8085-unknown-elf -Os -ffreestanding -fno-builtin \
    -nostdlib -I "${LIBI8085_DIR}/include" -I "${BUILTINS_DIR}" \
    -emit-llvm -c "${src}" -o "${tmpdir}/${base}.bc"
  "${LLC}" -mtriple=i8085-unknown-elf -filetype=obj \
    "${tmpdir}/${base}.bc" -o "${OUT_DIR}/${base}.o"
done

for base in "${int_sources_o0[@]}"; do
  src="${BUILTINS_DIR}/${base}.c"
  if [[ ! -f "${src}" ]]; then
    echo "missing source: ${src}" >&2
    exit 1
  fi
  "${CLANG}" -target i8085-unknown-elf -O0 -ffreestanding -fno-builtin \
    -nostdlib -I "${BUILTINS_DIR}" \
    -emit-llvm -c "${src}" -o "${tmpdir}/${base}.bc"
  "${LLC}" -mtriple=i8085-unknown-elf -filetype=obj \
    "${tmpdir}/${base}.bc" -o "${OUT_DIR}/${base}.o"
done

for base in "${int_sources_os[@]}"; do
  src="${BUILTINS_DIR}/${base}.c"
  if [[ ! -f "${src}" ]]; then
    echo "missing source: ${src}" >&2
    exit 1
  fi
  "${CLANG}" -target i8085-unknown-elf -Os -ffreestanding -fno-builtin \
    -nostdlib -I "${BUILTINS_DIR}" \
    -emit-llvm -c "${src}" -o "${tmpdir}/${base}.bc"
  "${LLC}" -mtriple=i8085-unknown-elf -filetype=obj \
    "${tmpdir}/${base}.bc" -o "${OUT_DIR}/${base}.o"
done

# Target-specific helpers.
if [[ -f "${LIBI8085_BUILTINS_DIR}/clzsi2.S" ]]; then
  "${CLANG}" -target i8085-unknown-elf -c "${LIBI8085_BUILTINS_DIR}/clzsi2.S" \
    -o "${OUT_DIR}/clzsi2.o"
else
  echo "missing source: ${LIBI8085_BUILTINS_DIR}/clzsi2.S" >&2
  exit 1
fi

# Target-specific helpers.
for helper in int_mul int_udiv int_sdiv int_divdi3; do
  src="${LIBI8085_BUILTINS_DIR}/${helper}.c"
  if [[ ! -f "${src}" ]]; then
    echo "missing source: ${src}" >&2
    exit 1
  fi
  # O0 avoids long-running llc compiles and keeps helpers simple.
  "${CLANG}" -target i8085-unknown-elf -O0 -ffreestanding -fno-builtin \
    -nostdlib -I "${BUILTINS_DIR}" -I "${LIBI8085_BUILTINS_DIR}" \
    -emit-llvm -c "${src}" -o "${tmpdir}/${helper}.bc"
  "${LLC}" -mtriple=i8085-unknown-elf -filetype=obj \
    "${tmpdir}/${helper}.bc" -o "${OUT_DIR}/${helper}.o"
done

# Local soft-float helpers built outside compiler-rt.
for helper in addsf3 subsf3 negsf2 mulsf3 divsf3; do
  src="${LIBI8085_BUILTINS_DIR}/${helper}.c"
  if [[ ! -f "${src}" ]]; then
    echo "missing source: ${src}" >&2
    exit 1
  fi
  "${CLANG}" -target i8085-unknown-elf -O0 -ffreestanding -fno-builtin \
    -nostdlib -I "${LIBI8085_BUILTINS_DIR}" -I "${BUILTINS_DIR}" \
    -emit-llvm -c "${src}" -o "${tmpdir}/${helper}.bc"
  "${LLC}" -mtriple=i8085-unknown-elf -filetype=obj \
    "${tmpdir}/${helper}.bc" -o "${OUT_DIR}/${helper}.o"
done

for helper in comparesf2 floatdisf floatundisf; do
  src="${LIBI8085_BUILTINS_DIR}/${helper}.c"
  if [[ ! -f "${src}" ]]; then
    echo "missing source: ${src}" >&2
    exit 1
  fi
  "${CLANG}" -target i8085-unknown-elf -O0 -ffreestanding -fno-builtin \
    -nostdlib -I "${LIBI8085_BUILTINS_DIR}" -I "${BUILTINS_DIR}" \
    -emit-llvm -c "${src}" -o "${tmpdir}/${helper}.bc"
  "${LLC}" -mtriple=i8085-unknown-elf -filetype=obj \
    "${tmpdir}/${helper}.bc" -o "${OUT_DIR}/${helper}.o"
done

rm -f "${SYSROOT}/lib/libgcc.a"
"${AR}" rcs "${SYSROOT}/lib/libgcc.a" "${OUT_DIR}"/*.o

echo "libgcc rebuilt into ${SYSROOT}/lib/libgcc.a"
