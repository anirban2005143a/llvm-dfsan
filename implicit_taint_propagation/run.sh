#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")" && pwd)"
BUILD="$(mktemp -d /tmp/implicit_taint_build.XXXXXX)"
STATE="$(mktemp /tmp/implicit_taint_state.XXXXXX)"
rm -f "$STATE"

cleanup() {
    rm -rf "$BUILD"
    rm -f "$STATE"
}
trap cleanup EXIT

cd "$ROOT"

clang++ -std=c++17 -fPIC -shared \
    FindTaintedCondition.cpp \
    -o "$BUILD/FindTaintedCondition.so" \
    $(llvm-config --cxxflags --ldflags --system-libs --libs core analysis passes)

clang -O0 -g -Xclang -disable-O0-optnone \
    -fsanitize=dataflow \
    -mllvm -dfsan-conditional-callbacks \
    -S -emit-llvm test.c \
    -o "$BUILD/dfsan.ll"

opt -load-pass-plugin="$BUILD/FindTaintedCondition.so" \
    -passes=implicit-taint \
    -S "$BUILD/dfsan.ll" \
    -o "$BUILD/instrumented.ll"

clang -c "$BUILD/instrumented.ll" \
    -o "$BUILD/instrumented.o"

clang -c implicit_runtime.c \
    -o "$BUILD/implicit_runtime.o"

clang "$BUILD/instrumented.o" "$BUILD/implicit_runtime.o" \
    -fsanitize=dataflow \
    -o "$BUILD/implicit_test"

IMPLICIT_STATE_FILE="$STATE" "$BUILD/implicit_test" 1 0 0
IMPLICIT_STATE_FILE="$STATE" "$BUILD/implicit_test" 0 1 0
IMPLICIT_STATE_FILE="$STATE" "$BUILD/implicit_test" 0 0 1
IMPLICIT_STATE_FILE="$STATE" IMPLICIT_FINAL=1 "$BUILD/implicit_test" 0 0 0
