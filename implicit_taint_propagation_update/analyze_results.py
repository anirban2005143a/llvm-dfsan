#!/usr/bin/env python3
"""Infer end-of-main implicit taint from multiple concrete test executions.

The first non-comment input header must be written as:
    # inputs: a b c s1 s2

Input columns whose names match a scalar variable passed by address to
`dfsan_set_label(..., &variable, ...)` in the supplied C source are classified
as tainted. All other named input columns are classified as public.

Trace format:
    F|flags|size|explicit_label|scope|name|value_hex
Scopes recorded for this task are `main` and `global`.
"""
from __future__ import annotations

import collections
import pathlib
import re
import struct
import sys
from dataclasses import dataclass
from typing import DefaultDict, Dict, Iterable, List, Mapping, Sequence, Set, Tuple

FLAG_POINTER = 1 << 0
FLAG_FLOATING = 1 << 1
FLAG_GLOBAL = 1 << 3
VariableKey = Tuple[str, str]
InputRow = Tuple[str, ...]


@dataclass(frozen=True)
class FinalRecord:
    flags: int
    size: int
    label: int
    value_hex: str


def read_test_cases(path: pathlib.Path) -> Tuple[List[str], List[InputRow]]:
    """Read named test columns and unique concrete rows, preserving order."""
    column_names: List[str] | None = None
    cases: List[InputRow] = []
    seen: Set[InputRow] = set()

    with path.open("r", encoding="utf-8") as fp:
        for line_number, raw in enumerate(fp, 1):
            stripped = raw.strip()
            if not stripped:
                continue
            if stripped.startswith("#"):
                comment = stripped[1:].strip()
                match = re.match(r"^(?:inputs|columns)\s*:\s*(.*?)\s*$", comment, re.I)
                if match:
                    if column_names is not None:
                        raise RuntimeError(
                            f"{path}:{line_number}: input header appears more than once"
                        )
                    names = match.group(1).split()
                    if not names:
                        raise RuntimeError(f"{path}:{line_number}: input header is empty")
                    if any(not re.fullmatch(r"[A-Za-z_][A-Za-z0-9_]*", n) for n in names):
                        raise RuntimeError(
                            f"{path}:{line_number}: input names must be C identifiers"
                        )
                    if len(set(names)) != len(names):
                        raise RuntimeError(f"{path}:{line_number}: duplicate input name")
                    column_names = names
                continue

            content = raw.split("#", 1)[0].strip()
            if not content:
                continue
            values = tuple(content.split())
            if column_names is None:
                raise RuntimeError(
                    f"{path}:{line_number}: add a header such as '# inputs: a b s1' "
                    "before test-case rows"
                )
            if len(values) != len(column_names):
                raise RuntimeError(
                    f"{path}:{line_number}: found {len(values)} values; "
                    f"header names {len(column_names)} inputs"
                )
            # Duplicate concrete inputs are redundant for this dynamic analysis.
            if values not in seen:
                cases.append(values)
                seen.add(values)

    if column_names is None:
        raise RuntimeError(f"test-case file has no '# inputs: ...' header: {path}")
    if not cases:
        raise RuntimeError(f"test-case file contains no test cases: {path}")
    return column_names, cases


