#!/usr/bin/env bash
# Benchmark suite for i8085 code generation
# Runs all examples at all optimization levels and collects metrics
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"

# Toolchain paths - prefer llvm-project build (most up-to-date with source)
TOOLCHAIN="${ROOT}/llvm-project/build-clang-8085/bin"

CLANG="${CLANG:-${TOOLCHAIN}/clang}"
LLD="${LLD:-${TOOLCHAIN}/ld.lld}"
OBJCOPY="${LLVM_OBJCOPY:-${TOOLCHAIN}/llvm-objcopy}"
SIZE="${LLVM_SIZE:-${TOOLCHAIN}/llvm-size}"
TRACE="${TRACE:-$ROOT/i8085-trace/build/i8085-trace}"

CRT="${CRT:-$ROOT/sysroot/crt/crt0.S}"
LIBGCC="${LIBGCC:-$ROOT/sysroot/lib/libgcc.a}"
LIBC="${LIBC:-$ROOT/sysroot/lib/libc.a}"
LINKER_DEFAULT="$ROOT/sysroot/ldscripts/i8085-32kram-32krom.ld"
LINKER_INPUT="$ROOT/sysroot/ldscripts/i8085-32kram-32krom-input.ld"
LINKER_INPUT48="$ROOT/sysroot/ldscripts/i8085-32kram-32krom-input48.ld"
LINKER_LARGE="$ROOT/sysroot/ldscripts/i8085-16kram-48krom.ld"

# Check dependencies
for tool in "${CLANG}" "${LLD}" "${OBJCOPY}" "${SIZE}" "${TRACE}"; do
  if [[ ! -x "${tool}" ]]; then
    echo "Error: missing tool ${tool}" >&2
    exit 1
  fi
done
for file in "${CRT}" "${LIBGCC}" "${LIBC}"; do
  if [[ ! -f "${file}" ]]; then
    echo "Error: missing file ${file}" >&2
    exit 1
  fi
done

