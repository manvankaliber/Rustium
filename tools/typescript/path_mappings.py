# Copyright 2023 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import argparse
import collections
import sys
import json
import io
import os


def _add_ui_webui_resources_mappings(path_mappings, root_gen_dir):
  # Calculate mappings for ui/webui/resources/ sub-folders that have a dedicated
  # ts_library() target. The naming of the ts_library() target is expected to
  # follow the default "build_ts" naming in the build_webui() rule. The output
  # folder is expected to be at '$root_gen_dir/ui/webui/resources/tsc/'.
  shared_ts_folders = [
    "cr_elements",
    "js",
    "mojo",
    "cr_components/app_management",
    "cr_components/color_change_listener",
    "cr_components/commerce",
    "cr_components/cr_lottie",
    "cr_components/cr_shortcut_input",
    "cr_components/customize_color_scheme_mode",
    "cr_components/customize_themes",
    "cr_components/help_bubble",
    "cr_components/history",
    "cr_components/history_clusters",
    "cr_components/localized_link",
    "cr_components/managed_dialog",
    "cr_components/managed_footnote",
    "cr_components/most_visited",
    "cr_components/page_image_service",
    "cr_components/searchbox",
    "cr_components/composebox",
    "cr_components/search",
    "cr_components/settings_prefs",
    "cr_components/theme_color_picker",
  ]

  for c in shared_ts_folders:
    path_mappings[f'//ui/webui/resources/{c}:build_ts'] = [
      (
        f'//resources/{c}/*',
        f'{root_gen_dir}/ui/webui/resources/tsc/{c}/*',
      )
    ]


def _add_third_party_polymer_mappings(path_mappings, root_src_dir):
  path_mappings['//third_party/polymer/v3_0:library'] = [
    (
      '//resources/polymer/v3_0/polymer/polymer_bundled.min.js',
      (
        f'{root_src_dir}/third_party/polymer/v3_0/components-chromium/'
        'polymer/polymer.d.ts'
      ),
    ),
    (
      '//resources/polymer/v3_0/*',
      f'{root_src_dir}/third_party/polymer/v3_0/components-chromium/*',
    ),
  ]


def _add_third_party_d3_mappings(path_mappings, root_src_dir):
  path_mappings['//third_party/d3:library'] = [
    ('//resources/d3/d3.min.js', f'{root_src_dir}/third_party/d3/src/d3.d.ts'),
  ]


def _add_third_party_lit_mappings(path_mappings, root_gen_dir):
  path_mappings['//third_party/lit/v3_0:build_ts'] = [
    (
      '//resources/lit/v3_0/lit.rollup.js',
      f'{root_gen_dir}/third_party/lit/v3_0/lit.d.ts',
    ),
  ]




def GetDepToPathMappings(root_gen_dir, root_src_dir):
  path_mappings = {}

  _add_ui_webui_resources_mappings(path_mappings, root_gen_dir)
  _add_third_party_d3_mappings(path_mappings, root_src_dir)
  _add_third_party_lit_mappings(path_mappings, root_gen_dir)
  _add_third_party_polymer_mappings(path_mappings, root_src_dir)


  return path_mappings






def _write_path_mappings_file(
  path_mappings, output_suffix, out_dir, pretty_print
):
  path_mappings_filename = f'path_mappings_{output_suffix}.json'
  if not os.path.exists(out_dir):
    os.makedirs(out_dir)
  out_file_path = os.path.join(out_dir, path_mappings_filename)
  with open(out_file_path, 'w', encoding='utf-8') as map_file:
    indent = 2 if pretty_print else None
    map_file.write(json.dumps(path_mappings, indent=indent))


def main(argv):
  parser = argparse.ArgumentParser()
  parser.add_argument('--raw_deps', nargs='*')
  parser.add_argument('--root_gen_dir', required=True)
  parser.add_argument('--root_src_dir', required=True)
  parser.add_argument('--gen_dir', required=True)
  parser.add_argument('--output_suffix', required=True)
  parser.add_argument(
    '--webui_context_type',
    choices=['trusted', 'untrusted', 'relative', 'trusted_only'],
    default='trusted',
  )
  parser.add_argument('--pretty_print', action='store_true')
  args = parser.parse_args(argv)

  dep_to_path_mappings = GetDepToPathMappings(
    args.root_gen_dir,
    # Sometimes root_src_dir has trailing slashes. Remove them if necessary.
    args.root_src_dir.rstrip('/'),
  )

  path_mappings = collections.defaultdict(list)

  # First, add a path mapping for '/strings.m.js', which is not tied to
  # `raw_deps` and is used extensively throughout WebUI.
  path_mappings['/strings.m.js'].append(
    f'{args.root_src_dir}/tools/typescript/definitions/strings.d.ts'.replace(
      '\\', '/'
    )
  )

  # Then add path mappings that can be derived from `raw_deps`.
  for dep in args.raw_deps:

    if dep not in dep_to_path_mappings:
      assert not dep.startswith("//ui/webui/resources"), (
        f'Missing path mapping for \'{dep}\'. Update '
        '//tools/typescript/path_mappings.py accordingly.'
      )

      # Path mappings outside of //ui/webui/resources are not inferred from
      # |args.deps| yet.
      continue

    mappings = dep_to_path_mappings[dep]
    scheme = (
      'chrome-untrusted:'
      if args.webui_context_type == 'untrusted'
      else 'chrome:'
    )
    for url, dir in mappings:
      if args.webui_context_type != 'trusted_only':
        path_mappings[url].append(os.path.join('./', dir).replace('\\', '/'))
      if url.startswith("//") and args.webui_context_type != 'relative':
        path_mappings[scheme + url].append(
          os.path.join('./', dir).replace('\\', '/')
        )

  _write_path_mappings_file(
    path_mappings, args.output_suffix, args.gen_dir, args.pretty_print
  )


if __name__ == '__main__':
  main(sys.argv[1:])
