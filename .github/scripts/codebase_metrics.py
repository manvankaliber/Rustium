#!/usr/bin/env python3
"""Build a static, tracked-file inventory for the Chromium codebase dashboard."""

from __future__ import annotations

import json
import os
import re
import subprocess
import sys
from collections import Counter
from datetime import datetime, timezone
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
PAGE = ROOT / ".github" / "codebase-dashboard" / "index.html"

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


def ignored_tracked_paths(paths: list[bytes]) -> set[bytes]:
    result = subprocess.run(
        ["git", "check-ignore", "--no-index", "-z", "--stdin"], cwd=ROOT,
        input=b"\0".join(paths) + b"\0", stdout=subprocess.PIPE, stderr=subprocess.PIPE,
    )
    return {path for path in result.stdout.split(b"\0") if path}


def strip_comments(line: str, language: str, in_block: bool) -> tuple[str, bool, bool]:
    """Return code left on a line, whether it has a comment, and block state."""
    style = COMMENT_STYLE.get(language)
    if not style:
        return line, False, in_block
    output: list[str] = []
    comment_seen = in_block
    quote = ""
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
        char = line[index]
        if quote:
            output.append(char)
            if char == "\\" and index + 1 < len(line):
                index += 1
                output.append(line[index])
            elif char == quote:
                quote = ""
            index += 1
            continue
        if char in "'\"`":
            quote = char
            output.append(char)
            index += 1
            continue
        if style == "html" and line.startswith("<!--", index):
            comment_seen = True
            in_block = True
            index += 4
            continue
        if style in {"slash", "sql"}:
            if style == "slash" and line.startswith("/*", index):
                comment_seen = True
                in_block = True
                index += 2
                continue
            marker = "//" if style == "slash" else "--"
            if line.startswith(marker, index):
                comment_seen = True
                break
        if style == "hash" and char == "#":
            comment_seen = True
            break
        output.append(char)
        index += 1
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