# Benchmarks to run (can be overridden via args)
BENCHMARKS=("$@")
if [[ ${#BENCHMARKS[@]} -eq 0 ]]; then
  BENCHMARKS=(fib q7_8_matmul opt_sanity deep_recursion crc32 crc32_lut bubble_sort json_parse mul_torture div_torture bitops_torture string_torture float_torture fp_bench arith64_torture)
fi

# Optimization levels to test
OPT_LEVELS=(O0 O1 O2 Os)

# Benchmark-specific configuration
declare -A DUMP_RANGE
declare -A MAX_STEPS
declare -A LINKER_SCRIPT
declare -A EXPECTED_FILE

DUMP_RANGE[fib]="0x0100:32"
MAX_STEPS[fib]="10000"

DUMP_RANGE[q7_8_matmul]="0x0200:8"
MAX_STEPS[q7_8_matmul]="500000"

DUMP_RANGE[opt_sanity]="0x0200:4"
MAX_STEPS[opt_sanity]="2000000"

DUMP_RANGE[deep_recursion]="0x0400:4"
MAX_STEPS[deep_recursion]="500000"

DUMP_RANGE[crc32]="0x0200:4"
MAX_STEPS[crc32]="2000000"

DUMP_RANGE[crc32_lut]="0x0200:8"
MAX_STEPS[crc32_lut]="20000000"

DUMP_RANGE[bubble_sort]="0x0200:32"
MAX_STEPS[bubble_sort]="2000000"

DUMP_RANGE[json_parse]="0x0200:4"
MAX_STEPS[json_parse]="2000000"

DUMP_RANGE[mul_torture]="0x0200:130"
MAX_STEPS[mul_torture]="5000000"

DUMP_RANGE[div_torture]="0x0200:256"
MAX_STEPS[div_torture]="50000000"

DUMP_RANGE[bitops_torture]="0x0200:4"
MAX_STEPS[bitops_torture]="5000000"

DUMP_RANGE[string_torture]="0x0200:4"
MAX_STEPS[string_torture]="500000"

DUMP_RANGE[float_torture]="0x0200:4"
MAX_STEPS[float_torture]="5000000"

DUMP_RANGE[fp_bench]="0x0200:12"
MAX_STEPS[fp_bench]="5000000"

DUMP_RANGE[arith64_torture]="0x0200:4"
MAX_STEPS[arith64_torture]="50000000"

DUMP_RANGE[coremark]="0x0200:4"
MAX_STEPS[coremark]="100000000"

# Linker scripts
LINKER_SCRIPT[fib]="${LINKER_DEFAULT}"
LINKER_SCRIPT[q7_8_matmul]="${LINKER_INPUT}"
LINKER_SCRIPT[opt_sanity]="${LINKER_INPUT}"
LINKER_SCRIPT[deep_recursion]="${LINKER_DEFAULT}"
LINKER_SCRIPT[crc32]="${LINKER_INPUT}"
LINKER_SCRIPT[crc32_lut]="${LINKER_INPUT}"
LINKER_SCRIPT[bubble_sort]="${LINKER_INPUT}"
LINKER_SCRIPT[json_parse]="${LINKER_INPUT48}"
LINKER_SCRIPT[mul_torture]="${LINKER_LARGE}"
LINKER_SCRIPT[div_torture]="${LINKER_LARGE}"
LINKER_SCRIPT[bitops_torture]="${LINKER_LARGE}"
LINKER_SCRIPT[string_torture]="${LINKER_INPUT}"
LINKER_SCRIPT[float_torture]="${LINKER_DEFAULT}"
LINKER_SCRIPT[fp_bench]="${LINKER_DEFAULT}"
LINKER_SCRIPT[arith64_torture]="${LINKER_LARGE}"
LINKER_SCRIPT[coremark]="${LINKER_LARGE}"

# Expected output files
EXPECTED_FILE[fib]="$ROOT/tooling/examples/fib/expected.hex"
EXPECTED_FILE[q7_8_matmul]=""
EXPECTED_FILE[opt_sanity]=""
EXPECTED_FILE[deep_recursion]=""
EXPECTED_FILE[crc32]=""
EXPECTED_FILE[crc32_lut]="$ROOT/tooling/examples/crc32_lut/expected.hex"
EXPECTED_FILE[bubble_sort]=""
EXPECTED_FILE[json_parse]=""
EXPECTED_FILE[mul_torture]=""
EXPECTED_FILE[div_torture]=""
EXPECTED_FILE[bitops_torture]=""
EXPECTED_FILE[string_torture]=""
EXPECTED_FILE[float_torture]=""
EXPECTED_FILE[fp_bench]=""
EXPECTED_FILE[arith64_torture]=""
EXPECTED_FILE[coremark]=""

# Output format
OUTPUT_FORMAT="${OUTPUT_FORMAT:-table}"  # table or csv

# Storage for results
declare -a RESULTS

build_and_run() {
  local bench="$1"
  local opt="$2"

  local src="${ROOT}/tooling/examples/${bench}/${bench}.c"
  local outdir="${ROOT}/tooling/examples/${bench}/build/${opt}"
  local linker="${LINKER_OVERRIDE:-${LINKER_SCRIPT[$bench]:-$LINKER_DEFAULT}}"
  local max_steps="${MAX_STEPS[$bench]:-2000}"
  local dump_range="${DUMP_RANGE[$bench]:-}"

  mkdir -p "${outdir}"

  # Compile CRT
  "${CLANG}" --target=i8085-unknown-elf -ffreestanding -fno-builtin -${opt} \
    -c "${CRT}" -o "${outdir}/crt0.o" 2>/dev/null

  # Compile source
  "${CLANG}" --target=i8085-unknown-elf -ffreestanding -fno-builtin -${opt} \
    -c "${src}" -o "${outdir}/${bench}.o" 2>/dev/null

  # Check for extra input files
  local extra_obj=()
  local extra_src="${ROOT}/tooling/examples/${bench}/${bench}_inputs.c"
  if [[ -f "${extra_src}" ]]; then
    "${CLANG}" --target=i8085-unknown-elf -ffreestanding -fno-builtin -${opt} \
      -c "${extra_src}" -o "${outdir}/${bench}_inputs.o" 2>/dev/null
    extra_obj=("${outdir}/${bench}_inputs.o")
  fi

  # Link
  if ! "${LLD}" -m i8085elf --gc-sections -T "${linker}" -Map "${outdir}/${bench}.map" \
    -o "${outdir}/${bench}.elf" "${outdir}/crt0.o" "${outdir}/${bench}.o" "${extra_obj[@]}" "${LIBGCC}" "${LIBC}" "${LIBGCC}" 2>/dev/null; then
    echo "${bench},${opt},LINK_ERR,?,?,FAILED"
    return
  fi

  # Create binary
  if ! "${OBJCOPY}" -O binary "${outdir}/${bench}.elf" "${outdir}/${bench}.bin" 2>/dev/null; then
    echo "${bench},${opt},OBJCOPY_ERR,?,?,FAILED"
    return
  fi

  # Get .text size
  local text_size
  text_size="$("${SIZE}" -A "${outdir}/${bench}.elf" 2>/dev/null | awk '$1 == ".text" { print $2 }')"
  text_size="${text_size:-0}"

  # Run through simulator to get cycle count
  local trace_args=("-e" "0x0000" "-l" "0x0000" "-n" "${max_steps}" "-S" "-q")
  if [[ -n "${dump_range}" ]]; then
    trace_args+=("-d" "${dump_range}")
  fi

  local summary_file="${outdir}/${bench}.summary.json"
  local dump_file="${outdir}/${bench}.dump.txt"

  "${TRACE}" "${trace_args[@]}" "${outdir}/${bench}.bin" > "${summary_file}" 2> "${dump_file}" || true

  # Parse results from JSON
  # Format: {"pc":"...","sp":"...","f":"...","clk":N,"steps":N,"halt":"...","sod":N,"r":[...]}
  local status="UNKNOWN"
  local instructions="?"
  local clocks="?"

  if [[ -f "${summary_file}" && -s "${summary_file}" ]]; then
    # Extract clk (clocks), steps (instructions), halt (status)
    clocks=$(grep -o '"clk":[0-9]*' "${summary_file}" 2>/dev/null | cut -d: -f2 || echo "?")
    instructions=$(grep -o '"steps":[0-9]*' "${summary_file}" 2>/dev/null | cut -d: -f2 || echo "?")
    status=$(grep -o '"halt":"[^"]*"' "${summary_file}" 2>/dev/null | cut -d'"' -f4 || echo "UNKNOWN")
  fi

  # Normalize status
  case "${status}" in
    hlt) status="HALTED" ;;
    max) status="TIMEOUT" ;;
    loop) status="LOOP" ;;
    "") status="UNKNOWN" ;;
    *) status="${status}" ;;
  esac

  echo "${bench},${opt},${text_size},${instructions},${clocks},${status}"
}

