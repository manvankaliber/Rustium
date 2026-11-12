#!/usr/bin/env python3
"""Archive the DataPack benchmark evidence and refresh the offline chart page."""

import argparse
import hashlib
import json
import math
import re
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--load-report", type=Path,
                        default=ROOT / ".scratch/data-pack/native-benchmark-results.json")
    parser.add_argument("--lookup-report", type=Path,
                        default=ROOT / ".scratch/data-pack-lookup/benchmark.json")
    parser.add_argument("--load-validation", type=Path,
                        default=ROOT / ".scratch/data-pack/validation-report.json")
    parser.add_argument("--lookup-validation", type=Path,
                        default=ROOT / ".scratch/data-pack-lookup/validation-report.json")
    parser.add_argument("--date", default="2026-11-12", help="Benchmark snapshot date (YYYY-MM-DD)")
    args = parser.parse_args()
    if not re.fullmatch(r"\d{4}-\d{2}-\d{2}", args.date):
        parser.error("--date must use YYYY-MM-DD")
    reports, sources = {}, {}
    for key, path in (("load", args.load_report), ("lookup", args.lookup_report)):
        content = path.read_bytes()
        reports[key] = json.loads(content)
        try:
            source_path = path.resolve().relative_to(ROOT).as_posix()
        except ValueError:
            source_path = str(path.resolve())
        sources[key] = {"source_path": source_path, "sha256": hashlib.sha256(content).hexdigest()}
    provenance = {}
    for key, path in (("load", args.load_validation), ("lookup", args.lookup_validation)):
        if not path.exists():
            provenance[key] = {"status": "Validation source snapshot not supplied"}
            continue
        content = path.read_bytes()
        validation = json.loads(content)
        provenance[key] = {
            "validation_report_sha256": hashlib.sha256(content).hexdigest(),
            "source_snapshot_sha256": validation.get("source_sha256", validation.get("source_sha256_at_validation", {})),
            "baseline": validation.get("baseline", {"commit": validation.get("baseline_commit", "not recorded")}),
            "note": "Hashes recorded for this measurement slice; not hashes of the current working tree.",
        }
    load = [{"key": r["case"], "label": r["case"].replace("1024", "1,024"),
             "iterations_per_sample": r["iterations_per_sample"],
             "baseline_median_ns": r["legacy_median_ns"], "rust_median_ns": r["rust_median_ns"],
             "change_percent": r["change_percent"], "bootstrap_95_ci_percent": r["bootstrap_95_ci_percent"]}
            for r in reports["load"]["cases"]]
    labels = {"hot": "Repeated ID", "direct": "Random hits", "alias": "Aliases", "miss": "Misses", "mixed": "Mixed"}
    lookup = [{**{k: r[k] for k in ("pack", "method", "pattern", "baseline_median_ns", "rust_median_ns", "change_percent", "bootstrap_95_ci_percent")},
               "key": r["pattern"], "label": labels[r["pattern"]]} for r in reports["lookup"]["cases"]]
    for row in load + lookup:
        values = [row["baseline_median_ns"], row["rust_median_ns"], row["change_percent"], *row["bootstrap_95_ci_percent"]]
        if not all(math.isfinite(value) for value in values) or row["baseline_median_ns"] <= 0 or row["rust_median_ns"] <= 0:
            raise ValueError(f"Invalid benchmark values: {row}")
        calculated = (row["rust_median_ns"] / row["baseline_median_ns"] - 1) * 100
        if not math.isclose(calculated, row["change_percent"], abs_tol=1e-8):
            raise ValueError(f"Time change does not match medians: {row}")
        if len(row["bootstrap_95_ci_percent"]) != 2 or row["bootstrap_95_ci_percent"][0] > row["bootstrap_95_ci_percent"][1]:
            raise ValueError(f"Invalid confidence interval: {row}")
        if row["bootstrap_95_ci_percent"][0] <= -100:
            raise ValueError(f"Time-change interval cannot be converted to a finite speedup: {row}")
    archive = Path(f"docs/rust/benchmarks/data-pack-{args.date}.json")
    payload = {"schema_version": 1, "snapshot_date": args.date, "sources": sources,
               "implementation_provenance": provenance,
               "not_recorded_in_benchmark_reports": ["CPU model", "Windows build", "machine power/thermal state"],
               "reports": reports}
    (ROOT / archive).parent.mkdir(parents=True, exist_ok=True)
    (ROOT / archive).write_text(json.dumps(payload, indent=2) + "\n", encoding="utf-8")
    settings = {key: {k: report[k] for k in ("repetitions", "affinity_mask", "iterations_per_sample", "warmup_lookups_per_sample") if k in report}
                for key, report in reports.items()}
    page = ROOT / "rust-performance.html"
    existing = json.loads(re.search(r'<script id="benchmark-data" type="application/json">(.*?)</script>', page.read_text(encoding="utf-8"), re.DOTALL)[1])
    chart_data = {**existing, "date": args.date, "archive": archive.as_posix(), "settings": {**existing.get("settings", {}), **settings}, "load": load, "lookup": lookup}
    contents, count = re.subn(r'(<script id="benchmark-data" type="application/json">).*?(</script>)',
                              lambda match: match[1] + json.dumps(chart_data, indent=2).replace("<", "\\u003c") + match[2],
                              page.read_text(encoding="utf-8"), flags=re.DOTALL)
    if count != 1:
        raise ValueError("Expected exactly one benchmark-data block")
    page.write_text(contents, encoding="utf-8")
    print(f"Updated {page.name}: {len(load)} load cases, {len(lookup)} lookup cases; raw evidence: {archive}")


if __name__ == "__main__":
    main()
