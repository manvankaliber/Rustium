#!/usr/bin/env python
# Copyright 2020 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
"""Tests for protoc_wrapper."""

from __future__ import print_function

import logging
import mmap
import os
from pathlib import Path
import sys
import tempfile
import unittest

if sys.version_info.major == 2:
  from StringIO import StringIO
  import mock
else:
  from io import StringIO
  from unittest import mock

import protoc_wrapper


class ProtocWrapperTest(unittest.TestCase):
  @mock.patch('subprocess.call', return_value=0)
  def test_minimal_arguments(self, mock_call):
    protoc_wrapper.main(
      ['--proto-in-dir', './', '--protoc', '/foo/protoc', 'foo.proto']
    )
    mock_call.assert_called_once_with(
      [
        os.path.realpath('/foo/protoc'), '--proto_path', '.',
        os.path.join('.', 'foo.proto')
      ]
    )

  @mock.patch('subprocess.call', return_value=0)
  def test_kythe_no_out(self, mock_call):
    protoc_wrapper.main(
      [
        '--proto-in-dir',
        './',
        '--enable-kythe-annotation',
        '--protoc',
        '/foo/protoc',
        'foo.proto',
      ]
    )
    mock_call.assert_called_once_with(
      [
        os.path.realpath('/foo/protoc'), '--proto_path', '.',
        os.path.join('.', 'foo.proto')
      ]
    )

  @mock.patch('subprocess.call', return_value=0)
  def test_kythe_cpp_out_no_options(self, mock_call):
    protoc_wrapper.main(
      [
        '--proto-in-dir',
        './',
        '--enable-kythe-annotation',
        '--cc-out-dir',
        './bar',
        '--protoc',
        '/foo/protoc',
        'foo.proto',
      ]
    )
    mock_call.assert_called_once_with(
      [
        os.path.realpath('/foo/protoc'),
        '--cpp_out',
        'annotate_headers,annotation_pragma_name=kythe_metadata,annotation_guard_name=KYTHE_IS_RUNNING:./bar',
        '--proto_path',
        '.',
        os.path.join('.', 'foo.proto'),
      ]
    )

  @mock.patch('subprocess.call', return_value=0)
  def test_kythe_cpp_out_with_options(self, mock_call):
    protoc_wrapper.main(
      [
        '--proto-in-dir',
        './',
        '--enable-kythe-annotation',
        '--cc-options',
        'foo=bar:',
        '--cc-out-dir',
        './bar',
        '--protoc',
        '/foo/protoc',
        'foo.proto',
      ]
    )
    mock_call.assert_called_once_with(
      [
        os.path.realpath('/foo/protoc'),
        '--cpp_out',
        'annotate_headers,annotation_pragma_name=kythe_metadata,annotation_guard_name=KYTHE_IS_RUNNING,foo=bar:./bar',
        '--proto_path',
        '.',
        os.path.join('.', 'foo.proto'),
      ]
    )

  @mock.patch('subprocess.call', return_value=0)
  def test_kythe_cpp_out_with_options_no_colon(self, mock_call):
    protoc_wrapper.main(
      [
        '--proto-in-dir',
        './',
        '--enable-kythe-annotation',
        '--cc-options',
        'foo=bar',
        '--cc-out-dir',
        './bar',
        '--protoc',
        '/foo/protoc',
        'foo.proto',
      ]
    )
    mock_call.assert_called_once_with(
      [
        os.path.realpath('/foo/protoc'),
        '--cpp_out',
        'annotate_headers,annotation_pragma_name=kythe_metadata,annotation_guard_name=KYTHE_IS_RUNNING,foo=bar:./bar',
        '--proto_path',
        '.',
        os.path.join('.', 'foo.proto'),
      ]
    )

  @mock.patch('subprocess.call', return_value=0)
  def test_cpp_out_with_options_no_colon(self, mock_call):
    protoc_wrapper.main(
      [
        '--proto-in-dir',
        './',
        '--cc-options',
        'foo=bar:',
        '--cc-out-dir',
        './bar',
        '--protoc',
        '/foo/protoc',
        'foo.proto',
      ]
    )
    mock_call.assert_called_once_with(
      [
        os.path.realpath('/foo/protoc'),
        '--cpp_out',
        'foo=bar:./bar',
        '--proto_path',
        '.',
        os.path.join('.', 'foo.proto'),
      ]
    )

  @mock.patch('protoc_wrapper.RewriteImports')
  @mock.patch('subprocess.call', return_value=0)
  def test_ts_out_with_options_no_colon(self, mock_call, mock_rewrite):
    protoc_wrapper.main(
      [
        '--proto-in-dir',
        './',
        '--ts-out-dir',
        './bar',
        '--protoc-gen-ts',
        '/foo/protoc-gen-ts_proto',
        '--protoc',
        '/foo/protoc',
        'foo.proto',
      ]
    )
    mock_call.assert_called_once_with(
      [
        os.path.realpath('/foo/protoc'),
        '--ts_proto_out=./bar',
        '--ts_proto_opt=env=browser,esModuleInterop=true,importSuffix=.js',
        '--ts_proto_opt=useOptionals=all',
        '--plugin=protoc-gen-ts_proto=' +
        os.path.realpath('/foo/protoc-gen-ts_proto'),
        '--proto_path',
        '.',
        os.path.join('.', 'foo.proto'),
      ]
    )
    mock_rewrite.assert_called_once_with([os.path.join('./bar', 'foo.ts')])


