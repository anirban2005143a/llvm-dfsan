#!/usr/bin/env python3
"""Read KLEE .ktest files and emit concrete command-line argument reports."""
from __future__ import annotations
import argparse
import csv
import json
import re
import shlex
import struct
import subprocess
import sys
from pathlib import Path
from typing import Any


def read_u32(data: bytes, offset: int) -> tuple[int, int]:
    if offset + 4 > len(data):
        raise ValueError("unexpected end of .ktest file while reading uint32")
    return struct.unpack_from(">I", data, offset)[0], offset + 4


def read_blob(data: bytes, offset: int) -> tuple[bytes, int]:
    size, offset = read_u32(data, offset)
    end = offset + size
    if end > len(data):
        raise ValueError("unexpected end of .ktest file while reading byte string")
    return data[offset:end], end


def parse_ktest(path: Path) -> dict[str, bytes]:
    data = path.read_bytes()
    if not data.startswith(b"KTEST"):
        raise ValueError(f"{path.name}: invalid KTEST magic")
    offset = 5
    version, offset = read_u32(data, offset)
    if version > 3:
        raise ValueError(f"{path.name}: unsupported KTEST version {version}")
    num_args, offset = read_u32(data, offset)
    for _ in range(num_args):
        _, offset = read_blob(data, offset)
    if version >= 2:
        _, offset = read_u32(data, offset)  # symArgvs
        _, offset = read_u32(data, offset)  # symArgvLen
    num_objects, offset = read_u32(data, offset)
    objects: dict[str, bytes] = {}
    for _ in range(num_objects):
        name_raw, offset = read_blob(data, offset)
        value, offset = read_blob(data, offset)
        name = name_raw.decode("utf-8", errors="strict")
        objects[name] = value
    return objects


def run_native(executable: Path, values: list[int]) -> tuple[int | None, str, str]:
    command = [str(executable), *(str(v) for v in values)]
    try:
        proc = subprocess.run(command, text=True, capture_output=True, timeout=15)
        return proc.returncode, proc.stdout, proc.stderr
    except subprocess.TimeoutExpired as exc:
        return None, exc.stdout or "", (exc.stderr or "") + "\nNATIVE REPLAY TIMEOUT"
    except OSError as exc:
        return None, "", f"NATIVE REPLAY ERROR: {exc}"


