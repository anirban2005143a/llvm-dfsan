# Implicit taint detection with LLVM + DFSan

This project infers implicit taint by comparing final values immediately before `main()` exits (and before a tracked VLA stack region is released). It does not instrument branches or use DFSan conditional callbacks.

## Files

- `FindTaintedCondition.cpp` — LLVM pass that registers eligible scalar variables and array elements in `main()` and inserts the final snapshot hook. Fixed-size arrays are registered element-by-element; VLAs are registered using their runtime element count.
- `implicit_runtime.c` — records each registered value and its DFSan label, and applies inferred labels during the second run.
- `analyze_results.py` — reads test cases, classifies public/secret inputs, creates the Cartesian product, builds stable argv segments for arrays, and writes the grouped report.
- `test.c` — sample program with scalar inputs and one VLA. It is only an example; the analyzer does not depend on its variable names or business logic.
- `test_cases.txt` — sample logical inputs.
- `test_analyzer.py` — Python regression tests.
- `run.sh` — builds the plugin and executes discovery and label-application runs.

## Test-case format

The header lists logical input variables in the same order that `main()` consumes their command-line arguments. Use `|` between logical input columns. Scalars use one value. Arrays use comma-separated integer elements.

Multiple fixed-size arrays are supported. For example:

```text
# inputs: type key[32] statemt[32]
2|1,3,4,5,4|2,3,4
3|1,3,4,5,4|2,3,4
```

The optional `[32]` annotation is validated against the fixed-size C declaration (`key[32]`, `statemt[32]`). You may also write `key` and `statemt`; their extents are inferred from the declarations in `main()`.

### How fixed-size array arguments are laid out

Each complete array remains **one logical input value** for public/secret tuple grouping and Cartesian-product generation. For process execution, however, each array occupies a stable argv segment containing exactly its declared C element count. In the example above, `type` consumes one argument, `key` consumes the next 32, and `statemt` consumes the following 32. The provided five `key` values and three `statemt` values are followed by zeroes to fill the remaining elements. A supplied list longer than the declared array size is rejected.

Therefore, the C program must read array arguments element-by-element in the same order and using the same fixed element counts as the test-case header/source declarations. The runner does not pass an entire comma-separated array as a single `argv` string.

A fixed-size array may appear anywhere in the input header, and multiple fixed-size arrays are supported. An array is a secret input only when the source explicitly labels the whole array, for example:

```c
dfsan_set_label(label, key, sizeof(key));
```

If that call is absent, the array is a public input. Each array is grouped as one public/secret tuple field, while the LLVM pass/runtime checks its individual elements separately. Partial or conditional array labeling is unsupported under the project assumptions.

The comma-separated input format currently supports one-dimensional numeric arrays. Multi-dimensional arrays, pointer arrays, and arrays whose element type is an external custom typedef not declared in the source are not recognized by the test-case parser.

### Variable-length arrays

One VLA is supported as the final logical input column. Its comma-separated values are flattened into the remaining argv entries, and the C program must determine its runtime length from the other input values/`argc` as appropriate. Multiple VLAs are not supported by the current argv protocol because their boundaries cannot be determined without explicit per-array length arguments. Fixed-size arrays can be combined with one final VLA.

## Taint assumptions and reporting

- Regex-based source inspection identifies direct `dfsan_set_label()` calls. For arrays, only whole-array labeling using the array name and `sizeof(array)` is supported. No conditional taint-setting is assumed.
- `DIRECT` means the discovery-run DFSan label is nonzero. Direct classification takes precedence even if the concrete value also changes across executions.
- `IMPLICIT` means the final concrete value differs across secret tuples for the same public tuple while all compared discovery labels are zero.
- `NOT TAINTED` means the value is stable with a zero label across the tested combinations. This is an observation over the supplied test set, not a proof for all possible inputs.
- `INCOMPLETE` means the value is absent in one or more compared executions.
- The report lists an explicit mapping from `S1`, `S2`, etc. to the exact secret-input values for each public-input group.
- For integer values, the report may show both signed and unsigned interpretations of the same bytes, such as `-10 (unsigned=4294967286)`. This is a representation aid, not an additional value or a taint label.
- The LLVM pass currently tracks globals and variables declared directly in `main()`. It excludes locals declared in nested lexical blocks and helper functions. It supports scalar integer/floating values and scalar elements of supported arrays; pointers, structs and unions are not reported as scalar leaves.

## Run

```bash
bash run.sh
```

The script expects a compatible LLVM/Clang toolchain with `llvm-config-21` (or `llvm-config`), `clang`, `clang++`, `opt`, and DataFlowSanitizer. Set `LLVM_CONFIG` if needed. `TEST_CASES_FILE`, `SOURCE_FILE`, and `REPORT_FILE` can override the defaults.

Run the Python tests independently:

```bash
python3 -m unittest -v test_analyzer.py
```

The complete LLVM plugin pipeline must be tested with the same LLVM major version used to build the pass. The Python tests validate input parsing, tuple expansion, argv layout, and report generation; they do not substitute for a full LLVM 21 run.
