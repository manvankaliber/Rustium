#!/usr/bin/env python3
# Copyright 2026 The Rustium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
"""Apply desktop integration patches to fetched dependencies."""

import argparse
import json
import os
from pathlib import Path
import subprocess


ROOT = Path(__file__).resolve().parents[1]
PATCHES = Path(__file__).resolve().parent / 'desktop_dependency_patches'


def checked_path(root, relative):
  path = Path(relative)
  if path.is_absolute() or '..' in path.parts or '.git' in path.parts:
    raise ValueError('Unsafe dependency patch path: ' + relative)
  result = (root / path).resolve()
  if not result.is_relative_to(root.resolve()):
    raise ValueError('Dependency patch path escapes checkout: ' + relative)
  return result


def git_apply(repository, patch, *options):
  return subprocess.run(
      ['git.exe' if os.name == 'nt' else 'git', 'apply',
       '--ignore-space-change', *options, str(patch)],
      cwd=repository,
      capture_output=True,
      encoding='utf-8',
      errors='replace')


def apply_patches(root, check=False):
  manifest = json.loads((PATCHES / 'manifest.json').read_text(encoding='utf-8'))
  if manifest['version'] != 1:
    raise ValueError('Unsupported desktop dependency patch manifest')
  results = []
  for entry in manifest['patches']:
    repository = checked_path(root, entry['repository'])
    patch = checked_path(PATCHES, entry['patch'])
    if not repository.is_dir() or not any(repository.iterdir()):
      results.append({'name': entry['name'], 'status': 'not checked out'})
      continue
    if git_apply(repository, patch, '--reverse', '--check').returncode == 0:
      results.append({'name': entry['name'], 'status': 'already applied'})
      continue
    pending = git_apply(repository, patch, '--check')
    if pending.returncode:
      raise RuntimeError(entry['name'] + ': ' + pending.stderr.strip())
    if check:
      results.append({'name': entry['name'], 'status': 'pending'})
      continue
    applied = git_apply(repository, patch)
    if applied.returncode:
      raise RuntimeError(entry['name'] + ': ' + applied.stderr.strip())
    results.append({'name': entry['name'], 'status': 'applied'})
  return results


def main():
  parser = argparse.ArgumentParser(description=__doc__)
  parser.add_argument('--root', type=Path, default=ROOT)
  parser.add_argument('--check', action='store_true')
  args = parser.parse_args()
  results = apply_patches(args.root.resolve(), args.check)
  print(json.dumps(results, indent=2))
  return int(any(item['status'] == 'pending' for item in results))


if __name__ == '__main__':
  raise SystemExit(main())