def parse_klee_counts(log_text: str) -> dict[str, str]:
    patterns = {
        "completed_paths": r"KLEE: done: completed paths\s*=\s*(\d+)",
        "partially_completed_paths": r"KLEE: done: partially completed paths\s*=\s*(\d+)",
        "generated_tests_reported_by_klee": r"KLEE: done: generated tests\s*=\s*(\d+)",
        "explored_paths": r"KLEE: done: explored paths\s*=\s*(\d+)",
    }
    result = {key: "not reported" for key in patterns}
    for key, pattern in patterns.items():
        match = re.search(pattern, log_text)
        if match:
            result[key] = match.group(1)
    result["klee_done_marker"] = "yes" if "KLEE: done:" in log_text else "no"
    return result


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--klee-out", type=Path, required=True)
    ap.add_argument("--native-exe", type=Path, required=True)
    ap.add_argument("--report-dir", type=Path, required=True)
    ap.add_argument("--arg-count", type=int, required=True)
    ap.add_argument("--klee-log", type=Path, required=True)
    ap.add_argument("--klee-exit", type=int, required=True)
    args = ap.parse_args()

    args.report_dir.mkdir(parents=True, exist_ok=True)
    ktests = sorted(args.klee_out.glob("test*.ktest"))
    records: list[dict[str, Any]] = []
    errors: list[str] = []
    expected_size = args.arg_count * 4

    for number, path in enumerate(ktests, start=1):
        try:
            objects = parse_ktest(path)
            raw = objects.get("argv_values")
            if raw is None:
                raise ValueError("symbolic object 'argv_values' not found")
            if len(raw) != expected_size:
                raise ValueError(f"argv_values has {len(raw)} bytes; expected {expected_size}")
            # KLEE is compiled for the user's x86-64 WSL host; int32_t is 4 bytes.
            values = list(struct.unpack("<" + "i" * args.arg_count, raw))
            return_code, stdout, stderr = run_native(args.native_exe.resolve(), values)
            records.append({
                "case_id": number,
                "ktest_file": path.name,
                "inputs": values,
                "argv": values,
                "native_replay_exit_code": return_code,
                "replay_command": shlex.join([str(args.native_exe.resolve()), *(str(v) for v in values)]),
                "native_stdout": stdout,
                "native_stderr": stderr,
            })
        except Exception as exc:
            errors.append(f"{path.name}: {exc}")

    (args.report_dir / "test_cases.json").write_text(json.dumps(records, indent=2), encoding="utf-8")
    if records:
        columns = ["case_id", "ktest_file"] + [f"arg{i}" for i in range(1, args.arg_count + 1)] + ["native_replay_exit_code", "replay_command"]
        with (args.report_dir / "test_cases.csv").open("w", newline="", encoding="utf-8") as handle:
            writer = csv.DictWriter(handle, fieldnames=columns)
            writer.writeheader()
            for record in records:
                row = {"case_id": record["case_id"], "ktest_file": record["ktest_file"],
                       "native_replay_exit_code": record["native_replay_exit_code"],
                       "replay_command": record["replay_command"]}
                row.update({f"arg{i+1}": value for i, value in enumerate(record["inputs"])})
                writer.writerow(row)
    else:
        (args.report_dir / "test_cases.csv").write_text("case_id,ktest_file\n", encoding="utf-8")

    lines = ["KLEE CONCRETE TEST CASES", "========================", f"Cases decoded: {len(records)}", ""]
    for record in records:
        lines.append(f"TEST CASE {record['case_id']} ({record['ktest_file']})")
        lines.append("  inputs: " + " ".join(str(v) for v in record["inputs"]))
        lines.append(f"  native replay exit code: {record['native_replay_exit_code']}")
        lines.append(f"  replay: {record['replay_command']}")
        if record["native_stderr"].strip():
            lines.append("  native stderr: " + record["native_stderr"].strip().replace("\n", " | "))
        lines.append("")
    if errors:
        lines.extend(["UNDECODED TEST CASES", "--------------------", *errors, ""])
    (args.report_dir / "test_cases.txt").write_text("\n".join(lines), encoding="utf-8")

    log_text = args.klee_log.read_text(encoding="utf-8", errors="replace") if args.klee_log.exists() else ""
    counts = parse_klee_counts(log_text)
    reported_tests = counts["generated_tests_reported_by_klee"]
    generated_count_matches = (
        reported_tests != "not reported" and int(reported_tests) == len(ktests)
    )
    all_replays_finished = all(record["native_replay_exit_code"] is not None for record in records)
    complete = (
        args.klee_exit == 0
        and counts["klee_done_marker"] == "yes"
        and counts["partially_completed_paths"] == "0"
        and generated_count_matches
        and len(records) == len(ktests)
        and not errors
        and all_replays_finished
    )
    summary = {
        "source_argument_count": args.arg_count,
        "symbolic_input_type": "signed 32-bit integers",
        "test_cases_found": len(ktests),
        "test_cases_decoded": len(records),
        "decode_errors": errors,
        "klee_exit_code": args.klee_exit,
        "klee_finished_with_done_marker": counts["klee_done_marker"],
        "bounded_exploration_complete": complete,
        **counts,
        "claim_scope": "All KLEE-completed feasible paths for this fixed-argc, signed-int argv model only. Not a proof for unsupported input mechanisms or paths KLEE could not model.",
    }
    (args.report_dir / "summary.json").write_text(json.dumps(summary, indent=2), encoding="utf-8")
    summary_lines = [
        "KLEE RUN SUMMARY", "================",
        f"Fixed command-line argument count: {args.arg_count}",
        "Each argument: signed 32-bit integer, rendered as decimal text for native replay.",
        f"KLEE exit code: {args.klee_exit}",
        f"KLEE done marker: {counts['klee_done_marker']}",
        f"KLEE completed paths: {counts['completed_paths']}",
        f"KLEE partially completed paths: {counts['partially_completed_paths']}",
        f"KLEE explored paths: {counts['explored_paths']}",
        f"KLEE generated tests (reported): {counts['generated_tests_reported_by_klee']}",
        f"KLEE generated-test count matches .ktest files: {'YES' if generated_count_matches else 'NO / NOT CONFIRMED'}",
        f".ktest files found: {len(ktests)}",
        f"Concrete test cases decoded: {len(records)}",
        f"Decode errors: {len(errors)}",
        f"Bounded exploration complete: {'YES' if complete else 'NO / NOT CONFIRMED'}",
        "",
        "Treat coverage as complete only when KLEE exits normally, prints its done summary, has no partially completed states, and warnings/errors have been reviewed.",
        "KLEE completion is within this fixed argument-count and signed 32-bit numeric-input model.",
    ]
    (args.report_dir / "summary.txt").write_text("\n".join(summary_lines) + "\n", encoding="utf-8")
    print(f"Decoded {len(records)} concrete test cases from {len(ktests)} .ktest files.")
    print(f"Report: {args.report_dir / 'summary.txt'}")
    return 0 if not errors else 2


if __name__ == "__main__":
    raise SystemExit(main())
