#!/usr/bin/env python3

import collections
import pathlib
import sys


# O|branch_id|outcome|context_label|flags|size|scope|name|value_hex
# F|flags|size|explicit_label|scope|name|value_hex
# Planned labels: branch_id|label|scope|name


def read_traces(trace_dir: pathlib.Path):
    final_records = collections.defaultdict(list)
    observations = collections.defaultdict(lambda: collections.defaultdict(set))
    branch_context_labels = collections.defaultdict(int)

    for trace_path in sorted(trace_dir.glob("*.trace")):
        with trace_path.open("r", encoding="utf-8") as fp:
            for raw in fp:
                line = raw.rstrip("\n")
                if not line:
                    continue

                parts = line.split("|")
                kind = parts[0]

                if kind == "O":
                    if len(parts) != 9:
                        raise RuntimeError(f"invalid observation record: {line}")

                    branch_id = int(parts[1])
                    outcome = int(parts[2])
                    context_label = int(parts[3])
                    flags = int(parts[4])
                    size = int(parts[5])
                    scope = parts[6]
                    name = parts[7]
                    value = parts[8]

                    if scope != "main" or (flags & 1):
                        continue

                    key = (scope, name, branch_id)
                    observations[key][outcome].add((size, value))
                    branch_context_labels[branch_id] |= context_label

                elif kind == "F":
                    if len(parts) != 7:
                        raise RuntimeError(f"invalid final record: {line}")

                    flags = int(parts[1])
                    size = int(parts[2])
                    explicit_label = int(parts[3])
                    scope = parts[4]
                    name = parts[5]
                    value = parts[6]

                    if scope != "main":
                        continue

                    final_records[(scope, name)].append(
                        (flags, size, explicit_label, value)
                    )

    return final_records, observations, branch_context_labels


def outcome_sets_differ(outcomes):
    if len(outcomes) < 2:
        return False

    sets = list(outcomes.values())
    first = sets[0]
    return any(current != first for current in sets[1:])


def explicit_labels_by_variable(final_records):
    labels = collections.defaultdict(int)
    for key, records in final_records.items():
        for _flags, _size, label, _value in records:
            labels[key] |= label
    return labels


def build_label_plan(trace_dir: pathlib.Path, plan_path: pathlib.Path):
    final_records, observations, branch_context_labels = read_traces(trace_dir)
    explicit_labels = explicit_labels_by_variable(final_records)
    planned = collections.defaultdict(int)

    for (scope, name, branch_id), outcomes in observations.items():
        if not outcome_sets_differ(outcomes):
            continue

        # Keep the original rule: infer implicit labels only for variables
        # whose ordinary DFSan label is zero at program exit in the discovery pass.
        if explicit_labels[(scope, name)] != 0:
            continue

        label = branch_context_labels[branch_id]
        if label == 0:
            continue

        planned[(branch_id, scope, name)] |= label

    plan_path.parent.mkdir(parents=True, exist_ok=True)
    with plan_path.open("w", encoding="utf-8") as fp:
        for (branch_id, scope, name), label in sorted(planned.items()):
            fp.write(f"{branch_id}|{label}|{scope}|{name}\n")


def analyze_final(trace_dir: pathlib.Path):
    final_records, _observations, _branch_context_labels = read_traces(trace_dir)
    results = []

    for (scope, name), records in final_records.items():
        label = 0
        flags = 0
        for record_flags, _size, record_label, _value in records:
            flags |= record_flags
            label |= record_label
        results.append((scope, name, label, flags))

    results.sort(key=lambda item: (item[0], item[1]))
    return results


def analyze_discovery(trace_dir: pathlib.Path):
    final_records, observations, branch_context_labels = read_traces(trace_dir)
    implicit_labels = collections.defaultdict(int)

    for (scope, name, branch_id), outcomes in observations.items():
        if outcome_sets_differ(outcomes):
            implicit_labels[(scope, name)] |= branch_context_labels[branch_id]

    results = []
    for key, records in final_records.items():
        scope, name = key
        explicit_label = 0
        flags = 0

        for record_flags, _size, record_label, _value in records:
            flags |= record_flags
            explicit_label |= record_label

        label = explicit_label if explicit_label != 0 else implicit_labels[key]
        results.append((scope, name, label, flags))

    results.sort(key=lambda item: (item[0], item[1]))
    return results


def print_results(results):
    for scope, name, label, _flags in results:
        print(f"{scope}::{name} = label = {label}")


def main():
    if len(sys.argv) == 4 and sys.argv[1] == "--build-plan":
        trace_dir = pathlib.Path(sys.argv[2])
        plan_path = pathlib.Path(sys.argv[3])
        if not trace_dir.is_dir():
            print(f"error: trace directory does not exist: {trace_dir}", file=sys.stderr)
            return 1
        try:
            build_label_plan(trace_dir, plan_path)
        except Exception as exc:
            print(f"error: {exc}", file=sys.stderr)
            return 1
        return 0

    if len(sys.argv) == 3 and sys.argv[1] == "--final":
        trace_dir = pathlib.Path(sys.argv[2])
        if not trace_dir.is_dir():
            print(f"error: trace directory does not exist: {trace_dir}", file=sys.stderr)
            return 1
        try:
            print_results(analyze_final(trace_dir))
        except Exception as exc:
            print(f"error: {exc}", file=sys.stderr)
            return 1
        return 0

    if len(sys.argv) != 2:
        print(
            "usage: analyze_results.py <trace-dir> | "
            "analyze_results.py --build-plan <trace-dir> <plan-file> | "
            "analyze_results.py --final <trace-dir>",
            file=sys.stderr,
        )
        return 1

    trace_dir = pathlib.Path(sys.argv[1])
    if not trace_dir.is_dir():
        print(f"error: trace directory does not exist: {trace_dir}", file=sys.stderr)
        return 1

    try:
        print_results(analyze_discovery(trace_dir))
    except Exception as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 1

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
