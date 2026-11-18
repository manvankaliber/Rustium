"""Data-only checks: no GN, compiler, source checkout or native programs."""
import argparse
import json
import os
from pathlib import Path, PurePosixPath
import subprocess
import tempfile
import unittest
from unittest.mock import patch

from build_network_consumers import (
    check_feedback_gtest, checkout_inputs, choose_output, compare_inputs, feedback_cases,
    native_processes, normal_configuration, resolve_targets, warm_feedback,
)


class BuildPlanningTest(unittest.TestCase):
    def test_native_process_check_does_not_need_powershell_on_path(self):
        windows = Path('C:/Windows')
        process = {'ProcessId': 42, 'Name': 'ninja.exe'}
        with patch.dict(os.environ, {'SystemRoot': str(windows), 'PATH': ''}), patch(
                'build_network_consumers.subprocess.check_output',
                return_value=b'[{"ProcessId":42,"Name":"ninja.exe"}]') as run:
            self.assertEqual(native_processes(), [process])
        self.assertEqual(run.call_args.args[0][0], str(
            windows / 'System32/WindowsPowerShell/v1.0/powershell.exe'))

    def test_nondefault_roots_use_current_outputs(self):
        label = '//base:base(//build/toolchain/win:win_clang_x64_with_system_allocator)'
        roots = {'build_roots': [label]}
        self.assertEqual(resolve_targets(roots, {label: {'outputs': [
            '//out/NetworkCI/allocator/base.dll']}}, PurePosixPath('out/NetworkCI')),
            {label: ['allocator/base.dll']})

    def test_missing_root_cannot_shrink_the_build(self):
        with self.assertRaisesRegex(ValueError, 'Missing evaluated'):
            resolve_targets({'build_roots': ['//net:net']}, {},
                            PurePosixPath('out/NetworkCI'))

    def test_nondefault_root_cannot_fall_back_to_default_alias(self):
        label = '//base:base(//build/toolchain/win:other)'
        with self.assertRaisesRegex(ValueError, 'Nondefault root has no output'):
            resolve_targets({'build_roots': [label]}, {label: {'outputs': []}},
                            PurePosixPath('out/NetworkCI'))

    def test_outputs_cannot_escape_or_reuse_other_configuration(self):
        for path in ['//out/Default/net.dll', '//out/NetworkCI/../net.dll',
                     '//out/NetworkCI/C:/net.dll']:
            with self.subTest(path=path), self.assertRaises(ValueError):
                resolve_targets({'build_roots': ['//net:net']},
                                {'//net:net': {'outputs': [path]}},
                                PurePosixPath('out/NetworkCI'))

    def test_default_group_keeps_its_exact_alias(self):
        self.assertEqual(resolve_targets({'build_roots': ['//net:test_support']},
                         {'//net:test_support': {'outputs': []}},
                         PurePosixPath('out/NetworkCI')),
                         {'//net:test_support': ['net:test_support']})


