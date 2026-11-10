#!/usr/bin/env python3
# Copyright 2018 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Parse saved Linux /proc/<pid>/smaps data for memory-dump analysis."""

import collections
import re


class Mapping:
  """A single entry (mapping) in /proc/[pid]/smaps."""

  def __init__(self, start, end, permissions, offset, pathname):
    """Initializes an instance.

    Args:
      start: (str) Start address of the mapping.
      end: (str) End address of the mapping.
      permissions: (str) Permission string, e.g. r-wp.
      offset: (str) Offset into the file or 0 if this is not a file mapping.
      pathname: (str) Path name, or pseudo-path, e.g. [stack]
    """
    self.start = int(start, 16)
    self.end = int(end, 16)
    self.permissions = permissions
    self.offset = int(offset, 16)
    self.pathname = pathname.strip()
    self.fields = collections.OrderedDict()

  def AddField(self, line):
    """Adds a field to an entry.

    Args:
      line: (str) As it appears in /proc/[pid]/smaps.
    """
    assert ':' in line
    split_index = line.index(':')
    k, v = line[:split_index].strip(), line[split_index + 1 :].strip()
    assert k not in self.fields
    if v.endswith('kB'):
      v = int(v[:-2])
    self.fields[k] = v

  def ToString(self):
    """Returns a string representation of a mapping.

    The returned string is similar (but not identical) to the /proc/[pid]/smaps
    entry it was generated from.
    """
    lines = []
    lines.append(
      '%x-%x %s %x %s'
      % (self.start, self.end, self.permissions, self.offset, self.pathname)
    )
    for name in self.fields:
      format_str = None
      if isinstance(self.fields[name], int):
        format_str = '%s: %d kB'
      else:
        format_str = '%s: %s'
      lines.append(format_str % (name, self.fields[name]))
    return '\n'.join(lines)


def _ParseProcSmapsLines(lines):
  SMAPS_ENTRY_START_RE = (
    # start-end
    '^([0-9a-f]{1,16})-([0-9a-f]{1,16}) '
    # Permissions
    '([r\-][w\-][x\-][ps]) '
    # Offset
    '([0-9a-f]{1,16}) '
    # Device
    '([0-9a-f]{2,3}:[0-9a-f]{2,3}) '
    # Inode
    '([0-9]*) '
    # Pathname
    '(.*)'
  )
  assert re.search(
    SMAPS_ENTRY_START_RE,
    '35b1800000-35b1820000 r-xp 00000000 08:02 135522  /usr/lib64/ld-2.15.so',
  )
  entry_re = re.compile(SMAPS_ENTRY_START_RE)

  mappings = []
  for line in lines:
    match = entry_re.search(line)
    if match:
      (start, end, perms, offset, _, _, pathname) = match.groups()
      mappings.append(Mapping(start, end, perms, offset, pathname))
    else:
      mappings[-1].AddField(line)
  return mappings
