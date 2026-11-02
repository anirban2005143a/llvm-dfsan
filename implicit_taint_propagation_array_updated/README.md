# Implicit taint detection with LLVM + DFSan

This project infers implicit taint by comparing final values immediately before `main()` returns. It does not instrument branches or use DFSan conditional callbacks.

## Files

- `FindTaintedCondition.cpp` — LLVM pass that registers eligible scalar variables and array elements in `main()` and inserts the final snapshot hook. Runtime-sized VLAs are registered using their actual element count for each execution.
- `implicit_runtime.c` — records each registered value and its DFSan label, and applies inferred labels during the second run.
- `analyze_results.py` — reads test cases, classifies public/secret inputs, creates the Cartesian product, and flattens array values into process arguments.
- `test.c` — sample program with public inputs, scalar secret inputs, and an array input. The array may be public or directly tainted as a whole. It does not parse comma-separated array data or print anything.
- `test_cases.txt` — sample logical inputs.
- `test_analyzer.py` — regression tests.
- `run.sh` — builds the plugin and executes discovery and label-application runs.

## Test-case format

The header lists logical input variables in the same order expected by `main()`. Use `|` between logical input columns. The final array column contains comma-separated integer elements:

```text
# inputs: public1 public2 arr
0|0|10,20
0|1|-10,40,-50
1|0|60
1|1|-70,80,90,100
```

The complete `arr` field is **one input tuple value** for unique-tuple detection and Cartesian-product generation. The analyzer does not split it into per-element tuple combinations. Different rows may contain arrays of different lengths. Whether `arr` belongs to the public or secret tuple is determined by the source code: a whole-array `dfsan_set_label(label, arr, sizeof(arr))` makes it a secret input; with no such call, it is a public input.

Before execution, the analyzer writes a separate argv file in which each array element becomes its own argument. For example, the logical input `0|0|10,20` becomes the process arguments `0 0 10 20`. Thus `test.c` can read the array directly:

```c
int arr[argc - 3];
for (int i = 0; i < argc - 3; ++i)
    arr[i] = atoi(argv[3 + i]);
dfsan_set_label(4, arr, sizeof(arr));
```

No CSV parsing is performed in `test.c`. The VLA length is determined by the number of remaining argv values and can differ between executions.

## Taint assumptions and reporting

- The array input is **not required to be tainted**. If the source calls `dfsan_set_label(label, arr, sizeof(arr))`, the complete array is one secret input. If it does not, the array is one public input. Partial or conditional array labeling is unsupported; when labeling is used, label the whole array by its name.
- The array remains one value for public/secret tuple grouping, while the LLVM pass/runtime registers and checks `arr[0]`, `arr[1]`, etc. separately. Thus an untainted input array can still have individual elements become directly tainted later through ordinary DFSan propagation from other tainted data.
- `DIRECT` means the discovery-run DFSan label is nonzero. `IMPLICIT` means the final concrete value varies across secret combinations for the same public tuple while discovery labels are zero. `NOT TAINTED` means the value remains stable with a zero label. `INCOMPLETE` means the value is absent in one or more compared executions.
- A variable-length array input must be the last logical test-case column. This allows its elements to occupy all remaining argv positions without a separate length argument. The current runner supports one variable-length array input.

## Run

```bash
bash run.sh
```

The script expects a compatible LLVM/Clang toolchain with `llvm-config-21` (or `llvm-config`), `clang`, `clang++`, `opt`, and DataFlowSanitizer. Set `LLVM_CONFIG` if needed. `TEST_CASES_FILE`, `SOURCE_FILE`, and `REPORT_FILE` can override the defaults.

Run the Python tests independently:

```bash
python3 -m unittest -v test_analyzer.py
```

The complete LLVM plugin pipeline must be tested with the same LLVM major version used to build the pass.
