#!/usr/bin/env python3
"""Regression tests for public/secret array-input classification."""
import struct
import tempfile
import unittest
from pathlib import Path

import analyze_results as analyzer


SOURCE_TEMPLATE = r'''#include <sanitizer/dfsan_interface.h>
#include <stdlib.h>
int main(int argc, char **argv) {
    int public1 = atoi(argv[1]);
    int secret1 = atoi(argv[2]);
    int secret2 = atoi(argv[3]);
    int n = atoi(argv[4]);
    int arr[n];
    dfsan_set_label(1, &secret1, sizeof(secret1));
    dfsan_set_label(2, &secret2, sizeof(secret2));
    {array_label}
    return 0;
}
'''

CASES = '''# inputs: public1 secret1 secret2 n arr
0|10|20|2|1,2
0|-10|20|2|1,2
1|10|20|2|3,4
1|-10|20|2|3,4
'''


class AnalyzerTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.root = Path(self.temp.name)
        self.source_path = self.root / "test.c"
        self.cases_path = self.root / "test_cases.txt"
        self.cases_path.write_text(CASES, encoding="utf-8")

    def tearDown(self):
        self.temp.cleanup()

    def write_source(self, array_label=True):
        label = (
            "dfsan_set_label(4, arr, sizeof(arr));"
            if array_label else ""
        )
        self.source_path.write_text(
            SOURCE_TEMPLATE.replace("{array_label}", label), encoding="utf-8"
        )

    def test_whole_array_label_makes_array_a_secret_input(self):
        self.write_source(array_label=True)
        columns, rows = analyzer.read_test_cases(self.cases_path)
        schema = analyzer._build_input_schema(columns, rows, self.source_path)

        self.assertEqual(schema.array_names, {"arr"})
        self.assertEqual(schema.secret_names, {"secret1", "secret2", "arr"})
        public, secret = analyzer._split_row_keys(rows[0], schema)
        self.assertNotIn("arr", dict(public))
        self.assertEqual(dict(secret)["arr"], "1,2")

    def test_unlabelled_array_is_a_public_input(self):
        self.write_source(array_label=False)
        columns, rows = analyzer.read_test_cases(self.cases_path)
        schema = analyzer._build_input_schema(columns, rows, self.source_path)

        self.assertEqual(schema.array_names, {"arr"})
        self.assertEqual(schema.secret_names, {"secret1", "secret2"})
        public, secret = analyzer._split_row_keys(rows[0], schema)
        self.assertEqual(dict(public)["arr"], "1,2")
        self.assertNotIn("arr", dict(secret))

    def test_unlabelled_array_participates_in_public_tuple_cartesian_expansion(self):
        self.write_source(array_label=False)
        expanded_path = self.root / "expanded.txt"
        count = analyzer.expand_test_cases(
            self.cases_path, self.source_path, expanded_path
        )
        columns, rows = analyzer.read_test_cases(expanded_path)

        self.assertEqual(count, 4)
        self.assertEqual(len(rows), 4)
        self.assertEqual(columns, ["public1", "secret1", "secret2", "n", "arr"])
        self.assertEqual({row[-1] for row in rows}, {"1,2", "3,4"})

        argv_path = self.root / "argv.txt"
        self.assertEqual(
            analyzer.write_argv_cases(expanded_path, self.source_path, argv_path), 4
        )
        argv_rows = [line.split("|") for line in argv_path.read_text().splitlines()]
        self.assertIn(["0", "10", "20", "2", "1", "2"], argv_rows)
        self.assertIn(["1", "-10", "20", "2", "3", "4"], argv_rows)

    def test_partial_array_labeling_is_rejected(self):
        self.write_source(array_label=False)
        source = self.source_path.read_text(encoding="utf-8")
        source = source.replace(
            "    return 0;",
            "    dfsan_set_label(4, &arr[0], sizeof(arr[0]));\n    return 0;",
        )
        self.source_path.write_text(source, encoding="utf-8")
        columns, rows = analyzer.read_test_cases(self.cases_path)
        with self.assertRaisesRegex(RuntimeError, "partial array labeling"):
            analyzer._build_input_schema(columns, rows, self.source_path)

    def test_variable_length_array_must_be_last_input_column(self):
        self.write_source(array_label=False)
        columns, rows = analyzer.read_test_cases(self.cases_path)
        reordered_columns = ["arr", "public1", "secret1", "secret2", "n"]
        reordered_rows = [
            (row[4], row[0], row[1], row[2], row[3]) for row in rows
        ]
        with self.assertRaisesRegex(RuntimeError, "must be the last test-case column"):
            analyzer._build_input_schema(
                reordered_columns, reordered_rows, self.source_path
            )

    def test_implicit_comparison_uses_unlabelled_array_as_public_group_key(self):
        self.write_source(array_label=False)
        expanded_path = self.root / "expanded.txt"
        analyzer.expand_test_cases(self.cases_path, self.source_path, expanded_path)
        _, rows = analyzer.read_test_cases(expanded_path)
        trace_dir = self.root / "traces"
        trace_dir.mkdir()

        for index, row in enumerate(rows):
            secret1 = int(row[1])
            implicit_value = 100 if secret1 > 0 else 200
            direct_value = secret1
            records = [
                self._record("implicit_value", implicit_value, label=0),
                self._record("direct_value", direct_value, label=4),
            ]
            (trace_dir / f"case_{index}.trace").write_text(
                "".join(records), encoding="utf-8"
            )

        plan_path = self.root / "implicit_labels.plan"
        candidates = analyzer.build_label_plan(
            trace_dir, plan_path, expanded_path, self.source_path
        )
        self.assertIn(("main", "implicit_value"), candidates)
        self.assertNotIn(("main", "direct_value"), candidates)

    def test_variable_names_are_not_hard_coded(self):
        source = SOURCE_TEMPLATE.replace(
            "{array_label}", "dfsan_set_label(4, payload, sizeof(payload));"
        )
        source = source.replace("secret1", "private_input").replace(
            "secret2", "key_input"
        ).replace("arr", "payload")
        self.source_path.write_text(source, encoding="utf-8")
        cases = CASES.replace("secret1", "private_input").replace(
            "secret2", "key_input"
        ).replace(" arr", " payload")
        self.cases_path.write_text(cases, encoding="utf-8")

        columns, rows = analyzer.read_test_cases(self.cases_path)
        schema = analyzer._build_input_schema(columns, rows, self.source_path)
        self.assertEqual(schema.array_names, {"payload"})
        self.assertEqual(schema.secret_names, {"private_input", "key_input", "payload"})

    def test_direct_label_precedence(self):
        direct = analyzer.FinalRecord(
            flags=0, size=4, label=4, value_hex=struct.pack("=i", 10).hex()
        )
        self.assertEqual(analyzer.classify_group_status([direct, None]), "DIRECT")

    def test_two_fixed_arrays_accept_size_annotations_and_zero_pad(self):
        source = r'''#include <sanitizer/dfsan_interface.h>
#include <stdlib.h>
int main(int argc, char **argv) {
    int type = atoi(argv[1]);
    unsigned char key[32], statemt[32];
    for (int i = 0; i < 32; ++i) {
        key[i] = (unsigned char)atoi(argv[2 + i]);
        statemt[i] = (unsigned char)atoi(argv[34 + i]);
    }
    dfsan_set_label(1, &type, sizeof(type));
    dfsan_set_label(2, key, sizeof(key));
    return 0;
}
'''
        self.source_path.write_text(source, encoding="utf-8")
        self.cases_path.write_text(
            "# inputs: type key[32] statemt[32]\n"
            "2|1,3,4,5,4|2,3,4\n"
            "3|1,3,4,5,4|2,3,4\n"
            "2|1,3,4,5,4|5,6,7\n"
            "3|1,3,4,5,4|5,6,7\n",
            encoding="utf-8",
        )

        columns, rows = analyzer.read_test_cases(self.cases_path)
        self.assertEqual(columns, ["type", "key", "statemt"])
        full_columns, full_rows, header_sizes = analyzer._read_test_cases_with_metadata(
            self.cases_path
        )
        schema = analyzer._build_input_schema(
            full_columns, full_rows, self.source_path, header_sizes
        )
        self.assertEqual(schema.array_names, {"key", "statemt"})
        self.assertEqual(schema.array_sizes, {"key": 32, "statemt": 32})
        self.assertEqual(schema.secret_names, {"type", "key"})

        expanded_path = self.root / "expanded_multi_array.txt"
        self.assertEqual(
            analyzer.expand_test_cases(self.cases_path, self.source_path, expanded_path),
            4,
        )
        expanded_columns, expanded_rows = analyzer.read_test_cases(expanded_path)
        self.assertEqual(expanded_columns, ["type", "key", "statemt"])
        self.assertEqual(len(expanded_rows[0][1].split(",")), 32)
        self.assertEqual(len(expanded_rows[0][2].split(",")), 32)
        self.assertEqual(expanded_rows[0][1].split(",")[:5], ["1", "3", "4", "5", "4"])
        self.assertEqual(expanded_rows[0][1].split(",")[5:], ["0"] * 27)

        argv_path = self.root / "argv_multi_array.txt"
        analyzer.write_argv_cases(expanded_path, self.source_path, argv_path)
        argv_rows = [line.split("|") for line in argv_path.read_text().splitlines()]
        self.assertEqual(len(argv_rows[0]), 65)  # type + key[32] + statemt[32]
        self.assertEqual(argv_rows[0][0], "2")
        self.assertEqual(argv_rows[0][1:6], ["1", "3", "4", "5", "4"])
        self.assertEqual(argv_rows[0][33:36], ["2", "3", "4"])
        self.assertEqual(argv_rows[0][36:], ["0"] * 29)

    def test_fixed_array_header_size_must_match_c_declaration(self):
        source = "int main(void) { int secret; int key[32]; dfsan_set_label(1, &secret, sizeof(secret)); return 0; }"
        self.source_path.write_text(source, encoding="utf-8")
        self.cases_path.write_text(
            "# inputs: secret key[31]\n1|1,2\n2|1,2\n", encoding="utf-8"
        )
        columns, rows, header_sizes = analyzer._read_test_cases_with_metadata(
            self.cases_path
        )
        with self.assertRaisesRegex(RuntimeError, "array size mismatch"):
            analyzer._build_input_schema(columns, rows, self.source_path, header_sizes)

    def test_fixed_array_rejects_more_values_than_its_capacity(self):
        source = "int main(void) { int secret; int key[2]; dfsan_set_label(1, &secret, sizeof(secret)); return 0; }"
        self.source_path.write_text(source, encoding="utf-8")
        self.cases_path.write_text(
            "# inputs: secret key[2]\n1|1,2,3\n2|1,2,3\n", encoding="utf-8"
        )
        columns, rows, header_sizes = analyzer._read_test_cases_with_metadata(
            self.cases_path
        )
        with self.assertRaisesRegex(RuntimeError, "exceeding its declared size"):
            analyzer._build_input_schema(columns, rows, self.source_path, header_sizes)

    def test_report_prints_exact_secret_tuple_for_each_s_id(self):
        source = r'''#include <sanitizer/dfsan_interface.h>
int main(void) {
    int secret;
    int key[4];
    int statemt[4];
    dfsan_set_label(1, &secret, sizeof(secret));
    dfsan_set_label(2, key, sizeof(key));
    return 0;
}
'''
        self.source_path.write_text(source, encoding="utf-8")
        self.cases_path.write_text(
            "# inputs: secret key[4] statemt[4]\n"
            "10|1,2|7,8\n"
            "20|1,2|7,8\n",
            encoding="utf-8",
        )
        expanded_path = self.root / "report_cases.txt"
        analyzer.expand_test_cases(self.cases_path, self.source_path, expanded_path)
        _, expanded_rows = analyzer.read_test_cases(expanded_path)
        final_dir = self.root / "final"
        discovery_dir = self.root / "discovery"
        final_dir.mkdir()
        discovery_dir.mkdir()
        for index, row in enumerate(expanded_rows):
            value = int(row[0])
            record = self._record("result", value, label=0)
            (final_dir / f"case_{index}.trace").write_text(record, encoding="utf-8")
            (discovery_dir / f"case_{index}.trace").write_text(record, encoding="utf-8")

        report_path = self.root / "report.md"
        analyzer.write_debug_report(
            final_dir, discovery_dir, expanded_path, self.source_path, report_path
        )
        report = report_path.read_text(encoding="utf-8")
        self.assertIn("| S1 | secret=10, key=[1,2,0,0] |", report)
        self.assertIn("| S2 | secret=20, key=[1,2,0,0] |", report)

    @staticmethod
    def _record(name, value, label):
        value_hex = struct.pack("=i", value).hex()
        return f"F|0|4|{label}|main|{name}|{value_hex}\n"


if __name__ == "__main__":
    unittest.main()
