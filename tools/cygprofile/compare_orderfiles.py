#!/usr/bin/env vpython3
# Copyright 2017 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Compares two local orderfiles and reports their symbol ordering statistics."""

from __future__ import print_function

import argparse
import collections
import logging
import sys


def ParseOrderfile(filename):
  """Parses an orderfile into a list of symbols.

  Args:
    filename: (str) Path to the orderfile.

  Returns:
    [str] List of symbols.
  """
  symbols = []
  lines = []
  already_seen = set()
  with open(filename, 'r') as f:
    lines = [line.strip() for line in f]

  # The (new) orderfiles that are oriented at the LLD linker contain only symbol
  # names (i.e. not prefixed with '.text'). The (old) orderfiles aimed at the
  # Gold linker were patched by duplicating symbols prefixed with '.text.hot.',
  # '.text.unlikely.' and '.text.', hence the appearance of '.text' on the first
  # symbol indicates such a legacy orderfile.
  if not lines[0].startswith('.text.'):
    for entry in lines:
      symbol_name = entry.rstrip('\n')
      assert symbol_name not in ('*', '.text')
      already_seen.add(symbol_name)
      symbols.append(symbol_name)
  else:
    for entry in lines:
      # Keep only (input) section names, not symbol names (only rare special
      # symbols contain '.'). We could only keep symbols, but then some even
      # older orderfiles would not be parsed.
      if '.' not in entry:
        continue
      # Example: .text.startup.BLA
      symbol_name = entry[entry.rindex('.') :]
      if symbol_name in already_seen or symbol_name == '*' or entry == '.text':
        continue
      already_seen.add(symbol_name)
      symbols.append(symbol_name)
  return symbols


def CommonSymbolsToOrder(symbols, common_symbols):
  """Returns s -> index for all s in common_symbols."""
  result = {}
  index = 0
  for s in symbols:
    if s not in common_symbols:
      continue
    result[s] = index
    index += 1
  return result


CompareResult = collections.namedtuple(
  'CompareResult',
  (
    'first_count',
    'second_count',
    'new_count',
    'removed_count',
    'average_fractional_distance',
  ),
)


def Compare(first_filename, second_filename):
  """Outputs a comparison of two orderfiles to stdout.

  Args:
    first_filename: (str) First orderfile.
    second_filename: (str) Second orderfile.

  Returns:
    An instance of CompareResult.
  """
  first_symbols = ParseOrderfile(first_filename)
  second_symbols = ParseOrderfile(second_filename)
  print(
    'Symbols count:\n\tfirst:\t%d\n\tsecond:\t%d'
    % (len(first_symbols), len(second_symbols))
  )
  first_symbols = set(first_symbols)
  second_symbols = set(second_symbols)
  new_symbols = second_symbols - first_symbols
  removed_symbols = first_symbols - second_symbols
  common_symbols = first_symbols & second_symbols
  # Distance between orderfiles.
  first_to_ordering = CommonSymbolsToOrder(first_symbols, common_symbols)
  second_to_ordering = CommonSymbolsToOrder(second_symbols, common_symbols)
  total_distance = sum(
    abs(first_to_ordering[s] - second_to_ordering[s]) for s in first_to_ordering
  )
  # Each distance is in [0, len(common_symbols)] and there are
  # len(common_symbols) entries, hence the normalization.
  average_fractional_distance = float(total_distance) / (
    len(common_symbols) ** 2
  )
  print('New symbols = %d' % len(new_symbols))
  print('Removed symbols = %d' % len(removed_symbols))
  print(
    'Average fractional distance = %.2f%%'
    % (100.0 * average_fractional_distance)
  )
  return CompareResult(
    len(first_symbols),
    len(second_symbols),
    len(new_symbols),
    len(removed_symbols),
    average_fractional_distance,
  )


def CreateArgumentParser():
  """Returns the argument parser for local orderfile comparison."""
  parser = argparse.ArgumentParser()
  parser.add_argument('--first', help='First orderfile')
  parser.add_argument('--second', help='Second orderfile')
  return parser


def main():
  logging.basicConfig(level=logging.INFO)
  parser = CreateArgumentParser()
  args = parser.parse_args()
  if args.first or args.second:
    assert args.first and args.second, 'Need both files.'
    Compare(args.first, args.second)
  else:
    return False
  return True


if __name__ == '__main__':
  sys.exit(0 if main() else 1)
