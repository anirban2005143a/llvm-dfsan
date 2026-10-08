# FILE: run_tests.sh

#!/usr/bin/env bash

set -euo pipefail

rm -f observations.tsv
rm -f taint_map.txt

TEST_ID=0

while read -r SECRET1 SECRET2 SECRET3; do

    if [[ -z "${SECRET1:-}" ]]; then
        continue
    fi

    if [[ "${SECRET1:0:1}" == "#" ]]; then
        continue
    fi

    TEST_ID=$((TEST_ID + 1))

    IMPLICIT_MODE=record \
    IMPLICIT_TEST_ID="$TEST_ID" \
    IMPLICIT_LOG=observations.tsv \
    ./test_dfsan \
        "$SECRET1" \
        "$SECRET2" \
        "$SECRET3" \
        >/dev/null

done < test_cases.txt

python3 analyze.py \
    observations.tsv \
    taint_map.txt

IMPLICIT_MODE=apply \
IMPLICIT_TAINT_MAP=taint_map.txt \
./test_dfsan 1 0 0
