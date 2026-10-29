# Generalized implicit taint propagation with LLVM + DFSan

This project infers implicit taint from **end-of-`main` concrete-value comparisons**. It does not instrument branches, compute control dependencies, or use DFSan conditional/event callbacks. For each public-input tuple, the analyzer compares final values across the supplied secret-input tuples. An eligible variable/array element whose value changes and has no direct DFSan label becomes an implicit-taint candidate. Candidates are unioned across public groups and applied in a second execution pass.

## Project files

- `FindTaintedCondition.cpp` — custom LLVM pass that discovers eligible source variables, registers globals and `main()` locals/array elements, tracks initialization, and inserts the final snapshot hook.
- `implicit_runtime.c` — records each eligible variable's final concrete value and DFSan label; applies inferred labels immediately before the final snapshot in pass two.
- `analyze_results.py` — parses test cases and `dfsan_set_label()` calls, builds public/secret input combinations, compares snapshots, writes the label plan, and generates grouped tables.
- `test.c` — sample source program with scalar/array values, conditionals, a loop, a helper function, globals and nested-scope variables.
- `test_cases.txt` — user-supplied concrete input rows.
- `test_analyzer.py` — Python analyzer unit tests.
- `run.sh` — builds the plugin and runs discovery + label-application phases.

## Test-case format and input roles

Use a header that names the `argv` input columns in order, for example:

```text
# inputs: a b c s1 s2 s3
0 0 1 0 0 0
0 0 1 0 1 0
```

A name that matches a variable passed directly by address to `dfsan_set_label(...)` in `test.c` is classified as secret/tainted. Every other header column is public. Header columns must match the source variables and positional `argv[]` parsing. The analyzer removes duplicate rows, finds unique public tuples and unique secret tuples, and expands their Cartesian product; it does not generate new input values.

## Inference model

For each fixed public tuple:

1. Read the final snapshots from all corresponding secret-input executions.
2. Ignore a variable as implicit-taint evidence if any recorded discovery value has a nonzero DFSan label.
3. Compare raw final-value bytes across secret executions. If the value changes and the variable is recorded in each required execution, mark it as an implicit-taint candidate for that public tuple.
4. Union candidates across public tuples.
5. In the second run, apply the candidate label using `dfsan_add_label()` immediately before the final snapshot.

This is evidence from the concrete input set, not proof for every possible execution. A variable that changes during execution but is overwritten to the same final value is not inferred as tainted.

## Tracked variable scope

The pass tracks primitive scalar global variables, primitive scalar variables declared directly in `main()`, and each element of fixed-size arrays whose leaf elements are primitive scalars. Multidimensional primitive arrays are flattened into entries such as `matrix[1][2]`. It excludes pointers, variable-length arrays, arrays of pointers/aggregates, structures, unions, function parameters, nested-block locals, and locals declared in helper functions. Debug information is required for reliable source-level scope filtering; the build uses `-g` and skips local variables that cannot be identified safely.

Each array element has its own registration, final concrete value and DFSan label. Taint of one element does not automatically mark the entire array.

## LLVM pass name

`-passes=implicit-taint` uses a **project-defined pipeline name** registered by `FindTaintedCondition.cpp` through LLVM's pass-plugin parsing callback. It is not a built-in LLVM pass name. The plugin also inserts `EscapeMainAllocasForDFSan` at the pipeline-start extension point so eligible `main()` locals use addressable DFSan shadow memory for labels added at runtime.

## Run

```bash
bash run.sh
```

The default script expects a compatible LLVM/Clang toolchain with `llvm-config-21` (or `llvm-config`), `clang`, `clang++`, `opt`, and DataFlowSanitizer. Set `LLVM_CONFIG` if the executable has a different name. `TEST_CASES_FILE`, `SOURCE_FILE`, and `REPORT_FILE` can override the default paths.

## Validation note

The Python analyzer unit tests and standalone C compilation can be run separately. The full LLVM plugin pipeline requires LLVM C++ development headers plus matching `llvm-config` and `opt`; if those are unavailable, end-to-end instrumentation cannot be claimed as tested. No branch-instrumentation callbacks are enabled or required by this value-comparison approach.