# Print table header
print_header() {
  if [[ "${OUTPUT_FORMAT}" == "csv" ]]; then
    echo "benchmark,opt,text_bytes,instructions,clocks,status"
  else
    printf "\n"
    printf "┌─────────────────┬─────┬────────────┬──────────────┬─────────────┬──────────┐\n"
    printf "│ %-15s │ %-3s │ %10s │ %12s │ %11s │ %-8s │\n" "Benchmark" "Opt" "Text(bytes)" "Instructions" "Clocks" "Status"
    printf "├─────────────────┼─────┼────────────┼──────────────┼─────────────┼──────────┤\n"
  fi
}

print_row() {
  local line="$1"
  IFS=',' read -r bench opt text insns clocks status <<< "${line}"

  if [[ "${OUTPUT_FORMAT}" == "csv" ]]; then
    echo "${line}"
  else
    printf "│ %-15s │ %-3s │ %10s │ %12s │ %11s │ %-8s │\n" "${bench}" "${opt}" "${text}" "${insns}" "${clocks}" "${status}"
  fi
}

print_separator() {
  if [[ "${OUTPUT_FORMAT}" != "csv" ]]; then
    printf "├─────────────────┼─────┼────────────┼──────────────┼─────────────┼──────────┤\n"
  fi
}

print_footer() {
  if [[ "${OUTPUT_FORMAT}" != "csv" ]]; then
    printf "└─────────────────┴─────┴────────────┴──────────────┴─────────────┴──────────┘\n"
  fi
}

# Get version info
COMMIT_HASH=$(cd "${ROOT}/llvm-project" && git rev-parse --short HEAD 2>/dev/null || echo "unknown")
TIMESTAMP=$(date "+%Y-%m-%d %H:%M:%S")

# Main
echo "i8085 Benchmark Suite"
echo "====================="
echo ""
echo "Date: ${TIMESTAMP}"
echo "Commit: ${COMMIT_HASH}"
echo "Toolchain: ${CLANG}"
echo "Benchmarks: ${BENCHMARKS[*]}"
echo "Opt levels: ${OPT_LEVELS[*]}"
echo ""
echo "Building and running benchmarks..."
echo ""

print_header

first_bench=true
for bench in "${BENCHMARKS[@]}"; do
  src="${ROOT}/tooling/examples/${bench}/${bench}.c"
  if [[ ! -f "${src}" ]]; then
    echo "Warning: skipping ${bench} (source not found)" >&2
    continue
  fi

  if [[ "${first_bench}" != "true" ]]; then
    print_separator
  fi
  first_bench=false

  for opt in "${OPT_LEVELS[@]}"; do
    result=$(build_and_run "${bench}" "${opt}")
    RESULTS+=("${result}")
    print_row "${result}"
  done
done

print_footer

# Check for any failures
failures=0
for result in "${RESULTS[@]}"; do
  status="${result##*,}"
  if [[ "${status}" != "HALTED" ]]; then
    failures=$((failures + 1))
  fi
done

echo ""
if [[ ${failures} -eq 0 ]]; then
  echo "All benchmarks completed successfully."
else
  echo "${failures} benchmark(s) did not complete (see Status column)."
fi
