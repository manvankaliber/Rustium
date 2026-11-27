#!/usr/bin/env python3
"""Count maintained implementation code in HEAD plus local source edits."""

from collections import Counter
import hashlib
import importlib.util
import inspect
import json
from pathlib import Path
import re
import sqlite3
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
SCRIPTS = ROOT / ".github/scripts"
sys.path.insert(0, str(SCRIPTS))
spec = importlib.util.spec_from_file_location("roadmap_codebase_metrics", SCRIPTS / "codebase_metrics.py")
scanner = importlib.util.module_from_spec(spec)
spec.loader.exec_module(scanner)

LANGUAGES = {"C", "C++", "C/C++", "Objective-C", "Objective-C++", "Rust",
             "Java", "Kotlin", "Swift", "Go", "Dart", "JavaScript", "TypeScript"}
SUPPORT_DIRS = {"tools", "tooling", "scripts", "build", "buildtools", "docs", "agents",
                "benchmarks-site", "out", ".scratch", "examples", "samples"}
TEST_DIRS = {"test", "tests", "testing", "testdata", "test_data", "test_support", "test_helpers", "junit", "javatests",
             "test_util", "test_utils", "fuzz", "fuzzers", "fuzzing", "benchmarks",
             "perftests", "perf_tests", "mocks", "mock", "fake"}
TEST_NAME = re.compile(
    r"(?:^|_)(?:test|tests|unittest|unittests|browsertest|browsertests|"
    r"uitest|apitest|pixeltest|perftest|perftests|perf_test|perf_tests|"
    r"fuzzer|fuzztest|fuzz|benchmark|benchmarks|mock|fake|testsupport|testing)(?:_|\.|$)"
    r"|(?:Test|Tests)\.[^.]+$|(?:^|[a-z])(?:Test|Mock|Fake)[A-Z]")
TEST_ATTRIBUTE = r"#\s*\[\s*(?:cfg\s*\(\s*test\s*\)|test)\s*\]"
# Strings/comments are tokens so their braces and attribute-looking text cannot
# change item boundaries. Lifetimes are not character literals.
RUST_TOKENS = re.compile(
    r'(?P<raw>(?:br|cr|r)(?P<hashes>\#*)"[\s\S]*?"(?P=hashes))'
    r'|(?P<string>(?:b|c)?"(?:\\[\s\S]|[^"\\])*")'
    r"|(?P<char>b?'(?:\\(?:u\{[0-9a-fA-F_]+\}|x[0-9a-fA-F]{2}|.)|[^'\\\r\n])')"
    r'|(?P<line>//[^\n]*)|(?P<block>/\*|\*/)'
    r'|(?P<test>' + TEST_ATTRIBUTE + r')|(?P<brace>[{};])')


def without_rust_tests(data):
    """Remove test-only items and comments, preserving source line boundaries."""
    text = data.decode("utf-8", errors="replace")
    output, previous, start, depth, position = [], 0, None, None, 0
    while token := RUST_TOKENS.search(text, position):
        position = token.end()
        kind = token.lastgroup
        if kind == "block":
            if token.group() == "/*":
                nesting = 1
                for marker in re.finditer(r"/\*|\*/", text[position:]):
                    nesting += 1 if marker.group() == "/*" else -1
                    if nesting == 0:
                        position += marker.end()
                        break
                else:
                    raise ValueError("Unterminated Rust block comment")
            if start is None:
                output.append(text[previous:token.start()])
                output.append("\n" * text[token.start():position].count("\n"))
                previous = position
            continue
        if kind == "line":
            if start is None:
                output.append(text[previous:token.start()])
                previous = position
            continue
        if kind == "test" and start is None:
            start = token.start()
        if start is None or kind != "brace":
            continue
        char = token.group()
        if depth is None:
            if char == "{":
                depth = 1
            elif char == ";":
                output.append(text[previous:start])
                output.append("\n" * text[start:token.end()].count("\n"))
                previous, start = token.end(), None
        else:
            depth += (char == "{") - (char == "}")
            if depth == 0:
                output.append(text[previous:start])
                output.append("\n" * text[start:token.end()].count("\n"))
                previous, start, depth = token.end(), None, None
    if start is not None:
        raise ValueError("Unterminated Rust test item; refusing a partial count")
    output.append(text[previous:])
    return "".join(output).encode("utf-8")


