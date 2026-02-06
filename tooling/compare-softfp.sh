#!/usr/bin/env bash
# Compare hand-written ASM soft-float vs compiler-rt C soft-float.
# Builds two libgcc variants and runs fp_bench + float_torture with each.
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
TOOLBIN="${TOOLBIN:-$ROOT/llvm-project/build-clang-8085/bin}"
SYSROOT="${SYSROOT:-$ROOT/sysroot}"

CLANG="${TOOLBIN}/clang"
LLC="${TOOLBIN}/llc"
AR="${TOOLBIN}/llvm-ar"
SIZE="${TOOLBIN}/llvm-size"

BUILTINS_DIR="${ROOT}/llvm-project/compiler-rt/lib/builtins"
LIBI8085_DIR="${SYSROOT}/libi8085"

# C soft-float needs >32K ROM; use 48K ROM linker for fair comparison.
LINKER_48K="${SYSROOT}/ldscripts/i8085-16kram-48krom.ld"

tmpdir="$(mktemp -d)"
trap 'rm -rf "${tmpdir}"' EXIT

echo "========================================"
echo " ASM vs C Soft-Float Comparison"
echo "========================================"
echo ""

# ---- Step 1: Build current (ASM) libgcc ----
echo "--- Building ASM libgcc (current) ---"
bash "${ROOT}/tooling/build-libgcc.sh" > /dev/null 2>&1
cp "${SYSROOT}/lib/libgcc.a" "${tmpdir}/libgcc_asm.a"

# ---- Step 2: Build C soft-float objects ----
echo "--- Compiling C soft-float from compiler-rt ---"
mkdir -p "${tmpdir}/c_objs"

c_float_sources=(
  addsf3 subsf3 negsf2 mulsf3 divsf3 comparesf2
  fixsfsi fixunssfsi floatsisf floatunsisf
  fp_mode
)

for base in "${c_float_sources[@]}"; do
  src="${BUILTINS_DIR}/${base}.c"
  # Use -Os for fairest comparison (smallest C code).
  # -O0 produces even larger code. -O2 is similar to -Os.
  "${CLANG}" -target i8085-unknown-elf -Os -ffreestanding -fno-builtin \
    -nostdlib -I "${LIBI8085_DIR}/include" -I "${BUILTINS_DIR}" \
    -emit-llvm -c "${src}" -o "${tmpdir}/c_objs/${base}.bc"
  "${LLC}" -mtriple=i8085-unknown-elf -filetype=obj \
    "${tmpdir}/c_objs/${base}.bc" -o "${tmpdir}/c_objs/${base}.o"
done

# ---- Step 3: Create alternate libgcc with C soft-float ----
echo "--- Building C libgcc (replacing softfp.o) ---"
cp "${tmpdir}/libgcc_asm.a" "${tmpdir}/libgcc_c.a"
"${AR}" d "${tmpdir}/libgcc_c.a" softfp.o 2>/dev/null || true
"${AR}" rcs "${tmpdir}/libgcc_c.a" "${tmpdir}/c_objs"/*.o

# ---- Step 4: Code size comparison ----
echo ""
echo "=== Code Size ==="
echo ""
printf "  %-18s %8s\n" "Object" "Text(bytes)"
printf "  %-18s %8s\n" "------------------" "-----------"

asm_size=$("${SIZE}" -t "${SYSROOT}/lib/builtins/softfp.o" 2>/dev/null | tail -1 | awk '{print $1}')
printf "  %-18s %8d  (hand-written ASM)\n" "softfp.o" "${asm_size}"

c_total=0
for base in "${c_float_sources[@]}"; do
  sz=$("${SIZE}" "${tmpdir}/c_objs/${base}.o" 2>/dev/null | tail -1 | awk '{print $1}')
  c_total=$((c_total + sz))
  printf "  %-18s %8d\n" "${base}.o" "${sz}"
done
printf "  %-18s %8s\n" "------------------" "-----------"
printf "  %-18s %8d  (compiler-rt C)\n" "C total" "${c_total}"
echo ""
printf "  ASM / C ratio: %.1fx smaller\n" "$(echo "scale=1; ${c_total} / ${asm_size}" | bc)"
echo ""

# ---- Step 5: Run benchmarks ----
echo "=== Performance (cycles @ -O1) ==="
echo ""

benchmarks=(float_torture fp_bench)
# Note: fp_bench with C soft-float may exceed ROM. This is expected —
# the C implementations are so large they don't fit in typical 8085 memory.

# Collect results for both variants
declare -A results

for bench in "${benchmarks[@]}"; do
  for variant in asm c; do
    libgcc="${tmpdir}/libgcc_${variant}.a"
    # Use 48K ROM linker to fit C variant
    output=$(LIBGCC="${libgcc}" LINKER_OVERRIDE="${LINKER_48K}" \
      bash "${ROOT}/tooling/examples/benchmark.sh" "${bench}" 2>&1)
    row=$(echo "${output}" | grep "│ ${bench}" | grep "│ O1" | head -1)
    if [[ -n "${row}" ]]; then
      text=$(echo "${row}" | awk -F'│' '{print $4}' | tr -d ' ')
      instr=$(echo "${row}" | awk -F'│' '{print $5}' | tr -d ' ')
      clocks=$(echo "${row}" | awk -F'│' '{print $6}' | tr -d ' ')
      status=$(echo "${row}" | awk -F'│' '{print $7}' | tr -d ' ')
      results["${bench}_${variant}_text"]="${text}"
      results["${bench}_${variant}_instr"]="${instr}"
      results["${bench}_${variant}_clocks"]="${clocks}"
      results["${bench}_${variant}_status"]="${status}"
    else
      results["${bench}_${variant}_text"]="?"
      results["${bench}_${variant}_instr"]="?"
      results["${bench}_${variant}_clocks"]="?"
      results["${bench}_${variant}_status"]="FAILED"
    fi
  done
done

printf "  %-15s %-8s %12s %12s %12s  %s\n" "Benchmark" "Variant" "Text(bytes)" "Instructions" "Clocks" "Status"
printf "  %-15s %-8s %12s %12s %12s  %s\n" "---------------" "--------" "-----------" "------------" "------" "------"

for bench in "${benchmarks[@]}"; do
  for variant in asm c; do
    label=$(echo "${variant}" | tr '[:lower:]' '[:upper:]')
    printf "  %-15s %-8s %12s %12s %12s  %s\n" \
      "${bench}" "${label}" \
      "${results[${bench}_${variant}_text]}" \
      "${results[${bench}_${variant}_instr]}" \
      "${results[${bench}_${variant}_clocks]}" \
      "${results[${bench}_${variant}_status]}"
  done
  # Compute speedup
  asm_clk="${results[${bench}_asm_clocks]}"
  c_clk="${results[${bench}_c_clocks]}"
  if [[ "${asm_clk}" != "?" && "${c_clk}" != "?" && "${asm_clk}" -gt 0 ]]; then
    speedup=$(echo "scale=2; ${c_clk} / ${asm_clk}" | bc)
    printf "  %-15s %-8s  -> ASM is %sx faster in cycles\n" "" "" "${speedup}"
  fi
  echo ""
done

echo "========================================"
echo " Done"
echo "========================================"
