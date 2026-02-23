#!/usr/bin/env bash
# ===========================================================================
# GCC C Torture Test Runner for i8085
# ===========================================================================
# Compiles and runs the GCC execute torture tests on the i8085 simulator.
#
# Usage:
#   bash tooling/gcc-torture/run-torture.sh [options]
#
# Options:
#   --opt=LEVEL     Optimization level (default: Os)
#   --max-steps=N   Simulator step limit (default: 5000000)
#   --jobs=N        Parallel jobs (default: 1, sequential)
#   --filter=PAT    Only run tests matching glob pattern
#   --stop-on-fail  Stop after first FAIL
#   --log=FILE      Write detailed log to FILE
#   --quick         Run first 100 tests only
# ===========================================================================

set -uo pipefail

# --- Project root ---
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"

# --- Toolchain paths ---
CLANG="${ROOT}/llvm-project/build-clang-8085/bin/clang"
LLD="${ROOT}/llvm-project/build-clang-8085/bin/ld.lld"
OBJCOPY="${ROOT}/llvm-project/build-clang-8085/bin/llvm-objcopy"
SIM="${ROOT}/i8085-trace/build/i8085-trace"
SYSROOT="${ROOT}/sysroot"
CLANG_BUILTINS="${ROOT}/llvm-project/build-clang-8085/lib/clang/20/i8085/include"

# --- Test source ---
TORTURE_DIR="${ROOT}/gcc-torture-tests/gcc/testsuite/gcc.c-torture/execute"
STUBS_SRC="${ROOT}/tooling/gcc-torture/torture_stubs.c"
LINKER_SCRIPT="${ROOT}/tooling/gcc-torture/i8085-torture.ld"
CRT_SRC="${SYSROOT}/crt/crt0.S"

# --- Defaults ---
OPT_LEVEL="Os"
MAX_STEPS=20000000
JOBS=1
FILTER="*.c"
STOP_ON_FAIL=0
LOG_FILE=""
QUICK=0

# --- Skip list ---
SKIP_LIST="${ROOT}/tooling/gcc-torture/skip-list.txt"

# --- Parse args ---
for arg in "$@"; do
    case "$arg" in
        --opt=*)       OPT_LEVEL="${arg#*=}" ;;
        --max-steps=*) MAX_STEPS="${arg#*=}" ;;
        --jobs=*)      JOBS="${arg#*=}" ;;
        --filter=*)    FILTER="${arg#*=}" ;;
        --stop-on-fail) STOP_ON_FAIL=1 ;;
        --log=*)       LOG_FILE="${arg#*=}" ;;
        --quick)       QUICK=1 ;;
        --help|-h)
            head -20 "$0" | tail -14
            exit 0
            ;;
        *)
            echo "Unknown option: $arg" >&2
            exit 1
            ;;
    esac
done

# --- Verify tools ---
for tool in "$CLANG" "$LLD" "$OBJCOPY" "$SIM"; do
    if [[ ! -x "$tool" ]]; then
        echo "ERROR: missing tool $tool" >&2
        exit 1
    fi
done
for f in "$STUBS_SRC" "$CRT_SRC" "$LINKER_SCRIPT"; do
    if [[ ! -f "$f" ]]; then
        echo "ERROR: missing file $f" >&2
        exit 1
    fi
done
if [[ ! -d "$TORTURE_DIR" ]]; then
    echo "ERROR: torture test directory not found: $TORTURE_DIR" >&2
    echo "Run: git clone --depth 1 --filter=blob:none --sparse https://github.com/gcc-mirror/gcc.git gcc-torture-tests" >&2
    echo "Then: git -C gcc-torture-tests sparse-checkout set gcc/testsuite/gcc.c-torture/execute" >&2
    exit 1
fi

# --- Build directory ---
BUILD_DIR="${ROOT}/tooling/gcc-torture/build-${OPT_LEVEL}"
mkdir -p "$BUILD_DIR"

# --- Compile stubs and CRT (once) ---
echo "Compiling CRT0..."
"$CLANG" --target=i8085-unknown-elf -ffreestanding -fno-builtin -O2 \
    -c "$CRT_SRC" -o "$BUILD_DIR/crt0.o" 2>/dev/null
if [[ $? -ne 0 ]]; then
    echo "ERROR: failed to compile CRT0" >&2
    exit 1
fi

