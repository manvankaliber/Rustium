#!/usr/bin/env python3
"""Compare saved verify/timing JSONL after qualification; do not launch a workload."""
import argparse
import json
from pathlib import Path


def load(path):
    records = []
    for number, line in enumerate(Path(path).read_text(encoding="utf-8").splitlines(), 1):
        try:
            records.append(json.loads(line))
        except json.JSONDecodeError as exc:
            raise SystemExit(f"{path}:{number}: {exc}") from exc
    return records


def validate(records):
    for record in records:
        if "verify_case" in record:
            assert set(record) == {"verify_case", "checksum"}
            assert len(record["checksum"]) == 16
            int(record["checksum"], 16)
        elif "case" in record:
            assert set(record) == {"case", "pair", "cpp_ns", "rust_ns", "lifecycles", "cpp_first"}
            assert isinstance(record["cpp_first"], bool)
        else:
            assert set(record) == {"fixture_creation_internal_us", "order_seed"}


def stable(record):
    # Durations are measurements, not reproducible equality assertions.
    return {k: v for k, v in record.items() if k not in ("cpp_ns", "rust_ns", "fixture_creation_internal_us")}


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("before")
    parser.add_argument("after")
    args = parser.parse_args()
    before, after = load(args.before), load(args.after)
    validate(before)
    validate(after)
    assert [stable(x) for x in before] == [stable(x) for x in after]
    print(f"{len(after)} records: schema, workload IDs, checksums and non-timing fields match")


if __name__ == "__main__":
    main()
