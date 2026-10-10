#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")" && pwd)"
BUILD="$(mktemp -d /tmp/implicit_taint_build.XXXXXX)"
DISCOVERY_TRACE_DIR="$BUILD/discovery_traces"
FINAL_TRACE_DIR="$BUILD/final_traces"
LABEL_PLAN="$BUILD/implicit_labels.plan"
mkdir -p "$DISCOVERY_TRACE_DIR" "$FINAL_TRACE_DIR"

cleanup() {
    rm -rf "$BUILD"
}
trap cleanup EXIT

cd "$ROOT"

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
    $("$LLVM_CONFIG" --cxxflags --ldflags --system-libs --libs core analysis passes)

# The plugin's pipeline-start callback escapes main's local allocas before
# DFSan instruments them. This keeps inferred merge-point labels visible to
# subsequent DFSan loads/stores.
"$CLANG" -O0 -g -Xclang -disable-O0-optnone \
    -fpass-plugin="$BUILD/FindTaintedCondition.so" \
    -fsanitize=dataflow \
    -mllvm -dfsan-conditional-callbacks \
    -mllvm -dfsan-event-callbacks \
    -S -emit-llvm test.c \
    -o "$BUILD/dfsan.ll"

"$OPT" \
    -load-pass-plugin="$BUILD/FindTaintedCondition.so" \
    -passes=implicit-taint \
    -S "$BUILD/dfsan.ll" \
    -o "$BUILD/instrumented.ll"

"$CLANG" -c "$BUILD/instrumented.ll" \
    -o "$BUILD/instrumented.o"

"$CLANG" -c implicit_runtime.c \
    -o "$BUILD/implicit_runtime.o"

"$CLANG" "$BUILD/instrumented.o" "$BUILD/implicit_runtime.o" \
    -fsanitize=dataflow \
    -o "$BUILD/implicit_test"

run_test_cases() {
    local trace_dir="$1"
    local plan_file="${2:-}"
    local case_index=0
    local line
    local trace_file
    local -a INPUT_VALUES

    while IFS= read -r line || [[ -n "$line" ]]; do
        line="${line%%#*}"
        if [[ "$line" =~ ^[[:space:]]*$ ]]; then
            continue
        fi

        # Forward the values on each test-case line without hardcoding names
        # such as secret1, secret2, or secret3 in this shell script.
        read -r -a INPUT_VALUES <<< "$line"
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
    done < test_cases.txt

    if [[ "$case_index" -eq 0 ]]; then
        echo "error: test_cases.txt contains no test cases" >&2
        return 1
    fi
}

# Pass 1: compare concrete values at merge points across all test cases.
run_test_cases "$DISCOVERY_TRACE_DIR"
python3 analyze_results.py --build-plan "$DISCOVERY_TRACE_DIR" "$LABEL_PLAN"

# Pass 2: apply discovered labels at those merge points. DFSan then propagates
# them through normal assignments and clears them on untainted constant stores.
run_test_cases "$FINAL_TRACE_DIR" "$LABEL_PLAN"
python3 analyze_results.py --final "$FINAL_TRACE_DIR"
