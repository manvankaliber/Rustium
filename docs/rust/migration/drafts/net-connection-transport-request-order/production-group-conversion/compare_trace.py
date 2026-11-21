#!/usr/bin/env python3
"""Compare retained oracle and candidate stdout, preserving line order."""
import sys
from pathlib import Path

if len(sys.argv) != 3:
    raise SystemExit("usage: compare_trace.py ORIGINAL_OUTPUT CANDIDATE_OUTPUT")
original = Path(sys.argv[1]).read_text(encoding="utf-8").splitlines()
candidate = Path(sys.argv[2]).read_text(encoding="utf-8").splitlines()
if original != candidate:
    for i in range(max(len(original), len(candidate))):
        left = original[i] if i < len(original) else "<missing>"
        right = candidate[i] if i < len(candidate) else "<missing>"
        if left != right:
            print(f"line {i + 1}: original={left!r}, candidate={right!r}")
    raise SystemExit(1)
print(f"Matched {len(original)} ordered trace lines")