def classify(path):
    parts = path.lower().split("/")
    name = Path(path).name
    if any(p in SUPPORT_DIRS for p in parts[:-1]):
        return None, "documentation_tooling_examples_or_dashboard"
    if any(p in TEST_DIRS for p in parts[:-1]) or TEST_NAME.search(name):
        return None, "tests_fuzzers_benchmarks_or_test_helpers"
    if path.startswith(("net/", "ui/base/resource/")) and re.search(
            r"(?:_reference(?:[_.]|$)|_rust_original(?:[_.]|$)|_test_support|_test_state|_test_adapter|_test_utils|_differential)", name):
        return None, "frozen_reference_or_validation_adapter"
    if re.search(r"(?:\.config|\.conf)\.[^.]+$", name):
        return None, "configuration"
    category, reason = scanner.classify_path(path)
    if category != "maintained":
        return None, reason or category
    language = scanner.LANGUAGES.get(Path(path).suffix.lower())
    if language not in LANGUAGES:
        return None, "non_implementation_language"
    return language, ""


def count():
    head = scanner.git("rev-parse", "HEAD").decode().strip()
    committed = {path: oid for path, oid, _ in scanner.committed_files(head)}
    # Limit Git's content checks to implementation languages and maintained
    # directories. An index rebuilt by another integration can have stale stat
    # entries for every file, including hundreds of thousands of large assets.
    pathspecs = [f":(glob)**/*{suffix}" for suffix, language in scanner.LANGUAGES.items()
                 if language in LANGUAGES]
    pathspecs += [f":(glob,exclude)**/{directory}/**"
                  for directory in sorted(SUPPORT_DIRS | TEST_DIRS | {"third_party", "vendor"})]
    # Raw stat changes are cheap even when the index has lost its stat cache.
    # Verify candidate contents below, rather than diffing every blob in Git.
    dirty = set(scanner.git("diff-files", "--name-only", "--no-renames", "-z",
                            "--", *pathspecs).decode().split("\0")) - {""}
    dirty.update(set(scanner.git("diff-index", "--cached", "--name-only", "--no-renames",
                                "-z", head, "--", *pathspecs).decode().split("\0")) - {""})
    untracked = set(scanner.git("ls-files", "--others", "--exclude-standard", "-z").decode().split("\0")) - {""}
    excluded, languages = Counter(), Counter()
    selected, local, missing = [], {}, {}
    for path in sorted(committed.keys() | dirty | untracked):
        language, reason = classify(path)
        if language is None:
            excluded[reason] += 1
            continue
        if path in dirty or path in untracked:
            p = ROOT / path
            if p.is_symlink() or not p.is_file():
                excluded["deleted_or_non_regular"] += 1
                continue
            data = p.read_bytes()
            # Git may normalize CRLF text to LF. Either spelling has the same
            # code lines; matching the committed blob avoids redundant parsing
            # without trusting timestamps as proof of unchanged contents.
            baseline = committed.get(path)
            candidates = [data]
            if b"\r\n" in data:
                candidates.append(data.replace(b"\r\n", b"\n"))
            if baseline and any(hashlib.sha1(b"blob " + str(len(value)).encode() + b"\0" + value).hexdigest() == baseline
                                for value in candidates):
                missing.setdefault(baseline, []).append((path, language))
                continue
            oid = hashlib.sha256(data).hexdigest()
            local[path] = {"sha256": oid, "untracked": path in untracked}
            result = analyze(data, language)
            selected.append((path, oid, language, result))
        else:
            missing.setdefault(committed[path], []).append((path, language))
    # Reuse the established per-blob comment-count cache read-only when its
    # scanner hash matches. Rust is rescanned to remove inline test-only items.
    cache_path = Path(scanner.git("rev-parse", "--git-path", "codebase-metrics.sqlite3").decode().strip())
    if not cache_path.is_absolute():
        cache_path = ROOT / cache_path
    cache = sqlite3.connect(cache_path.as_uri() + "?mode=ro", uri=True) if cache_path.exists() else None
    own_cache = scanner.BlobCache(cache_path.with_name("core-source-metrics.sqlite3"))
    # Scope changes do not invalidate unchanged per-blob lexical counts.
    cache_suffix = ":" + hashlib.sha256((RUST_TOKENS.pattern +
        inspect.getsource(without_rust_tests) + inspect.getsource(analyze)).encode()).hexdigest()
    try:
        valid = cache and cache.execute("SELECT version FROM metadata").fetchone() == (
            hashlib.sha256((SCRIPTS / "codebase_metrics.py").read_bytes()).hexdigest(),)
        for oid in list(missing):
            pending = []
            for path, language in missing[oid]:
                result = own_cache.get(oid, language + cache_suffix)
                if result is None and valid and language != "Rust":
                    row = cache.execute("SELECT result FROM blobs WHERE oid=? AND language=?", (oid, language)).fetchone()
                    result = json.loads(row[0]) if row else None
                if result is not None:
                    selected.append((path, oid, language, result))
                else:
                    pending.append((path, language))
            if pending:
                missing[oid] = pending
            else:
                del missing[oid]
    finally:
        if cache:
            cache.close()
    try:
        for oid, data in scanner.read_blobs(list(missing)):
            for path, language in missing[oid]:
                result = analyze(data, language)
                own_cache.put(oid, language + cache_suffix, result)
                selected.append((path, oid, language, result))
    finally:
        own_cache.close()
    manifest, files = [], Counter()
    for path, oid, language, result in sorted(selected):
        if "excluded" in result:
            excluded[result["excluded"]] += 1
            continue
        languages[language] += result["actual_loc"]
        files[language] += 1
        manifest.append([path, oid, language, result["actual_loc"]])
    total = sum(languages.values())
    return {
        "counting_rules_version": 1,
        "head": head,
        "scope": "Maintained implementation source in HEAD with local edits, deletions and non-ignored untracked files applied. Submodules and vendored sources are excluded.",
        "actual_loc": total,
        "files": sum(files.values()),
        "languages": [{"name": name, "lines": lines, "files": files[name], "share": lines / total if total else 0}
                      for name, lines in sorted(languages.items(), key=lambda item: (-item[1], item[0]))],
        "method": "Count nonblank lines after recognized comments are stripped; keep code beside trailing comments. C/C++ headers and FFI count. Remove Rust #[cfg(test)] and #[test] items. Lexical source inventory, not an AST or evaluated build graph.",
        "included_languages": sorted(LANGUAGES),
        "excluded": dict(sorted(excluded.items())),
        "exclusion_rules": {"support_directories": sorted(SUPPORT_DIRS), "test_directories": sorted(TEST_DIRS),
                            "test_filename_pattern": TEST_NAME.pattern,
                            "other": "Markdown/docs, configs, lockfiles, blank/comment-only lines, markup/styles, schemas/IDL, build scripts, generated files, fixtures, frozen net rewrite references and validation adapters; inherits classifier exclusions from .github/scripts/codebase_metrics.py"},
        "local_source_overrides": local,
        "included_manifest_sha256": hashlib.sha256(json.dumps(manifest, ensure_ascii=False, separators=(",", ":")).encode()).hexdigest(),
        "scanner_sha256": {"tools/core_source_metrics.py": hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
                           ".github/scripts/codebase_metrics.py": hashlib.sha256((SCRIPTS / "codebase_metrics.py").read_bytes()).hexdigest()},
        "limitations": "Includes unaccepted local Rust implementations. Path/header rules are heuristic; platform branches and embedded test code outside recognized Rust test items are not evaluated. Not a shipped-binary or migration-completion percentage.",
    }


def analyze(data, language):
    # Detect generated headers before removing Rust test items as well.
    if scanner.GENERATED_HEADER.search(data[:4096]) or b"\0" in data:
        return {"excluded": "generated" if b"\0" not in data else "binary"}
    if language == "Rust":
        code = without_rust_tests(data).decode("utf-8")
        return {"actual_loc": sum(bool(line.strip()) for line in code.splitlines())}
    return scanner.analyze_blob(data, language)


if __name__ == "__main__":
    print(json.dumps(count(), ensure_ascii=False, indent=2))