class PluginOutputTest(unittest.TestCase):
  def setUp(self):
    self.temp_dir = tempfile.TemporaryDirectory()
    self.addCleanup(self.temp_dir.cleanup)
    self.root = Path(self.temp_dir.name)
    previous_directory = Path.cwd()
    self.addCleanup(os.chdir, previous_directory)
    os.chdir(self.root)
    self.output = self.root / 'plugin'
    self.staging = None
    self.platform = mock.patch.object(protoc_wrapper.sys, 'platform', 'win32')
    self.platform.start()
    self.addCleanup(self.platform.stop)

  def run_generator(self, generate, extra_args=(), plugin_options=None):
    def call(command):
      argument = command[command.index('--plugin_out') + 1]
      prefix = protoc_wrapper.FormatGeneratorOptions(plugin_options)
      self.assertTrue(argument.startswith(prefix))
      self.staging = Path(argument[len(prefix):])
      self.assertNotEqual(self.staging.resolve(), self.output.resolve())
      self.assertEqual(self.staging.parent.resolve(), self.output.resolve())
      self.assertTrue(self.staging.is_dir())
      return generate(command, self.staging)

    args = [
      '--protoc', str(self.root / 'protoc'), '--proto-in-dir', '.',
      '--plugin', str(self.root / 'plugin.exe'), '--plugin-out-dir',
      str(self.output),
    ]
    if plugin_options is not None:
      args += ['--plugin-options', plugin_options]
    with mock.patch.object(protoc_wrapper.subprocess, 'call', side_effect=call):
      protoc_wrapper.main(args + list(extra_args) + ['foo.proto'])

  def write(self, root, name, contents):
    path = root / name
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_bytes(contents)
    return path

  def assert_cleaned(self):
    self.assertIsNotNone(self.staging)
    self.assertFalse(self.staging.exists())
    self.assertEqual(list(self.output.glob('.protoc-*')), [])

  def test_nested_new_outputs_and_options(self):
    for options in (None, 'wrapper_namespace=gen', 'wrapper_namespace=gen:'):
      with self.subTest(options=options):
        def generate(command, staging):
          self.write(staging, 'nested/new.gen.h', b'header\x00\xff\r\n')
          self.write(staging, 'new.gen.cc', b'implementation\n')
          self.assertFalse((self.output / 'nested/new.gen.h').exists())
          return 0

        self.run_generator(generate, plugin_options=options)
        self.assertEqual((self.output / 'nested/new.gen.h').read_bytes(),
                         b'header\x00\xff\r\n')
        self.assertEqual((self.output / 'new.gen.cc').read_bytes(),
                         b'implementation\n')
        self.assert_cleaned()
        (self.output / 'nested/new.gen.h').unlink()

  def test_unchanged_mapped_destination_keeps_mtime(self):
    final = self.write(self.output, 'nested/foo.gen.h', b'unchanged\r\n')
    timestamp = 1600000000000000000
    os.utime(final, ns=(timestamp, timestamp))
    before = final.stat()

    def generate(command, staging):
      self.write(staging, 'nested/foo.gen.h', final.read_bytes())
      return 0

    with final.open('rb') as reader:
      with mmap.mmap(reader.fileno(), 0, access=mmap.ACCESS_READ) as mapped:
        with mock.patch.object(protoc_wrapper.os, 'replace') as replace:
          self.run_generator(generate)
        replace.assert_not_called()
        self.assertEqual(mapped[:], b'unchanged\r\n')
    self.assertEqual(final.stat().st_mtime_ns, before.st_mtime_ns)
    self.assertEqual(final.stat().st_ino, before.st_ino)
    self.assert_cleaned()

  def test_changed_output_uses_atomic_replace_with_equal_size_and_mtime(self):
    final = self.write(self.output, 'foo.gen.h', b'old')
    before = final.stat()
    replace = os.replace

    def generate(command, staging):
      source = self.write(staging, 'foo.gen.h', b'new')
      os.utime(source, ns=(before.st_atime_ns, before.st_mtime_ns))
      self.assertEqual(final.read_bytes(), b'old')
      return 0

    with mock.patch.object(protoc_wrapper.os, 'replace', wraps=replace) as call:
      self.run_generator(generate)
    call.assert_called_once_with(str(self.staging / 'foo.gen.h'), str(final))
    self.assertEqual(final.read_bytes(), b'new')
    self.assert_cleaned()

  def test_failed_partial_generation_preserves_finals_and_diagnostic(self):
    final = self.write(self.output, 'foo.gen.h', b'old')
    before = final.stat().st_mtime_ns

    def generate(command, staging):
      self.write(staging, 'foo.gen.h', b'partial')
      self.write(staging, 'new.gen.h', b'partial new')
      print('generator diagnostic', file=sys.stderr)
      return 7

    with mock.patch.object(sys, 'stderr', new_callable=StringIO) as stderr:
      with self.assertRaisesRegex(RuntimeError, 'non-zero status: 7'):
        self.run_generator(generate)
    self.assertEqual(stderr.getvalue(), 'generator diagnostic\n')
    self.assertEqual(final.read_bytes(), b'old')
    self.assertEqual(final.stat().st_mtime_ns, before)
    self.assertFalse((self.output / 'new.gen.h').exists())
    self.assert_cleaned()

  def test_windows_exit_diagnostic(self):
    with self.assertRaisesRegex(RuntimeError, 'non-zero status: 0xC0000005'):
      self.run_generator(lambda command, staging: -1073741819)
    self.assert_cleaned()

  def test_shared_output_modes_are_postprocessed_before_publication(self):
    def generate(command, staging):
      self.assertEqual(command[command.index('--cpp_out') + 1], str(staging))
      self.assertEqual(command[command.index('--python_out') + 1], str(staging))
      self.assertIn('--ts_proto_out=' + str(staging), command)
      self.assertEqual(command[command.index('--js_out') + 1],
                       'one_output_file_per_input_file,binary:' + str(staging))
      self.write(staging, 'foo.pb.h',
                 (protoc_wrapper.PROTOC_INCLUDE_POINT + '\n').encode())
      self.write(staging, 'foo.ts',
                 b"import { x } from '@bufbuild/protobuf/wire';\n")
      self.write(staging, 'foo_pb2.py', b'python')
      self.write(staging, 'foo.js', b'javascript')
      return 0

    extra = [
      '--cc-out-dir', str(self.output) + '/', '--include', 'extra.h',
      '--py-out-dir', str(self.output), '--js-out-dir', str(self.output),
      '--protoc-gen-js', 'js.exe', '--ts-out-dir', str(self.output),
      '--protoc-gen-ts', 'ts.exe',
    ]
    self.run_generator(generate, extra)
    self.assertIn('#include "extra.h"', (self.output / 'foo.pb.h').read_text())
    self.assertIn("'/@bufbuild/protobuf/wire/index.js'",
                  (self.output / 'foo.ts').read_text())
    self.assertEqual((self.output / 'foo_pb2.py').read_bytes(), b'python')
    self.assertEqual((self.output / 'foo.js').read_bytes(), b'javascript')
    self.assert_cleaned()

  def test_failed_shared_postprocessing_does_not_publish(self):
    final = self.write(self.output, 'foo.pb.h', b'old')

    def generate(command, staging):
      self.write(staging, 'foo.pb.h', b'no insertion point')
      return 0

    with self.assertRaisesRegex(RuntimeError, 'Include point not found'):
      self.run_generator(generate, [
        '--cc-out-dir', str(self.output), '--include', 'extra.h',
      ])
    self.assertEqual(final.read_bytes(), b'old')
    self.assert_cleaned()

  def test_descriptor_depfile_only_rewrites_staged_output_prefixes(self):
    descriptor = self.root / 'descriptor.pb'
    depfile = self.root / 'descriptor.d'
    dependencies = ': input/foo.proto\\\n other.proto\n'
    expected = (str(self.output) + '/nested/foo.gen.h \\\n' +
                str(descriptor) + dependencies).encode()

    def generate(command, staging):
      self.assertNotIn('--include_imports', command)
      self.write(staging, 'nested/foo.gen.h', b'plugin')
      descriptor.write_bytes(b'descriptor')
      contents = (str(staging) + '/nested/foo.gen.h \\\n' +
                  str(descriptor) + dependencies)
      depfile.write_bytes(contents.encode())
      return 0

    self.run_generator(generate, [
      '--descriptor-set-out', str(descriptor),
      '--descriptor-set-dependency-file', str(depfile), '--exclude-imports',
    ])
    self.assertEqual(depfile.read_bytes(), expected)
    self.assertEqual(descriptor.read_bytes(), b'descriptor')
    self.assert_cleaned()

  def test_relative_output_directory_and_depfile(self):
    self.output = Path(os.path.relpath(self.output))
    depfile = self.root / 'descriptor.d'

    def generate(command, staging):
      self.write(staging, 'nested/foo.gen.h', b'plugin')
      # protoc removes a leading ./ from output paths in depfiles.
      path = str(staging).removeprefix('./') + '/nested/foo.gen.h'
      depfile.write_bytes((path + ': foo.proto').encode())
      return 0

    self.run_generator(generate, [
      '--descriptor-set-out', str(self.root / 'descriptor.pb'),
      '--descriptor-set-dependency-file', str(depfile),
    ])
    expected = (str(self.output) + '/nested/foo.gen.h: foo.proto').encode()
    self.assertEqual(depfile.read_bytes(), expected)
    self.assertEqual((self.output / 'nested/foo.gen.h').read_bytes(), b'plugin')
    self.assert_cleaned()

  def test_process_exception_cleans_staging(self):
    def generate(command, staging):
      self.write(staging, 'foo.gen.h', b'partial')
      raise OSError('cannot start protoc')

    with self.assertRaisesRegex(OSError, 'cannot start protoc'):
      self.run_generator(generate)
    self.assertFalse((self.output / 'foo.gen.h').exists())
    self.assert_cleaned()

  def test_publication_failure_is_not_retried_or_hidden(self):
    final = self.write(self.output, 'foo.gen.h', b'old')

    def generate(command, staging):
      self.write(staging, 'foo.gen.h', b'changed')
      return 0

    with mock.patch.object(
      protoc_wrapper.os, 'replace',
      side_effect=PermissionError('sharing violation')
    ) as call:
      with self.assertRaisesRegex(PermissionError, 'sharing violation'):
        self.run_generator(generate)
    call.assert_called_once()
    self.assertEqual(final.read_bytes(), b'old')
    self.assert_cleaned()

  @unittest.skipUnless(os.name == 'nt', 'Windows file sharing semantics')
  def test_changed_mapped_destination_fails_without_truncation(self):
    final = self.write(self.output, 'foo.gen.h', b'old')
    before = final.stat().st_mtime_ns

    def generate(command, staging):
      self.write(staging, 'foo.gen.h', b'changed')
      return 0

    with final.open('rb') as reader:
      with mmap.mmap(reader.fileno(), 0, access=mmap.ACCESS_READ) as mapped:
        with self.assertRaises(PermissionError):
          self.run_generator(generate)
        self.assertEqual(mapped[:], b'old')
    self.assertEqual(final.read_bytes(), b'old')
    self.assertEqual(final.stat().st_mtime_ns, before)
    self.assert_cleaned()

  def test_simultaneous_modes_preserve_command_and_postprocessing(self):
    cc = self.root / 'cpp'
    py = self.root / 'python'
    js = self.root / 'js'
    ts = self.root / 'ts'
    descriptor = self.root / 'descriptor.pb'
    depfile = self.root / 'descriptor.d'
    imports = self.root / 'imports'
    imports.mkdir()
    missing = self.root / 'missing'
    extra = [
      '--fatal_warnings', '--py-out-dir', str(py), '--js-out-dir', str(js),
      '--protoc-gen-js', str(self.root / 'js.exe'), '--ts-out-dir', str(ts),
      '--protoc-gen-ts', str(self.root / 'ts.exe'), '--cc-out-dir', str(cc),
      '--cc-options', 'foo=bar:', '--enable-kythe-annotations',
      '--include', 'extra.h', '--import-dir', str(imports),
      '--import-dir', str(missing), '--descriptor-set-out', str(descriptor),
      '--descriptor-set-dependency-file', str(depfile),
    ]

    def generate(command, staging):
      self.assertEqual(command, [
        os.path.realpath(self.root / 'protoc'), '--fatal_warnings',
        '--python_out', str(py), '--js_out',
        'one_output_file_per_input_file,binary:' + str(js),
        '--plugin=protoc-gen-js=' + os.path.realpath(self.root / 'js.exe'),
        '--ts_proto_out=' + str(ts),
        '--ts_proto_opt=env=browser,esModuleInterop=true,importSuffix=.js',
        '--ts_proto_opt=useOptionals=all',
        '--plugin=protoc-gen-ts_proto=' +
        os.path.realpath(self.root / 'ts.exe'),
        '--cpp_out', 'annotate_headers,annotation_pragma_name=kythe_metadata,'
        'annotation_guard_name=KYTHE_IS_RUNNING,foo=bar:' + str(cc),
        '--plugin',
        'protoc-gen-plugin=' + os.path.relpath(self.root / 'plugin.exe'),
        '--plugin_out', 'wrapper_namespace=gen:' + str(staging),
        '--proto_path', '.', '--proto_path', str(imports),
        os.path.join('.', 'foo.proto'), '--descriptor_set_out', str(descriptor),
        '--include_imports', '--dependency_out', str(depfile),
      ])
      self.write(staging, 'nested/foo.gen.h', b'plugin\r\n')
      self.write(cc, 'foo.pb.h',
                 (protoc_wrapper.PROTOC_INCLUDE_POINT + '\n').encode())
      self.write(ts, 'foo.ts',
                 b"import { x } from '@bufbuild/protobuf/wire';\n")
      self.write(py, 'foo_pb2.py', b'python\n')
      self.write(js, 'foo.js', b'javascript\n')
      descriptor.write_bytes(b'descriptor')
      depfile.write_bytes(b'descriptor.pb: foo.proto\n')
      return 0

    self.run_generator(generate, extra, 'wrapper_namespace=gen')
    self.assertIn('#include "extra.h"', (cc / 'foo.pb.h').read_text())
    self.assertIn("'/@bufbuild/protobuf/wire/index.js'",
                  (ts / 'foo.ts').read_text())
    self.assertEqual((self.output / 'nested/foo.gen.h').read_bytes(),
                     b'plugin\r\n')
    self.assertEqual(descriptor.read_bytes(), b'descriptor')
    self.assertEqual(depfile.read_bytes(), b'descriptor.pb: foo.proto\n')
    self.assert_cleaned()

  def test_non_windows_plugin_output_path_is_unchanged(self):
    args = [
      '--protoc', str(self.root / 'protoc'), '--proto-in-dir', '.', '--plugin',
      str(self.root / 'plugin.exe'), '--plugin-out-dir', str(self.output),
      '--plugin-options', 'wrapper_namespace=gen:', 'foo.proto',
    ]
    with mock.patch.object(protoc_wrapper.sys, 'platform', 'linux'):
      with mock.patch.object(
        protoc_wrapper.subprocess, 'call', return_value=0
      ) as call:
        protoc_wrapper.main(args)
    command = call.call_args.args[0]
    self.assertEqual(command[command.index('--plugin_out') + 1],
                     'wrapper_namespace=gen:' + str(self.output))
    self.assertFalse(self.output.exists())


if __name__ == '__main__':
  logging.basicConfig(
    level=logging.DEBUG if '-v' in sys.argv else logging.ERROR
  )
  unittest.main()
