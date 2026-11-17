"""Ordinary source/patch and temporary-root checks; no native child or timing."""
from pathlib import Path
import hashlib
import copy
import importlib.util
import json
import os
import shutil
import subprocess
import tempfile
import unittest
import sys

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
        parent = self.root / 'evidence'
        parent.mkdir()
        another = self.root / 'evidence2'
        another.mkdir()
        first = runner.execution_temporary_root(self.snapshot, parent)
        second = runner.execution_temporary_root(self.snapshot, another)
        self.assertNotEqual(first, second)
        self.assertTrue(first.is_dir() and second.is_dir())
        self.assertTrue(first.is_relative_to(parent.resolve()))
        self.assertLessEqual(len(str(first)), 96)
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
        parent = self.root / ('x' * 97)
        with self.assertRaises(ValueError):
            runner.execution_temporary_root(self.snapshot, parent)
        self.assertFalse(parent.exists())


class TestMembership(unittest.TestCase):
    def setUp(self):
        parent = Path('D:/RustiumTmp')
        parent.mkdir(exist_ok=True)
        self.folder = Path(tempfile.mkdtemp(prefix='membership-', dir=parent))
        self.data = {'all_tests': ['Example.Pass', 'Example.Disabled'],
                     'disabled_tests': ['Example.Disabled'],
                     'per_iteration_data': [{'Example.Pass': [{'status': 'SUCCESS'}]}]}

    def check(self, data, target='standalone_test'):
        (self.folder / 'summary.json').write_text(json.dumps(data))
        return runner.gtest_membership(self.folder, target)

    def test_accepts_one_complete_iteration_with_explicit_disabled_membership(self):
        self.assertEqual(self.check(self.data)['expected_enabled'], 1)

    def test_rejects_filtered_missing_extra_and_repeated_attempts(self):
        for kind in ['missing', 'extra', 'repeated', 'empty_attempt']:
            data = copy.deepcopy(self.data)
            if kind == 'missing':
                data['per_iteration_data'][0] = {}
            elif kind == 'extra':
                data['per_iteration_data'][0]['Unknown.Test'] = [{'status': 'SUCCESS'}]
            elif kind == 'repeated':
                data['per_iteration_data'][0]['Example.Pass'] *= 2
            else:
                data['per_iteration_data'][0]['Example.Pass'] = []
            with self.subTest(kind=kind), self.assertRaises(ValueError):
                self.check(data)

    def test_cli_run_help_preserves_existing_execution_options(self):
        result = subprocess.run([sys.executable, str(DRAFT / RUNNER['proposal']), 'run', '--help'],
                                capture_output=True, text=True)
        self.assertEqual(result.returncode, 0, result.stderr)
        for option in ['--snapshot', '--qualification', '--evidence', '--cpu', '--timeout']:
            self.assertIn(option, result.stdout)
        self.assertNotIn('--temporary-root', result.stdout)

    def test_rejects_missing_or_disabled_recovery_regressions(self):
        with self.assertRaises(ValueError):
            self.check(self.data, 'net_unittests')
        data = {'all_tests': sorted(runner.REQUIRED_RECOVERY_TESTS), 'disabled_tests': [],
                'per_iteration_data': [{name: [{'status': 'SUCCESS'}] for name in runner.REQUIRED_RECOVERY_TESTS}]}
        self.assertEqual(len(self.check(data, 'net_unittests')['required_recovery_cases']), 14)
        name = next(iter(runner.REQUIRED_RECOVERY_TESTS))
        data['disabled_tests'] = [name]
        del data['per_iteration_data'][0][name]
        with self.assertRaises(ValueError):
            self.check(data, 'net_unittests')

    def test_rejects_duplicate_enumeration_and_multiple_iterations(self):
        duplicate = copy.deepcopy(self.data)
        duplicate['all_tests'].append('Example.Pass')
        repeated = copy.deepcopy(self.data)
        repeated['per_iteration_data'] *= 2
        for data in [duplicate, repeated]:
            with self.assertRaises(ValueError):
                self.check(data)


def verify_patches():
    parent = Path('D:/Rustium/.rustium-runtime')
    copied = Path(tempfile.mkdtemp(prefix='harness-proposals-', dir=parent))
    for row in MANIFEST['sources']:
        frozen = DRAFT / row['source_snapshot']
        assert hashlib.sha256(frozen.read_bytes()).hexdigest() == row['before_sha256']
        observed = (Path('D:/Rustium') / row['path']).read_bytes()
        allowed = [frozen.read_bytes()]
        if 'proposal' in row:
            allowed.append((DRAFT / row['proposal']).read_bytes())
        assert observed in allowed, row['path']
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
    print('Verified', len(MANIFEST['sources']), 'source bindings and 2 exact patches; retained:', copied)


if __name__ == '__main__':
    verify_patches()
    unittest.main()
