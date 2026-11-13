"""Check unpublished benchmark archiving and immutable evidence boundaries."""

import contextlib
import hashlib
import io
import json
from pathlib import Path
import sys
import tempfile
import unittest
from unittest import mock

import update_rust_benchmark_charts as importer


class ArchiveOnlyTest(unittest.TestCase):
    def setUp(self):
        self.directory = tempfile.TemporaryDirectory()
        self.addCleanup(self.directory.cleanup)
        self.root = Path(self.directory.name)
        self.report = self.root / "raw.jsonl"
        self.metadata = self.root / "metadata.json"
        self.hashes = self.root / "sources.json"
        self.metadata.write_text(json.dumps({
            "module": "sample", "source": "sample", "label": "Sample",
            "measurement": "Isolated operation", "baseline": "Frozen C++",
            "groups": [{"id": "sample-cost", "title": "Cost", "operation": "Parse",
                        "cases": [{"key": "small", "label": "Small"},
                                  {"key": "large", "label": "Large"}]}],
        }), encoding="utf-8")
        self.hashes.write_text("{}", encoding="utf-8")
        # Preserve both an improvement and a regression.
        self.samples = [dict(case=case, repeat=i, baseline_ns=100, rust_ns=rust)
                        for case, rust in [("small", 50), ("large", 150)]
                        for i in range(20)]
        self.write_samples()

    def write_samples(self):
        self.report.write_text("".join(json.dumps(x) + "\n" for x in self.samples),
                               encoding="utf-8")

    def run_import(self, *extra):
        args = ["importer", str(self.report), "--metadata", str(self.metadata),
                "--source-hashes", str(self.hashes), "--run", "pending",
                "--label", "Pending", "--date", "2026-11-13", *extra]
        with mock.patch.object(importer, "ROOT", self.root), \
                mock.patch.object(sys, "argv", args), \
                contextlib.redirect_stdout(io.StringIO()):
            importer.main()

    @property
    def archive(self):
        return self.root / "docs/rust/benchmarks/sample-pending-2026-11-13.json"

    def test_archive_does_not_require_or_create_a_gallery(self):
        self.run_import("--archive-only")
        data = json.loads(self.archive.read_text(encoding="utf-8"))
        self.assertEqual(data["samples"], self.samples)
        self.assertEqual([x["change_percent"] for x in data["cases"]], [-50, 50])
        self.assertFalse((self.root / "rust-performance.html").exists())

    def test_later_publication_uses_identical_archived_evidence(self):
        page = self.root / "rust-performance.html"
        initial = '<script id="benchmark-data" type="application/json">{}</script>'
        page.write_text(initial, encoding="utf-8")
        self.run_import("--archive-only")
        archived = self.archive.read_bytes()
        self.assertEqual(page.read_text(encoding="utf-8"), initial)
        self.run_import()
        self.assertEqual(self.archive.read_bytes(), archived)
        self.assertNotEqual(page.read_text(encoding="utf-8"), initial)

    def test_changed_evidence_cannot_overwrite_an_archive_or_page(self):
        page = self.root / "rust-performance.html"
        page.write_text("untouched", encoding="utf-8")
        self.run_import("--archive-only")
        archived = self.archive.read_bytes()
        self.samples[0]["rust_ns"] = 51
        self.write_samples()
        with self.assertRaisesRegex(ValueError, "already exists"):
            self.run_import("--archive-only")
        self.assertEqual(self.archive.read_bytes(), archived)
        self.assertEqual(page.read_text(encoding="utf-8"), "untouched")

    def test_archive_only_does_not_bypass_final_provenance(self):
        with self.assertRaisesRegex(ValueError, "configured GN"):
            self.run_import("--archive-only", "--final")
        self.assertFalse(self.archive.exists())

    def test_archive_only_rejects_omitted_cases(self):
        self.samples = [x for x in self.samples if x["case"] == "small"]
        self.write_samples()
        with self.assertRaisesRegex(ValueError, "every measured case"):
            self.run_import("--archive-only")
        self.assertFalse(self.archive.exists())

    def test_native_pair_and_cpp_fields_preserve_raw_evidence_and_statistics(self):
        expected = importer.summarize(self.samples, [
            {"key": "small", "label": "Small"},
            {"key": "large", "label": "Large"}], "pending", seed=20261112)
        self.samples = [dict(case=x["case"], pair=x["repeat"],
                             cpp_ns=x["baseline_ns"], rust_ns=x["rust_ns"])
                        for x in self.samples]
        self.write_samples()
        raw = self.report.read_bytes()
        self.run_import("--archive-only")
        data = json.loads(self.archive.read_text(encoding="utf-8"))
        self.assertEqual(data["samples"], self.samples)
        self.assertEqual(data["cases"], expected)
        self.assertEqual(data["raw_report_sha256"], hashlib.sha256(raw).hexdigest())
        self.assertEqual(self.report.read_bytes(), raw)

    def test_conflicting_native_aliases_cannot_choose_a_favorable_value(self):
        for alias, value in [("cpp_ns", 200), ("pair", 19)]:
            with self.subTest(alias=alias):
                self.samples[0][alias] = value
                self.write_samples()
                with self.assertRaisesRegex(ValueError, "Conflicting"):
                    self.run_import("--archive-only")
                self.samples[0].pop(alias)
                self.assertFalse(self.archive.exists())

    def test_native_pair_alias_still_rejects_repeated_ids(self):
        for sample in self.samples:
            sample["pair"] = sample.pop("repeat")
        self.samples[1]["pair"] = self.samples[0]["pair"]
        self.write_samples()
        with self.assertRaisesRegex(ValueError, "unique paired repetitions"):
            self.run_import("--archive-only")
        self.assertFalse(self.archive.exists())

    def test_boolean_is_not_a_nanosecond_measurement(self):
        self.samples[0]["baseline_ns"] = True
        self.write_samples()
        with self.assertRaisesRegex(ValueError, "positive finite nanoseconds"):
            self.run_import("--archive-only")
        self.assertFalse(self.archive.exists())


if __name__ == "__main__":
    unittest.main()
