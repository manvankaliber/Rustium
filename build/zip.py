#!/usr/bin/env python3
# Copyright 2014 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
"""Archives files and merges ZIPs for platform-independent build actions."""

import argparse
import fnmatch
import json
import re
import sys
import zipfile

import action_helpers
import gn_helpers
import zip_helpers


def _expand_file_args(args):
    """Expand @FileArg(file:key:...) references used by build actions."""
    result = list(args)
    file_jsons = {}
    for index, arg in enumerate(args):
        match = re.search(r'@FileArg\((.*?)\)', arg)
        if not match:
            continue
        keys = match.group(1).split(':')
        file_path = keys[0].removesuffix('[]')
        if file_path not in file_jsons:
            with open(file_path, encoding='utf-8') as source:
                file_jsons[file_path] = json.load(source)
        expansion = file_jsons
        for key in keys:
            flatten = key.endswith('[]')
            expansion = expansion[key.removesuffix('[]')]
            if flatten:
                if not isinstance(expansion, list) or len(expansion) != 1:
                    raise ValueError('Expected single item list: %r' % expansion)
                expansion = expansion[0]
        replacement = (gn_helpers.ToGNString(expansion)
                       if isinstance(expansion, list) else str(expansion))
        result[index] = arg[:match.start()] + replacement + arg[match.end():]
    return result


def main(args):
    parser = argparse.ArgumentParser()
    parser.add_argument('--input-files', help='GN-list of files to zip.')
    parser.add_argument('--input-files-base-dir')
    parser.add_argument('--input-zips', help='GN-list of ZIPs to merge.')
    parser.add_argument('--input-zips-excluded-globs')
    parser.add_argument('--output', required=True)
    compress = parser.add_mutually_exclusive_group()
    compress.add_argument('--compress', action='store_true')
    compress.add_argument('--no-compress', action='store_false', dest='compress')
    parser.add_argument('--comment-json', action='append', metavar='KEY=VALUE',
                        type=lambda value: value.split('=', 1))
    action_helpers.add_depfile_arg(parser)
    options = parser.parse_args(_expand_file_args(args))

    dependencies = None
    with action_helpers.atomic_output(options.output) as output:
        with zipfile.ZipFile(output.name, 'w') as archive:
            if options.input_files:
                zip_helpers.add_files_to_zip(
                    action_helpers.parse_gn_list(options.input_files), archive,
                    base_dir=options.input_files_base_dir,
                    compress=options.compress)
            if options.input_zips:
                dependencies = action_helpers.parse_gn_list(options.input_zips)
                transform = None
                if options.input_zips_excluded_globs:
                    globs = action_helpers.parse_gn_list(
                        options.input_zips_excluded_globs)
                    transform = lambda path: (
                        None if any(fnmatch.fnmatch(path, pattern)
                                    for pattern in globs) else path)
                zip_helpers.merge_zips(archive, dependencies,
                                      path_transform=transform,
                                      compress=options.compress)
            if options.comment_json:
                archive.comment = json.dumps(dict(options.comment_json),
                                             sort_keys=True).encode('utf-8')
    if options.depfile:
        action_helpers.write_depfile(options.depfile, options.output,
                                     inputs=dependencies)


if __name__ == '__main__':
    main(sys.argv[1:])
