#!/usr/bin/env python3

import collections
import pathlib
import sys


# O|branch_id|outcome|context_label|flags|size|scope|name|value_hex
# F|flags|size|explicit_label|scope|name|value_hex


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

                    if scope != "main":
                        continue
                    if flags & 1:
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


def analyze(trace_dir: pathlib.Path):
    final_records, observations, branch_context_labels = read_traces(trace_dir)

    implicit_labels = collections.defaultdict(int)

    for (scope, name, branch_id), outcomes in observations.items():
        if not outcome_sets_differ(outcomes):
            continue

        implicit_labels[(scope, name)] |= branch_context_labels[branch_id]

    results = []

    for key, records in final_records.items():
        scope, name = key
        explicit_label = 0
        flags = records[-1][0]

        for record_flags, _size, record_label, _value in records:
            flags |= record_flags
            explicit_label |= record_label

        # Explicitly tainted variables keep their DFSan label. Implicit
        # variation analysis is only for variables that are not data-tainted.
        if explicit_label != 0:
            label = explicit_label
        else:
            label = implicit_labels[key]

        results.append((scope, name, label, flags))

    results.sort(key=lambda item: (item[0], item[1]))
    return results


def main():
    if len(sys.argv) != 2:
        print(f"usage: {sys.argv[0]} <trace-dir>", file=sys.stderr)
        return 1

    trace_dir = pathlib.Path(sys.argv[1])
    if not trace_dir.is_dir():
        print(f"error: trace directory does not exist: {trace_dir}", file=sys.stderr)
        return 1

    try:
        results = analyze(trace_dir)
    except Exception as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 1

    for scope, name, label, _flags in results:
        print(f"{scope}::{name} = label = {label}")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
