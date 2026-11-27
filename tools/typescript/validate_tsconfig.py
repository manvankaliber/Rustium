# Copyright 2023 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# Overview: ts_library() supports only a small subset of all possible tsconfig
# configurations. Some options cannot be used in general, and some options
# should only be set through the ts_library() gn args corresponding to them. In
# the latter case, there are requirements for how some of these args can be
# configured. This file contains validation logic for tsconfig files and the gn
# args corresponding to config options to limit the possibility of unsupported
# configurations proliferating in the codebase.

import os
import pathlib

from path_utils import getTargetPath

_CWD = os.getcwd().replace('\\', '/')
_HERE_DIR = os.path.dirname(__file__)
_SRC_DIR = os.path.normpath(os.path.join(_HERE_DIR, '..', '..')).replace(
  '\\', '/'
)

# Options configured by the ts_library should not be set separately.
_tsconfig_compiler_options_mappings = {
  'composite': 'composite=true',
  'declaration': 'composite=true',
  'inlineSourceMap': 'enable_source_maps=true',
  'inlineSources': 'enable_source_maps=true',
  'outDir': 'out_dir',
  'paths': 'path_mappings',
  'rootDir': 'root_dir',
  'tsBuildInfoFile': 'composite=true',
}

# Allowed options within tsconfig_base.json
_allowed_config_options = [
  'extends',
  'compilerOptions',
]

# Allowed compilerOptions. A 'None' value indicates that all values are allowed,
# otherwise only the set of specified values is allowed.
_allowed_compiler_options = {
  'allowUmdGlobalAccess': None,
  'isolatedModules': None,
  'lib': None,
  'module': ['NodeNext', 'preserve'],
  'moduleResolution': ['NodeNext', 'bundler'],
  'noPropertyAccessFromIndexSignature': None,
  'noUncheckedIndexedAccess': None,
  'noUncheckedSideEffectImports': None,
  'noUnusedLocals': None,
  'skipLibCheck': None,
  'strictPropertyInitialization': None,
  'target': ['ESNext', 'ES2024'],
  'typeRoots': None,
  'types': None,
}



def validateTsconfigJson(tsconfig, tsconfig_file, is_base_tsconfig):
  # Special exception for material_web_components, which uses ts_library()
  # in an unsupported way.
  if 'third_party/material_web_components/tsconfig_base.json' in tsconfig_file:
    return True, None


  # TODO(b/267329383): Migrate A11y to TypeScript. Accessibility code has
  # different requirements for the migration because of this we need both
  # allowjs and a custom tsconfig.
  if 'accessibility/tsconfig.base.json' in tsconfig_file:
    return True, None

  if not is_base_tsconfig:
    for param in tsconfig.keys():
      if param not in _allowed_config_options:
        return (
          False,
          f'Invalid |{param}| option detected in '
          + f'{tsconfig_file}.Only |extends| and |compilerOptions| may '
          + 'be specified.',
        )

  if 'compilerOptions' in tsconfig:
    for param in _tsconfig_compiler_options_mappings.keys():
      if param in tsconfig['compilerOptions']:
        tslibrary_flag = _tsconfig_compiler_options_mappings[param]
        return (
          False,
          f'Invalid |{param}| flag detected in {tsconfig_file}.'
          + f' Use the dedicated |{tslibrary_flag}| attribute in '
          + 'ts_library() instead.',
        )


    if not is_base_tsconfig:
      for param, param_value in tsconfig['compilerOptions'].items():
        if param not in _allowed_compiler_options:
          return (
            False,
            f'Disallowed |{param}| flag detected in '
            + f'\'{tsconfig_file}\'.',
          )
        else:
          allowed_values = _allowed_compiler_options[param]
          if allowed_values is not None and param_value not in allowed_values:
            return (
              False,
              f'Disallowed value |{param_value}| for |{param}| '
              + f'flag detected in \'{tsconfig_file}\'. Must be one of '
              + f'{allowed_values}.',
            )

  return True, None


