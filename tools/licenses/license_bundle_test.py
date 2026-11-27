#!/usr/bin/env python3
"""Integrity and compatibility tests for the consolidated legal documents."""

import hashlib
import json
from pathlib import Path
import tempfile
import unittest

import license_bundle as bundle
import licenses


class BundleTest(unittest.TestCase):
    def test_compaction_shares_terms_without_merging_notices_or_variants(self):
        terms = ("Permission is hereby granted, free of charge "
                 + " ".join(f"word{i}" for i in range(100))
                 + " other dealings in the software.")
        entries = {
            "first/LICENSE": ("Copyright First Holder\n" + terms).encode(),
            "second/LICENSE": ("Copyright Second Holder\n"
                               + terms.replace(" ", "\r\n")).encode(),
            "third/LICENSE": ("Copyright Third Holder\n"
                              + terms.replace("free of charge", "for payment")).encode(),
        }
        compact = bundle.compact_bundle(entries)
        self.assertEqual(bundle.parse_bundle(compact), entries)
        self.assertEqual(compact.count(bundle.SHARED_BEGIN), 1)
        self.assertIn(b"Copyright First Holder", compact)
        self.assertIn(b"Copyright Second Holder", compact)
        self.assertIn(b"for payment", compact)
        self.assertIn(b"spacing", compact)
        self.assertEqual(bundle.compact_bundle(bundle.parse_bundle(compact)), compact)
        with self.assertRaisesRegex(ValueError, "checksum mismatch"):
            bundle.parse_bundle(compact.replace(b"word50", b"wordXX"))
        with self.assertRaisesRegex(ValueError, "reference position"):
            bundle.parse_bundle(compact.replace(b"[See shared terms:",
                                                b"[BAD shared terms:", 1))
        with self.assertRaisesRegex(ValueError, "whitespace"):
            bundle.restore_spacing(terms, [[0, "changed terms"]])

    def test_parser_preserves_encodings_and_detects_modified_text(self):
        for original, encoding in ((b"BSD text\r\n", "utf-8"),
                                   (b"Copyright \xa9 Holder", "latin-1")):
            body = original.decode(encoding).encode("utf-8")
            record = {"paths": ["component/LICENSE", "other/LICENSE"],
                      "sha256": hashlib.sha256(original).hexdigest(),
                      "encoding": encoding, "text_bytes": len(body)}
            data = (bundle.MARKER + bundle.BEGIN + json.dumps(record).encode()
                    + b"\n" + body + bundle.END)
            self.assertEqual(bundle.parse_bundle(data),
                             {name: original for name in record["paths"]})
            with self.assertRaisesRegex(ValueError, "checksum mismatch"):
                bundle.parse_bundle(data.replace(body, b"X" + body[1:]))

    def test_materialization_round_trip_and_modified_file_protection(self):
        entries = bundle.load_bundle()
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            (root / "LICENSE").write_bytes(b"authoritative bundle")
            bundle.materialize(root, entries)
            for name, expected in entries.items():
                if name != "LICENSE":
                    self.assertEqual(root.joinpath(name).read_bytes(), expected)
            first = next(name for name in entries if name != "LICENSE")
            root.joinpath(first).write_bytes(b"local modification")
            for clean in (False, True):
                with self.assertRaisesRegex(ValueError, "locally modified"):
                    bundle.materialize(root, entries, clean=clean)
            root.joinpath(first).write_bytes(entries[first])
            bundle.materialize(root, entries, clean=True)
            self.assertEqual((root / "LICENSE").read_bytes(), b"authoritative bundle")
            self.assertEqual([p.relative_to(root) for p in root.rglob("*")
                              if p.is_file()], [Path("LICENSE")])

    def test_tampering_and_unsafe_paths_are_rejected(self):
        data = (bundle.ROOT / "LICENSE").read_bytes()
        with self.assertRaises(ValueError):
            bundle.parse_bundle(data[:-1])
        for name in ("../escape", "/escape", "C:/escape", "foo/../../escape",
                     "", ".git/config"):
            with self.assertRaises(ValueError):
                bundle.checked_path(bundle.ROOT, name)
        with tempfile.TemporaryDirectory() as directory:
            outside = Path(directory) / "outside"
            outside.mkdir()
            root = Path(directory) / "root"
            root.mkdir()
            # Directory junctions/symlinks must not permit path escape.
            try:
                (root / "link").symlink_to(outside, target_is_directory=True)
            except OSError:
                return  # Windows may require developer mode for symlinks.
            with self.assertRaises(ValueError):
                bundle.checked_path(root, "link/LICENSE")

    def test_existing_license_generator_reads_component_specific_text(self):
        entries = bundle.load_bundle()
        for name in ("LICENSE", "third_party/abseil-cpp/LICENSE",
                     "tools/licenses/tests/mock_root/third_party/sample/LICENSE"):
            absolute = bundle.ROOT / name
            self.assertEqual(licenses._read_file(str(absolute)),
                             entries[name].decode("utf-8", errors="replace"))
            self.assertEqual(Path(licenses._ResolvePath("", name, str(bundle.ROOT))).as_posix(), name)
        self.assertIn(str(bundle.ROOT / "LICENSE"), licenses._read_paths)
        self.assertIsNone(licenses._ResolvePath("", "../LICENSE", str(bundle.ROOT)))


if __name__ == "__main__":
    unittest.main()
