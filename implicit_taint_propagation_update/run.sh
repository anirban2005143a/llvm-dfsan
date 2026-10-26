#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")" && pwd)"
TEST_CASES_FILE="${TEST_CASES_FILE:-$ROOT/test_cases.txt}"
SOURCE_FILE="${SOURCE_FILE:-$ROOT/test.c}"
REPORT_FILE="${REPORT_FILE:-$ROOT/final_values_report.txt}"
BUILD="$(mktemp -d /tmp/implicit_taint_build.XXXXXX)"
EXECUTION_CASES_FILE="$BUILD/expanded_test_cases.txt"
DISCOVERY_TRACE_DIR="$BUILD/discovery_traces"
FINAL_TRACE_DIR="$BUILD/final_traces"
LABEL_PLAN="$BUILD/implicit_labels.plan"
mkdir -p "$DISCOVERY_TRACE_DIR" "$FINAL_TRACE_DIR"

cleanup() {
    rm -rf "$BUILD"
}
trap cleanup EXIT

cd "$ROOT"

if [[ ! -f "$TEST_CASES_FILE" ]]; then
    echo "error: test-case file was not found: $TEST_CASES_FILE" >&2
    exit 1
fi
if [[ ! -f "$SOURCE_FILE" ]]; then
    echo "error: source file was not found: $SOURCE_FILE" >&2
    exit 1
fi

# The user supplies values and the sets of public/secret tuples. Recombine those
# distinct tuples as their Cartesian product (e.g. 9 public x 4 secret = 36 runs).
python3 analyze_results.py --expand-cases \
    "$TEST_CASES_FILE" "$SOURCE_FILE" "$EXECUTION_CASES_FILE"

LLVM_CONFIG="${LLVM_CONFIG:-$(command -v llvm-config-21 || command -v llvm-config || true)}"
if [[ -z "$LLVM_CONFIG" ]]; then
    echo "error: llvm-config-21 or llvm-config was not found" >&2
    exit 1
fi

LLVM_BIN="$("$LLVM_CONFIG" --bindir)"
CLANG="$LLVM_BIN/clang"
CLANGXX="$LLVM_BIN/clang++"
OPT="$LLVM_BIN/opt"
if [[ ! -x "$CLANG" || ! -x "$CLANGXX" || ! -x "$OPT" ]]; then
    echo "error: clang/clang++/opt were not found in $LLVM_BIN" >&2
    exit 1
fi

"$CLANGXX" -std=c++17 -fPIC -shared \
    FindTaintedCondition.cpp \
    -o "$BUILD/FindTaintedCondition.so" \
    $($LLVM_CONFIG --cxxflags --ldflags --system-libs --libs core analysis passes)

# Keep main scalar allocas memory-backed under DFSan so runtime labels can be
# applied after cross-execution comparison. The pass also registers scalar globals.
"$CLANG" -O0 -g -Xclang -disable-O0-optnone \
    -fpass-plugin="$BUILD/FindTaintedCondition.so" \
    -fsanitize=dataflow \
    -mllvm -dfsan-conditional-callbacks \
    -mllvm -dfsan-event-callbacks \
    -S -emit-llvm "$SOURCE_FILE" \
    -o "$BUILD/dfsan.ll"

"$OPT" \
    -load-pass-plugin="$BUILD/FindTaintedCondition.so" \
    -passes=implicit-taint \
    -S "$BUILD/dfsan.ll" \
    -o "$BUILD/instrumented.ll"

"$CLANG" -c "$BUILD/instrumented.ll" -o "$BUILD/instrumented.o"
"$CLANG" -c implicit_runtime.c -o "$BUILD/implicit_runtime.o"
"$CLANG" "$BUILD/instrumented.o" "$BUILD/implicit_runtime.o" \
    -fsanitize=dataflow -o "$BUILD/implicit_test"

run_test_cases() {
    local trace_dir="$1"
    local plan_file="${2:-}"
    local case_index=0
    local line
    local trace_file
    local dedup_key
    local -a INPUT_VALUES
    declare -A SEEN_CASES=()

    while IFS= read -r line || [[ -n "$line" ]]; do
        line="${line%%#*}"
        if [[ "$line" =~ ^[[:space:]]*$ ]]; then
            continue
        fi
        read -r -a INPUT_VALUES <<< "$line"
        if [[ "${#INPUT_VALUES[@]}" -eq 0 ]]; then
            continue
        fi

        # Run only unique concrete rows, matching analyzer_results.py's order.
        dedup_key="$(printf '%s\034' "${INPUT_VALUES[@]}")"
        if [[ -n "${SEEN_CASES[$dedup_key]+seen}" ]]; then
            continue
        fi
        SEEN_CASES["$dedup_key"]=1

        trace_file="$trace_dir/case_${case_index}.trace"
        if [[ -n "$plan_file" ]]; then
            if ! IMPLICIT_LABELS_FILE="$plan_file" \
                IMPLICIT_TRACE_FILE="$trace_file" \
                "$BUILD/implicit_test" "${INPUT_VALUES[@]}" \
                >"$BUILD/case_${case_index}.stdout" \
                2>"$BUILD/case_${case_index}.stderr"; then
                cat "$BUILD/case_${case_index}.stderr" >&2
                return 1
            fi
        else
            if ! env -u IMPLICIT_LABELS_FILE \
                IMPLICIT_TRACE_FILE="$trace_file" \
                "$BUILD/implicit_test" "${INPUT_VALUES[@]}" \
                >"$BUILD/case_${case_index}.stdout" \
                2>"$BUILD/case_${case_index}.stderr"; then
                cat "$BUILD/case_${case_index}.stderr" >&2
                return 1
            fi
        fi
        case_index=$((case_index + 1))
    done < "$EXECUTION_CASES_FILE"

    if [[ "$case_index" -eq 0 ]]; then
        echo "error: test-case file contains no test cases: $TEST_CASES_FILE" >&2
        return 1
    fi
}

# Pass 1: record final values and union candidates across all public groups.
run_test_cases "$DISCOVERY_TRACE_DIR"
python3 analyze_results.py --build-plan \
    "$DISCOVERY_TRACE_DIR" "$LABEL_PLAN" "$EXECUTION_CASES_FILE" "$SOURCE_FILE"

# Pass 2: apply inferred labels immediately before main's final snapshot.
run_test_cases "$FINAL_TRACE_DIR" "$LABEL_PLAN"
python3 analyze_results.py --final "$FINAL_TRACE_DIR"
python3 analyze_results.py --report \
    "$FINAL_TRACE_DIR" "$DISCOVERY_TRACE_DIR" "$EXECUTION_CASES_FILE" "$SOURCE_FILE" "$REPORT_FILE"

echo "Debug report: $REPORT_FILE" >&2
