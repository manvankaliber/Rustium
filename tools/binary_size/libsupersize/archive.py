# Copyright 2017 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
"""Main Python API for analyzing binary size."""

import argparse
import collections
import dataclasses
import logging
import os
import re
import shlex
import subprocess
import time
import archive_util
import data_quality
import describe
import dir_metadata
import file_format
import function_signature
import json_config_parser
import models
import native
import pakfile
import path_util
import readelf


@dataclasses.dataclass
class NativeSpec:
  # Path to linker map file (if present).
  map_path: str = None
  # Path to unstripped ELF file (if present).
  elf_path: str = None
  # Path to unstripped ELF file before being split into partitions (if present).
  combined_elf_path: str = None
  # Whether to create symbols for each string literal.
  track_string_literals: bool = True
  # component to use for all symbols.
  component: str = None
  # Regular expression that will match generated files.
  gen_dir_regex: str = None
  # source_path prefix to use for all symbols.
  source_path_prefix: str = None

  @property
  def algorithm(self):
    if self.map_path:
      return 'linker_map'
    if self.elf_path:
      return 'dwarf'
    return 'sections'


@dataclasses.dataclass
class PakSpec:
  # Paths to resource packs.
  pak_paths: list = None
  pak_info_path: str = None


@dataclasses.dataclass
class ContainerSpec:
  container_name: str
  pak_spec: PakSpec
  native_spec: NativeSpec
  source_directory: str
  output_directory: str


# section_name -> full_name -> (full_name, template_name, name, added_flags).
# Many symbols share names, both within and across .size files.
_NORMALIZED_NAMES_CACHE = collections.defaultdict(dict)


def _NormalizeNames(raw_symbols):
  """Ensures that all names are formatted in a useful way.

  This includes:
    - Deriving |name| and |template_name| from |full_name|.
    - Stripping of return types (for functions).
    - Moving "vtable for" and the like to be suffixes rather than prefixes.
  """
  found_prefixes = set()
  for symbol in raw_symbols:
    full_name = symbol.full_name
    section_name = symbol.section_name
    cache = _NORMALIZED_NAMES_CACHE[section_name]
    cached = cache.get(full_name)
    if cached:
      symbol.full_name, symbol.template_name, symbol.name, added_flags = cached
      symbol.flags |= added_flags
      # Skipping |found_prefixes| is fine since it is used only for debug
      # logging (the prefix was logged when first seen, possibly by a prior
      # call).
      continue
    cache_key = full_name
    # Clear flags so that the ones added below can be cached.
    orig_flags = symbol.flags
    symbol.flags = 0

    # See comment in _CalculatePadding() about when this can happen. Don't
    # process names for non-native sections.
    if section_name in models.PAK_SECTIONS:
      # full_name: "about_ui_resources.grdp: IDR_ABOUT_UI_CREDITS_HTML".
      space_idx = full_name.rindex(' ')
      name = full_name[space_idx + 1:]
      symbol.template_name = name
      symbol.name = name
    elif (full_name.startswith('**') or full_name.startswith('Overhead:')
          or section_name == models.SECTION_OTHER):
      symbol.template_name = full_name
      symbol.name = full_name
    elif full_name == models.STRING_LITERAL_NAME or full_name.startswith('"'):
      # Handles native and DEX strings.
      symbol.full_name = full_name
      symbol.template_name = full_name
      symbol.name = full_name
    elif section_name in models.DEX_SECTIONS:
      symbol.full_name, symbol.template_name, symbol.name = (
          function_signature.ParseJava(full_name))
    elif section_name in models.NATIVE_SECTIONS:
      # Remove [clone] suffix, and set flag accordingly.
      # Search from left-to-right, as multiple [clone]s can exist.
      # Example name suffixes:
      #     [clone .part.322]  # GCC
      #     [clone .isra.322]  # GCC
      #     [clone .constprop.1064]  # GCC
      #     [clone .11064]  # clang
      # http://unix.stackexchange.com/questions/223013/function-symbol-gets-part-suffix-after-compilation
      idx = full_name.find(' [clone ')
      if idx != -1:
        full_name = full_name[:idx]
        symbol.flags |= models.FLAG_CLONE

      # Clones for C symbols.
      if section_name == models.SECTION_TEXT:
        idx = full_name.rfind('.')
        if idx != -1 and full_name[idx + 1:].isdigit():
          new_name = full_name[:idx]
          # Generated symbols that end with .123 but are not clones.
          # Find these via:
          # size_info.symbols.WhereInSection('t').WhereIsGroup().SortedByCount()
          if new_name not in ('__tcf_0', 'startup'):
            full_name = new_name
            symbol.flags |= models.FLAG_CLONE
            # Remove .part / .isra / .constprop.
            idx = full_name.rfind('.', 0, idx)
            if idx != -1:
              full_name = full_name[:idx]

      # E.g.: vtable for FOO
      idx = full_name.find(' for ', 0, 30)
      if idx != -1:
        found_prefixes.add(full_name[:idx + 4])
        full_name = '{} [{}]'.format(full_name[idx + 5:], full_name[:idx])

      # E.g.: virtual thunk to FOO
      idx = full_name.find(' to ', 0, 30)
      if idx != -1:
        found_prefixes.add(full_name[:idx + 3])
        full_name = '{} [{}]'.format(full_name[idx + 4:], full_name[:idx])

      # Strip out return type, and split out name, template_name.
      # Function parsing also applies to non-text symbols.
      # E.g. Function statics.
      symbol.full_name, symbol.template_name, symbol.name = (
          function_signature.Parse(full_name))

      # Remove anonymous namespaces (they just harm clustering).
      if '(anonymous namespace)::' in full_name:
        symbol.template_name = symbol.template_name.replace(
            '(anonymous namespace)::', '')
        symbol.full_name = symbol.full_name.replace('(anonymous namespace)::',
                                                    '')
        non_anonymous_name = symbol.name.replace('(anonymous namespace)::', '')
        if symbol.name != non_anonymous_name:
          symbol.flags |= models.FLAG_ANONYMOUS
          symbol.name = non_anonymous_name

    # Allow using "is" to compare names (and should help with RAM). This applies
    # to all symbols.
    function_signature.InternSameNames(symbol)
    cache[cache_key] = (
        symbol.full_name,
        symbol.template_name,
        symbol.name,
        symbol.flags,
    )
    symbol.flags |= orig_flags

  logging.debug('Found name prefixes of: %r', found_prefixes)