def _remove_c_comments_and_strings(source: str) -> str:
    # Strings are blanked before comments so comment-like text in string literals
    # cannot confuse the lightweight source scanner.
    source = re.sub(r'"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'', " ", source)
    source = re.sub(r"/\*.*?\*/|//[^\n]*", "", source, flags=re.S)
    return source


def _split_call_arguments(text: str) -> List[str]:
    args: List[str] = []
    start = 0
    parens = brackets = braces = 0
    for i, ch in enumerate(text):
        if ch == "(":
            parens += 1
        elif ch == ")":
            parens -= 1
        elif ch == "[":
            brackets += 1
        elif ch == "]":
            brackets -= 1
        elif ch == "{":
            braces += 1
        elif ch == "}":
            braces -= 1
        elif ch == "," and parens == 0 and brackets == 0 and braces == 0:
            args.append(text[start:i].strip())
            start = i + 1
    args.append(text[start:].strip())
    return args


def extract_dfsan_labelled_variables(source_path: pathlib.Path) -> Set[str]:
    """Extract direct scalar targets from source-level dfsan_set_label calls.

    Supports the usual forms `dfsan_set_label(label, &s1, sizeof(s1))` and
    simple casts around the address. Indirect pointer targets are intentionally
    not guessed: matching those to named input columns requires alias analysis.
    """
    source = _remove_c_comments_and_strings(source_path.read_text(encoding="utf-8"))
    labelled: Set[str] = set()
    call_pattern = re.compile(r"\bdfsan_set_label\s*\(")
    pos = 0
    while True:
        match = call_pattern.search(source, pos)
        if not match:
            break
        open_paren = source.find("(", match.start())
        depth = 1
        i = open_paren + 1
        while i < len(source) and depth:
            if source[i] == "(":
                depth += 1
            elif source[i] == ")":
                depth -= 1
            i += 1
        if depth != 0:
            raise RuntimeError(
                f"unclosed dfsan_set_label call near source offset {match.start()}"
            )
        args = _split_call_arguments(source[open_paren + 1 : i - 1])
        if len(args) >= 2:
            address_expr = re.sub(r"\s+", "", args[1])
            # Strip parentheses enclosing the entire expression, then strip
            # simple C casts (e.g. (void *)&s1 or ((void *)&s1)).
            changed = True
            while changed and address_expr.startswith("(") and address_expr.endswith(")"):
                depth = 0
                matching_outer = -1
                for index, char in enumerate(address_expr):
                    if char == "(":
                        depth += 1
                    elif char == ")":
                        depth -= 1
                        if depth == 0:
                            matching_outer = index
                            break
                if matching_outer == len(address_expr) - 1:
                    address_expr = address_expr[1:-1]
                    changed = True
                else:
                    changed = False
            cast_pattern = re.compile(
                r"^\((?:(?:const|volatile|restrict|signed|unsigned|short|long|void|char|int|float|double|[A-Za-z_][A-Za-z0-9_]*|\*)+)\)"
            )
            while True:
                cast_match = cast_pattern.match(address_expr)
                if not cast_match:
                    break
                address_expr = address_expr[cast_match.end():]
            direct = re.fullmatch(r"&([A-Za-z_][A-Za-z0-9_]*)", address_expr)
            if direct:
                labelled.add(direct.group(1))
        pos = i
    return labelled


def classify_inputs(
    column_names: Sequence[str], source_path: pathlib.Path
) -> Tuple[List[int], List[int], Set[str]]:
    labelled_names = extract_dfsan_labelled_variables(source_path)
    tainted_columns = [i for i, name in enumerate(column_names) if name in labelled_names]
    if not tainted_columns:
        found = ", ".join(sorted(labelled_names)) or "none detected"
        raise RuntimeError(
            "could not match any test-case input header name to a direct variable "
            f"target of dfsan_set_label; labelled source variables: {found}; "
            f"input columns: {', '.join(column_names)}. Ensure secret input names "
            "in '# inputs: ...' match the variable names passed as &variable."
        )
    public_columns = [i for i in range(len(column_names)) if i not in set(tainted_columns)]
    return public_columns, tainted_columns, labelled_names



def expand_test_cases(
    input_path: pathlib.Path, source_path: pathlib.Path, output_path: pathlib.Path
) -> int:
    """Build the Cartesian product of unique public and secret input tuples.

    No new scalar values are invented: only the distinct public tuples and
    secret tuples already present in the user's test file are recombined.
    """
    column_names, cases = read_test_cases(input_path)
    public_columns, tainted_columns, _ = classify_inputs(column_names, source_path)

    public_tuples: List[Tuple[str, ...]] = []
    secret_tuples: List[Tuple[str, ...]] = []
    seen_public: Set[Tuple[str, ...]] = set()
    seen_secret: Set[Tuple[str, ...]] = set()
    for row in cases:
        public_tuple = tuple(row[i] for i in public_columns)
        secret_tuple = tuple(row[i] for i in tainted_columns)
        if public_tuple not in seen_public:
            seen_public.add(public_tuple)
            public_tuples.append(public_tuple)
        if secret_tuple not in seen_secret:
            seen_secret.add(secret_tuple)
            secret_tuples.append(secret_tuple)

    expanded: List[InputRow] = []
    seen_rows: Set[InputRow] = set()
    for public_tuple in public_tuples:
        for secret_tuple in secret_tuples:
            row_values = [""] * len(column_names)
            for pos, col in enumerate(public_columns):
                row_values[col] = public_tuple[pos]
            for pos, col in enumerate(tainted_columns):
                row_values[col] = secret_tuple[pos]
            row = tuple(row_values)
            if row not in seen_rows:
                expanded.append(row)
                seen_rows.add(row)

    output_path.parent.mkdir(parents=True, exist_ok=True)
    with output_path.open("w", encoding="utf-8") as out:
        out.write("# inputs: " + " ".join(column_names) + "\n")
        for row in expanded:
            out.write(" ".join(row) + "\n")
    print(
        f"Expanded {len(cases)} supplied row(s) into {len(expanded)} executions "
        f"({len(public_tuples)} public tuple(s) x {len(secret_tuples)} secret tuple(s)).",
        file=sys.stderr,
    )
    return len(expanded)

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
                record = FinalRecord(flags, size, label, value_hex)
                if key in records:
                    raise RuntimeError(
                        f"duplicate final snapshot for {scope}::{name} in {trace_path}"
                    )
                records[key] = record
        snapshots.append(records)
    return snapshots


def choose_implicit_label(
    snapshots: Sequence[Mapping[VariableKey, FinalRecord]]
) -> int:
    combined = 0
    for snapshot in snapshots:
        for record in snapshot.values():
            if not (record.flags & FLAG_POINTER):
                combined |= record.label
    return combined if combined != 0 else 1


def build_label_plan(
    trace_dir: pathlib.Path,
    plan_path: pathlib.Path,
    cases_path: pathlib.Path,
    source_path: pathlib.Path,
) -> List[VariableKey]:
    column_names, cases = read_test_cases(cases_path)
    public_columns, tainted_columns, _ = classify_inputs(column_names, source_path)
    snapshots = read_snapshots(trace_dir, len(cases))

    groups: DefaultDict[Tuple[str, ...], List[int]] = collections.defaultdict(list)
    secret_tuples: Dict[int, Tuple[str, ...]] = {}
    for index, row in enumerate(cases):
        public_key = tuple(row[i] for i in public_columns)
        secret_key = tuple(row[i] for i in tainted_columns)
        groups[public_key].append(index)
        secret_tuples[index] = secret_key

    candidates: Set[VariableKey] = set()
    eligible_groups = 0
    for public_key, indexes in groups.items():
        unique_secret_inputs = {secret_tuples[index] for index in indexes}
        if len(unique_secret_inputs) < 2:
            continue
        eligible_groups += 1
        common_keys = set(snapshots[indexes[0]])
        for index in indexes[1:]:
            common_keys.intersection_update(snapshots[index].keys())

        for key in common_keys:
            records = [snapshots[index][key] for index in indexes]
            if any(record.flags & FLAG_POINTER for record in records):
                continue
            # A nonzero DFSan label means explicit dataflow already marks it.
            if any(record.label != 0 for record in records):
                continue
            if len({record.value_hex for record in records}) > 1:
                candidates.add(key)

    if eligible_groups == 0:
        raise RuntimeError(
            "no public-input group contains two or more distinct secret-input "
            "combinations; check the input header and provided test cases"
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
        try:
            if record.size == 4 and len(data) == 4:
                return repr(struct.unpack("=f", data)[0])
            if record.size == 8 and len(data) == 8:
                return repr(struct.unpack("=d", data)[0])
        except (struct.error, ValueError):
            pass
        return f"hex:{record.value_hex}"

    signed_value = int.from_bytes(data, byteorder=sys.byteorder, signed=True)
    unsigned_value = int.from_bytes(data, byteorder=sys.byteorder, signed=False)
    if signed_value == unsigned_value:
        return str(signed_value)
    return f"{signed_value} (unsigned={unsigned_value})"


def classify_group_status(records: Sequence[FinalRecord | None]) -> str:
    """Classify one variable for one public-input group using discovery traces."""
    present = [record for record in records if record is not None]
    if any(record.label != 0 for record in present):
        # Use the first pass: labels added by our implicit plan are absent here.
        return "DIRECT"
    if len(present) != len(records) or not records:
        return "INCOMPLETE"
    if any(record.flags & FLAG_POINTER for record in present):
        return "UNSUPPORTED"
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
    """Write one variable/value table per unique public tuple.

    Runs remain internal to the analysis. The report does not print every
    public-secret Cartesian-product input row: it defines each unique secret
    tuple once, then shows each tracked variable's final value under that
    secret tuple inside each public group.
    """
    column_names, cases = read_test_cases(cases_path)
    public_columns, tainted_columns, labelled_names = classify_inputs(column_names, source_path)
    final_snapshots = read_snapshots(final_trace_dir, len(cases))
    discovery_snapshots = read_snapshots(discovery_trace_dir, len(cases))

    public_groups: Dict[Tuple[str, ...], List[int]] = {}
    secret_tuples: List[Tuple[str, ...]] = []
    seen_secret_tuples: Set[Tuple[str, ...]] = set()
    for index, row in enumerate(cases):
        public_key = tuple(row[i] for i in public_columns)
        secret_key = tuple(row[i] for i in tainted_columns)
        public_groups.setdefault(public_key, []).append(index)
        if secret_key not in seen_secret_tuples:
            seen_secret_tuples.add(secret_key)
            secret_tuples.append(secret_key)

    secret_ids = {secret_key: f"S{i}" for i, secret_key in enumerate(secret_tuples, 1)}
    # Map each public group and secret tuple to its execution index.
    group_execution: Dict[Tuple[str, ...], Dict[Tuple[str, ...], int]] = {}
    for public_key, indexes in public_groups.items():
        mapping: Dict[Tuple[str, ...], int] = {}
        for index in indexes:
            row = cases[index]
            mapping[tuple(row[i] for i in tainted_columns)] = index
        group_execution[public_key] = mapping

    all_variables: List[VariableKey] = sorted(
        set().union(*(set(snapshot.keys()) for snapshot in final_snapshots)),
        key=lambda key: (key[0], key[1]),
    )
    report_path.parent.mkdir(parents=True, exist_ok=True)
    with report_path.open("w", encoding="utf-8") as out:
        out.write("END-OF-MAIN SCALAR VALUE REPORT\n")
        out.write("===============================\n\n")
        out.write(f"Unique public-input tuples: {len(public_groups)}\n")
        out.write(f"Unique secret-input tuples: {len(secret_tuples)}\n")
        out.write(f"Program executions: {len(cases)}\n")
        out.write("Snapshot point: immediately before main returns\n")
        out.write("Tracked variables: globals and primitive scalar variables declared directly in main scope\n")
        out.write("Excluded: variables in nested main blocks, locals in other functions, pointers, arrays, structures, and other aggregates\n\n")

        out.write("Input roles inferred from dfsan_set_label calls:\n")
        out.write("  Secret-marked source variable names: " +
                  (", ".join(sorted(labelled_names)) if labelled_names else "none detected") + "\n")
        for idx, name in enumerate(column_names):
            role = "SECRET/TAINTED" if idx in set(tainted_columns) else "PUBLIC"
            out.write(f"  {name}: {role}\n")

        out.write("\nUnique secret-input tuples (reused for every public group):\n")
        secret_names = [column_names[i] for i in tainted_columns]
        for secret_key in secret_tuples:
            formatted = ", ".join(
                f"{name}={value}" for name, value in zip(secret_names, secret_key)
            ) or "<no secret inputs>"
            out.write(f"  {secret_ids[secret_key]}: {formatted}\n")

        out.write("\nTaint status meanings: DIRECT = nonzero DFSan label in discovery run; "
                  "IMPLICIT = final concrete value differs across secret tuples for this public tuple while baseline labels are zero; "
                  "NOT TAINTED = value is stable and baseline label is zero; INCOMPLETE = a value was not recorded.\n")
        out.write("The taint-status column is based on discovery traces, before inferred implicit labels are applied.\n")

        for group_number, (public_key, indexes) in enumerate(public_groups.items(), 1):
            public_values = ", ".join(
                f"{column_names[i]}={value}"
                for i, value in zip(public_columns, public_key)
            ) or "<no public inputs>"
            out.write("\n\nPUBLIC GROUP " + str(group_number) + "\n")
            out.write("Public inputs: " + public_values + "\n\n")

            table_headers = ["Scope", "Variable"] + [secret_ids[key] for key in secret_tuples] + ["Taint status"]
            out.write("| " + " | ".join(table_headers) + " |\n")
            out.write("| " + " | ".join(["---"] * len(table_headers)) + " |\n")
            row_mapping = group_execution[public_key]

            for variable_key in all_variables:
                values: List[str] = []
                baseline_records: List[FinalRecord | None] = []
                for secret_key in secret_tuples:
                    execution_index = row_mapping.get(secret_key)
                    if execution_index is None:
                        values.append("<missing run>")
                        baseline_records.append(None)
                        continue
                    final_record = final_snapshots[execution_index].get(variable_key)
                    baseline_record = discovery_snapshots[execution_index].get(variable_key)
                    values.append(
                        markdown_cell(format_concrete_value(final_record))
                        if final_record is not None else "<not recorded>"
                    )
                    baseline_records.append(baseline_record)

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
    results = [(scope, name, label, flags) for (scope, name), (label, flags) in combined.items()]
    return sorted(results, key=lambda item: (item[0], item[1]))


def print_results(results: Iterable[Tuple[str, str, int, int]]) -> None:
    for scope, name, label, _flags in results:
        print(f"{scope}::{name} = label = {label}")


def main() -> int:
    try:
        if len(sys.argv) == 5 and sys.argv[1] == "--expand-cases":
            input_path, source_path, output_path = map(pathlib.Path, sys.argv[2:5])
            expand_test_cases(input_path, source_path, output_path)
            return 0

        if len(sys.argv) == 6 and sys.argv[1] == "--build-plan":
            trace_dir, plan_path = pathlib.Path(sys.argv[2]), pathlib.Path(sys.argv[3])
            cases_path, source_path = pathlib.Path(sys.argv[4]), pathlib.Path(sys.argv[5])
            if not trace_dir.is_dir():
                raise RuntimeError(f"trace directory does not exist: {trace_dir}")
            candidates = build_label_plan(trace_dir, plan_path, cases_path, source_path)
            print(
                f"Inferred end-of-main implicit taint for {len(candidates)} variable(s).",
                file=sys.stderr,
            )
            return 0

        if len(sys.argv) == 7 and sys.argv[1] == "--report":
            final_trace_dir = pathlib.Path(sys.argv[2])
            discovery_trace_dir = pathlib.Path(sys.argv[3])
            cases_path = pathlib.Path(sys.argv[4])
            source_path, report_path = pathlib.Path(sys.argv[5]), pathlib.Path(sys.argv[6])
            if not final_trace_dir.is_dir():
                raise RuntimeError(f"final trace directory does not exist: {final_trace_dir}")
            if not discovery_trace_dir.is_dir():
                raise RuntimeError(f"discovery trace directory does not exist: {discovery_trace_dir}")
            write_debug_report(
                final_trace_dir, discovery_trace_dir, cases_path, source_path, report_path
            )
            print(f"Wrote grouped final-value report: {report_path}", file=sys.stderr)
            return 0

        if len(sys.argv) == 3 and sys.argv[1] == "--final":
            trace_dir = pathlib.Path(sys.argv[2])
            if not trace_dir.is_dir():
                raise RuntimeError(f"trace directory does not exist: {trace_dir}")
            print_results(analyze_final(trace_dir))
            return 0

        print(
            "usage: analyze_results.py --expand-cases <input-cases> <source-file> <output-cases> | "
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
