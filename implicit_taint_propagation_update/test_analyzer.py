#!/usr/bin/env python3
"""Small synthetic tests for role inference, grouping, and report rendering."""
import struct
import tempfile
import unittest
from pathlib import Path

import analyze_results as analyzer


class AnalyzerTests(unittest.TestCase):
    def test_input_roles_auto_detected_from_set_label(self):
        columns, rows = analyzer.read_test_cases(Path(__file__).with_name("test_cases.txt"))
        public, secret, names = analyzer.classify_inputs(columns, Path(__file__).with_name("test.c"))
        self.assertEqual([columns[i] for i in public], ["a", "b", "c"])
        self.assertEqual([columns[i] for i in secret], ["s1", "s2", "s3"])
        self.assertEqual(names, {"s1", "s2", "s3"})
        self.assertEqual(len(rows), 18)

    def test_expand_cases_crosses_public_and_secret_tuple_sets(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            cases_file = root / "sparse_cases.txt"
            source_file = root / "sample.c"
            output_file = root / "expanded.txt"
            cases_file.write_text("# inputs: a s1\n0 0\n1 1\n")
            source_file.write_text(
                "#include <sanitizer/dfsan_interface.h>\n"
                "int main(int argc, char **argv) { int a=0; int s1=1; "
                "dfsan_set_label(1, &s1, sizeof(s1)); return a+s1; }\n"
            )
            count = analyzer.expand_test_cases(cases_file, source_file, output_file)
            names, expanded = analyzer.read_test_cases(output_file)
            self.assertEqual(count, 4)
            self.assertEqual(names, ["a", "s1"])
            self.assertEqual(expanded, [("0", "0"), ("0", "1"), ("1", "0"), ("1", "1")])

    def test_plan_unions_candidates_across_public_groups_and_grouped_report(self):
        base = Path(__file__).parent
        columns, cases = analyzer.read_test_cases(base / "test_cases.txt")
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            discovery = root / "discovery"
            final = root / "final"
            discovery.mkdir()
            final.mkdir()
            for index, row in enumerate(cases):
                a, _b, _c, s1, s2, s3 = map(int, row)
                x = 20 if s1 else (10 if s2 else 0)
                ratio = float(a + (1 if s1 > 0 else 0)) / 2.0
                records = [
                    (0, 4, 0, "main", "a", a.to_bytes(4, "little", signed=True).hex()),
                    (0, 4, 0, "main", "x", x.to_bytes(4, "little", signed=True).hex()),
                    (0, 4, 0, "main", "y", (s2 + s3).to_bytes(4, "little", signed=True).hex()),
                    (2, 4, 0, "main", "ratio", struct.pack("=f", ratio).hex()),
                    (0, 4, 1, "main", "s1", s1.to_bytes(4, "little", signed=True).hex()),
                    (0, 4, 2, "main", "s2", s2.to_bytes(4, "little", signed=True).hex()),
                    (0, 4, 4, "main", "s3", s3.to_bytes(4, "little", signed=True).hex()),
                    (8, 4, 0, "global", "global_count", a.to_bytes(4, "little", signed=True).hex()),
                ]
                for traces in (discovery, final):
                    with (traces / f"case_{index}.trace").open("w") as out:
                        for flags, size, label, scope, name, val in records:
                            # Simulate the final pass adding an inferred label to x/y/ratio.
                            effective_label = label
                            if traces == final and (scope, name) in {
                                ("main", "x"), ("main", "y"), ("main", "ratio")
                            }:
                                effective_label |= 7
                            out.write(
                                f"F|{flags}|{size}|{effective_label}|{scope}|{name}|{val}\n"
                            )

            plan = root / "plan.txt"
            candidates = analyzer.build_label_plan(
                discovery, plan, base / "test_cases.txt", base / "test.c"
            )
            self.assertIn(("main", "x"), candidates)
            self.assertIn(("main", "y"), candidates)
            self.assertIn(("main", "ratio"), candidates)
            self.assertNotIn(("main", "a"), candidates)
            self.assertNotIn(("main", "s1"), candidates)
            self.assertIn("EXIT|7|main|x", plan.read_text())

            report = root / "report.md"
            analyzer.write_debug_report(
                final, discovery, base / "test_cases.txt", base / "test.c", report
            )
            content = report.read_text()
            self.assertIn("Unique public-input tuples: 3", content)
            self.assertIn("Unique secret-input tuples: 6", content)
            self.assertIn("PUBLIC GROUP 1", content)
            self.assertIn("| Scope | Variable | S1 | S2 | S3 | S4 | S5 | S6 | Taint status |", content)
            self.assertIn("| main | x |", content)
            self.assertIn("| main | x | 0 | 10 | 20 | 20 | 20 | 0 | IMPLICIT |", content)
            self.assertIn("| main | s1 |", content)
            self.assertIn("DIRECT", content)
            self.assertNotIn("CASE 18", content)
            self.assertNotIn("Secret-input combination:", content)
            self.assertIn("| global | global_count |", content)


if __name__ == "__main__":
    unittest.main()
