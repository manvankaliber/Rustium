"""Data-only checks: no GN, compiler, source checkout or native programs."""
from pathlib import Path, PurePosixPath
import subprocess
import tempfile
import unittest

from build_network_consumers import checkout_inputs, choose_output, compare_inputs, resolve_targets


class BuildPlanningTest(unittest.TestCase):
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