def LoadAndPostProcessSizeInfo(path, file_obj=None):
  """Returns a SizeInfo for the given |path|."""
  logging.debug('Loading results from: %s', path)
  size_info = file_format.LoadSizeInfo(path, file_obj=file_obj)
  logging.info('Normalizing symbol names')
  _NormalizeNames(size_info.raw_symbols)
  logging.info('Loaded %d symbols', len(size_info.raw_symbols))
  return size_info


def LoadAndPostProcessDeltaSizeInfo(path, file_obj=None):
  """Returns a tuple of SizeInfos for the given |path|."""
  logging.debug('Loading results from: %s', path)
  before_size_info, after_size_info, _, _ = file_format.LoadDeltaSizeInfo(
      path, file_obj=file_obj)
  logging.info('Normalizing symbol names')
  _NormalizeNames(before_size_info.raw_symbols)
  _NormalizeNames(after_size_info.raw_symbols)
  logging.info(
      'Loaded %d + %d symbols',
      len(before_size_info.raw_symbols),
      len(after_size_info.raw_symbols),
  )
  return before_size_info, after_size_info


def CreateBuildConfig(output_directory, source_directory, url=None, title=None):
  """Creates the dict to use for SizeInfo.build_info."""
  logging.debug('Constructing build_config')
  build_config = {}
  if output_directory:
    gn_args = _ParseGnArgs(os.path.join(output_directory, 'args.gn'))
    build_config[models.BUILD_CONFIG_GN_ARGS] = gn_args
    build_config[models.BUILD_CONFIG_OUT_DIRECTORY] = os.path.relpath(
        output_directory, start=source_directory)
  git_rev = _DetectGitRevision(source_directory)
  if git_rev:
    build_config[models.BUILD_CONFIG_GIT_REVISION] = git_rev
  if url is not None:
    build_config[models.BUILD_CONFIG_URL] = url
  if title is not None:
    build_config[models.BUILD_CONFIG_TITLE] = title

  return build_config


