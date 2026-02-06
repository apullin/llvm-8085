#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
EXAMPLE="${1:-}"

if [[ -z "${EXAMPLE}" ]]; then
  echo "Usage: $0 <example>"
  echo "Examples: fib, q7_8_matmul, abi_torture, opt_sanity, deep_recursion, crc32, crc32_lut, bubble_sort, json_parse, mul_torture, bitops_torture"
  exit 1
fi

CLANG="${CLANG:-$ROOT/llvm-project/build-clang-8085/bin/clang}"
LLD="${LLD:-$ROOT/llvm-project/build-clang-8085/bin/ld.lld}"
OBJCOPY_DEFAULT="$ROOT/llvm-project/build-clang-8085/bin/llvm-objcopy"
OBJCOPY="${LLVM_OBJCOPY:-${OBJCOPY_DEFAULT}}"
TRACE="${TRACE:-$ROOT/i8085-trace/build/i8085-trace}"
CRT="${CRT:-$ROOT/sysroot/crt/crt0.S}"
LINKER_DEFAULT="$ROOT/sysroot/ldscripts/i8085-32kram-32krom.ld"
LINKER_INPUT="$ROOT/sysroot/ldscripts/i8085-32kram-32krom-input.ld"
LINKER_INPUT48="$ROOT/sysroot/ldscripts/i8085-32kram-32krom-input48.ld"
LINKER_LARGE="$ROOT/sysroot/ldscripts/i8085-16kram-48krom.ld"
LINKER="${LINKER:-}"
LIBGCC="${LIBGCC:-$ROOT/sysroot/lib/libgcc.a}"
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
if [[ ! -f "${LIBGCC}" ]]; then
  echo "Missing libgcc at ${LIBGCC}"
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
  abi_torture)
    DUMP_RANGE="0x0300:96"
    MAX_STEPS="200000"
    ;;
  opt_sanity)
    DUMP_RANGE="0x0200:4"
    MAX_STEPS="2000000"
    ;;
  deep_recursion)
    DUMP_RANGE="0x0400:4"
    MAX_STEPS="500000"
    ;;
  crc32)
    DUMP_RANGE="0x0200:4"
    MAX_STEPS="2000000"
    ;;
  crc32_lut)
    DUMP_RANGE="0x0200:8"
    MAX_STEPS="20000000"
    ;;
  bubble_sort)
    DUMP_RANGE="0x0200:32"
    MAX_STEPS="2000000"
    ;;
  json_parse)
    DUMP_RANGE="0x0200:4"
    MAX_STEPS="2000000"
    ;;
  mul_torture)
    DUMP_RANGE="0x0200:130"
    MAX_STEPS="5000000"
    ;;
  div_torture)
    DUMP_RANGE="0x0200:256"
    MAX_STEPS="50000000"
    ;;
  bitops_torture)
    DUMP_RANGE="0x0200:4"
    MAX_STEPS="5000000"
    ;;
  *)
    echo "Unknown example: ${EXAMPLE}"
    exit 1
    ;;
esac

if [[ -z "${LINKER}" ]]; then
  case "${EXAMPLE}" in
    abi_torture|mul_torture|div_torture|bitops_torture)
      if [[ -f "${LINKER_LARGE}" ]]; then
        LINKER="${LINKER_LARGE}"
      else
        LINKER="${LINKER_DEFAULT}"
      fi
      ;;
    q7_8_matmul|crc32|crc32_lut|bubble_sort|opt_sanity)
      LINKER="${LINKER_INPUT}"
      ;;
    json_parse)
      LINKER="${LINKER_INPUT48}"
      ;;
    *)
      LINKER="${LINKER_DEFAULT}"
      ;;
  esac
fi

mkdir -p "${OUTDIR}"

"${CLANG}" --target=i8085-unknown-elf -ffreestanding -fno-builtin -Oz \
  -c "${CRT}" -o "${OUTDIR}/crt0.o"

OPTFLAGS="${OPTFLAGS:-}"
if [[ -z "${OPTFLAGS}" ]]; then
  if [[ "${EXAMPLE}" == "opt_sanity" ]]; then
    OPTFLAGS="-O2"
  else
    # Use -Os by default. -Oz has known issues with stack frame calculation
    # for functions with complex stack-passed arguments.
    OPTFLAGS="-Os"
  fi
fi

"${CLANG}" --target=i8085-unknown-elf -ffreestanding -fno-builtin "${OPTFLAGS}" \
  -c "${SRC}" -o "${OUTDIR}/${EXAMPLE}.o"

