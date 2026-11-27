#!/usr/bin/env python3
"""Build a static, tracked-file inventory for the Chromium codebase dashboard."""

from __future__ import annotations

import argparse
import hashlib
import json
import os
import re
import sqlite3
import threading
import time
import subprocess
from collections import Counter
from datetime import datetime, timezone
from pathlib import Path

from metrics_history import COUNTING_RULES_VERSION, update_history


ROOT = Path(__file__).resolve().parents[2]
PAGE = ROOT / "index.html"
DASHBOARD_OUTPUTS = {"index.html", "metrics.json", "metrics-history.json"}

# Only these extensions count as source code in the stricter "actual LOC" view.
LANGUAGES = {
    ".c": "C", ".h": "C/C++", ".cc": "C++", ".cpp": "C++", ".cxx": "C++",
    ".hh": "C++", ".hpp": "C++", ".hxx": "C++", ".inc": "C/C++", ".inl": "C++",
    ".m": "Objective-C", ".mm": "Objective-C++", ".java": "Java", ".kt": "Kotlin",
    ".kts": "Kotlin", ".js": "JavaScript", ".jsx": "JavaScript", ".mjs": "JavaScript",
    ".cjs": "JavaScript", ".ts": "TypeScript", ".tsx": "TypeScript", ".py": "Python",
    ".pyi": "Python", ".rs": "Rust", ".go": "Go", ".swift": "Swift", ".dart": "Dart",
    ".html": "HTML", ".htm": "HTML", ".css": "CSS", ".scss": "SCSS", ".sass": "SCSS",
    ".less": "Less", ".sh": "Shell", ".bash": "Shell", ".zsh": "Shell", ".ps1": "PowerShell",
    ".bat": "Batch", ".cmd": "Batch", ".pl": "Perl", ".pm": "Perl", ".rb": "Ruby",
    ".php": "PHP", ".sql": "SQL", ".proto": "Protocol Buffers", ".mojom": "Mojo IDL",
    ".idl": "IDL", ".webidl": "Web IDL", ".fbs": "FlatBuffers", ".xml": "XML",
    ".svg": "SVG", ".vue": "Vue", ".svelte": "Svelte", ".gradle": "Gradle",
}

COMMENT_STYLE = {
    "C": "slash", "C/C++": "slash", "C++": "slash", "Objective-C": "slash",
    "Objective-C++": "slash", "Java": "slash", "Kotlin": "slash", "JavaScript": "slash",
    "TypeScript": "slash", "Rust": "slash", "Go": "slash", "Swift": "slash",
    "Dart": "slash", "CSS": "slash", "SCSS": "slash", "Less": "slash",
    "HTML": "html", "XML": "html", "SVG": "html", "Python": "hash", "Shell": "hash",
    "PowerShell": "hash", "Batch": "hash", "Ruby": "hash", "Perl": "hash", "PHP": "slash",
    "SQL": "sql", "Protocol Buffers": "slash", "Mojo IDL": "slash", "IDL": "slash",
    "Web IDL": "slash", "FlatBuffers": "slash", "Vue": "html", "Svelte": "html",
    "YAML": "hash", "TOML": "hash", "GN": "slash", "Make": "hash", "Bazel": "hash",
    "Gradle": "slash",
}

CONFIG_EXTENSIONS = {
    ".toml", ".yaml", ".yml", ".ini", ".cfg", ".conf", ".properties", ".xml",
    ".gn", ".gni", ".gradle", ".bzl", ".mk",
}
CONFIG_NAMES = {
    "BUILD.gn", "DEPS", ".gclient", "package.json", "Cargo.toml", "CMakeLists.txt",
    "Makefile", ".editorconfig", ".gitignore", ".gitattributes", ".clang-format",
    ".clang-tidy", ".clangd", ".bazelrc", "OWNERS",
}
CONFIG_JSON_NAMES = {"tsconfig.json", "jsconfig.json", "manifest.json", "settings.json"}
SKIP_NAMES = {
    "package-lock.json", "npm-shrinkwrap.json", "yarn.lock", "pnpm-lock.yaml",
    "cargo.lock", "Cargo.lock", "Gemfile.lock", "poetry.lock", "uv.lock",
}
NON_SOURCE_SUFFIXES = {".md", ".markdown", ".mdown", ".rst", ".yml", ".yaml", ".toml", ".json"}
BINARY_SUFFIXES = {
    ".7z", ".a", ".apk", ".aab", ".avi", ".bmp", ".class", ".dmg", ".dll", ".dylib",
    ".exe", ".flac", ".gif", ".gz", ".ico", ".jar", ".jpeg", ".jpg", ".m4a", ".mp3",
    ".mp4", ".o", ".obj", ".otf", ".pak", ".pdf", ".png", ".pyc", ".so", ".tar",
    ".tif", ".tiff", ".ttf", ".wasm", ".wav", ".webm", ".webp", ".woff", ".woff2", ".zip",
}