def main() -> None:
    raw_paths = [path for path in git("ls-files", "-z").split(b"\0") if path]
    ignored = ignored_tracked_paths(raw_paths)
    total_lines = 0
    without_comment_only_lines = 0
    comment_lines = 0
    actual_loc = 0
    source_bytes = 0
    google_mentions = 0
    config_files = 0
    root_folders: set[str] = set()
    language_lines: Counter[str] = Counter()
    obsolete_functions: set[tuple[str, int, str]] = set()

    for raw_path in raw_paths:
        if raw_path in ignored:
            continue
        path = raw_path.decode("utf-8", errors="surrogateescape").replace("\\", "/")
        name = Path(path).name
        if name in SKIP_NAMES or name.lower().endswith(".lock"):
            continue
        if "/" in path:
            root_folders.add(path.split("/", 1)[0])
        full_path = ROOT / Path(path)
        if full_path.is_symlink() or not full_path.is_file():
            continue
        if full_path.suffix.lower() in BINARY_SUFFIXES:
            continue
        try:
            data = full_path.read_bytes()
        except OSError:
            continue
        if b"\0" in data:
            continue
        text = data.decode("utf-8", errors="replace")
        lines = text.splitlines()
        if is_configuration(path):
            config_files += 1
        google_mentions += len(GOOGLE_WORD.findall(text))

        ext = full_path.suffix.lower()
        language = LANGUAGES.get(ext)
        comment_language = language or {
            ".yaml": "YAML", ".yml": "YAML", ".toml": "TOML", ".gn": "GN",
            ".gni": "GN", ".mk": "Make", ".bzl": "Bazel", ".gradle": "Gradle",
        }.get(ext)
        is_actual_source = bool(language) and not skip_from_actual(path)
        if is_actual_source:
            source_bytes += len(data)

        in_block = False
        stripped_lines: list[str] = []
        file_comment_only_lines = 0
        for line in lines:
            if line.strip():
                total_lines += 1
            if comment_language:
                stripped, has_comment, in_block = strip_comments(line, comment_language, in_block)
                code_remains = bool(stripped.strip())
                if has_comment and line.strip():
                    comment_lines += 1
                if has_comment and not code_remains and line.strip():
                    file_comment_only_lines += 1
                if is_actual_source and code_remains:
                    actual_loc += 1
                    language_lines[language] += 1
                stripped_lines.append(stripped)
            else:
                if line.strip():
                    without_comment_only_lines += 1
        if comment_language:
            source_nonblank = sum(bool(line.strip()) for line in lines)
            without_comment_only_lines += max(0, source_nonblank - file_comment_only_lines)

        if is_actual_source and language in FUNCTION_PATTERNS:
            clean_lines = [line for line in stripped_lines]
            for index, line in enumerate(clean_lines):
                function_name = None
                patterns = FUNCTION_PATTERNS[language]
                if language in {"C", "C/C++", "C++", "Java", "Kotlin", "Go", "Swift"}:
                    patterns = [GENERIC_FUNCTION_PATTERN]
                for pattern in patterns:
                    match = pattern.match(line)
                    if match:
                        function_name = next((group for group in match.groups() if group), None)
                        if function_name is None and language in {"JavaScript", "TypeScript"}:
                            name_match = re.search(r"([\w$]+)\s*\(", line)
                            if name_match and name_match.group(1) not in {
                                "if", "for", "while", "switch", "catch", "with",
                            }:
                                function_name = name_match.group(1)
                        break
                if not function_name:
                    continue
                start = max(0, index - 6)
                preceding = "\n".join(lines[start:index + 1])
                if OBSOLETE_MARKER.search(preceding):
                    obsolete_functions.add((path, index + 1, function_name))

    sha = os.environ.get("GITHUB_SHA")
    if not sha:
        sha = git("rev-parse", "HEAD").decode().strip()
    metrics = {
        "commit": sha,
        "updated": datetime.now(timezone.utc).isoformat(),
        "lines_of_code": total_lines,
        "lines_without_comment_only_lines": without_comment_only_lines,
        "comment_lines": comment_lines,
        "actual_loc": actual_loc,
        "source_bytes": source_bytes,
        "source_gb": source_bytes / 1_000_000_000,
        "configuration_files": config_files,
        "root_folder_count": len(root_folders),
        "root_folders": sorted(root_folders, key=str.casefold),
        "google_mentions": google_mentions,
        "obsolete_functions": len(obsolete_functions),
        "obsolete_function_examples": [
            {"path": path, "line": line, "name": name}
            for path, line, name in sorted(obsolete_functions)[:20]
        ],
        "languages": [
            {"name": name, "lines": count, "share": count / actual_loc if actual_loc else 0}
            for name, count in language_lines.most_common()
        ],
        "definitions": {
            "lines_of_code": "Nonblank lines in tracked text files, including docs and configuration.",
            "lines_without_comment_only_lines": "Tracked text lines excluding full-line comments in recognized source languages; inline-comment lines remain counted.",
            "comment_lines": "Source lines containing a recognized comment, including inline comments; each line counted once.",
            "actual_loc": "Nonblank source lines after stripping recognized comments; documentation, project/config files, lockfiles, ignored files, and unsupported extensions are excluded.",
            "source_gb": "Bytes in included source files divided by 1,000,000,000; includes comments and whitespace in those files.",
            "obsolete_functions": "Heuristic count of function declarations near deprecated/obsolete/remove markers; review before treating as deletable.",
        },
    }

    output = Path(sys.argv[1]) if len(sys.argv) > 1 else ROOT / "_site"
    output.mkdir(parents=True, exist_ok=True)
    (output / "index.html").write_bytes(PAGE.read_bytes())
    (output / "metrics.json").write_text(json.dumps(metrics, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    print(f"Wrote {output / 'index.html'} and metrics.json ({actual_loc:,} actual LOC)")


if __name__ == "__main__":
    main()
