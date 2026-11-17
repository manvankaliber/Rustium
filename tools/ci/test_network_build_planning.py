"""Data-only checks: no GN, compiler, source checkout or native programs."""
import unittest
from pathlib import PurePosixPath

from build_network_consumers import resolve_targets


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


if __name__ == '__main__':
    unittest.main()