FUNCTION_PATTERNS = {
    "Rust": [re.compile(r"^\s*(?:pub(?:\([^)]*\))?\s+)?(?:async\s+)?(?:unsafe\s+)?fn\s+([\w]+)\s*\(")],
    "Python": [re.compile(r"^\s*(?:async\s+)?def\s+([\w]+)\s*\(")],
    "JavaScript": [re.compile(r"^\s*(?:(?:export|default|async|static|public|private|protected)\s+)*(?:function\s+([\w$]+)\s*\(|(?:[\w$]+)\s*\([^;]*\)\s*\{)")],
    "TypeScript": [re.compile(r"^\s*(?:(?:export|default|async|static|public|private|protected)\s+)*(?:function\s+([\w$]+)\s*\(|(?:[\w$]+)\s*\([^;]*\)\s*\{)")],
    "C": [],
    "C/C++": [],
    "C++": [],
    "Java": [],
    "Kotlin": [],
    "Go": [],
    "Swift": [],
}
GENERIC_FUNCTION_PATTERN = re.compile(
    r"^\s*(?:[\w:<>,*&\[\]\s~]+\s+)(?:[\w~]+::)?(~?[\w]+)\s*"
    r"\([^;{}]*\)\s*(?:const\s*)?(?:noexcept\s*)?(?:override\s*)?"
    r"(?:final\s*)?[;{]"
)
OBSOLETE_MARKER = re.compile(
    r"\b(?:deprecated|obsolete|do not use|remove (?:this|it|me)|TODO\s*[:(]?.*\b(?:remove|delete))\b",
    re.IGNORECASE,
)
GOOGLE_WORD = re.compile(r"\bGoogle\b", re.IGNORECASE)


def git(*args: str, input_bytes: bytes | None = None) -> bytes:
    return subprocess.run(
        ["git", *args], cwd=ROOT, input=input_bytes, stdout=subprocess.PIPE,
        stderr=subprocess.PIPE, check=True,
    ).stdout


# Consume ordinary text and quoted strings in the regex engine rather than
# visiting every character in Python. Unterminated strings run to end of line,
# matching the original line-oriented scanner.
_QUOTED = r"\"(?:\\.|[^\"\\])*(?:\"|\\?$)|'(?:\\.|[^'\\])*(?:'|\\?$)|`(?:\\.|[^`\\])*(?:`|\\?$)"
_COMMENT_TOKENS = {
    style: re.compile(_QUOTED + "|" + marker)
    for style, marker in {
        "slash": r"/\*|//", "html": r"<!--", "hash": r"#", "sql": r"--",
    }.items()
}


def strip_comments(line: str, language: str, in_block: bool) -> tuple[str, bool, bool]:
    """Return code left on a line, whether it has a comment, and block state."""
    style = COMMENT_STYLE.get(language)
    if not style:
        return line, False, in_block
    tokens = _COMMENT_TOKENS[style]
    output: list[str] = []
    comment_seen = in_block
    index = 0
    while index < len(line):
        if in_block:
            close = "-->" if style == "html" else "*/"
            end = line.find(close, index)
            if end < 0:
                return "".join(output), True, True
            in_block = False
            index = end + len(close)
            continue
        match = tokens.search(line, index)
        if match is None:
            output.append(line[index:])
            break
        output.append(line[index:match.start()])
        token = match.group()
        if token[0] in "'\"`":
            output.append(token)
            index = match.end()
            continue
        comment_seen = True
        if token in {"/*", "<!--"}:
            in_block = True
            index = match.end()
        else:
            break
    return "".join(output), comment_seen, in_block


def is_configuration(path: str) -> bool:
    name = Path(path).name
    return (
        name in CONFIG_NAMES
        or name in CONFIG_JSON_NAMES
        or name.endswith(".config.json")
        or Path(name).suffix.lower() in CONFIG_EXTENSIONS
    )


def skip_from_actual(path: str) -> bool:
    name = Path(path).name
    lowered = name.lower()
    suffix = Path(name).suffix.lower()
    if name in SKIP_NAMES or lowered.endswith(".lock"):
        return True
    if name.upper() == "OWNERS" or name.upper().startswith("README"):
        return True
    if suffix in NON_SOURCE_SUFFIXES or suffix in CONFIG_EXTENSIONS or suffix not in LANGUAGES:
        return True
    if name in CONFIG_NAMES:
        return True
    return False



