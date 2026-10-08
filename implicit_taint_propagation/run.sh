#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")" && pwd)"
BUILD="$(mktemp -d /tmp/implicit_taint_build.XXXXXX)"
TRACE_DIR="$BUILD/traces"
mkdir -p "$TRACE_DIR"

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

LLVM_BIN="$($LLVM_CONFIG --bindir)"
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

"$CLANG" -O0 -g -Xclang -disable-O0-optnone \
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

CASE_INDEX=0

while IFS= read -r LINE || [[ -n "$LINE" ]]; do
    LINE="${LINE%%#*}"

    if [[ "$LINE" =~ ^[[:space:]]*$ ]]; then
        continue
    fi

    read -r SECRET1 SECRET2 SECRET3 EXTRA <<< "$LINE"

    if [[ -z "${SECRET1:-}" ||
          -z "${SECRET2:-}" ||
          -z "${SECRET3:-}" ||
          -n "${EXTRA:-}" ]]; then
        echo "error: invalid test case: $LINE" >&2
        echo "expected exactly 3 values: secret1 secret2 secret3" >&2
        exit 1
    fi

    TRACE_FILE="$TRACE_DIR/case_${CASE_INDEX}.trace"

    if ! IMPLICIT_TRACE_FILE="$TRACE_FILE" \
        "$BUILD/implicit_test" "$SECRET1" "$SECRET2" "$SECRET3" \
        >"$BUILD/case_${CASE_INDEX}.stdout" \
        2>"$BUILD/case_${CASE_INDEX}.stderr"; then
        cat "$BUILD/case_${CASE_INDEX}.stderr" >&2
        exit 1
    fi

    CASE_INDEX=$((CASE_INDEX + 1))
done < test_cases.txt

if [[ "$CASE_INDEX" -eq 0 ]]; then
    echo "error: test_cases.txt contains no test cases" >&2
    exit 1
fi

python3 analyze_results.py "$TRACE_DIR"
