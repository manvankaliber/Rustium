#!/usr/bin/env python3
"""Refresh the roadmap from committed metrics, acceptance evidence and local sources."""

import argparse
from datetime import datetime, timezone
import hashlib
from html import escape
import json
from pathlib import Path
import re
import subprocess

from core_source_metrics import count as count_core_sources

ROOT = Path(__file__).resolve().parents[1]
PAGE = ROOT / "rust-roadmap.html"
SNAPSHOT = ROOT / "docs/rust/roadmap-status.json"
START = "<!-- ROADMAP STATUS START -->"
END = "<!-- ROADMAP STATUS END -->"
WORK = [
    ("Backoff state and serialization", "net/base/backoff_rust_core.rs", "backoff-entry-wave2-*/report.json"),
    ("Host and port utilities", "net/base/host_port_rust_core.rs", "host-port-wave2-*/report.json"),
    ("Cookie parsing", "net/cookies/parsed_cookie_rust_core.rs", "parsed-cookie-wave2-*/report.json"),
    ("DNS query construction and parsing", "net/dns/dns_query_rust_core.rs", "dns-query-wave2-*/report.json"),
    ("WebSocket extension grammar", "net/websockets/websocket_extension_rust_core.rs", "websocket-extensions-wave2-*/report.json"),
    ("Numeric and range optimizations", "net/base/parse_number_parser.rs", "number-range-optimization-wave2-*/report.json"),
    ("Port policy optimizations", "net/base/port_policy_core.rs", "port-optimization-wave2-*/report.json"),
]


def load(path):
    return json.loads(path.read_text(encoding="utf-8-sig"))


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def collect():
    metrics = load(ROOT / "metrics.json")
    ledger = load(ROOT / "docs/rust/net-migration-status.json")
    accepted = ledger["accepted_rewrite_slices"]
    if accepted["count"] != len(accepted["slices"]):
        raise ValueError("Acceptance count differs from the recorded slice list")
    evidence = ROOT / accepted["evidence"]
    if not evidence.is_file():
        raise ValueError("Acceptance evidence is missing")
    inputs = [ROOT / "metrics.json", ROOT / "docs/rust/net-migration-status.json", evidence,
              ROOT / "tools/core_source_metrics.py", ROOT / ".github/scripts/codebase_metrics.py",
              ROOT / "docs/rust/language-scope-audit.json"]
    active = []
    for title, source, pattern in WORK:
        if not (ROOT / source).is_file():
            continue
        reports = sorted((ROOT / "docs/rust/benchmarks").glob(pattern))
        report = reports[-1] if reports else None
        record = load(report) if report else {}
        if report:
            inputs.append(report)
        active.append({"name": title, "source": source,
                       "report": report.relative_to(ROOT).as_posix() if report else None,
                       "archived_status": record.get("status", record.get("scope")),
                       "status": "In progress; no additional acceptance recorded"})
    # Explicitly bounded inventory: includes test/FFI/generated Rust, not archives.
    sources = sorted([*ROOT.glob("net/**/*.rs"), *ROOT.glob("ui/base/resource/*.rs")])
    inputs.extend(sources)
    return {
        "schema_version": 1,
        "head": subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=ROOT).decode().strip(),
        "metrics": {key: metrics[key] for key in
                    ("commit", "updated", "actual_loc", "files", "languages", "definitions")},
        "core_metrics": count_core_sources(),
        "accepted": accepted,
        "acceptance_ledger_refreshed_utc": ledger["refreshed_utc"],
        "network_inventory_groups": len(ledger["components"]),
        "groups_with_cpp_ownership": sum(c["ownership_status"] == "C++ ownership remains"
                                         for c in ledger["components"]),
        "ownership_statuses": sorted({c["ownership_status"] for c in ledger["components"]}),
        "active_work": active,
        "local_rust_inventory": {"scope": "net/**/*.rs and ui/base/resource/*.rs; includes tests, FFI and generated sources; excludes archives and build tree",
                                 "files": len(sources)},
        "inputs_sha256": {p.relative_to(ROOT).as_posix(): digest(p) for p in sorted(set(inputs))},
        "limitations": [
            "Core implementation source counts include local edits and untracked rewrite files; source share is not migration completion.",
            'The working tree is scanned file by file; this is a timestamped source snapshot, and concurrent rewrites can make it stale immediately.',
            "Acceptance applies to archived Windows x64 source snapshots, not automatically to current mutable files.",
            "Working source presence and archived native validation do not prove configured integration or acceptance.",
            "Work reports are selected by run slug, never filesystem modification time; current work may be newer than those reports.",
            "Full desktop shipped-dependency closure and cross-platform acceptance remain unverified.",
        ],
    }


