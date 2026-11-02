#!/usr/bin/env python3
"""Infer end-of-main implicit taint from supplied concrete executions.

Each test-case column is one input variable. A comma-separated array field is
kept as ONE input value for public/secret tuple grouping and Cartesian product
expansion. At runtime, LLVM/runtime instrumentation records each array element
separately as array_name[index], including its own DFSan label.
"""
from __future__ import annotations

import collections
import itertools
import pathlib
import re
import struct
import sys
from dataclasses import dataclass
from typing import DefaultDict, Dict, Iterable, List, Mapping, Sequence, Set, Tuple

FLAG_FLOATING = 1 << 1
VariableKey = Tuple[str, str]
InputRow = Tuple[str, ...]
InputTuple = Tuple[Tuple[str, str], ...]


@dataclass(frozen=True)
class FinalRecord:
    flags: int
    size: int
    label: int
    value_hex: str


@dataclass(frozen=True)
class InputSchema:
    column_names: List[str]
    array_names: Set[str]
    secret_names: Set[str]


def _normalise_field(value: str) -> str:
    value = value.strip()
    if "," not in value:
        return value
    elements = [item.strip() for item in value.split(",")]
    if any(not item for item in elements):
        raise RuntimeError(f"invalid comma-separated array input {value!r}: empty element")
    if any(not re.fullmatch(r"[+-]?\d+", item) for item in elements):
        raise RuntimeError(f"array input {value!r} must contain comma-separated integers")
    return ",".join(elements)


def read_test_cases(path: pathlib.Path) -> Tuple[List[str], List[InputRow]]:
    """Read unique test rows. Rows may use | or whitespace as field separators."""
    column_names: List[str] | None = None
    cases: List[InputRow] = []
    seen: Set[InputRow] = set()

    with path.open("r", encoding="utf-8") as fp:
        for line_number, raw in enumerate(fp, 1):
            stripped = raw.strip()
            if not stripped:
                continue
            if stripped.startswith("#"):
                match = re.match(r"^#\s*(?:inputs|columns)\s*:\s*(.*?)\s*$", stripped, re.I)
                if match:
                    if column_names is not None:
                        raise RuntimeError(f"{path}:{line_number}: input header appears more than once")
                    names = match.group(1).split()
                    if not names or any(
                        not re.fullmatch(r"[A-Za-z_][A-Za-z0-9_]*", name) for name in names
                    ):
                        raise RuntimeError(f"{path}:{line_number}: invalid or empty input header")
                    if len(set(names)) != len(names):
                        raise RuntimeError(f"{path}:{line_number}: duplicate input name")
                    column_names = names
                continue

            content = raw.split("#", 1)[0].strip()
            if not content:
                continue
            values = (
                tuple(_normalise_field(item) for item in content.split("|"))
                if "|" in content
                else tuple(_normalise_field(item) for item in content.split())
            )
            if column_names is None:
                raise RuntimeError(
                    f"{path}:{line_number}: add a header such as '# inputs: public1 secret1 array1'"
                )
            if len(values) != len(column_names):
                raise RuntimeError(
                    f"{path}:{line_number}: found {len(values)} values; "
                    f"header declares {len(column_names)} inputs"
                )
            if any(value == "" for value in values):
                raise RuntimeError(f"{path}:{line_number}: empty input fields are not supported")
            if values not in seen:
                seen.add(values)
                cases.append(values)

    if column_names is None:
        raise RuntimeError(f"test-case file has no '# inputs: ...' header: {path}")
    if not cases:
        raise RuntimeError(f"test-case file contains no test cases: {path}")
    return column_names, cases


