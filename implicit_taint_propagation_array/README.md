# Generalized implicit taint propagation with LLVM DFSan

This version compares final concrete values across supplied program executions. It identifies an implicitly tainted scalar or individual fixed-size primitive-array element if its value changes across distinct secret-input combinations while the public-input values are identical. Findings from all eligible public-input groups are **united** into one end-of-`main` label plan.

## Files

- `FindTaintedCondition.cpp` — LLVM pass that instruments `main`, registers eligible scalar locals/globals and each eligible fixed-size primitive-array element, and places the final snapshot hook before `main` returns.
- `implicit_runtime.c` — runtime records final values and applies inferred labels from the plan.
- `analyze_results.py` — identifies source variables explicitly marked by `dfsan_set_label`, infers input roles, compares snapshots, and writes the human-readable report.
- `test.c` — example with globals, primitive locals, `if` / `else if` / `else`, a loop, and a called function.
- `test_cases.txt` — user-provided concrete input rows.
- `run.sh` — build and two-pass execution flow.

## Test-case format and automatic input roles

The first header line names the positional `argv` inputs in row order:

```text
# inputs: a b c s1 s2 s3
0 0 1 0 0 0
0 0 1 0 1 0
```

The source should directly mark scalar secret variables, for example:

```c
dfsan_set_label(1, &s1, sizeof(s1));
dfsan_set_label(2, &s2, sizeof(s2));
```

Input-column names matching variables passed directly by address to `dfsan_set_label` are treated as secret/tainted. All remaining input columns are automatically treated as public. Therefore, names in `# inputs:` must match the variable names in those source calls. This source scanner handles direct address targets and simple casts; it does not infer arbitrary pointer aliases.

The analyzer removes duplicate rows while preserving the first occurrence. It extracts the unique **public tuples** and unique **secret tuples** present in those rows, then executes their Cartesian product. For example, 9 unique public tuples and 4 unique secret tuples become 36 executions. It does not invent new input values or generate new coverage-guided test values; it only recombines tuples supplied by the user.

## Array-element design

The pass enumerates fixed-size C arrays recursively and registers each primitive leaf element under an indexed name. The runtime reads and adds DFSan labels for each element's own address range, rather than labeling the array as a single object. This follows DFSan's memory-region interface (`dfsan_read_label` and `dfsan_add_label`) documented in the [Clang DataFlowSanitizer reference](https://clang.llvm.org/docs/DataFlowSanitizer.html).

## What is compared

For each distinct public-input tuple, runs are grouped together and the final values of every tracked scalar and array element are compared across distinct secret-input tuples. An eligible group requires at least two secret-input combinations. The inferred variable set is the union of candidates across all eligible public groups.

A variable is only used as evidence if it is recorded in every execution in that group, is not a pointer, and has DFSan label zero in every discovery run. Variables with existing explicit DFSan labels are not redundantly marked by the comparison.

## Debug report

After a successful run, `final_values_report.txt` is written at the project root by default. The report has one table per unique public-input tuple, rather than listing every public-secret Cartesian-product execution as a separate case. Each table row is a tracked scalar variable or one array element; columns `S1`, `S2`, etc. show its final value under each unique secret tuple (the secret tuples are defined once in the report). The last column reports `DIRECT`, `IMPLICIT`, `NOT TAINTED`, or `INCOMPLETE` for that public-input group.

`DIRECT` means the discovery execution had a nonzero DFSan label. `IMPLICIT` means the final concrete value changed across secret tuples for the same public tuple while the discovery labels remained zero. `NOT TAINTED` means the value stayed stable and its discovery label was zero. `INCOMPLETE` means some values could not be recorded. The report status is based on discovery traces before inferred implicit labels are injected, so inferred labels are not mistaken for direct labels.

The report includes primitive scalar globals, elements of fixed-size arrays whose elements are primitive scalars, and primitive scalar/array-element variables declared directly in `main`'s function scope. Array elements are separate entries such as `values[0]` and `values[1]`, so direct or implicit taint of one element does not automatically mark the entire array. It excludes variables declared in nested `if`/`else`/loop blocks, locals in called functions, pointers, arrays of pointers, variable-length arrays, structures, and other aggregates. Fixed-size multidimensional arrays of primitive scalar elements are flattened into names such as `matrix[1][2]`. The snapshot is taken immediately before `main` returns. Override the report destination with `REPORT_FILE=/path/to/report.txt`.

## Run

```bash
bash run.sh
```

The default build expects a matching LLVM/Clang installation with `llvm-config-21`, `clang`, `clang++`, `opt`, and DataFlowSanitizer. Set `LLVM_CONFIG` to a compatible `llvm-config` executable if needed. `TEST_CASES_FILE`, `SOURCE_FILE`, and `REPORT_FILE` can override the defaults.

## Scope and limitations

This is dynamic evidence from the concrete test cases, not a proof of all feasible executions. It compares final values, so a variable that changes during execution but is overwritten to the same final value is not inferred as tainted. Primitive scalar variables declared directly in `main`'s function scope, scalar globals, and individual elements of fixed-size arrays of primitive scalar elements are snapshotted. Parameters and variables declared inside nested blocks of `main`, as well as locals inside called functions, are excluded, although called functions can affect tracked globals and `main`-scope values. Array elements are snapshotted independently when they belong to fixed-size arrays of primitive scalar elements. Pointer contents and arrays whose elements are pointers/aggregates are not tracked. A local element is compared only when it was initialized and recorded in every run in the relevant public-input group.


## Validation performed for this package

The Python analyzer unit tests pass, and `test.c` plus `implicit_runtime.c` compile with the available Clang 17 C toolchain (including compiling `test.c` with `-fsanitize=dataflow`). The full LLVM pass-plugin pipeline was not executable in the packaging environment because `llvm-config`/`opt` and the LLVM C++ development headers are unavailable here. Run `bash run.sh` with the documented matching LLVM toolchain to validate the integrated instrumentation and regenerate the report.
