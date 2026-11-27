#!/usr/bin/env python3
"""Read the root license bundle and restore legacy paths for upstream builds."""

import argparse
from functools import lru_cache
import hashlib
import json
import re
from collections import Counter, defaultdict
from pathlib import Path, PurePosixPath

ROOT = Path(__file__).resolve().parents[2]
MARKER = b"\n===== RUSTIUM CONSOLIDATED LICENSES v1 =====\n"
BEGIN = b"----- BEGIN LICENSE RECORD -----\n"
END = b"\n----- END LICENSE RECORD -----\n"
COMPACT_MARKER = b"\n===== RUSTIUM CONSOLIDATED LICENSES v2 =====\n"
SHARED_BEGIN = b"----- BEGIN SHARED TERMS -----\n"
SHARED_END = b"\n----- END SHARED TERMS -----\n"


def checked_path(root, name):
    relative = PurePosixPath(name)
    if (not relative.parts or relative.parts[0] == '.git'
            or relative.is_absolute() or ".." in relative.parts
            or "\\" in name or ":" in name):
        raise ValueError(f"Unsafe license path: {name}")
    target = root.joinpath(*relative.parts)
    if not target.resolve().is_relative_to(root.resolve()):
        raise ValueError(f"License path escapes repository: {name}")
    return target


def parse_bundle(data):
    """Return original paths and byte-exact contents, validating every record."""
    compact = COMPACT_MARKER in data
    marker = COMPACT_MARKER if compact else MARKER
    if marker not in data:
        return {}
    _, remaining = data.split(marker, 1)
    entries = {}
    shared = {}
    documents = []
    while remaining:
        is_shared = remaining.startswith(SHARED_BEGIN)
        begin, end = (SHARED_BEGIN, SHARED_END) if is_shared else (BEGIN, END)
        if is_shared and not compact:
            raise ValueError("Shared terms require bundle version 2")
        if not remaining.startswith(begin):
            raise ValueError("Invalid license record header")
        metadata, remaining = remaining[len(begin):].split(b"\n", 1)
        record = json.loads(metadata)
        size = record["text_bytes"]
        if not isinstance(size, int) or size < 0:
            raise ValueError("Invalid license record length")
        body, remaining = remaining[:size], remaining[size:]
        if not remaining.startswith(end):
            raise ValueError("Truncated license record")
        remaining = remaining[len(end):]
        if is_shared:
            if record["id"] in shared:
                raise ValueError("Duplicate shared terms identifier")
            if hashlib.sha256(body).hexdigest() != record["sha256"]:
                raise ValueError("Shared terms checksum mismatch")
            shared[record["id"]] = body.decode("utf-8")
        else:
            documents.append((record, body.decode("utf-8")))
    for record, text in documents:
        pieces = []
        position = 0
        for reference in record.get("references", []):
            start = reference["start"]
            marker_text = reference_marker(reference["shared"])
            if (start < position or text[start:start + len(marker_text)] != marker_text):
                raise ValueError("Invalid shared terms reference position")
            pieces.append(text[position:start])
            if reference["shared"] not in shared:
                raise ValueError("Missing shared terms")
            terms = restore_spacing(shared[reference["shared"]],
                                    reference.get("spacing", []))
            pieces.append(terms)
            position = start + len(marker_text)
        pieces.append(text[position:])
        original = "".join(pieces).encode(record["encoding"])
        if hashlib.sha256(original).hexdigest() != record["sha256"]:
            raise ValueError("License record checksum mismatch")
        for name in record["paths"]:
            checked_path(ROOT, name)
            if name in entries:
                raise ValueError(f"Duplicate license path: {name}")
            entries[name] = original
    return entries


# Only identical non-whitespace text can be shared. These patterns find
# boundaries; they do not assign SPDX identities or equate license variants.
TERM_PATTERNS = (
    ("MIT-terms", r"Permission is hereby granted, free of charge.*?"
     r"other dealings in (?:the|this) software\."),
    ("BSD-terms", r"Redistribution and use in source and binary forms.*?"
     r"EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE\."),
    ("Apache-terms", r"Apache License\s+Version 2\.0.*?END OF TERMS AND CONDITIONS"),
    ("GPL-terms", r"GNU (?:LESSER |LIBRARY )?GENERAL PUBLIC LICENSE.*?"
     r"END OF TERMS AND CONDITIONS"),
)