class FamilyFeedbackTest(unittest.TestCase):
    target = 'connection_jobs_family_rust_unittests'
    first = 'connect_job_rust_perftests'
    profile = json.loads(Path(__file__).with_name('network-validation-inventory.json').read_bytes())
    focus = {'family': 'connection-jobs', 'separate_configuration':
             {'connection_jobs_upper_rust_jobs_perftests': 'QUIC'}}

    def test_feedback_uses_only_requested_active_family_and_configuration(self):
        names = self.first + ',' + self.target
        self.assertEqual([c['target'] for c in feedback_cases(self.profile, self.focus, names, '')],
                         names.split(','))
        for requested, selector in [('dns_resolution_family_rust_unittests', ''),
                                    ('connection_jobs_upper_rust_jobs_perftests', ''),
                                    (self.first, 'Fixture.One'), (names, 'Fixture.One'),
                                    (self.target, '*'), (self.target + ',' + self.target, '')]:
            with self.subTest(requested=requested, selector=selector), self.assertRaises(ValueError):
                feedback_cases(self.profile, self.focus, requested, selector)

    def test_filtered_gtest_requires_each_requested_case_to_pass_once(self):
        summary = {'per_iteration_data': [{'Fixture.One': [{'status': 'SUCCESS'}]}]}
        self.assertEqual(check_feedback_gtest(summary, 'Fixture.One'), {'passed': 1, 'skipped': 0})
        summary.update(all_tests=['Fixture.One'], disabled_tests=[])
        with self.assertRaisesRegex(ValueError, 'actually passed'):
            check_feedback_gtest(summary, '', b'[  SKIPPED ] Fixture.One (0 ms)')
        with self.assertRaises(ValueError):
            check_feedback_gtest(summary, 'Fixture.One:Fixture.Missing')
        for statuses in ([], ['SKIPPED'], ['NOTRUN'], ['FAILURE', 'SUCCESS']):
            summary['per_iteration_data'][0]['Fixture.One'] = [{'status': s} for s in statuses]
            with self.assertRaises(ValueError):
                check_feedback_gtest(summary, 'Fixture.One')

    def test_warm_run_builds_and_executes_in_order_with_separate_logs(self):
        with tempfile.TemporaryDirectory() as tempdir, patch.dict(os.environ):
            parent = Path(tempdir)
            root = parent / 'src'
            cfg = root / 'tools/ci/network-focus.json'
            cfg.parent.mkdir(parents=True)
            cfg.write_text(json.dumps(self.focus))
            vpython = parent / 'depot/.cipd_bin/vpython3.exe'
            vpython.parent.mkdir(parents=True)
            vpython.touch()
            evidence = parent / 'evidence'
            evidence.mkdir()
            output = root / 'out/NetworkCI/pr1-ad617-20261118a'
            output.mkdir(parents=True)
            (output / 'args.gn').write_text(normal_configuration())
            (output / 'build.ninja').write_text('pool build_toolchain_link_pool\n  depth = 1\n')
            calls = []
            def command(phase, argv):
                calls.append((phase, argv))
                log = evidence / (phase + '.log')
                log.touch()
                if phase.startswith('build-'):
                    (output / (phase[6:] + '.exe')).touch()
                if phase == 'test-' + self.first:
                    case = next(c for c in self.profile['cases'] if c['target'] == self.first)
                    log.write_text(case['completion']['message'])
                if phase == 'test-' + self.target:
                    summary = {'all_tests': ['Fixture.One', 'Fixture.Disabled'], 'disabled_tests': ['Fixture.Disabled'],
                               'per_iteration_data': [{'Fixture.One': [{'status': 'SUCCESS'}]}]}
                    (evidence / (self.target + '-summary.json')).write_text(json.dumps(summary))
                return log
            args = argparse.Namespace(feedback_targets=self.first + ',' + self.target,
                                      gtest_filter='', jobs=None, revision='a' * 40)
            for headroom, expected_jobs in [(3, 1), (6, 4)]:
                result = {'memory_before': {'available_bytes': 6 * 1024**3,
                                            'commit_headroom_bytes': headroom * 1024**3}}
                calls.clear()
                warm_feedback(root, evidence, args, (None, None, {'depot_tools': parent / 'depot'}),
                              self.profile, result, command)
                self.assertEqual([p for p, _ in calls], ['gn-gen', 'build-' + self.first,
                                 'test-' + self.first, 'build-' + self.target, 'test-' + self.target])
                self.assertIn(f'-j{expected_jobs}', calls[1][1])
                self.assertEqual(calls[2][1][1:], ['--verify-only'])
                self.assertIn('--test-launcher-print-test-stdio=always', calls[4][1])
                self.assertTrue(result['feedback_passed'])
                self.assertEqual(result['targets'][self.target]['cases'], {'passed': 1, 'skipped': 0})


