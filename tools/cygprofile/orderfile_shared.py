# Copyright 2025 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
"""Shared utilities for orderfile generation."""

import cluster
import process_profiles


def ReadNonEmptyStrippedFromFile(file_name):
  """Reads a file, strips leading/trailing whitespace and removes empty lines."""
  stripped_lines = []
  with open(file_name, 'r') as file:
    for line in file:
      stripped_line = line.strip()
      if stripped_line:
        stripped_lines.append(stripped_line)
  return stripped_lines


def ProcessProfiles(profile_files, lib_chrome_so_path):
  """Processes profiles to get an ordered list of symbols."""
  profiles = process_profiles.ProfileManager(profile_files)
  processor = process_profiles.SymbolOffsetProcessor(lib_chrome_so_path)
  ordered_symbols = cluster.ClusterOffsets(profiles, processor)
  if not ordered_symbols:
    raise Exception('Failed to get ordered symbols')
  for sym in ordered_symbols:
    assert not sym.startswith('OUTLINED_FUNCTION_'), (
      'Outlined function found in instrumented function, very likely '
      'something has gone very wrong!'
    )
  symbols_size = processor.SymbolsSize(ordered_symbols)
  return ordered_symbols, symbols_size