def _CreateMetadata(container_spec, elf_info):
  logging.debug('Constructing metadata')
  metadata = {}
  native_spec = container_spec.native_spec
  output_directory = container_spec.output_directory

  # Ensure all paths are relative to output directory to make them hermetic.
  if output_directory:
    shorten_path = lambda path: os.path.relpath(path, output_directory)
  else:
    # If output directory is unavailable, just store basenames.
    shorten_path = os.path.basename

  if native_spec:
    native_metadata = native.CreateMetadata(native_spec=native_spec,
                                            elf_info=elf_info,
                                            shorten_path=shorten_path)
    assert not (metadata.keys() & native_metadata.keys())
    metadata.update(native_metadata)

  logging.debug('Constructing metadata (done)')
  return metadata


def _CreatePakSymbols(*, pak_spec, pak_id_map, output_directory):
  logging.debug('Creating Pak symbols')
  section_ranges = {}
  raw_symbols = pakfile.CreatePakSymbolsFromFiles(section_ranges,
                                                  pak_spec.pak_paths,
                                                  pak_spec.pak_info_path,
                                                  output_directory, pak_id_map)
  return section_ranges, raw_symbols


def _CreateContainerSymbols(
    container_spec,
    ninja_source_mapper,
    pak_id_map,
    component_overrides,
):
  container_name = container_spec.container_name
  pak_spec = container_spec.pak_spec
  native_spec = container_spec.native_spec
  output_directory = container_spec.output_directory
  source_directory = container_spec.source_directory

  logging.info('Starting on container: %s', container_spec)

  raw_symbols = []
  section_sizes = {}
  metrics_by_file = {}
  default_component = ''

  def add_syms(
      section_ranges,
      new_raw_symbols,
      source_path_prefix=None,
      component=None,
      paths_already_normalized=False,
  ):
    new_section_sizes = {
        k: size
        for k, (address, size) in section_ranges.items()
    }
    if models.SECTION_OTHER in new_section_sizes:
      section_sizes[models.SECTION_OTHER] = (
          section_sizes.get(models.SECTION_OTHER, 0) +
          new_section_sizes[models.SECTION_OTHER])
      del new_section_sizes[models.SECTION_OTHER]

    assert not (set(section_sizes) & set(new_section_sizes)), (
        'Section collision: {}\n\n {}'.format(section_sizes, new_section_sizes))
    section_sizes.update(new_section_sizes)

    # E.g.: native.CreateSymbols() already calls NormalizePaths().
    if not paths_already_normalized:
      archive_util.NormalizePaths(new_raw_symbols)

    if source_path_prefix:
      # Prefix the source_path for all symbols that have a source_path assigned,
      # and that don't have it set to $APK or $GOOGLE3.
      for s in new_raw_symbols:
        if s.source_path and s.source_path[0] != '$':
          s.source_path = source_path_prefix + s.source_path

    if component is not None:
      for s in new_raw_symbols:
        s.component = component
    else:
      dir_metadata.PopulateComponents(
          new_raw_symbols,
          source_directory,
          component_overrides,
          default_component=default_component,
      )
    raw_symbols.extend(new_raw_symbols)

  elf_info = None
  if native_spec:
    section_ranges, native_symbols, elf_info, native_metrics_by_file = (
        native.CreateSymbols(
            native_spec=native_spec,
            output_directory=output_directory,
            ninja_source_mapper=ninja_source_mapper,
            pak_id_map=pak_id_map,
        ))
    add_syms(
        section_ranges,
        native_symbols,
        source_path_prefix=native_spec.source_path_prefix,
        component=native_spec.component,
        paths_already_normalized=True,
    )
    metrics_by_file.update(native_metrics_by_file)

  if pak_spec:
    section_ranges, pak_symbols = _CreatePakSymbols(
        pak_spec=pak_spec,
        pak_id_map=pak_id_map,
        output_directory=output_directory,
    )
    add_syms(section_ranges, pak_symbols)

  metadata = _CreateMetadata(container_spec, elf_info)
  container = models.Container(
      name=container_name,
      metadata=metadata,
      section_sizes=section_sizes,
      metrics_by_file=metrics_by_file,
  )
  for symbol in raw_symbols:
    symbol.container = container

  return raw_symbols