class CheckoutIdentityTest(unittest.TestCase):
    def git(self, root, *args):
        subprocess.run(['git', '-c', 'user.name=Fixture', '-c',
                        'user.email=fixture@example.invalid', *args],
                       cwd=root, check=True, capture_output=True)

    def setUp(self):
        temporary = tempfile.TemporaryDirectory()
        self.addCleanup(temporary.cleanup)
        parent = Path(temporary.name).resolve()
        (parent / '.gclient').write_text('fixture configuration', encoding='utf-8')
        self.root = parent / 'src'
        self.root.mkdir()
        self.git(self.root, 'init', '-q')
        (self.root / '.gitignore').write_text('/deps/\n/tool-cache/\n/out/\n', encoding='utf-8')
        self.git(self.root, 'add', '.gitignore')
        self.git(self.root, 'commit', '-qm', 'Fixture root')
        self.dep = self.root / 'deps/nested'
        self.dep.mkdir(parents=True)
        self.git(self.dep, 'init', '-q')
        (self.dep / 'input.cc').write_text('int x;\n', encoding='utf-8')
        self.git(self.dep, 'add', 'input.cc')
        self.git(self.dep, 'commit', '-qm', 'Fixture dependency')

    def test_same_revision_dirty_nested_checkout_is_rejected(self):
        before = checkout_inputs(self.root)
        self.assertIn('deps/nested', before['git_revisions'])
        (self.dep / 'input.cc').write_text('int different;\n', encoding='utf-8')
        with self.assertRaisesRegex(ValueError, 'Uncommitted inputs in deps/nested'):
            checkout_inputs(self.root)

    def test_new_untracked_source_is_rejected(self):
        (self.root / 'unexpected.gni').write_text('unexpected', encoding='utf-8')
        with self.assertRaisesRegex(ValueError, 'unexpected.gni'):
            checkout_inputs(self.root)

    def test_ignored_tool_input_is_hashed_but_native_output_is_excluded(self):
        tool = self.root / 'tool-cache/tool.exe'
        tool.parent.mkdir()
        tool.write_bytes(b'one')
        output = self.root / 'out/NetworkCI/object.obj'
        output.parent.mkdir(parents=True)
        output.write_bytes(b'old build')
        before = checkout_inputs(self.root)
        output.write_bytes(b'new build')
        compare_inputs(before, checkout_inputs(self.root))
        tool.write_bytes(b'two')
        with self.assertRaisesRegex(ValueError, 'tool-cache/tool.exe'):
            compare_inputs(before, checkout_inputs(self.root))

    def test_checkout_symlink_binds_target_and_target_contents(self):
        cache = self.root / 'tool-cache'
        cache.mkdir()
        for name in ('first', 'second'):
            (cache / name).write_bytes(b'input')
        commands = cache / '.bin'
        commands.mkdir()
        alias = commands / 'invoke'
        alias.symlink_to('../first')
        before = checkout_inputs(self.root)
        key = 'tool-cache/.bin/invoke'
        self.assertEqual(before['file_links'][key]['target'], os.readlink(alias))
        self.assertEqual(before['file_links'][key]['target_path'], 'tool-cache/first')
        self.assertIn('tool-cache/first', before['files'])
        self.assertNotIn(key, before['files'])
        alias.unlink()
        alias.symlink_to('../second')
        with self.assertRaisesRegex(ValueError, 'symlink identity changed'):
            compare_inputs(before, checkout_inputs(self.root))

    def test_checkout_symlink_cannot_include_external_or_excluded_file(self):
        cache = self.root / 'tool-cache'
        cache.mkdir()
        external = self.root.parent / 'external'
        external.write_bytes(b'outside the source')
        output = self.root / 'out/NetworkCI/object.obj'
        output.parent.mkdir(parents=True)
        output.write_bytes(b'outside the input record')
        alias = cache / 'alias'
        for target, reason in [('../../external', 'escapes source'),
                               ('../out/NetworkCI/object.obj', 'inventoried file')]:
            with self.subTest(target=target):
                alias.symlink_to(target)
                with self.assertRaisesRegex(ValueError, reason):
                    checkout_inputs(self.root)
                alias.unlink()

    def test_existing_outputs_cannot_be_reused_or_deleted(self):
        first = choose_output(self.root, 'gh-123-1', False)
        old = self.root / first / 'object.obj'
        old.write_bytes(b'previous run')
        with self.assertRaises(FileExistsError):
            choose_output(self.root, 'gh-123-1', False)
        second = choose_output(self.root, 'gh-123-2', False)
        self.assertEqual(old.read_bytes(), b'previous run')
        self.assertEqual(list((self.root / second).iterdir()), [])
        with self.assertRaisesRegex(ValueError, 'short unique run ID'):
            choose_output(self.root, '../../existing', True)


if __name__ == '__main__':
    unittest.main()
