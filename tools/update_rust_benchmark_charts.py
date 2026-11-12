#!/usr/bin/env python3
"""Archive paired rewrite measurements and refresh the offline benchmark gallery.

Metadata describes the module, measurement boundaries and groups of cases.
Each JSONL sample has case, repeat, baseline_ns and rust_ns, with optional
workload fields. Execution metadata and source hashes are separate evidence.
"""

import argparse
from datetime import date
import hashlib
import json
import math
from pathlib import Path
import random
import re
import statistics

ROOT = Path(__file__).resolve().parents[1]


def read_json(path):
    return json.loads(path.read_text(encoding="utf-8"))


def summarize(samples, cases, run, seed):
    rng = random.Random(seed)
    rows = []
    known = {case["key"] for case in cases}
    if len(known) != len(cases):
        raise ValueError("Every measured case must appear in exactly one chart")
    if {sample["case"] for sample in samples} != known:
        raise ValueError("Chart definitions must cover every measured case exactly")
    for case in cases:
        pairs = [sample for sample in samples if sample["case"] == case["key"]]
        if len(pairs) < 20 or len({sample["repeat"] for sample in pairs}) != len(pairs):
            raise ValueError(f"Need at least 20 unique paired repetitions: {case['key']}")
        for pair in pairs:
            if any(not isinstance(pair[key], (float, int)) or
                   not math.isfinite(pair[key]) or pair[key] <= 0
                   for key in ("baseline_ns", "rust_ns")):
                raise ValueError("Timings must be positive finite nanoseconds")
        baseline = statistics.median(pair["baseline_ns"] for pair in pairs)
        rust = statistics.median(pair["rust_ns"] for pair in pairs)
        changes = []
        absolute_changes = []
        for _ in range(5000):
            resampled = rng.choices(pairs, k=len(pairs))
            sample_rust = statistics.median(pair["rust_ns"] for pair in resampled)
            sample_baseline = statistics.median(pair["baseline_ns"] for pair in resampled)
            changes.append((sample_rust / sample_baseline - 1) * 100)
            absolute_changes.append(sample_rust - sample_baseline)
        changes.sort()
        absolute_changes.sort()
        row = {"key": case["key"], "label": case["label"], "run": run,
                     "baseline_median_ns": baseline, "rust_median_ns": rust,
                     "change_percent": (rust / baseline - 1) * 100,
                     "bootstrap_95_ci_percent": [changes[125], changes[4874]],
                     "absolute_time_change_ns": rust - baseline,
                     "bootstrap_90_ci_time_change_ns": [absolute_changes[250], absolute_changes[4749]],
                     "paired_repetitions": len(pairs),
                     "workload": {key: value for key, value in case.items()
                                  if key not in ("key", "label")}}
        bound = case.get("equivalence_bound_ns")
        if bound is not None:
            if not isinstance(bound, (int, float)) or not math.isfinite(bound) or bound <= 0:
                raise ValueError("Equivalence bounds must be positive finite nanoseconds")
            row["equivalence_bound_ns"] = bound
            lo, hi = row["bootstrap_90_ci_time_change_ns"]
            row["within_equivalence_bound"] = -bound < lo <= hi < bound
        rows.append(row)
    return rows


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("report", type=Path)
    parser.add_argument("--metadata", required=True, type=Path)
    parser.add_argument("--source-hashes", required=True, type=Path)
    parser.add_argument("--execution", type=Path)
    parser.add_argument("--run", required=True)
    parser.add_argument("--label", required=True)
    parser.add_argument("--final", action="store_true",
                        help="Make this uniquely named, validated configured run the accepted default")
    parser.add_argument("--date", default=date.today().isoformat())
    args = parser.parse_args()
    metadata = read_json(args.metadata)
    group_ids = [group["id"] for group in metadata["groups"]]
    if len(set(group_ids)) != len(group_ids):
        raise ValueError("Chart identifiers must be unique")
    if any(not 1 <= len(group["cases"]) <= 5 for group in metadata["groups"]):
        raise ValueError("Each readable chart must contain one to five cases")
    for identifier in [args.run, metadata["module"], metadata["source"],
                       *(group["id"] for group in metadata["groups"])]:
        if not re.fullmatch(r"[a-z][a-z0-9-]*", identifier):
            parser.error("Module, source, metric and run identifiers must be lowercase slugs")
    date.fromisoformat(args.date)
    samples = [json.loads(line) for line in args.report.read_text(encoding="utf-8").splitlines()
               if line.strip()]
    cases = [case for group in metadata["groups"] for case in group["cases"]]
    execution = read_json(args.execution) if args.execution else None
    if execution is not None:
        if execution.get('exit_code') != 0:
            raise ValueError('Cannot import an unsuccessful benchmark execution')
        if execution.get('module', metadata['module']) != metadata['module']:
            raise ValueError('Execution module does not match chart metadata')
        for key, path in [('raw_report_sha256', args.report),
                          ('source_hashes_file_sha256', args.source_hashes)]:
            expected = execution.get(key)
            if expected is not None and expected != hashlib.sha256(path.read_bytes()).hexdigest():
                raise ValueError('Execution provenance digest mismatch: ' + key)
    accepted_final = args.final or args.run == 'final'
    if accepted_final:
        if not execution or not execution.get('configured_flags') or not execution.get('executable_sha256'):
            raise ValueError('Final results require configured GN execution provenance')
        if execution.get('module') != metadata['module']:
            raise ValueError('Final execution must explicitly identify its module')
        if not execution.get('raw_report_sha256') or not execution.get('source_hashes_file_sha256'):
            raise ValueError('Final execution must bind raw measurements and source manifest')
    rows = summarize(samples, cases, args.run, seed=20261112)
    archive = Path("docs/rust/benchmarks") / f"{metadata['module']}-{args.run}-{args.date}.json"
    payload = {"schema_version": 1, "date": args.date, "run": args.run, "label": args.label,
               "validation_stage": "final" if accepted_final else "intermediate",
               "metadata": metadata, "execution": execution,
               "source_sha256": read_json(args.source_hashes),
               "raw_report_sha256": hashlib.sha256(args.report.read_bytes()).hexdigest(),
               "statistics": {"estimator": "ratio of paired-run medians",
                              "interval": "95% paired percentile bootstrap; 5000 resamples",
                              "seed": 20261112, "discarded_samples": 0},
               "cases": rows, "samples": samples}
    if execution and execution.get("started_utc"):
        payload["recorded_at"] = execution["started_utc"]
    page = ROOT / "rust-performance.html"
    contents = page.read_text(encoding="utf-8")
    match = re.search(r'(<script id="benchmark-data" type="application/json">)(.*?)(</script>)',
                      contents, re.DOTALL)
    if match is None:
        raise ValueError("Benchmark data block not found")
    data = json.loads(match[2])
    repetitions = sorted({row["paired_repetitions"] for row in rows})
    if len(repetitions) != 1:
        raise ValueError("Use consistent repetition counts within this report")
    data.setdefault("settings", {})[metadata["source"]] = {
        "repetitions": repetitions[0],
        "affinity_mask": execution.get("affinity_mask") if execution else None}
    metrics = data.setdefault("additional_metrics", [])
    for group in metadata["groups"]:
        metric = next((metric for metric in metrics if metric["id"] == group["id"]), None)
        if metric is None:
            metric = {"id": group["id"], "rows": [], "runs": [], "archives": {}}
            metrics.append(metric)
        # Migration status, bridge kind and timing boundaries belong to a run.
        # Preserve the old context before promoting a formerly deferred API.
        contexts = metric.setdefault("run_context", {})
        for old_run, _ in metric["runs"]:
            contexts.setdefault(old_run, {
                field: metric.get(field) for field in
                ("scope", "baseline", "production_status", "include_in_average", "bridge")})
        metric.update({"title": group["title"], "operation": group["operation"],
                       "source": metadata["source"], "source_label": metadata["label"],
                       "scope": group.get("chart_scope", metadata.get("chart_scope", metadata["measurement"])),
                       "baseline": metadata.get("chart_baseline", metadata["baseline"])})
        for field in ("production_status", "include_in_average", "bridge", "category"):
            if field in group:
                metric[field] = group[field]
            elif field in metadata:
                metric[field] = metadata[field]
        keys = {case["key"] for case in group["cases"]}
        metric["rows"] = [row for row in metric["rows"] if row["run"] != args.run]
        metric["rows"] += [row for row in rows if row["key"] in keys]
        metric["runs"] = [[args.run, args.label]] + [run for run in metric["runs"] if run[0] != args.run]
        metric["archives"][args.run] = archive.as_posix()
        contexts[args.run] = {
            field: metric.get(field) for field in
            ("scope", "baseline", "production_status", "include_in_average", "bridge")}
        if accepted_final:
            metric["final_run"] = args.run
    destination = ROOT / archive
    destination.parent.mkdir(parents=True, exist_ok=True)
    archive_text = json.dumps(payload, indent=2) + "\n"
    if destination.exists() and destination.read_text(encoding="utf-8") != archive_text:
        raise ValueError("This evidence archive already exists with different contents; "
                         "use a unique --run slug to preserve history: " + str(archive))
    destination.write_text(archive_text, encoding="utf-8")
    encoded = json.dumps(data, indent=2).replace("<", "\\u003c")
    page.write_text(contents[:match.start(2)] + encoded + contents[match.end(2):], encoding="utf-8")
    print(f"Archived {len(rows)} cases / {len(samples)} paired samples in {archive}")


if __name__ == "__main__":
    main()
