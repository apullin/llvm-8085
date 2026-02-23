#!/usr/bin/env bash
# Build and run all compiler-rt builtin unit tests on the i8085 simulator.
#
# Usage: bash run.sh [OPT_LEVEL]
#   OPT_LEVEL: O0, O1, O2, Os, Oz (default: Oz)
#
# Exit code: 0 if all tests pass, 1 if any fail.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$(cd "${SCRIPT_DIR}/../../.." && pwd)"

OPT="${1:-Oz}"

# Toolchain
CLANG="${ROOT}/llvm-project/build-clang-8085/bin/clang"
LLD="${ROOT}/llvm-project/build-clang-8085/bin/ld.lld"
OBJCOPY="${ROOT}/llvm-project/build-clang-8085/bin/llvm-objcopy"
SIZE="${ROOT}/llvm-project/build-clang-8085/bin/llvm-size"
TRACE="${ROOT}/i8085-trace/build/i8085-trace"

# Sysroot
CRT="${ROOT}/sysroot/crt/crt0.S"
LIBGCC="${ROOT}/sysroot/lib/libgcc.a"
LIBC="${ROOT}/sysroot/lib/libc.a"
LDSCRIPT="${ROOT}/sysroot/ldscripts/i8085-16kram-48krom.ld"

# Check dependencies
for tool in "${CLANG}" "${LLD}" "${OBJCOPY}" "${TRACE}"; do
    if [[ ! -x "${tool}" ]]; then
        echo "ERROR: missing tool ${tool}" >&2
        exit 1
    fi
done

# Tests to run
TESTS=(
    rt_test_mulsi3
    rt_test_divsi3
    rt_test_float_arith
    rt_test_float_conv
    rt_test_arith64
)

# Max simulator steps per test
declare -A MAX_STEPS
MAX_STEPS[rt_test_mulsi3]=5000000
MAX_STEPS[rt_test_divsi3]=50000000
MAX_STEPS[rt_test_float_arith]=20000000
MAX_STEPS[rt_test_float_conv]=20000000
MAX_STEPS[rt_test_arith64]=100000000

BUILDDIR="${SCRIPT_DIR}/build/${OPT}"
mkdir -p "${BUILDDIR}"

# Build CRT
echo "Building CRT (${OPT})..."
"${CLANG}" --target=i8085-unknown-elf -ffreestanding -fno-builtin "-${OPT}" \
    -c "${CRT}" -o "${BUILDDIR}/crt0.o" 2>/dev/null

total_pass=0
total_fail=0
total_tests=0
all_ok=true

printf "\n"
printf "compiler-rt Builtin Unit Tests for i8085\n"
printf "=========================================\n"
printf "Optimization: -%s\n\n" "${OPT}"
printf "%-25s %6s %6s %6s  %8s  %s\n" "Test" "Pass" "Fail" "Total" "Status" "Steps"
printf "%-25s %6s %6s %6s  %8s  %s\n" "----" "----" "----" "-----" "------" "-----"