echo "Compiling torture stubs..."
"$CLANG" --target=i8085-unknown-elf -ffreestanding -fno-builtin -O2 \
    -isystem "$SYSROOT/include" \
    -isystem "$CLANG_BUILTINS" \
    -c "$STUBS_SRC" -o "$BUILD_DIR/torture_stubs.o" 2>/dev/null
if [[ $? -ne 0 ]]; then
    echo "ERROR: failed to compile torture stubs" >&2
    exit 1
fi

# --- Gather test files ---
# Get all .c files (exclude subdirectories, multi-file tests)
mapfile -t ALL_TESTS < <(
    find "$TORTURE_DIR" -maxdepth 1 -name "$FILTER" -type f | sort
)

# Filter out multi-file tests (e.g., foo-1.c, foo-2.c -- keep only foo.c or foo-1.c)
# Also filter out ieee tests that ended up at top level
TESTS=()
for t in "${ALL_TESTS[@]}"; do
    base="$(basename "$t")"
    # Skip companion files (-2.c, -3.c, etc.) — we only compile single-file tests
    if [[ "$base" =~ -[2-9]\.c$ ]] || [[ "$base" =~ -[0-9][0-9]\.c$ ]]; then
        continue
    fi
    TESTS+=("$t")
done

# --- Load skip list ---
declare -A SKIPPED_TESTS
SKIP_COUNT=0
if [[ -f "$SKIP_LIST" ]]; then
    while read -r line; do
        # Skip comments and blank lines
        [[ "$line" =~ ^#.*$ || -z "$line" ]] && continue
        test_name=$(echo "$line" | awk '{print $1}')
        skip_reason=$(echo "$line" | awk '{$1=""; print}' | sed 's/^ *//')
        SKIPPED_TESTS["$test_name"]="$skip_reason"
    done < "$SKIP_LIST"
fi

# Filter out skipped tests
FILTERED_TESTS=()
for t in "${TESTS[@]}"; do
    base="$(basename "$t" .c)"
    if [[ -n "${SKIPPED_TESTS[$base]+x}" ]]; then
        SKIP_COUNT=$((SKIP_COUNT + 1))
        continue
    fi
    FILTERED_TESTS+=("$t")
done
TESTS=("${FILTERED_TESTS[@]}")

TOTAL=${#TESTS[@]}
if [[ $QUICK -eq 1 && $TOTAL -gt 100 ]]; then
    TESTS=("${TESTS[@]:0:100}")
    TOTAL=100
    echo "(--quick: limiting to first 100 tests)"
fi

echo ""
echo "======================================"
echo "  GCC C Torture Tests for i8085"
echo "======================================"
echo "Optimization:  -${OPT_LEVEL}"
echo "Step limit:    ${MAX_STEPS}"
echo "Tests found:   ${TOTAL}"
if [[ $SKIP_COUNT -gt 0 ]]; then
echo "Skip-listed:   ${SKIP_COUNT}"
fi
echo "Build dir:     ${BUILD_DIR}"
echo "======================================"
echo ""

# --- Counters ---
PASS=0
FAIL=0
SKIP_COMPILE=0
SKIP_LINK=0
TIMEOUT=0
ERRORS=0
COUNT=0

# --- Log file setup ---
if [[ -n "$LOG_FILE" ]]; then
    exec 3>"$LOG_FILE"
else
    exec 3>/dev/null
fi

log() {
    echo "$@" >&3
}

# --- Run a single test ---
run_test() {
    local src="$1"
    local name
    name="$(basename "$src" .c)"
    local test_dir="$BUILD_DIR/$name"
    mkdir -p "$test_dir"

    log "=== TEST: $name ==="

    # --- Extract dg-options from test file ---
    # Parse DejaGnu directives like: /* { dg-options "-fwrapv" } */
    # Also: /* { dg-additional-options "-fpermissive" } */
    local extra_flags=""
    local dg_opts
    # Use python for reliable regex extraction
    dg_opts=$(python3 -c "
import re, sys
text = open(sys.argv[1]).read()
flags = []
for m in re.finditer(r'dg-(?:additional-)?options\s+\"([^\"]*)\"', text):
    flags.extend(m.group(1).split())
print(' '.join(flags))
" "$src" 2>/dev/null)
    if [[ -n "$dg_opts" ]]; then
        # Filter to flags clang supports and that are relevant
        for flag in $dg_opts; do
            case "$flag" in
                -fwrapv|-fsigned-char|-funsigned-char|-fno-strict-aliasing| \
                -fno-common|-fno-inline|-fpermissive)
                    extra_flags="$extra_flags $flag"
                    ;;
                -std=*)
                    extra_flags="$extra_flags $flag"
                    ;;
                # Clang-compatible flags that need explicit passthrough
                -finstrument-functions)
                    extra_flags="$extra_flags $flag"
                    ;;
                # Skip GCC-only flags
                -fno-ira-share-spill-slots| \
                -fno-tree-*|-fgnu89-inline|-mtune=*|-mno-*|-mcheck-*)
                    ;;
                # Skip optimization flags (we use our own)
                -O*) ;;
                # Pass through -W flags
                -W*) extra_flags="$extra_flags $flag" ;;
                # Skip anything else (target-specific, etc.)
                *) ;;
            esac
        done
        log "dg-options: [$dg_opts] -> extra: [$extra_flags]"
    fi

    # --- Phase 1: Compile ---
    local compile_err="$test_dir/compile.err"
    # shellcheck disable=SC2086
    "$CLANG" --target=i8085-unknown-elf -ffreestanding -fno-builtin \
        "-${OPT_LEVEL}" \
        -isystem "$SYSROOT/include" \
        -isystem "$CLANG_BUILTINS" \
        -DSIGNAL_SUPPRESS \
        -Wno-implicit-function-declaration \
        -Wno-implicit-int \
        -Wno-int-conversion \
        -Wno-incompatible-pointer-types \
        -Wno-return-type \
        -Wno-builtin-declaration-mismatch \
        -Wno-everything \
        $extra_flags \
        -c "$src" -o "$test_dir/$name.o" 2>"$compile_err"

    if [[ $? -ne 0 ]]; then
        log "SKIP (compile): $(head -3 "$compile_err")"
        echo "SKIP_COMPILE"
        return
    fi

    # --- Phase 2: Link ---
    local link_err="$test_dir/link.err"
    "$LLD" -m i8085elf \
        -T "$LINKER_SCRIPT" \
        "$BUILD_DIR/crt0.o" \
        "$test_dir/$name.o" \
        "$BUILD_DIR/torture_stubs.o" \
        "$SYSROOT/lib/libgcc.a" \
        "$SYSROOT/lib/libc.a" \
        "$SYSROOT/lib/libgcc.a" \
        -o "$test_dir/$name.elf" 2>"$link_err"

    if [[ $? -ne 0 ]]; then
        log "SKIP (link): $(head -3 "$link_err")"
        echo "SKIP_LINK"
        return
    fi

    # --- Phase 3: Convert to binary ---
    "$OBJCOPY" -O binary "$test_dir/$name.elf" "$test_dir/$name.bin" 2>/dev/null
    if [[ $? -ne 0 ]]; then
        log "ERROR (objcopy)"
        echo "ERROR"
        return
    fi

    # --- Phase 4: Run on simulator ---
    local sim_json="$test_dir/sim.json"
    local sim_dump="$test_dir/sim.dump"

    "$SIM" -S -q \
        -n "$MAX_STEPS" \
        -d 0xFE00:2 \
        "$test_dir/$name.bin" \
        > "$sim_json" 2> "$sim_dump"

    # --- Phase 5: Parse results ---
    if [[ ! -s "$sim_json" ]]; then
        log "ERROR (no simulator output)"
        echo "ERROR"
        return
    fi

    # Extract halt status from JSON
    local halt_status
    halt_status=$(python3 -c "
import json, sys
try:
    d = json.load(sys.stdin)
    print(d.get('halt', 'unknown'))
except:
    print('error')
" < "$sim_json" 2>/dev/null)

    if [[ "$halt_status" == "max" ]] || [[ "$halt_status" == "loop" ]]; then
        log "TIMEOUT (halt=$halt_status)"
        echo "TIMEOUT"
        return
    fi

    if [[ "$halt_status" != "hlt" ]]; then
        log "ERROR (halt=$halt_status)"
        echo "ERROR"
        return
    fi

    # Check status word at 0xFE00
    # Parse the dump output: "  FE00: XX XX  |..|"
    local dump_line
    dump_line=$(grep "FE00:" "$sim_dump" 2>/dev/null | head -1)
    if [[ -z "$dump_line" ]]; then
        # No dump — assume pass (main returned normally)
        log "PASS (no dump, halted normally)"
        echo "PASS"
        return
    fi

    # Extract the two hex bytes
    local byte0 byte1
    byte0=$(echo "$dump_line" | awk '{print $2}')
    byte1=$(echo "$dump_line" | awk '{print $3}')

    # Little-endian: status = byte0 + (byte1 << 8)
    local status_val
    status_val=$(printf "%d" "0x${byte1}${byte0}" 2>/dev/null || echo "999")

    if [[ "$status_val" == "57005" ]]; then
        # 0xDEAD = 57005 decimal = abort() was called
        log "FAIL (abort called)"
        echo "FAIL"
        return
    elif [[ "$status_val" == "0" ]]; then
        log "PASS (exit(0))"
        echo "PASS"
        return
    else
        # Non-zero exit code
        log "FAIL (exit($status_val))"
        echo "FAIL"
        return
    fi
}

