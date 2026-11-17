"""Ordinary source/patch and temporary-root checks; no native child or timing."""
from pathlib import Path
import hashlib
import importlib.util
import json
import os
import shutil
import subprocess
import tempfile
import unittest

DRAFT = Path(__file__).resolve().parent
MANIFEST = json.loads((DRAFT / 'source-manifest.json').read_text())
RUNNER = next(r for r in MANIFEST['sources'] if r['path'].endswith('native_delivery.py'))
spec = importlib.util.spec_from_file_location('proposed_native_delivery', DRAFT / RUNNER['proposal'])
runner = importlib.util.module_from_spec(spec)
spec.loader.exec_module(runner)


class TemporaryRoots(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        parent = Path('D:/RustiumTmp')
        parent.mkdir(exist_ok=True)
        cls.root = Path(tempfile.mkdtemp(prefix='qa-', dir=parent))
        cls.snapshot = cls.root / 'sealed'
        cls.snapshot.mkdir()

    def test_unique_external_roots_leave_snapshot_and_environment_unchanged(self):
        before = dict(os.environ)
        parent = self.root / 'tmp'
        first = runner.execution_temporary_root(self.snapshot, parent)
        second = runner.execution_temporary_root(self.snapshot, parent)
        self.assertNotEqual(first, second)
        self.assertTrue(first.is_dir() and second.is_dir())
        self.assertTrue(first.is_relative_to(parent.resolve()))
        self.assertLessEqual(len(str(first)), 64)
        self.assertEqual(list(self.snapshot.iterdir()), [])
        self.assertEqual(dict(os.environ), before)

    def test_rejects_snapshot_and_ancestor_overlap(self):
        for parent in [self.snapshot, self.snapshot / 'tmp', self.root]:
            with self.subTest(parent=parent), self.assertRaises(ValueError):
                runner.execution_temporary_root(self.snapshot, parent)

    def test_rejects_resolved_overlap(self):
        with self.assertRaises(ValueError):
            runner.execution_temporary_root(self.snapshot, self.root / 'tmp' / '..' / 'sealed')

    def test_rejects_long_root_before_creating_it(self):
        parent = self.root / ('x' * 49)
        with self.assertRaises(ValueError):
            runner.execution_temporary_root(self.snapshot, parent)
        self.assertFalse(parent.exists())


def verify_patches():
    parent = Path('D:/Rustium/.rustium-runtime')
    copied = Path(tempfile.mkdtemp(prefix='harness-proposals-', dir=parent))
    for row in MANIFEST['sources']:
        frozen = DRAFT / row['source_snapshot']
        assert hashlib.sha256(frozen.read_bytes()).hexdigest() == row['before_sha256']
        assert frozen.read_bytes() == (Path('D:/Rustium') / row['path']).read_bytes()
        if 'proposal' not in row:
            continue
        target = copied / row['path']
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(frozen, target)
        patch = DRAFT / (Path(row['path']).stem + '.patch')
        for arguments in [['apply', '--check', str(patch)], ['apply', str(patch)]]:
            subprocess.run(['git', '-C', str(copied), *arguments], check=True, capture_output=True)
        assert target.read_bytes() == (DRAFT / row['proposal']).read_bytes()
        assert hashlib.sha256(target.read_bytes()).hexdigest() == row['proposed_sha256']
    print('Verified 5 source bindings and 2 exact patches; retained:', copied)


if __name__ == '__main__':
    verify_patches()
    unittest.main()