def _DetectGitRevision(directory):
  """Runs git rev-parse to get the SHA1 hash of the current revision.

  Args:
    directory: Path to directory where rev-parse command will be run.

  Returns:
    A string with the SHA1 hash, or None if an error occured.
  """
  try:
    git_rev = subprocess.check_output(
        ['git', '-C', directory, 'rev-parse', 'HEAD']).decode('ascii')
    return git_rev.rstrip()
  except Exception:
    logging.warning('Failed to detect git revision for file metadata.')
    return None


def _ParseGnArgs(args_path):
  """Returns a list of normalized "key=value" strings."""
  args = {}
  with open(args_path) as f:
    for l in f:
      # Strips #s even if within string literal. Not a problem in practice.
      parts = l.split('#')[0].split('=')
      if len(parts) != 2:
        continue
      args[parts[0].strip()] = parts[1].strip()
  return ["%s=%s" % x for x in sorted(args.items())]


def _AddContainerArguments(parser, is_top_args=False):
  """Add arguments applicable to a single container."""

  # Main file argument: Exactly one should be specified (perhaps via -f).
  # _IdentifyInputFile() should be kept updated.
  group = parser.add_argument_group(title='Main Input')
  group = group.add_mutually_exclusive_group(required=True)
  group.add_argument('-f',
                     metavar='FILE',
                     help='Auto-identify input file type.')
  group.add_argument('--elf-file', help='Path to input ELF file.')
  group.add_argument(
      '--map-file',
      help='Path to input .map(.gz) file. Defaults to '
      '{{elf_file}}.map(.gz)?. If given without '
      '--elf-file, no size metadata will be recorded.',
  )
  group.add_argument(
      '--pak-file',
      action='append',
      default=[],
      dest='pak_files',
      help='Paths to pak files.',
  )
  if is_top_args:
    group.add_argument('--ssargs-file',
                       help='Path to SuperSize multi-container arguments file.')

  group = parser.add_argument_group(title='What to Analyze')
  group.add_argument('--native-only',
                     action='store_true',
                     help='Run on only native symbols')
  group.add_argument('--no-native',
                     action='store_true',
                     help='Do not run on native symbols')
  if is_top_args:
    group.add_argument(
        '--container-filter',
        help='Regular expression for which containers to create',
    )

  group = parser.add_argument_group(title='Analysis Options for Native Code')
  group.add_argument('--aux-elf-file', help=argparse.SUPPRESS)
  group.add_argument(
      '--no-map-file',
      dest='ignore_linker_map',
      action='store_true',
      help='Use debug information to capture symbol sizes '
      'instead of linker map file.',
  )
  group.add_argument(
      '--aux-map-file',
      help='Path to linker map to use when --elf-file is provided',
  )

  group = parser.add_argument_group(title='Analysis Options for Pak Files')
  group.add_argument(
      '--pak-info-file',
      help='This file should contain all ids found in the pak '
      'files that have been passed in. If not specified, '
      '${pak_file}.info is assumed.',
  )

  group = parser.add_argument_group(title='Analysis Options (shared)')
  group.add_argument('--source-directory',
                     help='Custom path to the root source directory.')
  group.add_argument('--output-directory',
                     help='Path to the root build directory.')
  group.add_argument(
      '--no-string-literals',
      action='store_true',
      help=
      ('Do not create symbols for string literals (applies to DEX and Native).'
       ),
  )
  if is_top_args:
    group.add_argument('--json-config', help='Path to a supersize.json.')
    group.add_argument(
        '--no-output-directory',
        action='store_true',
        help='Do not auto-detect --output-directory.',
    )
    group.add_argument(
        '--check-data-quality',
        action='store_true',
        help='Perform sanity checks to ensure there is no missing data.',
    )


def AddArguments(parser):
  parser.add_argument('size_file', help='Path to output .size file.')
  parser.add_argument('--title',
                      help='Value for the "title" build_config entry.')
  parser.add_argument('--url', help='Value for the "url" build_config entry.')
  _AddContainerArguments(parser, is_top_args=True)


def _IdentifyInputFile(args, on_config_error):
  """Identify a standalone ELF, linker map, pak or multi-container input."""
  if args.f:
    if args.f.endswith('.so') or '.' not in os.path.basename(args.f):
      args.elf_file = args.f
    elif args.f.endswith(('.map', '.map.gz')):
      args.map_file = args.f
    elif args.f.endswith('.pak'):
      args.pak_files.append(args.f)
    elif args.f.endswith('.ssargs'):
      args.ssargs_file = args.f
    else:
      on_config_error('Cannot identify file ' + args.f)
    args.f = None
  ret = [args.elf_file,
         args.__dict__.get('ssargs_file'), args.map_file] + (args.pak_files
                                                             or [])
  ret = [v for v in ret if v]
  if not ret:
    on_config_error(
        'Must pass at least one of --elf-file, --map-file, --pak-file, --ssargs-file'
    )
  return ret[0]


