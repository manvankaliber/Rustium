#!/usr/bin/env python3
"""Archive a paired gzip-header benchmark and retain every run in the gallery."""
import argparse
import hashlib
import json
import platform
import random
import re
import statistics
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
GROUPS = [
    ("gzip-common", "Detect common gzip headers", "Gzip detection", [
        ("fixed", "Fixed header"), ("fixed_with_payload", "With payload"),
        ("not_gzip", "Non-gzip"), ("short_rejection", "Short invalid"), ("bad_method", "Bad method")]),
    ("gzip-optional", "Parse optional gzip fields", "Gzip optional fields", [
        ("all_options", "All options"), ("empty_extra", "Empty extra"),
        ("long_name", "Long name"), ("long_comment", "Long comment"), ("large_extra", "Large extra")]),
    ("gzip-stream", "Parse fragmented gzip headers", "Gzip streaming", [
        ("fixed_one_byte", "Fixed / 1 B"), ("options_one_byte", "Options / 1 B"),
        ("options_two_bytes", "Options / 2 B"), ("options_eight_bytes", "Options / 8 B"), ("options_64_bytes", "Options / 64 B")]),
]

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("report", type=Path)
    parser.add_argument("--run", required=True)
    parser.add_argument("--label", required=True)
    parser.add_argument("--source-hashes", type=Path, required=True)
    parser.add_argument("--date", default="2026-11-12")
    args = parser.parse_args()
    if not re.fullmatch(r"[a-z0-9-]+", args.run):
        parser.error("run must be a short lowercase identifier")
    samples = [json.loads(line) for line in args.report.read_text().splitlines() if line.strip()]
    known = {key for _, _, _, cases in GROUPS for key, _ in cases}
    if {sample['case'] for sample in samples} != known:
        raise ValueError('Gzip charts must cover every measured case exactly')
    rng = random.Random(20261112)
    rows = []
    for _, _, _, cases in GROUPS:
        for key, label in cases:
            pair = [r for r in samples if r["case"] == key]
            if len(pair) != 20 or len({r["repeat"] for r in pair}) != 20:
                raise ValueError(f"Expected 20 paired repetitions for {key}")
            b = statistics.median(r["baseline_ns"] for r in pair)
            n = statistics.median(r["rust_ns"] for r in pair)
            if min(b, n) <= 0:
                raise ValueError("Nonpositive timing")
            changes = []
            for _ in range(5000):
                resampled = rng.choices(pair, k=20)
                changes.append((statistics.median(r["rust_ns"] for r in resampled) / statistics.median(r["baseline_ns"] for r in resampled) - 1) * 100)
            changes.sort()
            rows.append({"key": key, "label": label, "run": args.run,
                         "baseline_median_ns": b, "rust_median_ns": n,
                         "change_percent": (n / b - 1) * 100,
                         "bootstrap_95_ci_percent": [changes[125], changes[4874]],
                         "chunk_bytes": pair[0]["chunk_bytes"], "input_bytes": pair[0]["input_bytes"],
                         "iterations_per_sample": pair[0]["iterations"]})
    archive = Path(f"docs/rust/benchmarks/gzip-header-{args.run}-{args.date}.json")
    execution_path = args.report.with_name(args.report.name.replace("-benchmark.jsonl", "-execution.json"))
    execution = (json.loads(execution_path.read_text())
                 if execution_path != args.report and execution_path.exists() else None)
    if execution is not None:
        if execution.get('exit_code') != 0 or execution.get('module', 'gzip-header') != 'gzip-header':
            raise ValueError('Unsuccessful or mismatched gzip execution')
        for key, path in [('raw_report_sha256', args.report),
                          ('source_hashes_file_sha256', args.source_hashes)]:
            expected = execution.get(key)
            if expected is not None and expected != hashlib.sha256(path.read_bytes()).hexdigest():
                raise ValueError('Gzip execution provenance digest mismatch: ' + key)
    if args.run == 'final' and (not execution or not execution.get('configured_flags')
                              or not execution.get('raw_report_sha256')
                              or not execution.get('source_hashes_file_sha256')):
        raise ValueError('Final gzip results require bound configured GN provenance')
    affinity_mask = execution.get("affinity_mask") if execution else None
    payload = {"schema_version": 1, "date": args.date, "run": args.run, "label": args.label,
               "measurement": "Same-process paired original C++ and Rust-backed GZipHeader APIs; warm input buffers; ns per complete detection/header-feed operation",
               "baseline": "Frozen original parser, same object size/alignment and out-of-line C++ API calls, same Chromium C++ compilation flags",
               "compiler": "Chromium Clang 24 and Rust 1.101; opt-level 3, debug assertions, CFG, panic abort",
               "repetitions": 20, "affinity_mask": affinity_mask, "warmup": "iterations_per_sample / 5 for each implementation",
               "order": "Seeded randomized reader order per paired repetition",
               "scope_excludes": ["disk I/O", "payload decompression", "network I/O", "browser startup"],
               "environment": {"platform": platform.platform(), "machine": platform.machine()},
               "source_sha256": json.loads(args.source_hashes.read_text()),
               "raw_report_sha256": hashlib.sha256(args.report.read_bytes()).hexdigest(),
               "cases": rows, "samples": samples}
    if execution is not None:
        payload["execution"] = execution
    (ROOT / archive).write_text(json.dumps(payload, indent=2) + "\n", encoding="utf-8")
    page = ROOT / "rust-performance.html"
    contents = page.read_text(encoding="utf-8")
    pattern = r'(<script id="benchmark-data" type="application/json">)(.*?)(</script>)'
    match = re.search(pattern, contents, re.DOTALL)
    data = json.loads(match[2])
    data.setdefault("settings", {})["gzip"] = {"repetitions": 20, "affinity_mask": affinity_mask}
    metrics = data.setdefault("additional_metrics", [])
    for identifier, title, operation, cases in GROUPS:
        metric = next((m for m in metrics if m["id"] == identifier), None)
        if metric is None:
            metric = {"id": identifier, "title": title, "operation": operation, "source": "gzip",
                      "scope": "C++ API + Rust bridge; warm buffers; no payload decompression",
                      "baseline": "Baseline: original C++ gzip-header parser.", "rows": [], "runs": [], "archives": {}}
            metrics.append(metric)
        keys = {key for key, _ in cases}
        metric["rows"] = [r for r in metric["rows"] if r["run"] != args.run] + [r for r in rows if r["key"] in keys]
        metric["runs"] = [[args.run, args.label]] + [r for r in metric["runs"] if r[0] != args.run]
        metric["archives"][args.run] = archive.as_posix()
    replacement = json.dumps(data, indent=2).replace("<", "\\u003c")
    page.write_text(contents[:match.start(2)] + replacement + contents[match.end(2):], encoding="utf-8")
    print(f"Archived {archive}; updated three charts with all 15 cases, including regressions.")

if __name__ == "__main__":
    main()
