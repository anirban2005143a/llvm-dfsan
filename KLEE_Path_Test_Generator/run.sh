#!/usr/bin/env bash
set -Eeuo pipefail

ROOT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"

usage() {
  cat <<'EOF'
Usage:
  ./run.sh /path/to/source.c

Optional environment variables:
  CLANG=/path/to/clang  Override the automatically selected matching Clang
  ARG_COUNT=N          Override automatic fixed argv-count inference
  CPPFLAGS='-I...'    Extra preprocessor/include flags (used for AST and compilation)
  CFLAGS='...'        Extra compiler flags for source compilation
  OUT_DIR=path        Choose result directory

Assumptions:
  - main has the form int main(int argc, char **argv) (parameter names may vary).
  - A fixed number of command-line arguments is used for one run.
  - Each input argument represents a signed 32-bit integer in decimal form.
  - The source reads numeric arguments using atoi(argv[i]); other atoi calls
    are retained with strtol-based parsing.
  - Arrays are compile-time fixed-size; VLAs are rejected.

Example:
  ./run.sh ./test.c
  ARG_COUNT=36 ./run.sh /absolute/path/to/your_program.c
EOF
}

if [[ $# -ne 1 ]]; then usage >&2; exit 2; fi
SOURCE="$(realpath -- "$1")"
[[ -f "$SOURCE" ]] || { echo "ERROR: source not found: $SOURCE" >&2; exit 2; }
[[ "${SOURCE##*.}" == "c" || "${SOURCE##*.}" == "C" ]] || { echo "ERROR: expected a .c source file" >&2; exit 2; }

KLEE="${KLEE:-klee}"
CLANG="${CLANG:-}"
PYTHON="${PYTHON:-python3}"
CPPFLAGS="${CPPFLAGS:-}"
CFLAGS="${CFLAGS:-}"
KLEE_ENV="${KLEE_ENV:-klee.env}"
KLEE_ENV_CFLAGS_TEXT=""
if command -v "$KLEE_ENV" >/dev/null 2>&1; then
  KLEE_ENV_CFLAGS_TEXT="$("$KLEE_ENV" CFLAGS)"
fi

for cmd in "$KLEE" "$PYTHON"; do
  command -v "$cmd" >/dev/null 2>&1 || { echo "ERROR: '$cmd' was not found in PATH." >&2; exit 127; }
done

KLEE_INFO="$("$KLEE" --version 2>&1)"
KLEE_VERSION="$(printf '%s\n' "$KLEE_INFO" | head -n 1)"
KLEE_LLVM_MAJOR="$(printf '%s\n' "$KLEE_INFO" | sed -nE 's/.*LLVM version ([0-9]+)\..*/\1/p' | head -n 1)"
if [[ -z "$KLEE_LLVM_MAJOR" ]]; then
  KLEE_LLVM_MAJOR="$(printf '%s\n' "$KLEE_INFO" | sed -nE 's/.*LLVM ([0-9]+)\..*/\1/p' | head -n 1)"
fi

# Select a compiler that emits bitcode compatible with KLEE's LLVM library.
# A newer system Clang is not automatically compatible with an older KLEE build.
if [[ -z "$CLANG" ]]; then
  if [[ -n "$KLEE_LLVM_MAJOR" ]]; then
    if command -v "clang-$KLEE_LLVM_MAJOR" >/dev/null 2>&1; then
      CLANG="$(command -v "clang-$KLEE_LLVM_MAJOR")"
    elif [[ -x "/usr/lib/llvm-$KLEE_LLVM_MAJOR/bin/clang" ]]; then
      CLANG="/usr/lib/llvm-$KLEE_LLVM_MAJOR/bin/clang"
    elif command -v clang >/dev/null 2>&1; then
      DEFAULT_CLANG="$(command -v clang)"
      DEFAULT_CLANG_VERSION="$("$DEFAULT_CLANG" --version 2>&1 | head -n 1)"
      DEFAULT_CLANG_MAJOR="$(printf '%s\n' "$DEFAULT_CLANG_VERSION" | sed -nE 's/.*clang version ([0-9]+)\..*/\1/p')"
      if [[ "$DEFAULT_CLANG_MAJOR" == "$KLEE_LLVM_MAJOR" ]]; then
        CLANG="$DEFAULT_CLANG"
      fi
    fi
    if [[ -z "$CLANG" ]]; then
      echo "ERROR: KLEE is built with LLVM $KLEE_LLVM_MAJOR, but no matching Clang compiler was found." >&2
      echo "Your installed Clang may be newer, but its LLVM bitcode is not guaranteed to be readable by this KLEE build." >&2
      echo "Install clang-$KLEE_LLVM_MAJOR, or rebuild KLEE against a supported LLVM version and use its matching Clang." >&2
      exit 127
    fi
  elif command -v clang >/dev/null 2>&1; then
    CLANG="$(command -v clang)"
  else
    echo "ERROR: could not detect KLEE's LLVM version and no 'clang' compiler was found. Set CLANG explicitly." >&2
    exit 127
  fi
fi

command -v "$CLANG" >/dev/null 2>&1 || { echo "ERROR: compiler '$CLANG' was not found. Set CLANG to its executable path." >&2; exit 127; }
CLANG="$(command -v "$CLANG")"
CLANG_VERSION="$("$CLANG" --version 2>&1 | head -n 1)"
CLANG_MAJOR="$(printf '%s\n' "$CLANG_VERSION" | sed -nE 's/.*clang version ([0-9]+)\..*/\1/p')"
if [[ -n "$KLEE_LLVM_MAJOR" && -n "$CLANG_MAJOR" && "$KLEE_LLVM_MAJOR" != "$CLANG_MAJOR" ]]; then
  echo "ERROR: LLVM major-version mismatch: KLEE uses LLVM $KLEE_LLVM_MAJOR but $CLANG_VERSION." >&2
  echo "Use Clang $KLEE_LLVM_MAJOR, or rebuild KLEE against a compatible LLVM/Clang toolchain." >&2
  exit 2
fi

# Capture help before searching: this avoids SIGPIPE with set -o pipefail.
KLEE_HELP="$($KLEE --help 2>&1 || true)"
# The POSIX runtime is required for the program's standard C library model.
if ! grep -q -- '--posix-runtime' <<< "$KLEE_HELP"; then
  echo "ERROR: this KLEE build does not advertise --posix-runtime." >&2
  echo "Install/build KLEE with POSIX runtime support enabled." >&2
  exit 2
fi

SOURCE_NAME="$(basename -- "$SOURCE")"
SOURCE_STEM="${SOURCE_NAME%.*}"
STAMP="$(date +%Y%m%d-%H%M%S)"
OUT_DIR="${OUT_DIR:-$ROOT_DIR/results/${SOURCE_STEM}-${STAMP}}"
mkdir -p -- "$(dirname -- "$OUT_DIR")"
OUT_DIR="$(cd -- "$(dirname -- "$OUT_DIR")" && pwd)/$(basename -- "$OUT_DIR")"
[[ ! -e "$OUT_DIR" ]] || { echo "ERROR: output directory already exists: $OUT_DIR" >&2; exit 2; }
mkdir -p "$OUT_DIR"

ALL_CPPFLAGS="$CPPFLAGS $KLEE_ENV_CFLAGS_TEXT"
INFER_ARGS=("$PYTHON" "$ROOT_DIR/src/infer_arg_count.py" "$SOURCE" --clang "$CLANG" --cppflags "$ALL_CPPFLAGS $CFLAGS")
if [[ -n "${ARG_COUNT:-}" ]]; then INFER_ARGS+=(--arg-count "$ARG_COUNT"); fi
INFER_OUTPUT="$("${INFER_ARGS[@]}")" || exit $?
ARG_COUNT_VALUE="$(printf '%s\n' "$INFER_OUTPUT" | sed -n 's/^ARG_COUNT=//p' | tail -n 1)"
INFERENCE_METHOD="$(printf '%s\n' "$INFER_OUTPUT" | sed -n 's/^INFERENCE_METHOD=//p' | tail -n 1)"
[[ "$ARG_COUNT_VALUE" =~ ^[1-9][0-9]*$ ]] || { echo "ERROR: could not determine argument count." >&2; exit 2; }

DRIVER="$OUT_DIR/klee_driver.c"
BITCODE="$OUT_DIR/klee_driver.bc"
NATIVE_EXE="$OUT_DIR/native_program"
KLEE_OUT="$OUT_DIR/klee-out"
KLEE_LOG="$OUT_DIR/klee.log"

"$PYTHON" "$ROOT_DIR/src/create_driver.py" "$SOURCE" "$DRIVER" --arg-count "$ARG_COUNT_VALUE"

# Clang must emit bitcode compatible with the LLVM version linked into KLEE.
# -O0 preserves source-level control flow; disable-optnone lets KLEE optimise IR.
read -r -a EXTRA_CPPFLAGS <<< "$CPPFLAGS"
read -r -a EXTRA_CFLAGS <<< "$CFLAGS"
read -r -a KLEE_ENV_CFLAGS <<< "$KLEE_ENV_CFLAGS_TEXT"
"$CLANG" -std=gnu11 -emit-llvm -c -g -O0 -Xclang -disable-O0-optnone \
  "${EXTRA_CPPFLAGS[@]}" "${EXTRA_CFLAGS[@]}" "${KLEE_ENV_CFLAGS[@]}" "$DRIVER" -o "$BITCODE"

# Compile the untouched source for replaying the concrete values outside KLEE.
"$CLANG" -std=gnu11 -O0 -g "${EXTRA_CPPFLAGS[@]}" "${EXTRA_CFLAGS[@]}" \
  "$SOURCE" -o "$NATIVE_EXE"

cat <<EOF
Source:             $SOURCE
KLEE:               $KLEE_VERSION
Compiler:           $CLANG_VERSION
Argument count:     $ARG_COUNT_VALUE (inferred by $INFERENCE_METHOD)
Input model:        signed 32-bit decimal integers
Array assumption:   compile-time fixed-size arrays (VLA check passed)
KLEE env flags:     ${KLEE_ENV_CFLAGS_TEXT:-not provided; no klee.env found}
Bitcode:            $BITCODE
Output:             $OUT_DIR
EOF

set +e
"$KLEE" --posix-runtime --search=dfs --emit-all-errors \
  --output-dir="$KLEE_OUT" "$BITCODE" >"$KLEE_LOG" 2>&1
KLEE_STATUS=$?
set -e

"$PYTHON" "$ROOT_DIR/src/extract_tests.py" \
  --klee-out "$KLEE_OUT" --native-exe "$NATIVE_EXE" \
  --report-dir "$OUT_DIR" --arg-count "$ARG_COUNT_VALUE" \
  --klee-log "$KLEE_LOG" --klee-exit "$KLEE_STATUS" || EXTRACT_STATUS=$?
EXTRACT_STATUS="${EXTRACT_STATUS:-0}"

printf '\nKLEE exit status: %s\n' "$KLEE_STATUS"
printf 'Concrete test cases: %s\n' "$OUT_DIR/test_cases.txt"
printf 'Summary: %s\n' "$OUT_DIR/summary.txt"
printf 'Full KLEE log: %s\n' "$KLEE_LOG"

if (( KLEE_STATUS != 0 )); then
  echo "WARNING: KLEE did not exit successfully. Treat results as incomplete; inspect klee.log and klee-out/warnings.txt." >&2
  exit "$KLEE_STATUS"
fi
if (( EXTRACT_STATUS != 0 )); then
  echo "WARNING: one or more .ktest files could not be decoded. Inspect test_cases.txt." >&2
  exit "$EXTRACT_STATUS"
fi