def ParseSsargs(lines):
  """Parses .ssargs data.

  An .ssargs file is a text file to specify multiple containers as input to
  SuperSize-archive. After '#'-based comments, start / end whitespaces, and
  empty lines are stripped, each line specifies a distinct container. Format:
  * Positional argument: |name| for the container.
  * Main input file specified by -f, --elf-file, etc.:
    * Can be an absolute path.
    * Can be a relative path. In this case, it's up to the caller to supply the
      base directory.
    * -f switch must not specify another .ssargs file.
  * For supported switches: See _AddContainerArguments().

  Args:
    lines: An iterator containing lines of .ssargs data.
  Returns:
    A list of arguments, one for each container.
  Raises:
    ValueError: Parse error, including input line number.
  """
  sub_args_list = []
  parser = argparse.ArgumentParser(add_help=False)
  parser.error = lambda msg: (_ for _ in ()).throw(ValueError(msg))
  parser.add_argument('name')
  _AddContainerArguments(parser)
  try:
    for lineno, line in enumerate(lines, 1):
      toks = shlex.split(line, comments=True)
      if not toks:  # Skip if line is empty after stripping comments.
        continue
      sub_args_list.append(parser.parse_args(toks))
  except ValueError as e:
    e.args = ('Line %d: %s' % (lineno, e.args[0]), )
    raise e
  return sub_args_list


def _MakeNativeSpec(json_config, **kwargs):
  native_spec = NativeSpec(**kwargs)
  if native_spec.elf_path or native_spec.map_path:
    basename = os.path.basename(native_spec.elf_path or native_spec.map_path)
    native_spec.component = json_config.ComponentForNativeFile(basename)
    native_spec.gen_dir_regex = json_config.GenDirRegexForNativeFile(basename)
    native_spec.source_path_prefix = json_config.SourcePathPrefixForNativeFile(
        basename)

  if not native_spec.map_path:
    # TODO(crbug.com/40757867): Implement string literal tracking without map
    #     files. nm emits some string literal symbols, but most are missing.
    native_spec.track_string_literals = False
    return native_spec

  return native_spec


def _DeduceMapPath(elf_path):
  to_try = [
      elf_path + '.map',
      elf_path + '.map.gz',
  ]
  for path in to_try:
    if os.path.exists(path):
      logging.debug('Detected map_path=%s', path)
      return path
  return None


def _CreateNativeSpecs(*, elf_path, map_path, track_string_literals,
                       ignore_linker_map, json_config, on_config_error):
  if ignore_linker_map:
    map_path = None
  elif map_path and not map_path.endswith(('.map', '.map.gz')):
    on_config_error('Expected --map-file to end with .map or .map.gz')
  elif elf_path and not map_path:
    map_path = _DeduceMapPath(elf_path)
  if not map_path and not elf_path:
    return []
  return [
      _MakeNativeSpec(json_config,
                      map_path=map_path,
                      elf_path=elf_path,
                      combined_elf_path=None,
                      track_string_literals=track_string_literals)
  ]


def _ReadMultipleArgsFromStream(lines, base_dir, err_prefix, on_config_error):
  try:
    ret = ParseSsargs(lines)
  except ValueError as e:
    on_config_error('%s: %s' % (err_prefix, e.args[0]))
  for sub_args in ret:
    for k, v in sub_args.__dict__.items():
      # Translate file arguments to be relative to |sub_dir|.
      if (k.endswith('_file') or k == 'f') and isinstance(v, str):
        sub_args.__dict__[k] = os.path.join(base_dir, v)
  return ret


def _ReadMultipleArgsFromFile(ssargs_file, on_config_error):
  with open(ssargs_file, 'r') as fh:
    lines = list(fh)
  err_prefix = 'In file ' + ssargs_file
  # Supply |base_dir| as the directory containing the .ssargs file, to ensure
  # consistent behavior wherever SuperSize-archive runs.
  base_dir = os.path.dirname(os.path.abspath(ssargs_file))
  return _ReadMultipleArgsFromStream(lines, base_dir, err_prefix,
                                     on_config_error)