def _remove_c_comments_and_strings(source: str) -> str:
    source = re.sub(r'"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'', " ", source)
    return re.sub(r"/\*.*?\*/|//[^\n]*", "", source, flags=re.S)


def _split_call_arguments(text: str) -> List[str]:
    args: List[str] = []
    start = 0
    parens = brackets = braces = 0
    for index, char in enumerate(text):
        if char == "(":
            parens += 1
        elif char == ")":
            parens -= 1
        elif char == "[":
            brackets += 1
        elif char == "]":
            brackets -= 1
        elif char == "{":
            braces += 1
        elif char == "}":
            braces -= 1
        elif char == "," and parens == 0 and brackets == 0 and braces == 0:
            args.append(text[start:index].strip())
            start = index + 1
    args.append(text[start:].strip())
    return args


def _strip_outer_parentheses_and_casts(expression: str) -> str:
    expression = re.sub(r"\s+", "", expression)
    while expression.startswith("(") and expression.endswith(")"):
        depth = 0
        matching_outer = -1
        for index, char in enumerate(expression):
            if char == "(":
                depth += 1
            elif char == ")":
                depth -= 1
                if depth == 0:
                    matching_outer = index
                    break
        if matching_outer != len(expression) - 1:
            break
        expression = expression[1:-1]

    cast_pattern = re.compile(
        r"^\((?:(?:const|volatile|restrict|signed|unsigned|short|long|void|char|int|float|double|[A-Za-z_][A-Za-z0-9_]*|\*)+)\)"
    )
    while True:
        match = cast_pattern.match(expression)
        if not match:
            break
        expression = expression[match.end():]
    return expression


def _find_declared_input_arrays(source: str, column_names: Sequence[str]) -> Set[str]:
    """Find primitive arrays, including VLAs whose bound is a runtime expression."""
    primitive = r"(?:char|short|int|long|float|double|_Bool)"
    qualifiers = r"(?:(?:static|extern|const|volatile|unsigned|signed|short|long)\s+)*"
    declaration = re.compile(
        r"\b" + qualifiers + primitive + r"\s+([A-Za-z_][A-Za-z0-9_]*)\s*\[[^\]]*\]"
    )
    declared = {match.group(1) for match in declaration.finditer(source)}
    return set(column_names) & declared


def _extract_direct_label_targets(
    source_path: pathlib.Path, array_names: Set[str]
) -> Set[str]:
    """Find direct DFSan sources; arrays count only when labeled as a whole.

    An array with no whole-array dfsan_set_label call is a public input. Partial
    array labeling remains unsupported because array inputs are tuple fields.
    """
    source = _remove_c_comments_and_strings(source_path.read_text(encoding="utf-8"))
    secret_names: Set[str] = set()
    call_pattern = re.compile(r"\bdfsan_set_label\s*\(")
    position = 0

    while True:
        match = call_pattern.search(source, position)
        if not match:
            break
        open_paren = source.find("(", match.start())
        depth = 1
        index = open_paren + 1
        while index < len(source) and depth:
            if source[index] == "(":
                depth += 1
            elif source[index] == ")":
                depth -= 1
            index += 1
        if depth:
            raise RuntimeError(f"unclosed dfsan_set_label call near source offset {match.start()}")

        args = _split_call_arguments(source[open_paren + 1:index - 1])
        if len(args) >= 2 and not re.search(r"\bdfsan_read_label\s*\(", args[0]):
            target_expression = _strip_outer_parentheses_and_casts(args[1])
            target = re.fullmatch(
                r"(&)?([A-Za-z_][A-Za-z0-9_]*)(?:\[([^\]]+)\])?",
                target_expression,
            )
            if target:
                has_address, name, element_index = target.groups()
                if name in array_names:
                    if element_index is not None:
                        raise RuntimeError(
                            f"partial array labeling is not supported: {target_expression}. "
                            f"Label the whole array by name: dfsan_set_label(label, {name}, sizeof({name}));"
                        )
                    if len(args) < 3 or not re.fullmatch(
                        rf"sizeof\(?{re.escape(name)}\)?", re.sub(r"\s+", "", args[2])
                    ):
                        raise RuntimeError(
                            f"array '{name}' must be labeled over its complete runtime size "
                            f"using sizeof({name})"
                        )
                    secret_names.add(name)
                elif has_address and element_index is None:
                    secret_names.add(name)
        position = index

    return secret_names


def _build_input_schema(
    column_names: List[str], cases: Sequence[InputRow], source_path: pathlib.Path
) -> InputSchema:
    source = _remove_c_comments_and_strings(source_path.read_text(encoding="utf-8"))
    array_names = _find_declared_input_arrays(source, column_names)

    for index, name in enumerate(column_names):
        if any("," in row[index] for row in cases) and name not in array_names:
            raise RuntimeError(
                f"input '{name}' contains comma-separated values, but no primitive array "
                f"named '{name}' is declared in the source"
            )

    if len(array_names) > 1:
        raise RuntimeError(
            "this argv layout supports one variable-length array input; "
            "combine the required elements into one array input"
        )
    if array_names and column_names[-1] not in array_names:
        raise RuntimeError(
            "the variable-length array input must be the last test-case column "
            "so its elements can occupy the remaining argv entries"
        )

    # A detected array is not automatically a secret input. It belongs to the
    # secret tuple only when the source explicitly labels the complete array
    # with dfsan_set_label(label, array_name, sizeof(array_name)); otherwise it
    # remains part of the public-input tuple.
    secret_names = _extract_direct_label_targets(source_path, array_names)

    for name in array_names:
        column_index = column_names.index(name)
        for row in cases:
            if any(not re.fullmatch(r"[+-]?\d+", item) for item in row[column_index].split(",")):
                raise RuntimeError(f"array input '{name}' must contain comma-separated integers")

    if not secret_names.intersection(column_names):
        raise RuntimeError("no test-case input column matches a direct dfsan_set_label target")
    return InputSchema(column_names, array_names, secret_names)


def _split_row_keys(row: InputRow, schema: InputSchema) -> Tuple[InputTuple, InputTuple]:
    """Classify each input column as one indivisible public or secret value."""
    public: List[Tuple[str, str]] = []
    secret: List[Tuple[str, str]] = []
    for name, value in zip(schema.column_names, row):
        (secret if name in schema.secret_names else public).append((name, value))
    return tuple(public), tuple(secret)


def expand_test_cases(
    input_path: pathlib.Path, source_path: pathlib.Path, output_path: pathlib.Path
) -> int:
    """Generate unique public tuples x unique secret tuples.

    A packed array string (including its length) is one input tuple value. Its
    public/secret role is determined by whether the whole array is directly
    labeled; it is never split into element-level tuple combinations.
    """
    column_names, supplied_rows = read_test_cases(input_path)
    schema = _build_input_schema(column_names, supplied_rows, source_path)

    public_tuples = list(dict.fromkeys(_split_row_keys(row, schema)[0] for row in supplied_rows))
    secret_tuples = list(dict.fromkeys(_split_row_keys(row, schema)[1] for row in supplied_rows))
    if len(secret_tuples) < 2:
        raise RuntimeError(
            "at least two distinct secret-input combinations are required for implicit-taint comparison"
        )

    expanded: List[InputRow] = []
    seen_rows: Set[InputRow] = set()
    for public_key, secret_key in itertools.product(public_tuples, secret_tuples):
        public_values = dict(public_key)
        secret_values = dict(secret_key)
        row = tuple(
            secret_values[name] if name in schema.secret_names else public_values[name]
            for name in column_names
        )
        if row not in seen_rows:
            seen_rows.add(row)
            expanded.append(row)

    output_path.parent.mkdir(parents=True, exist_ok=True)
    with output_path.open("w", encoding="utf-8") as out:
        out.write("# inputs: " + " ".join(column_names) + "\n")
        for row in expanded:
            out.write("|".join(row) + "\n")

    print(
        f"Expanded {len(supplied_rows)} supplied row(s): "
        f"{len(public_tuples)} unique public tuple(s) x "
        f"{len(secret_tuples)} unique secret tuple(s) = {len(expanded)} execution(s).",
        file=sys.stderr,
    )
    return len(expanded)


def write_argv_cases(
    input_path: pathlib.Path, source_path: pathlib.Path, output_path: pathlib.Path
) -> int:
    """Flatten each packed array field into individual argv entries.

    The logical execution-case file keeps an array such as ``10,20`` as ONE
    public or secret tuple value, depending on its source label. This second
    file is only the process argument layout: the array becomes two separate
    arguments (``10`` and ``20``), so C can read them directly with ``arr[i] = atoi(argv[array_start + i])``.
    """
    column_names, cases = read_test_cases(input_path)
    schema = _build_input_schema(column_names, cases, source_path)

    output_path.parent.mkdir(parents=True, exist_ok=True)
    with output_path.open("w", encoding="utf-8") as out:
        for row in cases:
            argv_values: List[str] = []
            for name, value in zip(column_names, row):
                if name in schema.array_names:
                    argv_values.extend(value.split(","))
                else:
                    argv_values.append(value)
            out.write("|".join(argv_values) + "\n")

    print(
        f"Wrote {len(cases)} execution row(s) as flattened argv values to {output_path}.",
        file=sys.stderr,
    )
    return len(cases)


def read_snapshots(
    trace_dir: pathlib.Path, case_count: int
) -> List[Dict[VariableKey, FinalRecord]]:
    snapshots: List[Dict[VariableKey, FinalRecord]] = []
    for case_index in range(case_count):
        trace_path = trace_dir / f"case_{case_index}.trace"
        if not trace_path.is_file():
            raise RuntimeError(f"missing trace for test case {case_index}: {trace_path}")
        records: Dict[VariableKey, FinalRecord] = {}
        with trace_path.open("r", encoding="utf-8") as fp:
            for raw in fp:
                line = raw.rstrip("\n")
                if not line:
                    continue
                parts = line.split("|")
                if parts[0] != "F":
                    continue
                if len(parts) != 7:
                    raise RuntimeError(f"invalid final record in {trace_path}: {line}")
                flags, size, label = int(parts[1]), int(parts[2]), int(parts[3])
                scope, name, value_hex = parts[4], parts[5], parts[6]
                if scope not in {"main", "global"}:
                    continue
                key = (scope, name)
                if key in records:
                    raise RuntimeError(f"duplicate final snapshot for {scope}::{name} in {trace_path}")
                records[key] = FinalRecord(flags, size, label, value_hex)
        snapshots.append(records)
    return snapshots


def choose_implicit_label(
    snapshots: Sequence[Mapping[VariableKey, FinalRecord]]
) -> int:
    combined = 0
    for snapshot in snapshots:
        for record in snapshot.values():
            combined |= record.label
    return combined if combined != 0 else 1


def build_label_plan(
    trace_dir: pathlib.Path,
    plan_path: pathlib.Path,
    cases_path: pathlib.Path,
    source_path: pathlib.Path,
) -> List[VariableKey]:
    column_names, cases = read_test_cases(cases_path)
    schema = _build_input_schema(column_names, cases, source_path)
    snapshots = read_snapshots(trace_dir, len(cases))

    groups: DefaultDict[InputTuple, List[int]] = collections.defaultdict(list)
    secret_tuples: Dict[int, InputTuple] = {}
    for index, row in enumerate(cases):
        public_key, secret_key = _split_row_keys(row, schema)
        groups[public_key].append(index)
        secret_tuples[index] = secret_key

    candidates: Set[VariableKey] = set()
    eligible_groups = 0
    for indexes in groups.values():
        if len({secret_tuples[index] for index in indexes}) < 2:
            continue
        eligible_groups += 1
        common_keys = set(snapshots[indexes[0]])
        for index in indexes[1:]:
            common_keys.intersection_update(snapshots[index].keys())

        for key in common_keys:
            records = [snapshots[index][key] for index in indexes]
            if any(record.label != 0 for record in records):
                continue
            if len({record.value_hex for record in records}) > 1:
                candidates.add(key)

    if eligible_groups == 0:
        raise RuntimeError(
            "no public-input group contains two or more distinct secret-input combinations"
        )

    label = choose_implicit_label(snapshots)
    plan_path.parent.mkdir(parents=True, exist_ok=True)
    with plan_path.open("w", encoding="utf-8") as fp:
        for scope, name in sorted(candidates):
            fp.write(f"EXIT|{label}|{scope}|{name}\n")
    return sorted(candidates)


def format_concrete_value(record: FinalRecord) -> str:
    try:
        data = bytes.fromhex(record.value_hex)
    except ValueError:
        return f"hex:{record.value_hex}"
    if not data:
        return "<empty>"
    if record.flags & FLAG_FLOATING:
        if record.size == 4 and len(data) == 4:
            return repr(struct.unpack("=f", data)[0])
        if record.size == 8 and len(data) == 8:
            return repr(struct.unpack("=d", data)[0])
        return f"hex:{record.value_hex}"

    signed_value = int.from_bytes(data, byteorder=sys.byteorder, signed=True)
    unsigned_value = int.from_bytes(data, byteorder=sys.byteorder, signed=False)
    return str(signed_value) if signed_value == unsigned_value else f"{signed_value} (unsigned={unsigned_value})"


def classify_group_status(records: Sequence[FinalRecord | None]) -> str:
    """Use discovery labels first, then compare final values across secret inputs."""
    present = [record for record in records if record is not None]
    if any(record.label != 0 for record in present):
        return "DIRECT"
    if len(present) != len(records) or not records:
        return "INCOMPLETE"
    if len({record.value_hex for record in present}) > 1:
        return "IMPLICIT"
    return "NOT TAINTED"


def markdown_cell(value: str) -> str:
    return value.replace("|", r"\|").replace("\n", " ")


def write_debug_report(
    final_trace_dir: pathlib.Path,
    discovery_trace_dir: pathlib.Path,
    cases_path: pathlib.Path,
    source_path: pathlib.Path,
    report_path: pathlib.Path,
) -> None:
    column_names, cases = read_test_cases(cases_path)
    schema = _build_input_schema(column_names, cases, source_path)
    final_snapshots = read_snapshots(final_trace_dir, len(cases))
    discovery_snapshots = read_snapshots(discovery_trace_dir, len(cases))

    public_groups: Dict[InputTuple, List[Tuple[int, InputTuple]]] = {}
    for index, row in enumerate(cases):
        public_key, secret_key = _split_row_keys(row, schema)
        public_groups.setdefault(public_key, []).append((index, secret_key))

    all_variables = sorted(
        set().union(*(set(snapshot.keys()) for snapshot in final_snapshots)),
        key=lambda key: (key[0], key[1]),
    )
    report_path.parent.mkdir(parents=True, exist_ok=True)
    with report_path.open("w", encoding="utf-8") as out:
        out.write("END-OF-MAIN VALUE REPORT (SCALARS AND ARRAY ELEMENTS)\n")
        out.write("=====================================================\n\n")
        out.write(f"Unique public-input tuples: {len(public_groups)}\n")
        out.write(f"Unique secret-input tuples: {len({secret for entries in public_groups.values() for _, secret in entries})}\n")
        out.write(f"Program executions: {len(cases)}\n")
        out.write("Snapshot point: immediately before main returns\n")
        out.write("Each packed array is ONE input tuple value; it is public unless the whole array is directly labeled.\n")
        out.write("At runtime, every existing array element is recorded and label-checked separately.\n")
        out.write("Array lengths may differ between executions; absent elements are shown as <not recorded>.\n\n")

        out.write("Input roles:\n")
        for name in column_names:
            if name in schema.array_names:
                role = (
                    "SECRET INPUT (whole array; elements label-checked separately)"
                    if name in schema.secret_names
                    else "PUBLIC INPUT (elements label-checked separately)"
                )
            else:
                role = "SECRET INPUT" if name in schema.secret_names else "PUBLIC INPUT"
            out.write(f"  {name}: {role}\n")

        out.write(
            "\nTaint status meanings: DIRECT = nonzero DFSan label in discovery run; "
            "IMPLICIT = final concrete value differs across secret tuples for the same public tuple "
            "while all compared discovery labels are zero; NOT TAINTED = stable value with zero label; "
            "INCOMPLETE = value absent in one or more compared executions.\n"
        )
        out.write("Statuses are based on discovery traces, before inferred implicit labels are applied.\n")

        for group_number, (public_key, entries) in enumerate(public_groups.items(), 1):
            public_values = ", ".join(f"{name}={value}" for name, value in public_key) or "<no public inputs>"
            execution_by_secret: Dict[InputTuple, int] = {}
            for execution_index, secret_key in entries:
                execution_by_secret.setdefault(secret_key, execution_index)
            secret_tuples_in_group = list(execution_by_secret)
            secret_ids = {key: f"S{i}" for i, key in enumerate(secret_tuples_in_group, 1)}

            out.write(f"\n\nPUBLIC GROUP {group_number}\n")
            out.write(f"Public inputs: {public_values}\n")
            out.write(f"Secret combinations: {len(secret_tuples_in_group)}\n\n")
            headers = ["Scope", "Variable"] + [secret_ids[key] for key in secret_tuples_in_group] + ["Taint status"]
            out.write("| " + " | ".join(headers) + " |\n")
            out.write("| " + " | ".join(["---"] * len(headers)) + " |\n")

            for variable_key in all_variables:
                values: List[str] = []
                baseline_records: List[FinalRecord | None] = []
                for secret_key in secret_tuples_in_group:
                    execution_index = execution_by_secret[secret_key]
                    final_record = final_snapshots[execution_index].get(variable_key)
                    baseline_records.append(discovery_snapshots[execution_index].get(variable_key))
                    values.append(
                        markdown_cell(format_concrete_value(final_record))
                        if final_record is not None else "<not recorded>"
                    )
                status = classify_group_status(baseline_records)
                fields = [variable_key[0], variable_key[1], *values, status]
                out.write("| " + " | ".join(markdown_cell(str(field)) for field in fields) + " |\n")


def analyze_final(trace_dir: pathlib.Path) -> List[Tuple[str, str, int, int]]:
    combined: DefaultDict[VariableKey, Tuple[int, int]] = collections.defaultdict(lambda: (0, 0))
    for trace_path in sorted(trace_dir.glob("case_*.trace")):
        with trace_path.open("r", encoding="utf-8") as fp:
            for raw in fp:
                line = raw.rstrip("\n")
                if not line:
                    continue
                parts = line.split("|")
                if parts[0] != "F":
                    continue
                if len(parts) != 7:
                    raise RuntimeError(f"invalid final record in {trace_path}: {line}")
                flags, label, scope, name = int(parts[1]), int(parts[3]), parts[4], parts[5]
                if scope not in {"main", "global"}:
                    continue
                key = (scope, name)
                old_label, old_flags = combined[key]
                combined[key] = (old_label | label, old_flags | flags)
    return sorted(
        [(scope, name, label, flags) for (scope, name), (label, flags) in combined.items()],
        key=lambda item: (item[0], item[1]),
    )


def print_results(results: Iterable[Tuple[str, str, int, int]]) -> None:
    for scope, name, label, _flags in results:
        print(f"{scope}::{name} = label = {label}")


def main() -> int:
    try:
        if len(sys.argv) == 5 and sys.argv[1] == "--expand-cases":
            expand_test_cases(pathlib.Path(sys.argv[2]), pathlib.Path(sys.argv[3]), pathlib.Path(sys.argv[4]))
            return 0

        if len(sys.argv) == 5 and sys.argv[1] == "--write-argv":
            write_argv_cases(
                pathlib.Path(sys.argv[2]), pathlib.Path(sys.argv[3]),
                pathlib.Path(sys.argv[4]),
            )
            return 0

        if len(sys.argv) == 6 and sys.argv[1] == "--build-plan":
            trace_dir, plan_path = pathlib.Path(sys.argv[2]), pathlib.Path(sys.argv[3])
            if not trace_dir.is_dir():
                raise RuntimeError(f"trace directory does not exist: {trace_dir}")
            candidates = build_label_plan(
                trace_dir, plan_path, pathlib.Path(sys.argv[4]), pathlib.Path(sys.argv[5])
            )
            print(f"Inferred end-of-main implicit taint for {len(candidates)} variable(s).", file=sys.stderr)
            return 0

        if len(sys.argv) == 7 and sys.argv[1] == "--report":
            final_dir, discovery_dir = pathlib.Path(sys.argv[2]), pathlib.Path(sys.argv[3])
            if not final_dir.is_dir() or not discovery_dir.is_dir():
                raise RuntimeError("final and discovery trace directories must exist")
            write_debug_report(
                final_dir, discovery_dir, pathlib.Path(sys.argv[4]),
                pathlib.Path(sys.argv[5]), pathlib.Path(sys.argv[6]),
            )
            print(f"Wrote grouped final-value report: {sys.argv[6]}", file=sys.stderr)
            return 0

        if len(sys.argv) == 3 and sys.argv[1] == "--final":
            trace_dir = pathlib.Path(sys.argv[2])
            if not trace_dir.is_dir():
                raise RuntimeError(f"trace directory does not exist: {trace_dir}")
            print_results(analyze_final(trace_dir))
            return 0

        print(
            "usage: analyze_results.py --expand-cases <input-cases> <source-file> <output-cases> | "
            "--write-argv <logical-cases> <source-file> <argv-cases> | "
            "--build-plan <trace-dir> <plan-file> <test-cases-file> <source-file> | "
            "--report <final-trace-dir> <discovery-trace-dir> <test-cases-file> <source-file> <report-file> | "
            "--final <trace-dir>",
            file=sys.stderr,
        )
        return 2
    except Exception as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