def link(path, label):
    return f'<a href="{escape(path, quote=True)}">{escape(label)}</a>'


def render(data):
    m = data["core_metrics"]
    rust = next(x for x in m["languages"] if x["name"] == "Rust")
    cpp = sum(x["lines"] for x in m["languages"] if x["name"] in {"C", "C++", "C/C++"})
    total = m["actual_loc"]
    rp, cp = rust["lines"] / total * 100, cpp / total * 100
    op = 100 - rp - cp
    accepted = data["accepted"]
    display_names = {"C/C++": "C/C++ headers and includes", "C": "C (.c implementation files)"}
    language_rows = "".join(
        f'<tr><th scope="row">{escape(display_names.get(item["name"], item["name"]))}</th><td>{item["files"]:,}</td>'
        f'<td>{item["lines"]:,}</td><td>{item["share"] * 100:.4f}%</td></tr>'
        for item in m["languages"])
    rows = "".join(f'<tr><th scope="row">{escape(s["rust_scope"])}</th><td>{escape(s["remaining_cpp_seams"])}</td></tr>'
                   for s in accepted["slices"])
    working_rows = ""
    for item in data["active_work"]:
        report = link(item["report"], "Archived work report") if item["report"] else "No work report recorded"
        archive_status = item["archived_status"] or "No configured acceptance recorded"
        status = escape(archive_status if isinstance(archive_status, str) else
                        json.dumps(archive_status, ensure_ascii=False))
        working_rows += (f'<tr><th scope="row">{link(item["source"], item["name"])}</th>'
                         f'<td>{escape(item["status"])}<details><summary>Evidence and archive status</summary>'
                         f'<p>{report}. {status}</p></details></td></tr>')
    return f'''{START}
<p><small>Refreshed {escape(data['observed_utc'])}. Editing checkout <code>{data['head'][:12]}</code> with local work included in the status inventory.</small></p>
<div class="stats"><div class="box"><small>Recorded accepted rewrite slices</small><span class="value">{accepted['count']}</span><small>Windows x64 archived acceptance</small></div><div class="box"><small>Rust in maintained core code</small><span class="value">{rp:.3f}%</span><small>{rust['lines']:,} of {total:,} code lines · includes local work</small></div><div class="box"><small>Current migration priority</small><span class="value">Networking</span><small>Entire stack, then the remaining browser</small></div></div>
<p>{accepted['count']} scoped feature rewrites are recorded as accepted in the migration ledger. It records C++ ownership remaining in {data['groups_with_cpp_ownership']} of {data['network_inventory_groups']} network inventory groups. Whole-browser completion has no verified percentage.</p>
<details open><summary>Core code composition and counting method</summary>
<div class="bar" role="img" aria-label="Maintained core code: Rust {rp:.3f} percent, C and C++ {cp:.3f} percent, other core languages {op:.3f} percent"><span style="width:{rp:.8f}%;min-width:2px;background:var(--chart-2,#d7895b)"></span><span style="width:{cp:.8f}%;background:var(--chart-1,#739acc)"></span><span style="flex:1;background:var(--secondary,#444)"></span></div>
<div class="legend"><span>Rust {rp:.3f}%</span><span>C / C++ {cp:.3f}%</span><span>Other {op:.3f}%</span></div>
<p><b>Counted:</b> C, C++, headers, Rust and other implementation languages: Objective-C/Objective-C++, Java, Kotlin, Swift, Go, Dart, JavaScript and TypeScript. Runtime UI logic and FFI count as implementation code. {m['files']:,} included files in the local checkout, based on commit <code>{m['head']}</code> with working edits and new source files applied.</p>
<p><b>Excluded:</b> tests and test helpers, fuzzers, benchmarks, inline Rust test-only items, Markdown/docs, configuration, comments and blank lines, tooling/build scripts, markup/styles, schemas/IDL, generated source, fixtures, frozen rewrite references, archives and dashboards. Vendored sources and submodules remain outside the maintained-source scope.</p>
<p>{escape(m['method'])} {escape(m['limitations'])}</p>
<div class="table-scroll"><table><thead><tr><th scope="col">Core language</th><th scope="col">Files</th><th scope="col">Code lines</th><th scope="col">Share</th></tr></thead><tbody>{language_rows}</tbody></table></div>
<p><b>Why these languages remain:</b> Objective-C and Objective-C++ implement macOS integration, including native windows and updater registration. The first-party Java Android remnants have been removed. Generic vendored JVM protobuf and metadata libraries remain outside this maintained-source scope; they are not Android platform support. The five maintained C files implement macOS startup/updater helpers and the Linux setuid sandbox. Windows also retains vendored C dependencies such as zlib, outside this maintained-source denominator. See the {link('docs/rust/language-scope-audit.json', 'language usage audit')} for paths, build evidence and limitations. Source inventory includes platform remnants and inactive branches; it is not a count of code shipped by the Windows build.</p>
<p><b>Snapshot scope:</b> Ongoing networking rewrites may change the working tree during and after collection. The recorded observation and source hashes identify this count; use the refresh command for a newer snapshot.</p>
<p>Source shares include pre-existing Rust and unaccepted local rewrites; they do not measure migration completion. This narrower core-code scope replaces the previous maintained-source total, which also included tooling and non-implementation formats. Source removal and scope changes can change the share without a rewrite. {link('docs/rust/roadmap-status.json', 'Exact counts, exclusions, local source hashes and provenance')}.</p>
</details>
<h2>Recorded Rust implementation</h2>
<p>Acceptance covers the scoped logic below on {escape(accepted['platform'])}. It refers to archived source snapshots; current optimization edits require their own validation. See the {link('docs/rust/net-migration-status.json', 'acceptance ledger')} and {link(accepted['evidence'], 'configured validation report')}.</p>
<div class="table-scroll"><table><thead><tr><th scope="col">Accepted Rust scope</th><th scope="col">Remaining C++ boundary or dependency</th></tr></thead><tbody>{rows}</tbody></table></div>
<h2>Current rewrite and optimization work</h2>
<p>New work covers backoff state, host/port utilities, cookie parsing, DNS queries and WebSocket extensions. Numeric/range and port-policy optimizations continue alongside it. The following entries are in progress; their linked reports describe the archived run, which may precede current edits.</p>
<div class="table-scroll"><table><thead><tr><th scope="col">Work in the editing checkout</th><th scope="col">Recorded status</th></tr></thead><tbody>{working_rows}</tbody></table></div>
<p><b>Next gate:</b> finish production and GN integration, synchronize owned files to <code>D:/RustiumBuild/src</code>, run required configured correctness checks, and measure the complete workload batch in a coordinated quiet window. Archive every result and optimization debt. Hold benchmark-gallery publication until each workload meets the existing improvement/no-measurable-regression target, or has a demonstrated hard blocker and concrete near-term rewrite follow-up. Bridge presence alone does not establish a blocker.</p>
<p><small>{link('docs/rust/roadmap-status.json', 'Snapshot, source hashes and limitations')} · {link('docs/rust/net-migration-plan.md', 'Network migration plan')} · {link('docs/rust/benchmarks/optimization-backlog.json', 'Optimization backlog')}. Refresh: <code>python tools/update_rust_roadmap.py</code>; stale check: <code>python tools/update_rust_roadmap.py --check</code>.</small></p>
{END}'''


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--check", action="store_true", help="Fail if the snapshot or generated HTML is stale")
    args = parser.parse_args()
    data = collect()
    saved = load(SNAPSHOT) if SNAPSHOT.exists() else {}
    # Preserve the observation time for identical inputs so repeat runs are stable.
    old = {k: v for k, v in saved.items() if k != "observed_utc"}
    data["observed_utc"] = (saved["observed_utc"] if old == data else
                            datetime.now(timezone.utc).isoformat())
    page = PAGE.read_text(encoding="utf-8")
    if page.count(START) != 1 or page.count(END) != 1:
        raise ValueError("Roadmap must contain exactly one generated status block")
    updated = re.sub(re.escape(START) + r"[\s\S]*?" + re.escape(END),
                     lambda _: render(data), page, count=1)
    if args.check:
        if saved != data or updated != page:
            raise SystemExit("Roadmap is stale; run python tools/update_rust_roadmap.py")
        print("Roadmap snapshot and embedded status match their recorded inputs.")
    else:
        SNAPSHOT.write_text(json.dumps(data, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
        PAGE.write_text(updated, encoding="utf-8")
        print(f"Updated {PAGE.name} and {SNAPSHOT.relative_to(ROOT).as_posix()}")


if __name__ == "__main__":
    main()