def reference_marker(identifier):
    return f"[See shared terms: {identifier}]"


def word_layout(text):
    matches = list(re.finditer(r"\S+", text))
    words = [match.group() for match in matches]
    gaps = [text[left.end():right.start()]
            for left, right in zip(matches, matches[1:])]
    return words, gaps, matches


def restore_spacing(text, overrides):
    words, gaps, _ = word_layout(text)
    seen = set()
    for index, whitespace in overrides:
        if (not 0 <= index < len(gaps) or index in seen
                or not whitespace or not whitespace.isspace()):
            raise ValueError("Invalid whitespace restoration entry")
        seen.add(index)
        gaps[index] = whitespace
    return "".join(word + (gaps[i] if i < len(gaps) else "")
                   for i, word in enumerate(words))


def _serialize_record(metadata, text, shared=False):
    body = text.encode("utf-8")
    metadata = dict(metadata, text_bytes=len(body))
    begin, end = (SHARED_BEGIN, SHARED_END) if shared else (BEGIN, END)
    return (begin + json.dumps(metadata, ensure_ascii=False,
                               separators=(",", ":")).encode("utf-8")
            + b"\n" + body + end)


def compact_bundle(entries):
    """Share repeated full terms while retaining all original words and bytes.

    Whitespace-only layout differences are recorded separately so upstream
    tools can recover their original files. Required notices stay visible;
    nothing is paraphrased, inferred from a license name, or externally linked.
    """
    groups = defaultdict(list)
    for name, data in entries.items():
        checked_path(ROOT, name)
        groups[data].append(name)
    documents = []
    candidates = {}
    for data, paths in sorted(groups.items(), key=lambda item: item[1][0]):
        try:
            text, encoding = data.decode("utf-8"), "utf-8"
        except UnicodeDecodeError:
            text, encoding = data.decode("latin-1"), "latin-1"
        words, gaps, matches = word_layout(text)
        normal = " ".join(words)
        offsets = {}
        offset = 0
        for index, word in enumerate(words):
            offsets[offset] = index
            offset += len(word) + 1
        doc = dict(data=data, paths=sorted(paths), text=text, encoding=encoding,
                   normal=normal, matches=matches, offsets=offsets)
        documents.append(doc)
        spans = []
        if len(words) >= 80:
            spans.append(("Terms", 0, len(normal)))
        for label, pattern in TERM_PATTERNS:
            for match in re.finditer(pattern, normal, re.IGNORECASE):
                if len(match.group().split()) >= 80:
                    spans.append((label, match.start(), match.end()))
        for label, start, end in spans:
            if start not in offsets or end + 1 not in offsets and end != len(normal):
                continue
            first = offsets[start]
            last = offsets.get(end + 1, len(words)) - 1
            original = text[matches[first].start():matches[last].end()]
            candidates.setdefault(normal[start:end], (label, original))
    candidates = {text: info for text, info in candidates.items()
                  if sum((" " + text + " ") in (" " + doc["normal"] + " ")
                         for doc in documents) >= 2}
    ordered = sorted(candidates, key=lambda text: (-len(text), text))
    uses = Counter()
    for doc in documents:
        occupied = []
        selected = []
        for candidate in ordered:
            offset = 0
            while True:
                start = doc["normal"].find(candidate, offset)
                if start < 0:
                    break
                end = start + len(candidate)
                offset = end
                if (start not in doc["offsets"]
                        or end != len(doc["normal"]) and end + 1 not in doc["offsets"]
                        or any(start < right and end > left for left, right in occupied)):
                    continue
                occupied.append((start, end))
                selected.append((start, end, candidate))
                uses[candidate] += 1
        doc["selected"] = selected
    terms = {}
    for candidate in ordered:
        if uses[candidate] < 2:
            continue
        label, text = candidates[candidate]
        identifier = label + "-" + hashlib.sha256(candidate.encode()).hexdigest()[:16]
        if identifier in terms:
            raise ValueError("Shared terms identifier collision")
        terms[identifier] = text
    records = []
    for doc in documents:
        output, references, position, output_size = [], [], 0, 0
        for start, end, candidate in sorted(doc["selected"]):
            if uses[candidate] < 2:
                continue
            label, terms_text = candidates[candidate]
            identifier = label + "-" + hashlib.sha256(candidate.encode()).hexdigest()[:16]
            first = doc["offsets"][start]
            last = doc["offsets"].get(end + 1, len(doc["matches"])) - 1
            left, right = doc["matches"][first].start(), doc["matches"][last].end()
            literal = doc["text"][position:left]
            output.append(literal)
            output_size += len(literal)
            marker_text = reference_marker(identifier)
            _, actual_gaps, _ = word_layout(doc["text"][left:right])
            _, shared_gaps, _ = word_layout(terms_text)
            overrides = [[i, gap] for i, gap in enumerate(actual_gaps)
                         if gap != shared_gaps[i]]
            reference = dict(start=output_size, shared=identifier)
            if overrides:
                reference["spacing"] = overrides
            references.append(reference)
            output.append(marker_text)
            output_size += len(marker_text)
            position = right
        output.append(doc["text"][position:])
        metadata = dict(paths=doc["paths"], encoding=doc["encoding"],
                        sha256=hashlib.sha256(doc["data"]).hexdigest())
        if references:
            metadata["references"] = references
        records.append(_serialize_record(metadata, "".join(output)))
    intro = b"""Rustium - licenses and attributions

This file contains the license terms and notices for inherited material.
Original source paths identify each document's provenance. Notices embedded
in source remain in place. Moving code does not change its applicable terms.

Repeated terms appear once under SHARED TERMS below. A component's document
includes its local notices plus each referenced shared text, in the order
shown. References combine text; they do not introduce a choice of licenses.
Only alternatives and exceptions stated in the original terms apply.

Different wording remains separate, even under the same license family.
Whitespace restoration data preserves original layouts for legacy tooling.
All original words, notices, exceptions, and disclaimers remain recoverable;
no external license repository or compressed legal-text archive is required.
Attribution lists are not license grants. Test-fixture documents remain test
data and do not establish Rustium's license.

Include this entire file with standalone source and binary distributions.
Other requirements of the applicable licenses continue to apply. This file
does not relicense any component. See README.md for tooling instructions.
"""
    result = intro + COMPACT_MARKER + b"".join(records)
    for identifier, text in terms.items():
        result += _serialize_record(dict(id=identifier,
            sha256=hashlib.sha256(text.encode("utf-8")).hexdigest()), text, shared=True)
    if parse_bundle(result) != entries:
        raise ValueError("Compaction changed original legal documents")
    return result


