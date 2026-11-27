#!/usr/bin/env python
#
# Copyright 2014 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Integration test for breakpad in content shell.

This test checks that content shell and breakpad are correctly hooked up, as
well as that the tools can symbolize a stack trace."""

from __future__ import print_function
from __future__ import absolute_import
import glob
import json
import argparse
import os
import shutil
import subprocess
import sys
import tempfile
import time

TOP_SRC_DIR = os.path.join(os.path.dirname(__file__), '..', '..', '..')



CONCURRENT_TASKS = 4
BREAKPAD_TOOLS_DIR = os.path.join(
  TOP_SRC_DIR, 'components', 'crash', 'content', 'tools'
)








def run_test(
  options, crash_dir, symbols_dir, platform, additional_arguments=[]
):
  global failure

  print('# Run content_shell and make it crash.')
  cmd = [
    options.binary,
    '--run-web-tests',
    'chrome://crash',
    '--enable-crash-reporter',
    '--crash-dumps-dir=%s' % crash_dir,
  ]
  cmd += additional_arguments

  if options.verbose:
    print(' '.join(cmd))
  failure = 'Failed to run content_shell.'
  if options.verbose:
    subprocess.check_call(cmd)
  else:
    # On Windows, using os.devnull can cause check_call to never return,
    # so use a temporary file for the output.
    with tempfile.TemporaryFile() as tmpfile:
      subprocess.check_call(cmd, stdout=tmpfile, stderr=tmpfile)

  print('# Retrieve crash dump.')
  dmp_dir = crash_dir
  # TODO(crbug.com/41354248): This test should not reach directly into the
  # Crashpad database, but instead should use crashpad_database_util.
  if platform == 'darwin' or platform.startswith('linux'):
    dmp_dir = os.path.join(dmp_dir, 'pending')
  elif platform == 'win32':
    dmp_dir = os.path.join(dmp_dir, 'reports')

  dmp_files = glob.glob(os.path.join(dmp_dir, '*.dmp'))
  failure = 'Expected 1 crash dump, found %d.' % len(dmp_files)
  if len(dmp_files) != 1:
    raise Exception(failure)
  minidump = dmp_files[0]

  print('# Symbolize crash dump.')
  if platform == 'win32':
    cdb_exe = os.path.join(options.build_dir, 'cdb', 'cdb.exe')
    cmd = [
      cdb_exe,
      '-y',
      options.build_dir,
      '-c',
      '.lines;.excr;k30;q',
      '-z',
      minidump,
    ]
    if options.verbose:
      print(' '.join(cmd))
    failure = 'Failed to run cdb.exe.'
    proc = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    stack = proc.communicate()[0].decode('utf-8')
  else:
    minidump_stackwalk = os.path.join(options.build_dir, 'minidump_stackwalk')
    cmd = [minidump_stackwalk, minidump, symbols_dir]
    if options.verbose:
      print(' '.join(cmd))
    failure = 'Failed to run minidump_stackwalk.'
    proc = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    stack = proc.communicate()[0].decode('utf-8')

  # Check whether the stack contains a CrashIntentionally symbol.
  found_symbol = 'CrashIntentionally' in stack
  os.remove(minidump)

  if options.no_symbols:
    if found_symbol:
      if options.verbose:
        print(stack)
      failure = 'Found unexpected reference to CrashIntentionally in stack'
      raise Exception(failure)
  else:
    if not found_symbol:
      if options.verbose:
        print(stack)
      failure = 'Could not find reference to CrashIntentionally in stack.'
      raise Exception(failure)


def main():
  global failure

  parser = argparse.ArgumentParser()
  parser.add_argument(
    '--build-dir', default='', help='The build output directory.'
  )
  parser.add_argument(
    '--binary',
    default='',
    help='The path of the binary to generate symbols for and '
    'then run for the test.',
  )
  parser.add_argument(
    '--additional-binary',
    default='',
    help='An additional binary for which to generate '
    'symbols. On Mac this is used for specifying the '
    '"Content Shell Framework" library, which is not '
    'linked into --binary.',
  )
  parser.add_argument(
    '--no-symbols',
    default=False,
    action='store_true',
    help='Symbols are not expected to work.',
  )
  parser.add_argument(
    '-j',
    '--jobs',
    default=CONCURRENT_TASKS,
    type=int,
    help='Number of parallel tasks to run.',
  )
  parser.add_argument(
    '-v', '--verbose', action='store_true', help='Print verbose status output.'
  )
  parser.add_argument('--json', default='', help='Path to JSON output.')
  parser.add_argument(
    '--platform', default=sys.platform, help='Platform to run the test on.'
  )

  options, unrecognized = parser.parse_known_args()

  if not options.build_dir:
    print('Required option --build-dir missing.')
    return 1

  if not options.binary:
    print('Required option --binary missing.')
    return 1

  if not os.access(options.binary, os.X_OK):
    print('Cannot find %s.' % options.binary)
    return 1

  failure = ''

  # Create a temporary directory to store the crash dumps and symbols in.
  crash_dir = tempfile.mkdtemp()
  symbols_dir = os.path.join(crash_dir, 'symbols')

  crash_service = None

  try:

    if options.platform != 'win32':
      print('# Generate symbols.')
      bins = [options.binary]
      if options.additional_binary:
        bins.append(options.additional_binary)
      generate_symbols = os.path.join(
        BREAKPAD_TOOLS_DIR, 'generate_breakpad_symbols.py'
      )
      for binary in bins:
        cmd = [
          generate_symbols,
          '--build-dir=%s' % options.build_dir,
          '--binary=%s' % binary,
          '--symbols-dir=%s' % symbols_dir,
          '--jobs=%d' % options.jobs,
          '--platform=%s' % options.platform,
        ]
        if options.verbose:
          cmd.append('--verbose')
          print(' '.join(cmd))
        failure = 'Failed to run generate_breakpad_symbols.py.'
        subprocess.check_call(cmd)

    run_test(options, crash_dir, symbols_dir, options.platform, unrecognized)

  except:  # noqa: E722
    if failure == '':
      failure = '%s: %s' % sys.exc_info()[:2]
    print('FAIL: %s' % failure)
    if options.json:
      with open(options.json, 'w') as json_file:
        json.dump([failure], json_file)

    return 1

  else:
    print('PASS: Breakpad integration test ran successfully.')
    if options.json:
      with open(options.json, 'w') as json_file:
        json.dump([], json_file)
    return 0

  finally:
    if crash_service:
      crash_service.terminate()
      crash_service.wait()
    try:
      shutil.rmtree(crash_dir)
    except:  # noqa: E722
      print('Failed to delete temp directory "%s".' % crash_dir)


if '__main__' == __name__:
  sys.exit(main())
