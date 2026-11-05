# KLEE Path Test Generator

Generate concrete command-line test inputs for a C program by using KLEE symbolic execution.

## Run

```bash
chmod +x run.sh
./run.sh ./test.c
```

For your own source file, supply only its path:

```bash
./run.sh /absolute/path/to/program.c
```

If the argument count cannot be inferred safely, supply the known number of user arguments:

```bash
ARG_COUNT=7 ./run.sh /absolute/path/to/program.c
```

Results are written to `results/<source>-<timestamp>/`.

## Input model and assumptions

This implementation targets the following well-defined class of C programs:

- `main` is defined with exactly two named parameters, conventionally `int main(int argc, char **argv)`.
- One run has a fixed number of user-supplied command-line arguments.
- Every user argument is a decimal representation of a signed 32-bit integer.
- The source reads these values through `atoi(argv[i])` (directly or via a pointer that still refers to the same `argv` string).
- Arrays are compile-time fixed-size. C variable-length arrays are rejected.
- The source and any required headers/libraries are available to the selected Clang compiler.

The tool does not require edits to the original source. It creates a generated driver that includes the original `.c` file, renames its `main`, creates a fixed-size symbolic `int32_t` input vector with `klee_make_symbolic`, and invokes the original entry point with a synthetic `argv` vector. Calls to `atoi` on those synthetic `argv` pointers are mapped to the corresponding symbolic integers. A separate native executable is compiled from the untouched original source and used to replay each concrete input as ordinary decimal command-line arguments.

This is not a universal C-input inference tool. Programs that read arbitrary strings, use `scanf`/stdin/files, use `strtol` or custom parsers for command-line values, inspect the raw characters of `argv`, use multiple optional argument layouts, or allocate input-dependent buffers need a suitable input adapter. `ARG_COUNT` can specify the fixed number of arguments, but it does not change the supported integer/`atoi` input model.

## What is generated

Each `.ktest` contains one concrete assignment for a KLEE-explored execution state. The package decodes its `argv_values` object and writes:

- `test_cases.txt` — human-readable concrete inputs and replay commands.
- `test_cases.csv` — one row per generated test case.
- `test_cases.json` — structured input and replay data.
- `summary.txt` and `summary.json` — completion and path/test counts when KLEE reports them.
- `klee.log` and `klee-out/` — KLEE's full log, `.ktest` files, errors, warnings and statistics.
- `klee_driver.c` and `klee_driver.bc` — generated driver source and LLVM bitcode.
- `native_program` — executable compiled from the original source for replay.

The script reads `.ktest` files directly. It does not require the separate `ktest-tool` command. With the Snap package, helper commands are named with a `klee.` prefix (for example, `klee.ktest-tool`).

## KLEE / LLVM setup

The KLEE version reported by the user is:

```text
KLEE 3.2
Ubuntu LLVM version 13.0.1
```

KLEE 3.2 supports LLVM 13 (LLVM versions below 13 were removed in 3.2). Use Clang 13 to generate compatible bitcode. Check:

```bash
klee --version
clang-13 --version
```

If Clang 13 is not installed and your Ubuntu repositories provide it:

```bash
sudo apt update
sudo apt install clang-13 llvm-13
```

The script defaults to `CLANG=clang-13` and checks the LLVM major version against KLEE before compiling. The official KLEE Snap provides `klee.env CFLAGS`; the script uses these flags automatically when that helper is available.

The script also checks that the KLEE build advertises `--posix-runtime`, which is used for the standard C library/runtime model. KLEE must be installed with that runtime enabled.

## Exploration settings and interpretation

The script invokes KLEE with:

```bash
klee --posix-runtime --search=dfs --emit-all-errors ...
```

- DFS selects depth-first state exploration; it changes scheduling, not which paths are feasible.
- The script does not set artificial `--max-time`, `--max-forks`, or `--max-tests` cutoffs.
- `--only-output-states-covering-new` is not enabled, so KLEE is not asked to suppress tests for states that do not add new instruction coverage.
- KLEE can still fail to finish because of path explosion, solver difficulty, unsupported external calls, memory limits, or other runtime constraints.

A run is labelled `Bounded exploration complete: YES` only if KLEE exits successfully, prints its normal completion summary, reports zero partially completed paths, and all `.ktest` files are decoded. This is evidence that KLEE exhausted the paths it could model for this one fixed-argument, signed-integer input model. It is not a guarantee for other argument counts, arbitrary string inputs, or unsupported library/environment behavior. Always inspect `klee-out/warnings.txt`, generated error reports, and `klee.log` before claiming complete path coverage.

For a finite program with bounded loops and supported input operations, KLEE can systematically explore feasible symbolic paths. It cannot make path explosion disappear; a program with many independent symbolic branches may still have an impractically large number of paths.

## Included test program

`test.c` is a small standalone example with:

- Three scalar integer arguments and a fixed `int data[4]` array (seven integer arguments total).
- An exact `argc` guard that lets the tool infer the input count.
- A helper function, short-circuit `&&` / `||` conditions, comparisons, parity checks and fixed-bound loops.
- No variable-length arrays or unbounded loops.

Run it using `./run.sh ./test.c`. The script should infer seven user arguments from `argc == 8`.

## Official references

- KLEE 3.2 release notes / LLVM support: <https://github.com/klee/klee/releases>
- KLEE 3.2 Getting Started: <https://klee-se.org/releases/docs/v3.2/getting-started/>
- KLEE 3.2 build guide: <https://klee-se.org/releases/docs/v3.2/build/build-from-source/>
- KLEE command-line options: <https://klee-se.org/docs/options/>
- KLEE generated files and `.ktest`: <https://klee-se.org/docs/files/>
- KLEE tools: <https://klee-se.org/docs/tools/>
- Official KLEE Snap (`klee.env`, helper command names): <https://github.com/klee/klee-snap>