@lru_cache(maxsize=1)
def load_bundle():
    return parse_bundle((ROOT / "LICENSE").read_bytes())


def archived_bytes(path):
    """Look up a legacy path, including the original Chromium root license."""
    try:
        name = Path(path).resolve().relative_to(ROOT).as_posix()
    except ValueError:
        return None
    return load_bundle().get(name)


def materialize(root, entries, clean=False):
    """Restore original legal documents; never overwrite differing local files."""
    targets = []
    for name, data in entries.items():
        if name == "LICENSE":
            continue  # Never replace the authoritative bundle.
        target = checked_path(root, name)
        if target.exists() and (not target.is_file() or target.read_bytes() != data):
            raise ValueError(f"Refusing to change locally modified file: {name}")
        targets.append((target, data))
    for target, data in targets:
        if clean:
            if target.is_file():
                target.unlink()
        else:
            target.parent.mkdir(parents=True, exist_ok=True)
            target.write_bytes(data)
    return len(targets)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    actions = parser.add_mutually_exclusive_group(required=True)
    actions.add_argument("--verify", action="store_true")
    actions.add_argument("--compact", action="store_true")
    actions.add_argument("--materialize", action="store_true")
    actions.add_argument("--clean-materialized", action="store_true")
    args = parser.parse_args()
    entries = load_bundle()
    if not entries:
        parser.error("Root LICENSE is not a consolidated bundle")
    if args.verify:
        print(f"Verified {len(entries)} original legal documents")
    elif args.compact:
        output = compact_bundle(entries)
        (ROOT / "LICENSE").write_bytes(output)
        print(f"Compacted {len(entries)} legal documents into one root LICENSE")
    else:
        count = materialize(ROOT, entries, clean=args.clean_materialized)
        print(f"Processed {count} legacy legal-document paths")


if __name__ == "__main__":
    main()
