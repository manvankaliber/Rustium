# Copyright 2022 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
"""For interfacing with --json-config file."""

import json


class JsonConfig:

  def __init__(self, json_obj):
    self._json_obj = json_obj

  def _NativeFile(self, basename):
    return self._json_obj.get('native_files', {}).get(basename, {})

  def ComponentForNativeFile(self, basename):
    return self._NativeFile(basename).get('component')

  def GenDirRegexForNativeFile(self, basename):
    return self._NativeFile(basename).get('gen_dir_regex')

  def SourcePathPrefixForNativeFile(self, basename):
    return self._NativeFile(basename).get('source_path_prefix')

  def ComponentOverrides(self):
    """Tuple of (path_prefix, component) tuples."""
    return tuple(self._json_obj.get('component_overrides', {}).items())


def Parse(path, on_config_error):
  try:
    with open(path) as f:
      return JsonConfig(json.load(f))
  except Exception as e:
    on_config_error(f'Error while parsing {path}: {e}')