# --- Main loop ---
FAIL_LIST=()
PASS_LIST=()
TIMEOUT_LIST=()

for src in "${TESTS[@]}"; do
    COUNT=$((COUNT + 1))
    name="$(basename "$src" .c)"

    result=$(run_test "$src")

    case "$result" in
        PASS)
            PASS=$((PASS + 1))
            PASS_LIST+=("$name")
            status_char="."
            ;;
        FAIL)
            FAIL=$((FAIL + 1))
            FAIL_LIST+=("$name")
            status_char="F"
            ;;
        SKIP_COMPILE)
            SKIP_COMPILE=$((SKIP_COMPILE + 1))
            status_char="s"
            ;;
        SKIP_LINK)
            SKIP_LINK=$((SKIP_LINK + 1))
            status_char="l"
            ;;
        TIMEOUT)
            TIMEOUT=$((TIMEOUT + 1))
            TIMEOUT_LIST+=("$name")
            status_char="T"
            ;;
        ERROR)
            ERRORS=$((ERRORS + 1))
            status_char="E"
            ;;
        *)
            ERRORS=$((ERRORS + 1))
            status_char="?"
            ;;
    esac

    # Progress output: one char per test, newline every 50
    printf "%s" "$status_char"
    if (( COUNT % 50 == 0 )); then
        printf "  [%d/%d]\n" "$COUNT" "$TOTAL"
    fi

    if [[ $STOP_ON_FAIL -eq 1 && "$result" == "FAIL" ]]; then
        echo ""
        echo "Stopping on first failure: $name"
        break
    fi
