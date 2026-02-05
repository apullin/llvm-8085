#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"

EXAMPLES=("$@")
if [[ ${#EXAMPLES[@]} -eq 0 ]]; then
  EXAMPLES=(fib q7_8_matmul opt_sanity deep_recursion)
fi

pick_tool() {
  local primary="$1"
  local fallback="$2"
  if [[ -x "${primary}" ]]; then
    echo "${primary}"
  else
    echo "${fallback}"
  fi
}

CLANG_DEFAULT="$ROOT/tooling/build/build-clang-8085/bin/clang"
CLANG_FALLBACK="$ROOT/llvm-project/build-clang-8085/bin/clang"
LLD_DEFAULT="$ROOT/tooling/build/build-clang-8085/bin/ld.lld"
LLD_FALLBACK="$ROOT/llvm-project/build-clang-8085/bin/ld.lld"
SIZE_DEFAULT="$ROOT/tooling/build/build-clang-8085/bin/llvm-size"
SIZE_FALLBACK="$ROOT/llvm-project/build-clang-8085/bin/llvm-size"
OBJDUMP_DEFAULT="$ROOT/tooling/build/build-clang-8085/bin/llvm-objdump"
OBJDUMP_FALLBACK="$ROOT/llvm-project/build-clang-8085/bin/llvm-objdump"

CLANG="${CLANG:-$(pick_tool "${CLANG_DEFAULT}" "${CLANG_FALLBACK}")}"
LLD="${LLD:-$(pick_tool "${LLD_DEFAULT}" "${LLD_FALLBACK}")}"
SIZE="${LLVM_SIZE:-$(pick_tool "${SIZE_DEFAULT}" "${SIZE_FALLBACK}")}"
OBJDUMP="${LLVM_OBJDUMP:-$(pick_tool "${OBJDUMP_DEFAULT}" "${OBJDUMP_FALLBACK}")}"

CRT="${CRT:-$ROOT/sysroot/crt/crt0.S}"
LIBGCC="${LIBGCC:-$ROOT/sysroot/lib/libgcc.a}"
LINKER_DEFAULT="$ROOT/sysroot/ldscripts/i8085-32kram-32krom.ld"
LINKER_Q78="$ROOT/sysroot/ldscripts/i8085-32kram-32krom-input.ld"
LINKER_LARGE="$ROOT/sysroot/ldscripts/i8085-16kram-48krom.ld"

if [[ ! -x "${CLANG}" || ! -x "${LLD}" || ! -x "${SIZE}" || ! -x "${OBJDUMP}" ]]; then
  echo "missing toolchain binaries; set CLANG/LLD/LLVM_SIZE/LLVM_OBJDUMP" >&2
  exit 1
fi
if [[ ! -f "${CRT}" || ! -f "${LIBGCC}" ]]; then
  echo "missing sysroot crt/libgcc; rebuild with tooling/build-libgcc.sh" >&2
  exit 1
fi

OPTS=(O1 O2 Os)

printf "example,opt,text,data,bss,insns\n"

for ex in "${EXAMPLES[@]}"; do
  src="${ROOT}/tooling/examples/${ex}/${ex}.c"
  if [[ ! -f "${src}" ]]; then
    echo "missing source ${src}" >&2
    exit 1
  fi

  linker="${LINKER_DEFAULT}"
  if [[ "${ex}" == "q7_8_matmul" && -f "${LINKER_Q78}" ]]; then
    linker="${LINKER_Q78}"
  elif [[ "${ex}" == "abi_torture" && -f "${LINKER_LARGE}" ]]; then
    linker="${LINKER_LARGE}"
  fi

  for opt in "${OPTS[@]}"; do
    outdir="${ROOT}/tooling/examples/${ex}/build/${opt}"
    mkdir -p "${outdir}"

    "${CLANG}" --target=i8085-unknown-elf -ffreestanding -fno-builtin -${opt} \
      -c "${CRT}" -o "${outdir}/crt0.o"

    "${CLANG}" --target=i8085-unknown-elf -ffreestanding -fno-builtin -${opt} \
      -c "${src}" -o "${outdir}/${ex}.o"

    extra_obj=()
    extra_src="${ROOT}/tooling/examples/${ex}/${ex}_inputs.c"
    if [[ -f "${extra_src}" ]]; then
      "${CLANG}" --target=i8085-unknown-elf -ffreestanding -fno-builtin -${opt} \
        -c "${extra_src}" -o "${outdir}/${ex}_inputs.o"
      extra_obj=("${outdir}/${ex}_inputs.o")
    fi

    "${LLD}" -m i8085elf --gc-sections -T "${linker}" -Map "${outdir}/${ex}.map" \
      -o "${outdir}/${ex}.elf" "${outdir}/crt0.o" "${outdir}/${ex}.o" "${extra_obj[@]}" "${LIBGCC}"

    size_line="$("${SIZE}" -A "${outdir}/${ex}.elf" | awk '
      $1 == ".text" { text = $2 }
      $1 == ".data" { data = $2 }
      $1 == ".bss"  { bss = $2 }
      END { printf "%d,%d,%d", text+0, data+0, bss+0 }
    ')"

    insns="$("${OBJDUMP}" -d --no-show-raw-insn "${outdir}/${ex}.elf" \
      | awk '/^[[:space:]]*[0-9a-fA-F]+:/{count++} END{print count+0}')"

    printf "%s,%s,%s,%s\n" "${ex}" "${opt}" "${size_line}" "${insns}"
  done
done
