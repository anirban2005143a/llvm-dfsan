#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")" && pwd)"
TEST_CASES_FILE="${TEST_CASES_FILE:-$ROOT/test_cases.txt}"
SOURCE_FILE="${SOURCE_FILE:-$ROOT/test.c}"
REPORT_FILE="${REPORT_FILE:-$ROOT/final_values_report.txt}"
BUILD="$(mktemp -d /tmp/implicit_taint_build.XXXXXX)"
EXECUTION_CASES_FILE="$BUILD/expanded_test_cases.txt"
EXECUTION_ARGV_FILE="$BUILD/expanded_argv_cases.txt"
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

# Each logical input column is one tuple field. A comma-separated array remains
# one secret field for Cartesian expansion, then is flattened into argv elements.
python3 analyze_results.py --expand-cases \
    "$TEST_CASES_FILE" "$SOURCE_FILE" "$EXECUTION_CASES_FILE"
python3 analyze_results.py --write-argv \
    "$EXECUTION_CASES_FILE" "$SOURCE_FILE" "$EXECUTION_ARGV_FILE"

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

# Keep tracked main allocas memory-backed under DFSan. Runtime-sized VLA
# allocas are registered with their actual element count in each execution.
"$CLANG" -std=c11 -O0 -g -Xclang -disable-O0-optnone \
    -fpass-plugin="$BUILD/FindTaintedCondition.so" \
    -fsanitize=dataflow \
    -S -emit-llvm "$SOURCE_FILE" \
    -o "$BUILD/dfsan.ll"

"$OPT" \
    -load-pass-plugin="$BUILD/FindTaintedCondition.so" \
    -passes=implicit-taint \
    -S "$BUILD/dfsan.ll" \
    -o "$BUILD/instrumented.ll"

"$CLANG" -c "$BUILD/instrumented.ll" -o "$BUILD/instrumented.o"
"$CLANG" -std=c11 -c implicit_runtime.c -o "$BUILD/implicit_runtime.o"
"$CLANG" "$BUILD/instrumented.o" "$BUILD/implicit_runtime.o" \
    -fsanitize=dataflow -o "$BUILD/implicit_test"

# Execute one program instance without treating main's return value as an
# analysis failure. The LLVM pass snapshots tracked variables immediately
# before every `ret` from main; an ordinary non-zero process exit status is
# therefore valid input to this analysis. Python is used because on POSIX
# subprocess.returncode is negative only when the process was terminated by
# a signal, which can be distinguished from `return <non-zero>;` in C.
run_program_case() {
    local trace_file="$1"
    local stdout_file="$2"
    local stderr_file="$3"
    local plan_file="$4"
    shift 4

    python3 - "$BUILD/implicit_test" "$trace_file" "$stdout_file" \
        "$stderr_file" "$plan_file" "$@" <<'PY_RUN_CASE'
import os
import signal
import subprocess
import sys

binary, trace_path, stdout_path, stderr_path, labels_path, *program_args = sys.argv[1:]
env = os.environ.copy()
env["IMPLICIT_TRACE_FILE"] = trace_path
if labels_path:
    env["IMPLICIT_LABELS_FILE"] = labels_path
else:
    env.pop("IMPLICIT_LABELS_FILE", None)

try:
    with open(stdout_path, "wb") as stdout_file, open(stderr_path, "wb") as stderr_file:
        completed = subprocess.run(
            [binary, *program_args],
            env=env,
            stdout=stdout_file,
            stderr=stderr_file,
            check=False,
        )
except OSError as exc:
    print(f"error: could not execute test program: {exc}", file=sys.stderr)
    sys.exit(1)

# A positive return code, including 1..255, is a normal C process exit status.
# Only signal termination means that this execution did not complete normally.
if completed.returncode < 0:
    signal_number = -completed.returncode
    try:
        signal_name = signal.Signals(signal_number).name
    except ValueError:
        signal_name = f"signal {signal_number}"
    print(
        f"error: test program was terminated by {signal_name}; "
        f"trace: {trace_path}",
        file=sys.stderr,
    )
    try:
        with open(stderr_path, "r", encoding="utf-8", errors="replace") as fp:
            captured_stderr = fp.read()
        if captured_stderr:
            print(captured_stderr, file=sys.stderr, end="" if captured_stderr.endswith("\n") else "\n")
    except OSError:
        pass
    sys.exit(1)

# Deliberately do not reject completed.returncode != 0. That value is the
# program's own return value, not the success/failure status of taint analysis.
sys.exit(0)
PY_RUN_CASE
}

run_test_cases() {
    local trace_dir="$1"
    local plan_file="${2:-}"
    local case_index=0
    local line
    local trace_file
    local -a INPUT_ARGS

    while IFS= read -r line || [[ -n "$line" ]]; do
        line="${line%%#*}"
        if [[ "$line" =~ ^[[:space:]]*$ ]]; then
            continue
        fi

        # The analyzer has already split the whole-array CSV field into one
        # argv entry per element. The logical array remains one tuple field in
        # EXECUTION_CASES_FILE; only this argv file is flattened for the process.
        IFS='|' read -r -a INPUT_ARGS <<< "$line"
        if [[ "${#INPUT_ARGS[@]}" -eq 0 ]]; then
            continue
        fi

        trace_file="$trace_dir/case_${case_index}.trace"
        run_program_case \
            "$trace_file" \
            "$BUILD/case_${case_index}.stdout" \
            "$BUILD/case_${case_index}.stderr" \
            "$plan_file" \
            "${INPUT_ARGS[@]}"
        case_index=$((case_index + 1))
    done < "$EXECUTION_ARGV_FILE"

    if [[ "$case_index" -eq 0 ]]; then
        echo "error: test-case file contains no test cases: $TEST_CASES_FILE" >&2
        return 1
    fi
}

# Pass 1: record final scalar/array-element values and DFSan labels.
run_test_cases "$DISCOVERY_TRACE_DIR"
python3 analyze_results.py --build-plan \
    "$DISCOVERY_TRACE_DIR" "$LABEL_PLAN" "$EXECUTION_CASES_FILE" "$SOURCE_FILE"

# Pass 2: apply inferred labels immediately before main's final snapshot.
run_test_cases "$FINAL_TRACE_DIR" "$LABEL_PLAN"
python3 analyze_results.py --final "$FINAL_TRACE_DIR"
python3 analyze_results.py --report \
    "$FINAL_TRACE_DIR" "$DISCOVERY_TRACE_DIR" "$EXECUTION_CASES_FILE" "$SOURCE_FILE" "$REPORT_FILE"

echo "Debug report: $REPORT_FILE" >&2
