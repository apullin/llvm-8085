#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
EXAMPLE="${1:-}"

if [[ -z "${EXAMPLE}" ]]; then
  echo "Usage: $0 <example>"
  echo "Examples: fib, q7_8_matmul"
  exit 1
fi

CLANG="${CLANG:-$ROOT/llvm-project/build-clang-8085/bin/clang}"
LLD="${LLD:-$ROOT/llvm-project/build-clang-8085/bin/ld.lld}"
OBJCOPY_DEFAULT="$ROOT/llvm-project/build-clang-8085/bin/llvm-objcopy"
OBJCOPY="${LLVM_OBJCOPY:-${OBJCOPY_DEFAULT}}"
TRACE="${TRACE:-$ROOT/i8085-trace/build/i8085-trace}"
CRT="${CRT:-$ROOT/sysroot/crt/crt0.S}"
LINKER="${LINKER:-$ROOT/sysroot/ldscripts/i8085-32kram-32krom.ld}"
SRC="${ROOT}/tooling/examples/${EXAMPLE}/${EXAMPLE}.c"
OUTDIR="${ROOT}/tooling/examples/${EXAMPLE}/build"

if [[ ! -x "${CLANG}" ]]; then
  echo "Missing clang at ${CLANG}"
  exit 1
fi
if [[ ! -x "${LLD}" ]]; then
  echo "Missing ld.lld at ${LLD}"
  exit 1
fi
if [[ ! -x "${OBJCOPY}" ]]; then
  echo "Missing llvm-objcopy at ${OBJCOPY}"
  exit 1
fi
if [[ ! -x "${TRACE}" ]]; then
  echo "Missing i8085-trace at ${TRACE}"
  exit 1
fi
if [[ ! -f "${SRC}" ]]; then
  echo "Missing source ${SRC}"
  exit 1
fi

case "${EXAMPLE}" in
  fib)
    DUMP_RANGE="0x0100:32"
    ;;
  q7_8_matmul)
    DUMP_RANGE="0x0200:8"
    ;;
  *)
    echo "Unknown example: ${EXAMPLE}"
    exit 1
    ;;
esac

mkdir -p "${OUTDIR}"

"${CLANG}" --target=i8085-unknown-elf -ffreestanding -fno-builtin -Oz \
  -c "${CRT}" -o "${OUTDIR}/crt0.o"

"${CLANG}" --target=i8085-unknown-elf -ffreestanding -fno-builtin -Oz \
  -c "${SRC}" -o "${OUTDIR}/${EXAMPLE}.o"

"${LLD}" -m i8085elf -T "${LINKER}" -Map "${OUTDIR}/${EXAMPLE}.map" \
  -o "${OUTDIR}/${EXAMPLE}.elf" "${OUTDIR}/crt0.o" "${OUTDIR}/${EXAMPLE}.o"

"${OBJCOPY}" -O binary "${OUTDIR}/${EXAMPLE}.elf" "${OUTDIR}/${EXAMPLE}.bin"

"${TRACE}" -e 0x0000 -l 0x0000 -n 2000 -d "${DUMP_RANGE}" "${OUTDIR}/${EXAMPLE}.bin"