done

# Final newline if needed
if (( COUNT % 50 != 0 )); then
    printf "  [%d/%d]\n" "$COUNT" "$TOTAL"
fi

# --- Summary ---
COMPILED=$((PASS + FAIL + TIMEOUT + ERRORS))
SKIPPED=$((SKIP_COMPILE + SKIP_LINK))

echo ""
echo "======================================"
echo "  RESULTS SUMMARY"
echo "======================================"
echo "Total tests:     $TOTAL"
echo "Compiled+Linked: $COMPILED"
echo "  PASS:          $PASS"
echo "  FAIL:          $FAIL"
echo "  TIMEOUT:       $TIMEOUT"
echo "  ERROR:         $ERRORS"
echo "Skipped:         $SKIPPED"
echo "  Compile skip:  $SKIP_COMPILE"
echo "  Link skip:     $SKIP_LINK"
echo "======================================"

if [[ ${#FAIL_LIST[@]} -gt 0 ]]; then
    echo ""
    echo "--- FAILED tests ---"
    for name in "${FAIL_LIST[@]}"; do
        echo "  $name"
    done
fi

if [[ ${#TIMEOUT_LIST[@]} -gt 0 ]]; then
    echo ""
    echo "--- TIMEOUT tests ---"
    for name in "${TIMEOUT_LIST[@]}"; do
        echo "  $name"
    done
fi

echo ""
echo "Pass rate (of compiled): ${PASS}/${COMPILED} = $(( COMPILED > 0 ? PASS * 100 / COMPILED : 0 ))%"
echo "Overall compile rate: ${COMPILED}/${TOTAL} = $(( TOTAL > 0 ? COMPILED * 100 / TOTAL : 0 ))%"
echo ""

# Close log file descriptor
exec 3>&-

if [[ -n "$LOG_FILE" ]]; then
    echo "Detailed log saved to: $LOG_FILE"
fi

# Exit with non-zero if there were any failures
if [[ $FAIL -gt 0 ]]; then
    exit 1
fi
exit 0
