#!/bin/bash

BINARY="./bin/program"
TESTDIR="./test/milestone-3"
OUTDIR="./test/milestone-3/output"

mkdir -p "$OUTDIR"

pass=0
fail=0
total=0

for input in "$TESTDIR"/*.txt; do
    filename=$(basename "$input")
    output="$OUTDIR/${filename}"

    echo "Running $filename..."
    "$BINARY" "$input" "$output"

    if [ $? -eq 0 ]; then
        echo "  OK"
        ((pass++))
    else
        echo "  FAIL (exit code $?)"
        ((fail++))
    fi

    ((total++))
    echo ""
done

echo "=============================="
echo "Results: $pass/$total passed"
if [ $fail -gt 0 ]; then
    echo "Failed: $fail"
fi