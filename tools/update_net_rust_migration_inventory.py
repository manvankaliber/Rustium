#!/usr/bin/env python3
"""Refresh the net worklist; explicit component decisions, never Rust-file heuristics.

GN extraction is lexical (all conditional branches), not GN evaluation. Ninja is
read-only evidence for one configured platform, never proof of current integration.
"""
import argparse
import collections
import datetime
import fnmatch
import hashlib
import json
from pathlib import Path
import re


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def read(path):
    return path.read_text(encoding="utf-8", errors="replace")


def refresh(root, status, build):
    data = json.loads(read(status))
    components = data["components"]
    ids = {c["id"] for c in components}
    assert len(ids) == len(components), "duplicate component"
    for c in components:
        assert set(c["depends_on"]) <= ids, c["id"]
        if c["logic_status"] == "validated":
            assert c.get("configured_validation_evidence"), c["id"]
    files = sorted(p for p in (root / "net").rglob("*") if p.is_file())
    gn = []
    source_targets = collections.defaultdict(list)
    for p in files:
        if p.name != "BUILD.gn":
            continue
        content = read(p)
        matches = list(re.finditer(r'^\s*(\w+)\("([^"\n]+)"\)\s*\{', content, re.M))
        for m in matches:
            # Balanced braces suffice for checked-in GN; labels remain lexical.
            depth, end = 1, m.end()
            while end < len(content) and depth:
                depth += (content[end] == "{") - (content[end] == "}")
                end += 1
            body = content[m.end():end]
            label = "//" + p.parent.relative_to(root).as_posix() + ":" + m[2]
            strings = re.findall(r'"([^"\n]+)"', body)
            sources = []
            for s in strings:
                if s.endswith((".cc", ".mm", ".h", ".rs", ".c", ".proto")) and "$" not in s:
                    q = (p.parent / s).resolve()
                    if q.is_relative_to(root):
                        rel = q.relative_to(root).as_posix()
                        sources.append(rel)
                        source_targets[rel].append(label)
            gn.append(dict(label=label, kind=m[1], file=p.relative_to(root).as_posix(),
                           line=content[:m.start()].count("\n") + 1,
                           lexical_sources=sorted(set(sources)),
                           lexical_labels=sorted(set(s for s in strings if s.startswith(("//", ":"))))))
    ninja_files = sorted((build / "out/Default/obj/net").rglob("*.ninja")) if build.exists() else []
    ninja_sources = collections.defaultdict(list)
    for p in ninja_files:
        for s in set(re.findall(r'\.\./\.\./(net/[^\s|]+\.(?:cc|mm|c|rs))', read(p))):
            ninja_sources[s].append(p.relative_to(build).as_posix())
    entries = []
    for p in files:
        rel = p.relative_to(root).as_posix()
        owner = next((c for c in components if (c["id"] != "root" or rel.count("/") == 1)
                      and any(fnmatch.fnmatchcase(rel, g) for g in c["paths"])), None)
        assert owner, f"Unassigned source: {rel} (add an explicit component)"
        lower = rel.lower()
        if rel.startswith("net/third_party/"):
            category = "external_dependency"
        elif rel.startswith("net/data/"):
            category = "test_data"
        elif rel.startswith(("net/test/", "net/tools/", "net/extras/")):
            category = "tests" if rel.startswith("net/test/") else "tools_and_support"
        elif any(s in lower for s in ("unittest", "perftest", "fuzzer", "fuzztest", "test_util", "mock_", "test_")):
            category = "tests"
        elif p.suffix in (".proto", ".mojom", ".template") or "generated" in lower:
            category = "generated_or_generator_input"
        elif p.suffix == ".rs":
            category = "rust_source"
        elif p.suffix in (".cc", ".mm", ".c"):
            category = "implementation"
        elif p.suffix in (".h", ".hpp"):
            category = "header"
        else:
            category = "build_docs_data_or_other"
        platform = sorted(set(re.findall(r'(?:^|[/_])(win|posix|linux|mac|ios|android|fuchsia|apple)(?:[/_.]|$)', lower)))
        text = read(p) if p.suffix in (".rs", ".cc", ".mm", ".h", ".c") else ""
        includes = sorted(set(re.findall(r'#include\s+["<]((?:base|url|crypto|third_party|net)/[^">]+)', text)))
        decision = next((d for d in data["file_decisions"] if fnmatch.fnmatchcase(rel, d["path"])), None)
        state = (decision["state"] if decision else "external dependency" if category == "external_dependency" else
                 "tests" if category in ("tests", "test_data") else
                 "platform adapter" if platform and category in ("implementation", "header") else
                 "Rust source; migration unverified" if category == "rust_source" else "C++" if category in ("implementation", "header") else "support")
        entries.append(dict(path=rel, sha256=sha(p), component=owner["id"], category=category,
                            implementation_state=state, platform_tags=platform,
                            classification_basis="explicit decision" if decision else "path/content inventory heuristic; review per slice",
                            dependencies=includes,
                            gn_lexical_targets=sorted(set(source_targets[rel])),
                            configured_ninja_files=ninja_sources[rel],
                            decision_note=decision.get("note") if decision else None))
    data["inventory"] = entries
    # Sparse editing trees do not contain every vendored implementation. List
    # the configured copy separately rather than silently treating it as absent.
    for external in data["external_sources"]:
        relative = external["path"]
        variants = []
        for tree in (root, build):
            directory = tree / relative
            sources = sorted(p.relative_to(tree).as_posix() for p in directory.rglob("*")
                             if p.is_file() and p.suffix in (".cc", ".cpp", ".c", ".h", ".hpp", ".mm", ".rs")) if directory.exists() else []
            variants.append(dict(root=str(tree), exists=directory.exists(),
                                 source_paths=sources, count=len(sources)))
        external["source_locations"] = variants
        external["inventory_note"] = "Path inventory only; external bytes not hashed or validated; no replacement claimed. Editing and configured copies are distinct."
    data["build_evidence"] = dict(gn_targets=gn, gn_semantics="lexical, includes inactive branches; not evaluated",
        gn_manifests=[dict(path=p.relative_to(root).as_posix(), sha256=sha(p)) for p in files if p.name == "BUILD.gn"],
        configured_root=str(build), ninja_files=[dict(path=p.relative_to(build).as_posix(), sha256=sha(p)) for p in ninja_files],
        limitation="Windows out/Default only; files may be stale. No GN/Ninja execution or configured validation performed.")
    data["summary"] = dict(total_files=len(entries), categories=dict(collections.Counter(e["category"] for e in entries)),
        components={c["id"]: dict(collections.Counter(e["category"] for e in entries if e["component"] == c["id"])) for c in components},
        warning="File counts are inventory, never module completion percentages.")
    data["refreshed_utc"] = datetime.datetime.now(datetime.timezone.utc).isoformat()
    status.write_text(json.dumps(data, indent=2) + "\n", encoding="utf-8")
    return data


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, default=Path(__file__).resolve().parents[1])
    parser.add_argument("--build-root", type=Path, default=Path("D:/RustiumBuild/src"))
    args = parser.parse_args()
    root = args.root.resolve()
    data = refresh(root, root / "docs/rust/net-migration-status.json", args.build_root)
    print(json.dumps(data["summary"]["categories"], sort_keys=True))


if __name__ == "__main__":
    main()
