import copy
import hashlib
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

import compare_harness_records as records

ROOT = Path(__file__).parent
CONTRACTS = json.loads((ROOT / "harness-contracts.json").read_text())


def fixture(contract, timing=False):
    result = []
    if contract["fixture"]:
        result.append({"fixture_creation_internal_us": 123456789, "order_seed": contract["order_seed"]})
    result.extend({"verify_case": case, "checksum": "abcdef0123456789"} for case in contract["cases"])
    if timing:
        order, mask = contract["order_seed"], (1 << 64) - 1
        for case in contract["cases"]:
            for pair in range(contract["pairs"]):
                order ^= (order << 13) & mask
                order ^= order >> 7
                order ^= (order << 17) & mask
                result.append({"case": case, "pair": pair, "cpp_ns": 0,
                               "rust_ns": 1.25e-100, "lifecycles": contract["lifecycles"],
                               "cpp_first": bool(order & 1)})
    return result


class StrictRecordsTest(unittest.TestCase):
    def test_complete_contracts_accept_and_allow_different_durations(self):
        for contract in CONTRACTS.values():
            for mode in ["verify", "timing"]:
                before = fixture(contract, mode == "timing")
                after = copy.deepcopy(before)
                for row in after:
                    if "cpp_ns" in row:
                        row.update(cpp_ns=123.456, rust_ns=3e20)
                records.compare(before, after, contract, mode)

    def test_equally_incomplete_or_duplicated_outputs_reject(self):
        for contract in CONTRACTS.values():
            good = fixture(contract, True)
            variants = [[], good[:-1], good[1:], good + [good[-1]], list(reversed(good))]
            duplicated = copy.deepcopy(good)
            duplicated[-1] = duplicated[-2]
            variants.append(duplicated)
            for bad in variants:
                with self.subTest(length=len(bad)), self.assertRaises(records.InvalidRecords):
                    records.compare(bad, bad, contract, "timing")

    def test_strict_json_and_blank_records(self):
        for text in ['{"a":1,"a":2}', '{"a":NaN}', '{"a":Infinity}', '{"a":-Infinity}']:
            with self.subTest(text=text), self.assertRaises(ValueError):
                records.parse(text)
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "bad.jsonl"
            path.write_text("{}\n\n")
            with self.assertRaises(records.InvalidRecords):
                records.load(path)

    def test_every_timing_type_range_and_order_field(self):
        contract = CONTRACTS["cookie_family"]
        good = fixture(contract, True)
        index = len(contract["cases"])
        for key, values in {
            "pair": [True, -1, 0.0, "0", 1],
            "lifecycles": [True, 0, -1, 64.0, "64", 63],
            "cpp_first": [0, 1, "true", not good[index]["cpp_first"]],
            "case": [None, "unknown", contract["cases"][1]],
            "cpp_ns": [True, "bad", -1, float("nan"), float("inf"), 10**400],
            "rust_ns": [False, None, -0.1, float("-inf")],
        }.items():
            for value in values:
                bad = copy.deepcopy(good)
                bad[index][key] = value
                with self.subTest(key=key, value=str(value)[:30]), self.assertRaises(records.InvalidRecords):
                    records.compare(bad, bad, contract, "timing")

    def test_verification_checksum_membership_and_extra_fields(self):
        contract = CONTRACTS["cookie_family"]
        good = fixture(contract)
        for checksum in [None, 123, "ABCDEF0123456789", "z" * 16, "0" * 15]:
            bad = copy.deepcopy(good)
            bad[0]["checksum"] = checksum
            with self.assertRaises(records.InvalidRecords):
                records.compare(bad, bad, contract, "verify")
        bad = copy.deepcopy(good)
        bad[0]["extra"] = 1
        with self.assertRaises(records.InvalidRecords):
            records.compare(bad, bad, contract, "verify")
        changed = copy.deepcopy(good)
        changed[0]["checksum"] = "0123456789abcdef"
        with self.assertRaises(records.InvalidRecords):
            records.compare(good, changed, contract, "verify")

    def test_fixture_seed_timestamp_and_mode_are_bound(self):
        contract = CONTRACTS["cookie_family_store"]
        good = fixture(contract)
        for key, value in [("order_seed", True), ("order_seed", 1), ("fixture_creation_internal_us", "123"),
                           ("fixture_creation_internal_us", 0), ("fixture_creation_internal_us", 1 << 63)]:
            bad = copy.deepcopy(good)
            bad[0][key] = value
            with self.assertRaises(records.InvalidRecords):
                records.compare(bad, bad, contract, "verify")
        bad = copy.deepcopy(good)
        bad[0]["fixture_creation_internal_us"] += 1
        with self.assertRaises(records.InvalidRecords):
            records.compare(good, bad, contract, "verify")
        with self.assertRaises(records.InvalidRecords):
            records.compare(good, good, contract, "timing")

    def test_cli_optimized_python_and_hash_pins(self):
        contract = CONTRACTS["cookie_family"]
        with tempfile.TemporaryDirectory() as directory:
            before, after = [Path(directory) / x for x in ["before", "after"]]
            raw = "".join(json.dumps(row) + "\n" for row in fixture(contract)).encode()
            before.write_bytes(raw)
            after.write_bytes(raw)
            command = [sys.executable, "-O", str(ROOT / "compare_harness_records.py"), str(before), str(after),
                       "--baseline-sha256", hashlib.sha256(raw).hexdigest(), "--contracts", str(ROOT / "harness-contracts.json"),
                       "--contracts-sha256", hashlib.sha256((ROOT / "harness-contracts.json").read_bytes()).hexdigest(),
                       "--harness", "cookie_family", "--mode", "verify"]
            self.assertEqual(0, subprocess.run(command, capture_output=True).returncode)
            after.write_text("")
            self.assertNotEqual(0, subprocess.run(command, capture_output=True).returncode)
            after.write_bytes(raw)
            before.write_bytes(raw + b"\n")
            self.assertNotEqual(0, subprocess.run(command, capture_output=True).returncode)


if __name__ == "__main__":
    unittest.main()
