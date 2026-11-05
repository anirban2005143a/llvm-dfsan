#!/usr/bin/env python3
"""Infer a fixed argv count from a Clang JSON AST for conventional C main()."""
from __future__ import annotations
import argparse
import json
import shlex
import subprocess
import sys
from pathlib import Path
from typing import Any, Iterator


def walk(node: Any) -> Iterator[dict[str, Any]]:
    if isinstance(node, dict):
        yield node
        for child in node.get("inner", []):
            yield from walk(child)
    elif isinstance(node, list):
        for child in node:
            yield from walk(child)


def has_declref(node: Any, name: str) -> bool:
    return any(
        n.get("kind") == "DeclRefExpr"
        and n.get("referencedDecl", {}).get("name") == name
        for n in walk(node)
    )


def integer_constant(node: Any) -> int | None:
    """Evaluate the integer-only constant-expression subset used in argc guards/macros."""
    if not isinstance(node, dict):
        return None
    kind = node.get("kind")
    if kind == "IntegerLiteral" and "value" in node:
        try:
            return int(str(node["value"]), 0)
        except ValueError:
            try:
                return int(str(node["value"]), 10)
            except ValueError:
                return None
    if kind == "ConstantExpr" and "value" in node:
        try:
            return int(str(node["value"]), 0)
        except ValueError:
            pass
    inner = node.get("inner", [])
    if kind in ("CStyleCastExpr", "ImplicitCastExpr", "ParenExpr", "ConstantExpr") and len(inner) == 1:
        return integer_constant(inner[0])
    if kind == "UnaryOperator" and inner:
        value = integer_constant(inner[0])
        if value is None:
            return None
        op = node.get("opcode")
        return {"-": lambda: -value, "+": lambda: value, "~": lambda: ~value}.get(op, lambda: None)()
    if kind == "BinaryOperator" and len(inner) >= 2:
        left, right = integer_constant(inner[0]), integer_constant(inner[1])
        if left is None or right is None:
            return None
        op = node.get("opcode")
        operations = {
            "+": lambda: left + right, "-": lambda: left - right,
            "*": lambda: left * right, "<<": lambda: left << right,
            ">>": lambda: left >> right, "|": lambda: left | right,
            "&": lambda: left & right, "^": lambda: left ^ right,
        }
        try:
            return operations[op]() if op in operations else None
        except (ArithmeticError, ValueError):
            return None
    return None


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("source", type=Path)
    ap.add_argument("--clang", required=True)
    ap.add_argument("--arg-count", type=int, default=None)
    ap.add_argument("--cppflags", default="")
    args = ap.parse_args()

    source = args.source.resolve()
    if not source.is_file():
        print(f"ERROR: source does not exist: {source}", file=sys.stderr)
        return 2

    command = [args.clang, "-std=gnu11", "-fsyntax-only", "-Xclang", "-ast-dump=json"]
    command += shlex.split(args.cppflags)
    command += [str(source)]
    proc = subprocess.run(command, text=True, capture_output=True)
    if proc.returncode != 0:
        print("ERROR: Clang could not parse the source for input discovery.", file=sys.stderr)
        if proc.stderr:
            print(proc.stderr.strip(), file=sys.stderr)
        return 2
    try:
        ast = json.loads(proc.stdout)
    except json.JSONDecodeError as exc:
        print(f"ERROR: could not parse Clang AST JSON: {exc}", file=sys.stderr)
        return 2

    nodes = list(walk(ast))
    vla_nodes = [n for n in nodes if n.get("kind") == "VariableArrayType"]
    if vla_nodes:
        print("ERROR: variable-length array detected. This generator assumes compile-time fixed-size arrays.", file=sys.stderr)
        return 2

    mains = [n for n in nodes if n.get("kind") == "FunctionDecl" and n.get("name") == "main"]
    definitions = [n for n in mains if any(c.get("kind") == "CompoundStmt" for c in n.get("inner", []))]
    if len(definitions) != 1:
        print("ERROR: expected exactly one definition of int main(int argc, char **argv).", file=sys.stderr)
        return 2
    main_node = definitions[0]
    params = [c for c in main_node.get("inner", []) if c.get("kind") == "ParmVarDecl"]
    if len(params) != 2 or not params[0].get("name") or not params[1].get("name"):
        print("ERROR: expected main to have exactly two named parameters: argc and argv.", file=sys.stderr)
        return 2
    argc_name, argv_name = params[0]["name"], params[1]["name"]
    body = next(c for c in main_node.get("inner", []) if c.get("kind") == "CompoundStmt")

    max_argv_index = 0
    dynamic_argv_index = False
    argc_exact_values: set[int] = set()
    has_atoi_argv_call = False
    for n in walk(body):
        if n.get("kind") == "ArraySubscriptExpr":
            children = n.get("inner", [])
            if len(children) >= 2 and has_declref(children[0], argv_name):
                idx = integer_constant(children[1])
                if idx is None:
                    dynamic_argv_index = True
                elif idx >= 0:
                    max_argv_index = max(max_argv_index, idx)
        if n.get("kind") == "CallExpr":
            call_children = n.get("inner", [])
            if len(call_children) >= 2 and has_declref(call_children[0], "atoi"):
                if any(
                    child.get("kind") == "ArraySubscriptExpr"
                    and len(child.get("inner", [])) >= 2
                    and has_declref(child["inner"][0], argv_name)
                    for child in walk(call_children[1])
                ):
                    has_atoi_argv_call = True
        if n.get("kind") == "BinaryOperator" and n.get("opcode") in ("==", "!="):
            children = n.get("inner", [])
            if len(children) >= 2:
                if has_declref(children[0], argc_name):
                    val = integer_constant(children[1])
                elif has_declref(children[1], argc_name):
                    val = integer_constant(children[0])
                else:
                    val = None
                if val is not None and val >= 1:
                    argc_exact_values.add(val - 1)

    if not has_atoi_argv_call:
        print(
            "ERROR: this version expects numeric command-line inputs parsed with atoi(argv[index]). "
            "A different input parser needs a matching adapter.",
            file=sys.stderr,
        )
        return 2

    if args.arg_count is not None:
        if args.arg_count < 1:
            print("ERROR: --arg-count must be at least 1.", file=sys.stderr)
            return 2
        if argc_exact_values and args.arg_count not in argc_exact_values:
            expected = ", ".join(str(value) for value in sorted(argc_exact_values))
            print(
                f"ERROR: ARG_COUNT={args.arg_count} conflicts with the source's exact argc check(s), "
                f"which imply: {expected} user arguments.",
                file=sys.stderr,
            )
            return 2
        if max_argv_index > args.arg_count:
            print(
                f"ERROR: source accesses argv[{max_argv_index}], but ARG_COUNT={args.arg_count} is too small.",
                file=sys.stderr,
            )
            return 2
        count = args.arg_count
        method = "user-specified"
    else:
        if len(argc_exact_values) > 1:
            print(
                "ERROR: multiple different exact argc checks were found. "
                "This source may support multiple argument layouts; supply ARG_COUNT explicitly for one layout.",
                file=sys.stderr,
            )
            return 2
        inferred_exact = next(iter(argc_exact_values), None)
        if inferred_exact is not None:
            if max_argv_index > inferred_exact:
                print(
                    f"ERROR: source indexes argv[{max_argv_index}] but an exact argc check implies only {inferred_exact} user arguments.",
                    file=sys.stderr,
                )
                return 2
            count = max(inferred_exact, max_argv_index)
            method = "exact argc check"
        elif max_argv_index > 0 and not dynamic_argv_index:
            count = max_argv_index
            method = "largest constant argv index"
        else:
            print(
                "ERROR: could not safely infer a fixed argument count. "
                "Use ARG_COUNT=N ./run.sh /path/to/source.c for this source.",
                file=sys.stderr,
            )
            return 2

    print(f"ARG_COUNT={count}")
    print(f"INFERENCE_METHOD={method}")
    if dynamic_argv_index and args.arg_count is None and not argc_exact_values:
        print("WARNING: dynamic argv indexing detected; the chosen count may be insufficient.", file=sys.stderr)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