# These are explicit source-scope rules, not filesystem ignore rules. Keeping
# them here makes the same committed snapshot produce the same counts on CI.
SKIP_NAMES_LOWER = {name.lower() for name in SKIP_NAMES}
FIXTURE_DIRS = {"testdata", "test_data", "fixtures", "corpus", "corpora"}
GENERATED_DIRS = {"gen", "generated", "node_modules", "__pycache__"}
GENERATED_SUFFIXES = (".min.js", ".min.css", ".bundle.js", ".bundle.css",
                      ".pb.cc", ".pb.h", ".pb.c", ".pb.go", "_pb2.py",
                      ".generated.cc", ".generated.h", ".generated.ts")
GENERATED_HEADER = re.compile(
    rb"^[ \t]*(?://|/\*+|\*|#|<!--|--)[ \t]*"
    rb"(?:(?:this (?:file|code) (?:is|was) |@)?"
    rb"(?:auto[- ]generated|automatically generated|generated (?:by|from))|"
    rb"code generated .*do not edit|<auto-generated)", re.IGNORECASE | re.MULTILINE)
COUNT_KEYS = ("lines_of_code", "lines_without_comment_only_lines", "comment_lines",
              "actual_loc", "source_bytes", "google_mentions")


def classify_path(path: str) -> tuple[str | None, str]:
    """Return a source category or a reason to skip before reading content."""
    parts = path.lower().split("/")
    name = parts[-1]
    if path in DASHBOARD_OUTPUTS:
        return None, "dashboard"
    if name in SKIP_NAMES_LOWER or name.endswith(".lock"):
        return None, "lockfile"
    if Path(path).suffix.lower() in BINARY_SUFFIXES:
        return None, "binary"
    if any(part in GENERATED_DIRS for part in parts[:-1]) or name.endswith(GENERATED_SUFFIXES):
        return None, "generated"
    if (any(part in FIXTURE_DIRS for part in parts[:-1])
            or any(parts[i] in {"test", "tests", "testing"} and parts[i + 1] == "data"
                   for i in range(len(parts) - 1))):
        return None, "fixture"
    if skip_from_actual(path):
        return None, "configuration" if is_configuration(path) else "non_source"
    if "third_party" in parts[:-1] or "vendor" in parts[:-1]:
        return "vendored", ""
    if (any(part in {"test", "tests", "testing"} for part in parts[:-1])
            or re.search(r"(?:^test_|_test\.|_tests\.|_unittest\.|_unittests\.|"
                         r"_browsertest\.|_browsertests\.|\.test\.|\.spec\.)", name)
            or Path(path).stem.endswith("Test") or Path(path).stem.endswith("Tests")):
        return "tests", ""
    return "maintained", ""


def analyze_blob(data: bytes, language: str) -> dict:
    # Inspect the header only: an ordinary source file may mention generated
    # code later without itself being generated. This is intentionally heuristic.
    if b"\0" in data:
        return {"excluded": "binary"}
    if GENERATED_HEADER.search(data[:4096]):
        return {"excluded": "generated"}
    lines = data.decode("utf-8", errors="replace").splitlines()
    counts = dict.fromkeys(COUNT_KEYS, 0)
    counts["source_bytes"] = len(data)
    clean_lines = []
    in_block = False
    for line in lines:
        nonblank = bool(line.strip())
        stripped, has_comment, in_block = strip_comments(line, language, in_block)
        code_remains = bool(stripped.strip())
        counts["lines_of_code"] += nonblank
        counts["comment_lines"] += has_comment and nonblank
        counts["lines_without_comment_only_lines"] += nonblank and (not has_comment or code_remains)
        counts["actual_loc"] += code_remains
        counts["google_mentions"] += len(GOOGLE_WORD.findall(line))
        clean_lines.append(stripped)

    obsolete = []
    if language in FUNCTION_PATTERNS:
        marker_lines = [i for i, line in enumerate(lines) if OBSOLETE_MARKER.search(line)]
        candidates = sorted({i for marker in marker_lines
                             for i in range(marker, min(marker + 7, len(lines)))})
        patterns = FUNCTION_PATTERNS[language] or [GENERIC_FUNCTION_PATTERN]
        for index in candidates:
            line = clean_lines[index]
            if "(" not in line:
                continue
            for pattern in patterns:
                match = pattern.match(line)
                if not match:
                    continue
                name = next((group for group in match.groups() if group), None)
                if name is None and language in {"JavaScript", "TypeScript"}:
                    name_match = re.search(r"([\w$]+)\s*\(", line)
                    if name_match and name_match.group(1) not in {
                        "if", "for", "while", "switch", "catch", "with",
                    }:
                        name = name_match.group(1)
                if name:
                    obsolete.append([index + 1, name])
                break
    return {**counts, "obsolete": obsolete}