def _CreateContainerSpecs(top_args, sub_args, json_config, base_container_name,
                          on_config_error):
  sub_args.source_directory = sub_args.source_directory or top_args.source_directory
  sub_args.output_directory = sub_args.output_directory or top_args.output_directory
  analyze_native = not (sub_args.no_native or top_args.no_native)
  analyze_pak = not (sub_args.native_only or top_args.native_only) and bool(
      sub_args.output_directory)
  pak_spec = None
  if analyze_pak and sub_args.pak_files:
    pak_info_path = sub_args.pak_info_file
    if pak_info_path and not os.path.exists(pak_info_path):
      on_config_error(f'File not found: {pak_info_path}. '
                      'Ensure is_official_build=true, or use --native-only')
    pak_spec = PakSpec(pak_paths=sub_args.pak_files,
                       pak_info_path=pak_info_path)
  native_specs = []
  if analyze_native:
    native_specs = _CreateNativeSpecs(
        elf_path=sub_args.elf_file or sub_args.aux_elf_file,
        map_path=sub_args.map_file or sub_args.aux_map_file,
        track_string_literals=not (top_args.no_string_literals
                                   or sub_args.no_string_literals),
        ignore_linker_map=top_args.ignore_linker_map
        or sub_args.ignore_linker_map,
        json_config=json_config,
        on_config_error=on_config_error)
  assert len(native_specs) <= 1
  return [
      ContainerSpec(container_name=base_container_name,
                    pak_spec=pak_spec,
                    native_spec=native_specs[0] if native_specs else None,
                    source_directory=sub_args.source_directory,
                    output_directory=sub_args.output_directory)
  ]


def _CreateAllContainerSpecs(top_args, json_config, on_config_error):
  main_file = _IdentifyInputFile(top_args, on_config_error)
  if top_args.no_output_directory:
    top_args.output_directory = None
  else:
    output_directory_finder = path_util.OutputDirectoryFinder(
        value=top_args.output_directory,
        any_path_within_output_directory=main_file,
    )
    top_args.output_directory = output_directory_finder.Finalized()

  if not top_args.source_directory:
    top_args.source_directory = path_util.GetSrcRootFromOutputDirectory(
        top_args.output_directory)
    assert top_args.source_directory

  if top_args.ssargs_file:
    sub_args_list = _ReadMultipleArgsFromFile(top_args.ssargs_file,
                                              on_config_error)
  else:
    sub_args_list = [top_args]

  # Do a quick first pass to ensure inputs have been built.
  for sub_args in sub_args_list:
    main_file = _IdentifyInputFile(sub_args, on_config_error)
    if not os.path.exists(main_file):
      raise Exception('Input does not exist: ' + main_file)

  # Each element in |sub_args_list| specifies a container.
  ret = []
  for sub_args in sub_args_list:
    main_file = _IdentifyInputFile(sub_args, on_config_error)
    if hasattr(sub_args, 'name'):
      container_name = sub_args.name
    else:
      container_name = os.path.basename(main_file)
    if set(container_name) & set('<>?'):
      on_config_error('Container name cannot have characters in "<>?"')
    ret += _CreateContainerSpecs(
        top_args,
        sub_args,
        json_config,
        container_name,
        on_config_error,
    )
  all_names = [c.container_name for c in ret]
  assert len(
      set(all_names)) == len(all_names), ('Found duplicate container names: ' +
                                          '\n'.join(sorted(all_names)))

  return ret


def _FilterContainerSpecs(container_specs, container_re=None):
  ret = []
  seen_container_names = set()
  for container_spec in container_specs:
    container_name = container_spec.container_name
    if container_name in seen_container_names:
      raise ValueError('Duplicate container name: {}'.format(container_name))
    seen_container_names.add(container_name)

    if container_re and not container_re.search(container_name):
      logging.info('Skipping filtered container %s', container_name)
      continue
    ret.append(container_spec)
  return ret