# Check for extra input files
EXTRA_OBJ=()
EXTRA_SRC="${ROOT}/tooling/examples/${EXAMPLE}/${EXAMPLE}_inputs.c"
if [[ -f "${EXTRA_SRC}" ]]; then
  "${CLANG}" --target=i8085-unknown-elf -ffreestanding -fno-builtin "${OPTFLAGS}" \
    -c "${EXTRA_SRC}" -o "${OUTDIR}/${EXAMPLE}_inputs.o"
  EXTRA_OBJ=("${OUTDIR}/${EXAMPLE}_inputs.o")
fi

LIBC="${LIBC:-$ROOT/sysroot/lib/libc.a}"

"${LLD}" -m i8085elf --gc-sections -T "${LINKER}" -Map "${OUTDIR}/${EXAMPLE}.map" \
  -o "${OUTDIR}/${EXAMPLE}.elf" "${OUTDIR}/crt0.o" "${OUTDIR}/${EXAMPLE}.o" "${EXTRA_OBJ[@]}" "${LIBGCC}" "${LIBC}" "${LIBGCC}"

"${OBJCOPY}" -O binary "${OUTDIR}/${EXAMPLE}.elf" "${OUTDIR}/${EXAMPLE}.bin"

MAX_STEPS="${MAX_STEPS:-2000}"
TRACE_ARGS=("-e" "0x0000" "-l" "0x0000" "-n" "${MAX_STEPS}")
if [[ -n "${DUMP_RANGE}" ]]; then
  TRACE_ARGS+=("-d" "${DUMP_RANGE}")
fi
if [[ "${EXAMPLE}" == "abi_torture" || "${EXAMPLE}" == "opt_sanity" || "${EXAMPLE}" == "deep_recursion" || "${EXAMPLE}" == "crc32" || "${EXAMPLE}" == "crc32_lut" || "${EXAMPLE}" == "bubble_sort" || "${EXAMPLE}" == "json_parse" || "${EXAMPLE}" == "mul_torture" || "${EXAMPLE}" == "div_torture" || "${EXAMPLE}" == "bitops_torture" ]]; then
  TRACE_ARGS+=("-S" "-q")
fi
if [[ -n "${COV:-}" ]]; then
  TRACE_ARGS+=("--cov" "${OUTDIR}/${EXAMPLE}.cov.json")
fi

DUMP_LOG="${OUTDIR}/${EXAMPLE}.dump.txt"
SUMMARY_LOG=""
if [[ "${EXAMPLE}" == "abi_torture" || "${EXAMPLE}" == "opt_sanity" || "${EXAMPLE}" == "deep_recursion" || "${EXAMPLE}" == "crc32" || "${EXAMPLE}" == "crc32_lut" || "${EXAMPLE}" == "bubble_sort" || "${EXAMPLE}" == "json_parse" || "${EXAMPLE}" == "mul_torture" || "${EXAMPLE}" == "div_torture" || "${EXAMPLE}" == "bitops_torture" ]]; then
  SUMMARY_LOG="${OUTDIR}/${EXAMPLE}.summary.json"
fi

if [[ -n "${SUMMARY_LOG}" ]]; then
  "${TRACE}" "${TRACE_ARGS[@]}" "${OUTDIR}/${EXAMPLE}.bin" > "${SUMMARY_LOG}" 2> "${DUMP_LOG}"
else
  "${TRACE}" "${TRACE_ARGS[@]}" "${OUTDIR}/${EXAMPLE}.bin" 2> "${DUMP_LOG}"
fi
cat "${DUMP_LOG}"

EXPECTED="${ROOT}/tooling/examples/${EXAMPLE}/expected.hex"
if [[ -f "${EXPECTED}" ]]; then
  "${ROOT}/tooling/examples/verify_dump.py" --dump "${DUMP_LOG}" --expected "${EXPECTED}"
fi

if [[ "${EXAMPLE}" == "opt_sanity" || "${EXAMPLE}" == "deep_recursion" || "${EXAMPLE}" == "crc32" || "${EXAMPLE}" == "crc32_lut" || "${EXAMPLE}" == "bubble_sort" || "${EXAMPLE}" == "json_parse" || "${EXAMPLE}" == "mul_torture" || "${EXAMPLE}" == "div_torture" || "${EXAMPLE}" == "bitops_torture" ]]; then
  "${ROOT}/tooling/examples/verify_summary.py" --summary "${SUMMARY_LOG}" --expect-halt hlt
fi