# Note 1: DO NOT add any directories here corresponding to newly added or
# existing TypeScript code. Instead use TypeScript, which is a requirement.
# Note 2: Only add a directory here if you are in the process of migrating
# legacy JS code to TS. Any new entries here should be accompanied by a bug
# tracking the TS migration.
def validateJavaScriptAllowed(source_dir, out_dir):


  # Specific exceptions for directories that are still migrating to TS.
  migrating_directories = [
    # TODO(crbug.com/385341235): Migrate inspect to TypeScript.
    'chrome/browser/resources/inspect',
    'chrome/browser/resources/net_internals',
    'chrome/test/data/webui',
    'components/autofill/core/browser/autofill_and_password_manager_internals',
    'components/net_log/resources',
    'components/safe_browsing/content/browser/web_ui/resources',
    'components/translate/translate_internals',
    'content/browser/webrtc/resources',
    'ui/webui/resources/js',
    'ui/webui/resources/mojo',
    # TODO(crbug.com/40280699) : Migrate to TypeScript.
    'chrome/test/data/webui/media_internals',
    'content/browser/resources/media',
  ]
  for directory in migrating_directories:
    if source_dir.endswith(directory) or source_dir.endswith(
      directory + '/preprocessed'
    ):
      return True, None

  return (
    False,
    'Invalid JS file detected for input directory '
    + f'{source_dir} and output directory {out_dir}, all new '
    + 'code should be added in TypeScript.',
  )






# |root_dir| shouldn't refer to any parent directories. Specifically it should
# be either:
#   - within the folder tree starting at the ts_library() target's location
#   - within the folder tree starting at the ts_library() target's corresponding
#     target_gen_dir location.
def validateRootDir(root_dir, gen_dir, root_gen_dir):
  root_gen_dir_from_build = os.path.normpath(
    os.path.join(gen_dir, root_gen_dir)
  ).replace('\\', '/')
  target_path = os.path.relpath(gen_dir, root_gen_dir_from_build).replace(
    '\\', '/'
  )



  target_path_src = os.path.relpath(
    os.path.join(_SRC_DIR, target_path), _CWD
  ).replace('\\', '/')
  root_path_from_gen = os.path.relpath(
    root_dir, root_gen_dir_from_build
  ).replace('\\', '/')
  root_path_from_src = os.path.relpath(
    os.path.join(_CWD, root_dir), _SRC_DIR
  ).replace('\\', '/')

  if root_path_from_gen.startswith(
    target_path
  ) or root_path_from_src.startswith(target_path):
    return True, None

  return (
    False,
    f'Error: root_dir ({root_dir}) should be within {gen_dir} '
    + f'or {target_path_src}.',
  )


def validateDefinitionDeps(
  definitions_files, target_path, gen_dir, root_gen_dir, definitions
):
  # Root gen dir relative to the current working directory (essentially 'gen')
  gen_dir_from_build = os.path.normpath(
    os.path.join(gen_dir, root_gen_dir)
  ).replace('\\', '/')

  def getPathFromCwd(exception):
    return os.path.relpath(os.path.join(_SRC_DIR, exception), _CWD).replace(
      '\\', '/'
    )

  # TODO(https://crbug.com/326005022): Determine if the following are actually
  # safe for computation of gn input values.
  exceptions_list = [
    'third_party/d3/',
    'third_party/material_web_components/',
    'third_party/node/node_modules/',
    'third_party/polymer/v3_0/',
    'third_party/typescript/',
    'tools/typescript/tests/',
  ]
  exceptions = [getPathFromCwd(e) for e in exceptions_list]
  definitions_normalized = [d.replace('\\', '/') for d in definitions]

  missing_inputs = []
  for f in definitions_files:
    # File path relative to the current working directory.
    f_from_cwd = os.path.relpath(f, _CWD).replace('\\', '/')
    is_gen_file = f_from_cwd.startswith(gen_dir_from_build)
    f_from_gen = os.path.relpath(f, gen_dir).replace('\\', '/')
    if (
      not is_gen_file
      and f_from_gen not in definitions_normalized
      and not any(f_from_cwd.startswith(exception) for exception in exceptions)
    ):
      missing_inputs.append(
        os.path.relpath(f_from_cwd, _SRC_DIR).replace('\\', '/')
      )

  if not missing_inputs:
    return True, None

  errorMessage = (
    'Undeclared dependencies to definition files encountered '
    + f'while building {target_path}. Please list the following '
    + 'file(s) in |definitions|:\n'
  )
  for missing_input in missing_inputs:
    errorMessage += f'//{missing_input}\n'

  return False, errorMessage