for test in "${TESTS[@]}"; do
    src="${SCRIPT_DIR}/${test}.c"

    if [[ ! -f "${src}" ]]; then
        printf "%-25s %6s %6s %6s  %8s  %s\n" "${test}" "-" "-" "-" "MISSING" "-"
        all_ok=false
        continue
    fi

    # Compile
    if ! "${CLANG}" --target=i8085-unknown-elf -ffreestanding -fno-builtin "-${OPT}" \
        -I"${SCRIPT_DIR}" -c "${src}" -o "${BUILDDIR}/${test}.o" 2>/dev/null; then
        printf "%-25s %6s %6s %6s  %8s  %s\n" "${test}" "-" "-" "-" "CC_ERR" "-"
        all_ok=false
        continue
    fi

    # Link
    if ! "${LLD}" -m i8085elf --gc-sections -T "${LDSCRIPT}" \
        -o "${BUILDDIR}/${test}.elf" \
        "${BUILDDIR}/crt0.o" "${BUILDDIR}/${test}.o" \
        "${LIBGCC}" "${LIBC}" "${LIBGCC}" 2>/dev/null; then
        printf "%-25s %6s %6s %6s  %8s  %s\n" "${test}" "-" "-" "-" "LINK_ERR" "-"
        all_ok=false
        continue
    fi

    # Create binary
    if ! "${OBJCOPY}" -O binary "${BUILDDIR}/${test}.elf" "${BUILDDIR}/${test}.bin" 2>/dev/null; then
        printf "%-25s %6s %6s %6s  %8s  %s\n" "${test}" "-" "-" "-" "BIN_ERR" "-"
        all_ok=false
        continue
    fi

    # Run simulator
    max_steps="${MAX_STEPS[$test]:-10000000}"
    summary=$("${TRACE}" -e 0x0000 -l 0x0000 -n "${max_steps}" -S -q \
        -d "0x0200:8" "${BUILDDIR}/${test}.bin" 2>"${BUILDDIR}/${test}.dump.txt" || true)

    # Parse simulator output
    halt_status=$(echo "${summary}" | grep -o '"halt":"[^"]*"' | cut -d'"' -f4 || echo "unknown")
    steps=$(echo "${summary}" | grep -o '"steps":[0-9]*' | cut -d: -f2 || echo "?")

    # Parse dump output: 8 bytes at 0x0200 = pass(2), fail(2), total(2), first_fail(2)
    dump=$(cat "${BUILDDIR}/${test}.dump.txt" 2>/dev/null || echo "")

    # The dump is hex bytes; parse them
    # Format from i8085-trace: "0200: XX XX XX XX XX XX XX XX"
    pass_count=0
    fail_count=0
    test_count=0

    if [[ -n "${dump}" ]]; then
        # Extract hex bytes from the data line (starts with "  0200:")
        # Format: "  0200: 28 00 00 00 28 00 FF FF  |(...(...|"
        data_line=$(echo "${dump}" | grep '^ *0200:' || echo "")
        if [[ -n "${data_line}" ]]; then
            # Strip address prefix and ASCII trailer, keep only hex bytes
            hex_part=$(echo "${data_line}" | sed 's/^ *[0-9a-fA-F]*: //' | sed 's/ *|.*$//' | tr -s ' ')
            byte_array=(${hex_part})

            if [[ ${#byte_array[@]} -ge 6 ]]; then
                # Little-endian 16-bit: pass = byte[0] + byte[1]*256
                b0=$((16#${byte_array[0]}))
                b1=$((16#${byte_array[1]}))
                pass_count=$((b0 + b1 * 256))

                b2=$((16#${byte_array[2]}))
                b3=$((16#${byte_array[3]}))
                fail_count=$((b2 + b3 * 256))

                b4=$((16#${byte_array[4]}))
                b5=$((16#${byte_array[5]}))
                test_count=$((b4 + b5 * 256))
            fi
        fi
    fi

    # Determine status
    status="FAIL"
    if [[ "${halt_status}" == "hlt" ]]; then
        if [[ ${fail_count} -eq 0 && ${test_count} -gt 0 ]]; then
            status="PASS"
        elif [[ ${test_count} -eq 0 ]]; then
            status="NO_TESTS"
        else
            status="FAIL"
            all_ok=false
        fi
    elif [[ "${halt_status}" == "max" ]]; then
        status="TIMEOUT"
        all_ok=false
    else
        status="CRASH"
        all_ok=false
    fi

    printf "%-25s %6d %6d %6d  %8s  %s\n" "${test}" "${pass_count}" "${fail_count}" "${test_count}" "${status}" "${steps}"

    total_pass=$((total_pass + pass_count))
    total_fail=$((total_fail + fail_count))
    total_tests=$((total_tests + test_count))
done

printf "%-25s %6s %6s %6s  %8s  %s\n" "----" "----" "----" "-----" "------" "-----"
printf "%-25s %6d %6d %6d\n" "TOTAL" "${total_pass}" "${total_fail}" "${total_tests}"
printf "\n"

if ${all_ok}; then
    echo "All tests PASSED (${total_pass}/${total_tests})"
    exit 0
else
    echo "FAILURES detected (${total_fail} failures out of ${total_tests} tests)"
    exit 1
fi
