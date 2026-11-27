#!/usr/bin/env python3

# Copyright 2014 The Crashpad Authors
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

import argparse
import os
import subprocess
import sys

CRASHPAD_DIR = os.path.join(os.path.dirname(os.path.abspath(__file__)),
                            os.pardir)
IS_WINDOWS_HOST = sys.platform.startswith('win')


def _EnableVTProcessingOnWindowsConsole():
    """Enables virtual terminal processing for ANSI/VT100-style escape sequences
    on a Windows console attached to standard output. Returns True on success.
    Returns False if standard output is not a console or if virtual terminal
    processing is not supported. The feature was introduced in Windows 10.
    """

    import pywintypes
    import win32console
    import winerror

    stdout_console = win32console.GetStdHandle(win32console.STD_OUTPUT_HANDLE)
    try:
        console_mode = stdout_console.GetConsoleMode()
    except pywintypes.error as e:
        if e.winerror == winerror.ERROR_INVALID_HANDLE:
            # Standard output is not a console.
            return False
        raise

    try:
        # From <wincon.h>. This would be
        # win32console.ENABLE_VIRTUAL_TERMINAL_PROCESSING, but it’s too new to
        # be defined there.
        ENABLE_VIRTUAL_TERMINAL_PROCESSING = 0x0004

        stdout_console.SetConsoleMode(console_mode
                                      | ENABLE_VIRTUAL_TERMINAL_PROCESSING)
    except pywintypes.error as e:
        if e.winerror == winerror.ERROR_INVALID_PARAMETER:
            # ANSI/VT100-style escape sequence processing isn’t supported before
            # Windows 10.
            return False
        raise

    return True


# This script is primarily used from the waterfall so that the list of tests
# that are run is maintained in-tree, rather than in a separate infrastructure
# location in the recipe.
def main(args):
    parser = argparse.ArgumentParser(description='Run Crashpad unittests.')
    parser.add_argument('binary_dir', help='Root of build dir')
    parser.add_argument('test', nargs='*', help='Specific test(s) to run.')
    parser.add_argument(
        '--gtest_filter',
        help='Google Test filter applied to Google Test binary runs.')
    args = parser.parse_args()

    # Tell 64-bit Windows tests where to find 32-bit test executables, for
    # cross-bitted testing. This relies on the fact that the GYP build by
    # default uses {Debug,Release} for the 32-bit build and {Debug,Release}_x64
    # for the 64-bit build. This is not a universally valid assumption, and if
    # it’s not met, 64-bit tests that require 32-bit build output will disable
    # themselves dynamically.
    if (sys.platform == 'win32' and args.binary_dir.endswith('_x64')
            and 'CRASHPAD_TEST_32_BIT_OUTPUT' not in os.environ):
        binary_dir_32 = args.binary_dir[:-4]
        if os.path.isdir(binary_dir_32):
            os.environ['CRASHPAD_TEST_32_BIT_OUTPUT'] = binary_dir_32

    tests = [
        'crashpad_client_test',
        'crashpad_handler_test',
        'crashpad_minidump_test',
        'crashpad_snapshot_test',
        'crashpad_test_test',
        'crashpad_util_test',
    ]
    if IS_WINDOWS_HOST:
        tests.append('snapshot/win/end_to_end_test.py')

    if args.test:
        for t in args.test:
            if t not in tests:
                print('Unrecognized test:', t, file=sys.stderr)
                return 3
        tests = args.test

    for test in tests:
        print('-' * 80)
        print(test)
        print('-' * 80)
        if test.endswith('.py'):
            subprocess.check_call([
                sys.executable,
                os.path.join(CRASHPAD_DIR, test), args.binary_dir
            ])
        else:
            extra_command_line = []
            if args.gtest_filter:
                extra_command_line.append('--gtest_filter=' + args.gtest_filter)
            subprocess.check_call([os.path.join(args.binary_dir, test)] +
                                  extra_command_line)

    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
