# Copyright 2021 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
"""This script provides the necessary flags to symbolize proto traces."""

import logging
import optparse

TRACING_LOGGER_NAME = 'chromium:tools/tracing'


def GeneralOptions(parser):
  """Build option group for general options.

  Args:
    parser: OptionParser object for parsing the command-line.

  Returns:
    Option group that contains general options.
  """
  general_options = optparse.OptionGroup(parser, 'General Options')

  general_options.add_option(
      '-o',
      '--output',
      help=
      ('Save trace output to file. Defaults to trace_file + "_symbolized_trace."'
       ),
      dest='output_file',
  )
  general_options.add_option(
      '-v',
      '--verbose',
      help='Increase output verbosity.',
      action='count',
      dest='verbosity',
      default=0,
  )
  general_options.add_option(
      '--view',
      help='Open resulting trace file in a browser.',
      action='store_true',
      dest='view',
  )

  return general_options


def SymbolizeOptions(parser):
  """Build option group for proto trace symbolization.

  Args:
    parser: OptionParser object for parsing the command-line.

  Returns:
    Option group that contains symbolization options.
  """
  symbolization_options = optparse.OptionGroup(parser, 'Symbolization Options')

  symbolization_options.add_option(
      '--breakpad_output_dir',
      help='Optional path to empty directory to store fetched trace symbols '
      'and extracted breakpad symbol files for official builds. Useful '
      'for symbolizing multiple traces from the same Chrome build. Use '
      '--local_breakpad_dir when you already have breakpad files fetched. '
      'Automatically uses temporary directory if flag not specified.',
      dest='breakpad_output_dir',
  )
  symbolization_options.add_option(
      '--local_breakpad_dir',
      help='Optional path to local directory containing breakpad symbol files '
      'used to symbolize the given trace. Breakpad files should be named '
      'with module id in upper case hexadecimal and ".breakpad" '
      'suffix. Ex: 12EFA32BE.breakpad',
      dest='local_breakpad_dir',
  )
  symbolization_options.add_option(
      '--local_build_dir',
      help='Optional path to a local build directory containing symbol files.',
      dest='local_build_dir',
  )
  symbolization_options.add_option(
      '--dump_syms',
      help=('Path to a dump_syms binary. Required when symbolizing an official '
            'build. Optional on local builds as we can use --local_build_dir '
            'to locate a binary. If built locally with '
            'autoninja -C out/Release dump_syms then the binary path is '
            'out/Release/dump_syms'),
      dest='dump_syms_path',
  )
  symbolization_options.add_option(
      '--symbolizer',
      help=('Path to the trace_to_text binary for symbolization. Defaults to '
            '"third_party/perfetto/tools/traceconv". traceconv is the same as '
            'trace_to_text, except that tracevonv finds a prebuilt '
            'trace_to_text binary to use.'),
      default='third_party/perfetto/tools/traceconv',
      dest='symbolizer_path',
  )
  symbolization_options.add_option(
      '--trace_processor',
      help=('Optional path to trace processor binary. '
            'Automatically downloads binary if flag not specified.'),
      dest='trace_processor_path',
  )
  symbolization_options.add_option(
      '--cloud_storage_bucket',
      help=('Optional cloud storage bucket to where symbol files reside. '
            'Defaults to "chrome-unsigned".'),
      default='chrome-unsigned',
      dest='cloud_storage_bucket',
  )

  return symbolization_options


def GetTracingLogger():
  """Returns the logger for tools/tracing."""
  return logging.getLogger(TRACING_LOGGER_NAME)


def SetupLogging(verbosity):
  """Setup logging levels.

  Sets default level to warning.

  Args:
    verbosity: None or integer type that specifies the logging level.
      (options.verbosity)
  """
  logger = GetTracingLogger()
  if verbosity == 1:
    logging.basicConfig(level=logging.INFO)
    logger.setLevel(level=logging.INFO)
  elif verbosity >= 2:
    logging.basicConfig(level=logging.DEBUG)
    logger.setLevel(level=logging.DEBUG)
  else:
    logging.basicConfig(level=logging.WARNING)
    logger.setLevel(level=logging.WARNING)
