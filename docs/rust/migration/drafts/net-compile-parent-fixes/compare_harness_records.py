#!/usr/bin/env python3
"""Strictly compare retained records against a pinned complete workload contract.

Inputs combine the captured stderr records followed by captured stdout records.
No workload is launched. Hash pins must come from the retained source/baseline
evidence; computing a pin does not qualify that evidence as an independent run.
"""
import argparse
import hashlib
import json
import math
from pathlib import Path
import re


class InvalidRecords(ValueError):
    pass


def require(condition, message):
    if not condition:
        raise InvalidRecords(message)


def unique_object(pairs):
    result = {}
    for key, value in pairs:
        require(key not in result, f"duplicate JSON key: {key}")
        result[key] = value
    return result


def invalid_constant(value):
    raise InvalidRecords(f"non-finite JSON constant: {value}")


def parse(text):
    return json.loads(text, object_pairs_hook=unique_object,
                      parse_constant=invalid_constant)


def load(path):
    result = []
    for number, line in enumerate(Path(path).read_text(encoding="utf-8").splitlines(), 1):
        try:
            result.append(parse(line))
        except (ValueError, TypeError) as exc:
            raise InvalidRecords(f"{path}:{number}: {exc}") from exc
    return result


def integer(value, minimum=0, maximum=None):
    return (type(value) is int and value >= minimum
            and (maximum is None or value <= maximum))


def duration(value):
    # JSON integers can be arbitrarily large. Avoid coercion/overflow and do
    # not let bool pass Python's numeric subtype checks.
    return ((type(value) is int and 0 <= value <= 1.7976931348623157e308)
            or (type(value) is float and math.isfinite(value) and value >= 0))


def validate(records, contract, mode):
    require(mode in ("verify", "timing"), "unknown record mode")
    cases = contract["cases"]
    require(type(cases) is list and cases
            and all(type(x) is str and x for x in cases)
            and len(set(cases)) == len(cases), "invalid contract cases")
    require(integer(contract["pairs"], 1)
            and integer(contract["lifecycles"], 1)
            and integer(contract["order_seed"], 0, (1 << 64) - 1)
            and type(contract["fixture"]) is bool, "invalid contract policy")
    expected = len(cases) + int(contract["fixture"])
    if mode == "timing":
        expected += len(cases) * contract["pairs"]
    require(type(records) is list and len(records) == expected,
            f"record count: expected {expected}, received {len(records)}")
    require(all(type(x) is dict for x in records), "records must be objects")
    offset = 0
    if contract["fixture"]:
        record = records[0]
        require(set(record) == {"fixture_creation_internal_us", "order_seed"},
                "fixture fields differ")
        require(integer(record["fixture_creation_internal_us"], 1, (1 << 63) - 1),
                "invalid fixture time")
        require(integer(record["order_seed"]) and record["order_seed"] == contract["order_seed"],
                "order seed differs")
        offset = 1
    for case in cases:
        record = records[offset]
        offset += 1
        require(set(record) == {"verify_case", "checksum"}
                and record["verify_case"] == case, "verification membership/order differs")
        require(type(record["checksum"]) is str
                and re.fullmatch(r"[0-9a-f]{16}", record["checksum"]) is not None,
                "invalid checksum")
    if mode == "verify":
        return
    order = contract["order_seed"]
    mask = (1 << 64) - 1
    for case in cases:
        for pair in range(contract["pairs"]):
            order ^= (order << 13) & mask
            order ^= order >> 7
            order ^= (order << 17) & mask
            record = records[offset]
            offset += 1
            require(set(record) == {"case", "pair", "cpp_ns", "rust_ns", "lifecycles", "cpp_first"},
                    "timing fields differ")
            require(record["case"] == case and integer(record["pair"])
                    and record["pair"] == pair, "timing membership/order differs")
            require(integer(record["lifecycles"], 1)
                    and record["lifecycles"] == contract["lifecycles"], "lifecycle count differs")
            require(type(record["cpp_first"]) is bool
                    and record["cpp_first"] == bool(order & 1), "sample order differs")
            require(duration(record["cpp_ns"]) and duration(record["rust_ns"]),
                    "invalid duration")


def compare(before, after, contract, mode):
    validate(before, contract, mode)
    validate(after, contract, mode)
    for index, (left, right) in enumerate(zip(before, after)):
        # Actual durations may differ; every value still must be valid above.
        stable_left = {k: v for k, v in left.items() if k not in ("cpp_ns", "rust_ns")}
        stable_right = {k: v for k, v in right.items() if k not in ("cpp_ns", "rust_ns")}
        require(stable_left == stable_right, f"baseline fields differ at record {index}")


def pinned(path, expected):
    require(re.fullmatch(r"[0-9a-f]{64}", expected) is not None, "invalid SHA-256 pin")
    require(hashlib.sha256(Path(path).read_bytes()).hexdigest() == expected,
            f"identity mismatch: {path}")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("before")
    parser.add_argument("after")
    parser.add_argument("--baseline-sha256", required=True)
    parser.add_argument("--contracts", required=True)
    parser.add_argument("--contracts-sha256", required=True)
    parser.add_argument("--harness", required=True)
    parser.add_argument("--mode", required=True, choices=("verify", "timing"))
    args = parser.parse_args()
    try:
        pinned(args.before, args.baseline_sha256)
        pinned(args.contracts, args.contracts_sha256)
        contract = parse(Path(args.contracts).read_text(encoding="utf-8"))[args.harness]
        before, after = load(args.before), load(args.after)
        compare(before, after, contract, args.mode)
    except (ValueError, TypeError, KeyError, OSError) as exc:
        parser.exit(1, f"record comparison failed: {exc}\n")
    print(f"{len(after)} complete {args.mode} records: schema and pinned baseline fields match; durations are not compared")


if __name__ == "__main__":
    main()