def CreateSizeInfo(
    container_specs,
    build_config,
    json_config,
):

  def sort_key(container_spec):
    # Native containers come first to ensure pak_id_map is populated before
    # any pak_spec is encountered.
    if container_spec.native_spec:
      # Do the most complicated container first, since its most likely to fail.
      if container_spec.native_spec.algorithm == 'linker_map':
        native_key = 0
      elif container_spec.native_spec.algorithm == 'dwarf':
        native_key = 1
      else:
        native_key = 2
    else:
      native_key = 3
    return (native_key, container_spec.container_name)

  container_specs.sort(key=sort_key)

  ninja_containers = [
      c for c in container_specs
      if c.native_spec and c.output_directory and c.native_spec.map_path
  ]
  ninja_source_mapper = None
  if ninja_containers:
    elf_paths_to_find_inputs_for = [
        c.native_spec.combined_elf_path or c.native_spec.elf_path
        for c in ninja_containers
        if c.native_spec.combined_elf_path or c.native_spec.elf_path
    ]
    ninja_source_mapper = native.ParseNinjaFiles(
        ninja_containers[0].output_directory, elf_paths_to_find_inputs_for)

  raw_symbols_list = []
  pak_id_map = pakfile.PakIdMap()
  for container_spec in container_specs:
    raw_symbols = _CreateContainerSymbols(
        container_spec,
        ninja_source_mapper,
        pak_id_map,
        json_config.ComponentOverrides(),
    )
    assert raw_symbols, f'{container_spec.container_name} had no symbols.'
    raw_symbols_list.append(raw_symbols)

  # Normalize names before sorting.
  logging.info('Normalizing symbol names')
  for raw_symbols in raw_symbols_list:
    _NormalizeNames(raw_symbols)

  # Sorting must happen after normalization.
  logging.info('Sorting symbols')
  for raw_symbols in raw_symbols_list:
    file_format.SortSymbols(raw_symbols)

  logging.debug('Accumulating symbols')
  # Containers should always have at least one symbol.
  container_list = [syms[0].container for syms in raw_symbols_list]
  all_raw_symbols = []
  for raw_symbols in raw_symbols_list:
    all_raw_symbols += raw_symbols

  file_format.CalculatePadding(all_raw_symbols)

  return models.SizeInfo(build_config, container_list, all_raw_symbols)


def Run(top_args, on_config_error):
  path_util.CheckLlvmToolsAvailable()

  if not top_args.size_file.endswith('.size'):
    on_config_error('size_file must end with .size')
  if top_args.check_data_quality:
    start_time = time.time()

  container_re = None
  if top_args.container_filter:
    try:
      container_re = re.compile(top_args.container_filter)
    except Exception as e:
      on_config_error(f'Bad --container-filter input: {e}')

  json_config_path = top_args.json_config
  if not json_config_path:
    json_config_path = path_util.GetDefaultJsonConfigPath()
    logging.info('Using --json-config=%s', json_config_path)
  json_config = json_config_parser.Parse(json_config_path, on_config_error)
  container_specs = _CreateAllContainerSpecs(top_args, json_config,
                                             on_config_error)
  container_specs = _FilterContainerSpecs(container_specs, container_re)

  build_config = CreateBuildConfig(
      top_args.output_directory,
      top_args.source_directory,
      url=top_args.url,
      title=top_args.title,
  )
  size_info = CreateSizeInfo(
      container_specs,
      build_config,
      json_config,
  )

  if logging.getLogger().isEnabledFor(logging.DEBUG):
    for line in data_quality.DescribeSizeInfoCoverage(size_info):
      logging.debug(line)
  logging.info('Recorded info for %d symbols', len(size_info.raw_symbols))
  for container in size_info.containers:
    logging.info(
        'Recording metadata: \n  %s',
        '\n  '.join(describe.DescribeDict(container.metadata)),
    )

  logging.info('Saving result to %s', top_args.size_file)
  file_format.SaveSizeInfo(size_info, top_args.size_file)
  size_in_mb = os.path.getsize(top_args.size_file) / 1024.0 / 1024.0
  logging.info('Done. File size is %.2fMiB.', size_in_mb)

  if top_args.check_data_quality:
    logging.info('Checking data quality')
    data_quality.CheckDataQuality(size_info, not top_args.no_string_literals)
    duration = (time.time() - start_time) / 60
    if duration > 10:
      raise data_quality.QualityCheckError(
          'Command should not take longer than 10 minutes.'
          ' Took {:.1f} minutes.'.format(duration))