def committed_files(revision: str) -> list[tuple[str, str, str]]:
    """One tree listing; excludes symlinks, submodules, and Git internals."""
    entries = []
    for record in git("ls-tree", "-rz", "--full-tree", revision).split(b"\0"):
        if not record:
            continue
        metadata, raw_path = record.split(b"\t", 1)
        mode, kind, oid = metadata.split()
        if kind == b"blob" and mode in {b"100644", b"100755"}:
            entries.append((raw_path.decode("utf-8", errors="surrogateescape"),
                            oid.decode("ascii"), mode.decode("ascii")))
    return entries


def read_blobs(oids: list[str]):
    """Stream requested objects through one Git process, bounded to one blob.

    A writer runs alongside the reader to prevent stdin/stdout pipe deadlocks.
    Never use --batch-all-objects: that would include history and unused blobs.
    """
    if not oids:
        return
    process = subprocess.Popen(["git", "cat-file", "--batch"], cwd=ROOT,
                               stdin=subprocess.PIPE, stdout=subprocess.PIPE)
    errors = []

    def write_requests():
        try:
            for oid in oids:
                process.stdin.write(oid.encode("ascii") + b"\n")
            process.stdin.close()
        except (OSError, ValueError) as error:
            errors.append(error)

    writer = threading.Thread(target=write_requests, daemon=True)
    writer.start()
    try:
        for oid in oids:
            header = process.stdout.readline().split()
            if len(header) != 3 or header[0].decode("ascii") != oid or header[1] != b"blob":
                raise RuntimeError(f"Invalid Git object response for {oid}: {header!r}")
            size = int(header[2])
            data = process.stdout.read(size)
            if len(data) != size or process.stdout.read(1) != b"\n":
                raise RuntimeError(f"Truncated Git object {oid}")
            yield oid, data
        writer.join()
        if process.wait() or errors:
            raise RuntimeError(f"Git batch reader failed: {errors}")
    finally:
        if process.poll() is None:
            process.terminate()
        process.wait()
        writer.join()
        process.stdout.close()
        if not process.stdin.closed:
            process.stdin.close()


class BlobCache:
    def __init__(self, path: Path):
        path.parent.mkdir(parents=True, exist_ok=True)
        self.db = sqlite3.connect(path)
        self.db.execute("CREATE TABLE IF NOT EXISTS metadata (version TEXT)")
        self.db.execute("CREATE TABLE IF NOT EXISTS blobs "
                        "(oid TEXT, language TEXT, result TEXT, PRIMARY KEY (oid, language))")
        # All scanner and scope changes invalidate old results automatically.
        version = hashlib.sha256(Path(__file__).read_bytes()).hexdigest()
        row = self.db.execute("SELECT version FROM metadata").fetchone()
        if row != (version,):
            self.db.execute("DELETE FROM blobs")
            self.db.execute("DELETE FROM metadata")
            self.db.execute("INSERT INTO metadata VALUES (?)", (version,))
        self.db.commit()

    def get(self, oid: str, language: str):
        row = self.db.execute("SELECT result FROM blobs WHERE oid=? AND language=?",
                              (oid, language)).fetchone()
        return json.loads(row[0]) if row else None

    def put(self, oid: str, language: str, result: dict):
        self.db.execute("INSERT OR REPLACE INTO blobs VALUES (?, ?, ?)",
                        (oid, language, json.dumps(result)))

    def close(self):
        self.db.commit()
        self.db.close()


