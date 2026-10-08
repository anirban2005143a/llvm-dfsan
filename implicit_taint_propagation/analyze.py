# // FILE: analyze.py
# // REPLACE THE ENTIRE FILE WITH THIS

import sys
from collections import defaultdict


def main() -> int:
    if len(sys.argv) != 3:
        return 1

    observations_file = sys.argv[1]
    taint_map_file = sys.argv[2]

    observations = defaultdict(list)

    with open(
        observations_file,
        "r",
        encoding="utf-8",
    ) as f:

        for line in f:

            line = line.rstrip("\n")

            if not line:
                continue

            parts = line.split("\t")

            if len(parts) != 9:
                continue

            (
                test_id,
                join_id,
                variable_id,
                variable,
                source_line,
                source_col,
                path_hash,
                control_mask,
                value,
            ) = parts

            try:
                item = {
                    "test_id": int(test_id),
                    "join_id": int(join_id),
                    "variable_id": int(variable_id),
                    "variable": variable,
                    "line": int(source_line),
                    "col": int(source_col),
                    "path": int(path_hash),
                    "mask": int(control_mask),
                    "value": int(value),
                }
            except ValueError:
                continue

            key = (
                item["join_id"],
                item["variable_id"],
            )

            observations[key].append(item)

    results = []

    for key, items in observations.items():

        items.sort(
            key=lambda x: x["test_id"]
        )

        baseline = items[0]

        seen_paths = {
            baseline["path"]
        }

        final_label = 0

        for current in items[1:]:

            if current["path"] in seen_paths:
                continue

            seen_paths.add(
                current["path"]
            )

            if (
                current["value"]
                != baseline["value"]
            ):
                final_label |= (
                    baseline["mask"]
                    | current["mask"]
                )

        if final_label == 0:
            continue

        join_id, variable_id = key

        results.append(
            (
                join_id,
                variable_id,
                final_label,
                baseline["line"],
                baseline["col"],
                baseline["variable"],
            )
        )

    results.sort()

    with open(
        taint_map_file,
        "w",
        encoding="utf-8",
    ) as f:

        for (
            join_id,
            variable_id,
            label,
            line,
            col,
            variable,
        ) in results:

            f.write(
                f"{join_id}\t"
                f"{variable_id}\t"
                f"{label}\n"
            )

            print(
                f"line={line} "
                f"col={col} "
                f"variable={variable} "
                f"label={label}"
            )

    return 0


if __name__ == "__main__":
    raise SystemExit(main())