def build_metrics(revision: str, cache_path: Path) -> dict:
    started = time.monotonic()
    sha = git("rev-parse", "--verify", revision + "^{commit}").decode().strip()
    print(f"Listing committed files at {sha[:12]}...", flush=True)
    entries = committed_files(sha)
    roots = sorted({path.split("/", 1)[0] for path, _, _ in entries if "/" in path}, key=str.casefold)
    categories = {name: {**dict.fromkeys(COUNT_KEYS, 0), "files": 0,
                         "languages": Counter(), "obsolete_functions": 0}
                  for name in ("maintained", "tests", "vendored")}
    excluded = Counter()
    config_files = 0
    selected = []
    for path, oid, _ in entries:
        config_files += is_configuration(path)
        category, reason = classify_path(path)
        if category is None:
            excluded[reason] += 1
        else:
            selected.append((path, oid, LANGUAGES[Path(path).suffix.lower()], category))
    print(f"Selected {len(selected):,} source paths out of {len(entries):,}; "
          f"excluded {sum(excluded.values()):,} without opening files", flush=True)
    cache = BlobCache(cache_path)
    examples = []
    hits = 0
    misses = {}

    def accumulate(path, category, language, result):
        if "excluded" in result:
            excluded[result["excluded"]] += 1
            return
        target = categories[category]
        target["files"] += 1
        for key in COUNT_KEYS:
            target[key] += result[key]
        target["languages"][language] += result["actual_loc"]
        target["obsolete_functions"] += len(result["obsolete"])
        if category == "maintained":
            examples.extend({"path": path, "line": line, "name": name}
                            for line, name in result["obsolete"])

    try:
        for path, oid, language, category in selected:
            result = cache.get(oid, language)
            if result is not None:
                hits += 1
                accumulate(path, category, language, result)
            else:
                misses.setdefault(oid, []).append((path, language, category))
        print(f"Cache: {hits:,} path hits; reading {len(misses):,} unique blobs via Git batch", flush=True)
        for number, (oid, data) in enumerate(read_blobs(list(misses)), 1):
            results = {}
            for path, language, category in misses[oid]:
                if language not in results:
                    results[language] = analyze_blob(data, language)
                    cache.put(oid, language, results[language])
                accumulate(path, category, language, results[language])
            if number % 1000 == 0:
                cache.db.commit()
                print(f"Read {number:,}/{len(misses):,} blobs "
                      f"({time.monotonic() - started:.1f}s)", flush=True)
    finally:
        cache.close()

    for target in categories.values():
        loc = target["actual_loc"]
        target["languages"] = [
            {"name": name, "lines": count, "share": count / loc if loc else 0}
            for name, count in sorted(target["languages"].items(), key=lambda item: (-item[1], item[0]))]
        target["source_gb"] = target["source_bytes"] / 1_000_000_000
    maintained = categories["maintained"]
    return {
        "counting_rules_version": COUNTING_RULES_VERSION,
        **maintained, "commit": sha, "updated": datetime.now(timezone.utc).isoformat(),
        "configuration_files": config_files, "root_folder_count": len(roots), "root_folders": roots,
        "obsolete_function_examples": sorted(examples, key=lambda item: (item["path"], item["line"], item["name"]))[:20],
        "categories": categories, "excluded_files": dict(sorted(excluded.items())),
        "scan": {"tracked_files": len(entries), "selected_files": len(selected),
                 "cached_paths": hits, "read_blobs": len(misses),
                 "elapsed_seconds": round(time.monotonic() - started, 3)},
        "definitions": {
            "scope": "Committed snapshot only; local edits and untracked files are not counted. "
                     "Maintained production source is the headline; tests and vendored source are separate.",
            "lines_of_code": "Nonblank maintained production source lines, including comments.",
            "lines_without_comment_only_lines": "Maintained production source excluding blank and comment-only lines.",
            "actual_loc": "Maintained production source after recognized comments are stripped. "
                          "Excludes docs, configuration, lockfiles, binaries, unsupported formats, "
                          "generated code and fixtures; generated-header detection is heuristic.",
            "comment_lines": "Maintained production source lines containing recognized comments.",
            "source_gb": "Included maintained production source bytes divided by one billion.",
            "configuration_files": "Configuration paths counted from the tree without reading their contents.",
            "google_mentions": "Case-insensitive whole-word matches in maintained production source.",
            "obsolete_functions": "Heuristic declarations within six lines of obsolete/removal markers.",
        },
    }


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", nargs="?", type=Path, default=ROOT)
    parser.add_argument("--revision", default="HEAD", help="Committed snapshot to measure (default HEAD)")
    parser.add_argument("--cache", type=Path, help="SQLite cache path (default inside Git metadata)")
    args = parser.parse_args()
    cache_path = args.cache
    if cache_path is None:
        cache_path = Path(os.fsdecode(git("rev-parse", "--git-path", "codebase-metrics.sqlite3").strip()))
        if not cache_path.is_absolute():
            cache_path = ROOT / cache_path
    metrics = build_metrics(args.revision, cache_path)
    args.output.mkdir(parents=True, exist_ok=True)
    (args.output / "index.html").write_bytes(PAGE.read_bytes())
    (args.output / "metrics.json").write_text(
        json.dumps(metrics, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    update_history(args.output / "metrics-history.json", metrics)
    print(f"Wrote {args.output / 'metrics.json'} "
          f"({metrics['actual_loc']:,} maintained production LOC; "
          f"{metrics['scan']['elapsed_seconds']:.1f}s)", flush=True)


if __name__ == "__main__":
    main()
