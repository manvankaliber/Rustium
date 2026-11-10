#!/usr/bin/env vpython3
# Copyright 2023 The Chromium Authors
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
'''Runs benchmarks as described in docs/pgo.md, and similar to the PGO bots.

You need to build chrome with chrome_pgo_phase=1 (and the args.gn described
in docs/pgo.md for stage 1), and then run this like

    tools/pgo/generate_profile.py -C out/builddir

After that, the final profile will be in out/builddir/profile.profdata.
With that, you can do a second build with:

    is_official_build
    pgo_data_path = "//out/builddir/profile.profdata"

and with chrome_pgo_phase _not_ set. (It defaults to =2 in official builds.)

**IMPORTANT**: If you add any new deps for this script, make sure that it is
    also added to the template in //tools/pgo/BUILD.gn as data or data_deps, as
    this script is run as an isolated script on the bots, which means that only
    listed data and data_deps are available to it when run on the bot.
'''

import argparse
import glob
import json
import logging
import os
import pathlib
import shutil
import subprocess
import sys
import time
import psutil
import tempfile
from dataclasses import dataclass, field
from typing import List, Optional

_EXE_EXT = '.exe' if sys.platform == 'win32' else ''
_THIS_DIR = os.path.dirname(__file__)
_ROOT_DIR = f'{_THIS_DIR}/../..'
_UPDATE_PY = f'{_THIS_DIR}/../clang/scripts/update.py'
_LLVM_DIR = f'{_ROOT_DIR}/third_party/llvm-build/Release+Asserts'
_PROFDATA = f'{_LLVM_DIR}/bin/llvm-profdata{_EXE_EXT}'

# Enabled by testing/variations/fieldtrial_testing_config.json, so every
# benchmark run has to turn them off explicitly. Stops the hang watcher from
# generating dumps, see https://crbug.com/425223287.
_HANG_WATCHER_FEATURES = (
    'EnableHangWatcher',
    'EnableHangWatcherOnGpuProcess',
)

# Benchmark files to serve from a local HTTP file server, since devices in
# isolated lab environments have no access to the live benchmark URLs. The
# version has to match the one crossbench resolves for the benchmark name, see
# the `aliases()` of the classes in
# third_party/crossbench/crossbench/benchmarks/{speedometer,jetstream}/. Names
# absent from this map keep crossbench's default (live) URL.
_CROSSBENCH_LOCAL_BENCHMARK_DIRS = {
    'speedometer3': 'third_party/speedometer/v3.1',
    'jetstream3': 'third_party/jetstream/v3.0',
}

# This is necessary to get proper logging on bots and locally. If this script is
# run through run_isolated_script_test.py, a root logger would have already been
# set up. Thus for this script's logging to appear (and not disrupt other
# loggers) it needs to use its own logger.
_LOGGER = logging.getLogger(__name__)


@dataclass
class Benchmark:
    '''Describes a benchmark and the set of arguments needed to run it.'''

    name: str
    args: List[str]
    enable_features: List[str] = field(default_factory=list)
    disable_features: List[str] = field(default_factory=list)
    pageset_repeat: int = 1
    weight: int = 1

    def is_crossbench(self) -> bool:
        return self.name.endswith('.crossbench')

    def ProduceBrowserArgs(self,
                           extra_disabled_features: Optional[List[str]] = None
                           ):
        '''Returns the browser flags this benchmark needs.'''
        if extra_disabled_features is None:
            extra_disabled_features = []

        all_disabled_features = self.disable_features + extra_disabled_features

        intersect_features = set(
            self.enable_features).intersection(all_disabled_features)
        if intersect_features:
            raise RuntimeError(
                f'Features {intersect_features} were both enabled and disabled.'
            )

        browser_args = []
        if self.enable_features:
            browser_args.append('--enable-features=' +
                                ','.join(self.enable_features))
        if all_disabled_features:
            browser_args.append('--disable-features=' +
                                ','.join(all_disabled_features))
        return browser_args

    def ProduceBenchmarkArgs(
        self,
        extra_disabled_features: Optional[List[str]] = None,
        browser_args: Optional[List[str]] = None,
    ):
        if any(a.startswith('--extra-browser-args') for a in self.args):
            raise RuntimeError(
                '--extra-browser-args was added to benchmark args.')

        if browser_args is None:
            browser_args = self.ProduceBrowserArgs(extra_disabled_features)

        final_args = self.args.copy()
        if browser_args:
            # No quotes around the space separated arguments is needed.
            final_args.append(f'--extra-browser-args={" ".join(browser_args)}')

        return final_args


# This error is raised when LLVM failed to merge successfully.
class MergeError(RuntimeError):
    pass


# Use this custom Namespace to provide type checking and type hinting.
class OptionsNamespace(argparse.Namespace):
    builddir: str
    # Technically profiledir and outputdir default to `None`, but they are
    # always set before parse_args returns, so leave it as `str` to avoid type
    # errors for methods that take an OptionsNamespace instance.
    outputdir: str
    profiledir: str
    keep_temps: bool
    target_arch: Optional[str]
    skip_profdata: bool
    run_public_benchmarks_only: bool
    temporal_trace_length: Optional[int]
    separate_renderer_pgo: bool
    repeats: int
    verbose: int
    quiet: int
    # The following are bot-specific args.
    isolated_script_test_output: Optional[str]
    isolated_script_test_perf_output: Optional[str]


def parse_args():
    parser = argparse.ArgumentParser(
        epilog=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    # ▼▼▼▼▼ Please update OptionsNamespace when adding or modifying args. ▼▼▼▼▼
    parser.add_argument('-C',
                        '--builddir',
                        help='Path to build directory.',
                        required=True)
    parser.add_argument(
        '--outputdir',
        help='Path to store final outputs, default is builddir.')
    parser.add_argument(
        '--profiledir',
        help='Path to store temporary profiles, default is builddir/profile.',
    )
    parser.add_argument(
        '--keep-temps',
        action='store_true',
        default=False,
        help='Whether to keep temp files',
    )
    parser.add_argument(
        '--target-arch',
        choices=['arm', 'arm64', 'x86', 'x64'],
        default='x64',
        help='The target architecture the browser was built for. Only used to '
        'pick the benchmarks to run, see main().',
    )
    parser.add_argument(
        '--skip-profdata',
        action='store_true',
        default=False,
        help='Only run benchmarks and skip merging profile '
        'data. Used for sample-based profiling for Propeller '
        'and BOLT',
    )
    parser.add_argument(
        '--dry-run',
        action='store_true',
        default=False,
        help='Skip running the benchmarks.',
    )
    parser.add_argument(
        '--run-public-benchmarks-only',
        action='store_true',
        default=False,
        help='Only run benchmarks that do not require any special access. See '
        'https://www.chromium.org/developers/telemetry/upload_to_cloud_storage/#request-access-for-google-partners '
        'for more information.',
    )
    parser.add_argument(
        '--temporal-trace-length',
        type=int,
        help='Add flags necessary for temporal PGO (experimental).',
    )
    parser.add_argument(
        '--separate-renderer-pgo',
        action='store_true',
        default=False,
        help='Generate a separate PGO profile for the renderer binary.',
    )
    parser.add_argument(
        '-r',
        '--repeats',
        type=int,
        default=5,
        help='Number of times to attempt each benchmark if it fails, default 5',
    )
    parser.add_argument(
        '-v',
        '--verbose',
        action='count',
        default=0,
        help='Increase verbosity level (repeat as needed)',
    )
    parser.add_argument(
        '-q',
        '--quiet',
        action='count',
        default=0,
        help='Decrease verbosity level (passed through to run_benchmark.)',
    )
    parser.add_argument(
        '--isolated-script-test-output',
        help='Output.json file that the script can write to.',
    )
    parser.add_argument(
        '--isolated-script-test-perf-output',
        help='Deprecated and ignored, but bots pass it.',
    )
    # ▲▲▲▲▲ Please update OptionsNamespace when adding or modifying args. ▲▲▲▲▲

    args = parser.parse_args(namespace=OptionsNamespace())

    # This is load-bearing:
    # - `open` (used by run_benchmark) needs absolute paths
    # - `open` sets the cwd to `/`, so LLVM_PROFILE_FILE must
    #   be an absolute path
    # Both are based off this variable.
    # See also https://crbug.com/1478279
    args.builddir = os.path.realpath(args.builddir)
    _LOGGER.info(f"Build directory: {args.builddir}")

    if not args.profiledir:
        args.profiledir = f'{args.builddir}/profile'

    if not args.outputdir:
        args.outputdir = args.builddir

    if args.isolated_script_test_output:
        args.outputdir = os.path.dirname(args.isolated_script_test_output)

    _LOGGER.info(f"Output directory: {args.outputdir}")
    _LOGGER.info(f"Profile directory: {args.profiledir}")

    return args


def run_profdata_merge(output_path, input_files, args: OptionsNamespace):
    _LOGGER.info(
        f"Merging {len(input_files)} profile files into {output_path}")
    if args.temporal_trace_length:
        extra_args = [
            '--temporal-profile-max-trace-length',
            str(args.temporal_trace_length),
        ]
        _LOGGER.debug(
            f"Using temporal trace length: {args.temporal_trace_length}")

    else:
        extra_args = []

    cmd = [_PROFDATA, 'merge', '-o', output_path] + extra_args + input_files
    _LOGGER.debug(f"Running command: {' '.join(cmd)}")

    proc = subprocess.run(cmd, check=True, capture_output=True, text=True)
    output = str(proc.stdout) + str(proc.stderr)
    _LOGGER.debug(f"llvm-profdata output:\n{output}")
    if 'invalid profile created' in output:
        # This is necessary because for some reason this invalid data is kept
        # and only a warning is issued by llvm.
        raise MergeError('Failed to generate valid profile data.')


def run_profdata_show(file_name, topn=1000):
    _LOGGER.info(f'Calculating topn={topn} for {file_name}')
    cmd = [_PROFDATA, 'show', '-topn', str(topn), file_name]
    _LOGGER.debug(f"Running command: {' '.join(cmd)}")
    proc = subprocess.run(cmd, check=True, capture_output=True, text=True)
    _LOGGER.debug(proc.stdout)
    return proc.stdout


def get_max_internal_block_count(file_name):
    for line in run_profdata_show(file_name).splitlines():
        if line.startswith("Maximum internal block count: "):
            return int(line.split(":", 1)[1])
    return None


def get_crossbench_local_file_server_args(benchmark_name: str):
    '''Returns the --local-file-server args for a crossbench benchmark.'''
    local_dir = _CROSSBENCH_LOCAL_BENCHMARK_DIRS.get(benchmark_name)
    if not local_dir:
        raise FileNotFoundError(
            f"Local files for {benchmark_name} are missing")

    local_path = os.path.abspath(f'{_ROOT_DIR}/{local_dir}')
    if not os.path.isdir(local_path):
        raise FileNotFoundError(
            f'{local_path} is missing, cannot serve {benchmark_name} locally. '
            'It should be brought in by //tools/perf:perf, see '
            '//tools/perf/BUILD.gn.')
    return [f'--local-file-server={local_path}']


def get_crossbench_driver_path(args: OptionsNamespace):
    '''Returns the chromedriver crossbench should use, if it was built.'''
    # Cross-compilation can place the host chromedriver in a toolchain subdirectory.
    for path in (
            f'{args.builddir}/clang_x64/chromedriver{_EXE_EXT}',
            f'{args.builddir}/chromedriver{_EXE_EXT}',
    ):
        if os.path.exists(path):
            return path

    if args.dry_run:
        return f'{args.builddir}/chromedriver{_EXE_EXT}'

    raise FileNotFoundError(f"chromedriver not found for {args.builddir}")


def run_benchmark(benchmark: Benchmark, args: OptionsNamespace):
    '''Puts profdata in {profiledir}/{benchmark.name}.profdata'''

    is_crossbench = benchmark.is_crossbench()

    disabled_features = [
        # Disabling spare renderer features when profiling prevent dumping
        # profile data too early during benchmarks which would result in
        # incomplete profraw files. See https://crbug.com/366235732.
        'SpareRendererForSitePerProcess',
    ]
    disabled_features.extend(_HANG_WATCHER_FEATURES)
    if sys.platform == 'win32':
        # Prevents flakiness on Windows 10 devices, see
        # https://crbug.com/457520120 and https://crbug.com/461512702.
        disabled_features.append('SessionRestoreInfobar')

    browser_args = benchmark.ProduceBrowserArgs(disabled_features)
    if (sys.platform == 'win32' and args.target_arch == 'x86'):
        # TODO(crbug.com/555762263): Sandboxed 32-bit renderers crash at
        # startup in PGO-instrumented builds. Remove once root-caused.
        browser_args.append('--no-sandbox')
    if is_crossbench:
        benchmark_args = benchmark.args.copy()
    else:
        benchmark_args = benchmark.ProduceBenchmarkArgs(
            browser_args=browser_args)

    pageset_repeat_str = (f' with pageset_repeat={benchmark.pageset_repeat}'
                          if benchmark.pageset_repeat != 1 else '')
    _LOGGER.info(
        f"Running benchmark: {' '.join(benchmark_args)}{pageset_repeat_str}")

    # Include the story suffix since per-story benchmarks use
    # [name, --story=s].
    name = benchmark.name
    if len(benchmark_args) > 1 and benchmark_args[1].startswith('--story='):
        story_name = benchmark_args[1].split('=', 1)[1]
        name += f'_{story_name}'

    # Clean up intermediate files from previous runs.
    profraw_path = f'{args.profiledir}/{name}/raw'
    _LOGGER.debug(f"Raw profile path: {profraw_path}")

    if os.path.exists(profraw_path):
        _LOGGER.debug(
            f"Removing existing raw profile directory: {profraw_path}")
        shutil.rmtree(profraw_path)
    os.makedirs(profraw_path, exist_ok=True)

    profdata_path = f'{args.profiledir}/{name}.profdata'
    _LOGGER.debug(f"profdata path: {profdata_path}")
    if os.path.exists(profdata_path):
        _LOGGER.debug(f"Removing existing profdata file: {profdata_path}")
        os.remove(profdata_path)

    env = os.environ.copy()
    env['LLVM_PROFILE_FILE'] = f'{profraw_path}/default-%2m.profraw'
    _LOGGER.debug("Set environment variable "
                  f"LLVM_PROFILE_FILE={env['LLVM_PROFILE_FILE']}")

    if is_crossbench:
        cmd = ([
            'vpython3', 'third_party/crossbench/cb.py'
        ] + benchmark_args + [
            '-r',
            str(benchmark.pageset_repeat),
            '--no-splash',
            # Warn rather than prompting or failing on environment
            # validation checks during automated/PGO runs.
            '--env-validation=warn',
            # Crossbench otherwise fills its output directory with symlinked
            # views of the same results (per run/session/story). Those make
            # the profraw glob below return each file hundreds of times
            # over, and merging a profile with itself multiplies its
            # counters by a factor that depends on the benchmark's story
            # count, which skews how the benchmarks are weighted against
            # each other in the final profile.
            '--no-symlinks',
            # Crossbench asserts that its output directory does not exist
            # yet, so point it inside (not at) the already created
            # profraw_path.
            '--out-dir',
            f'{profraw_path}/crossbench',
        ])
        # Serve local benchmark files for isolated environments without internet access.
        cmd += get_crossbench_local_file_server_args(benchmark_args[0])
    else:
        cmd = (['vpython3', 'tools/perf/run_benchmark'] + benchmark_args + [
            f'--chromium-output-directory={args.builddir}',
            '--assert-gpu-compositing',
            f'--pageset-repeat={benchmark.pageset_repeat}',
            # Abort immediately when any story fails, since a failed story
            # fails to produce valid profdata. Fail fast and rely on repeats
            # to get a valid profdata.
            '--max-failures=0',
        ])

    # Add N copies of verbose/quiet flag
    cmd += ['-v'] * args.verbose + ['-q'] * args.quiet
    if sys.platform == 'darwin':
        candidates = [
            os.path.join(
                args.builddir,
                'Google Chrome.app/Contents/MacOS/Google Chrome',
            ),
            os.path.join(
                args.builddir,
                'Chromium.app/Contents/MacOS/Chromium',
            ),
        ]
        exe_path = next((c for c in candidates if os.path.exists(c)), None)
        if exe_path is None:
            if args.dry_run:
                exe_path = candidates[0]
            else:
                raise FileNotFoundError(
                    f'No Chrome/Chromium app found in {args.builddir}')
    else:
        exe_path = f'{args.builddir}/chrome' + _EXE_EXT
    if is_crossbench:
        driver_path = get_crossbench_driver_path(args)
        cmd += ['-b', exe_path, '--driver-path', driver_path]
        cmd += ['--enable-field-trial-config=benchmarking']
        cmd += ['--'] + browser_args
    else:
        cmd += [
            '--browser=exact',
            f'--browser-executable={exe_path}',
        ]

    _LOGGER.debug(f"Running benchmark locally with command: {' '.join(cmd)}")

    if not args.dry_run:
        subprocess.run(
            cmd,
            check=True,
            shell=sys.platform == 'win32',
            env=env,
            cwd=_ROOT_DIR,
        )

    # When using separate renderer binaries, child processes (e.g., renderers
    # or helpers) need time to exit cleanly so that LLVM profile handlers
    # write out all .profraw files before classification and merging begin.
    if not args.dry_run:
        builddir_abs = os.path.join(os.path.abspath(args.builddir).lower(), '')
        _LOGGER.info(
            f"Waiting for all child processes in {args.builddir} to exit...")
        max_wait_seconds = 30
        start_time = time.time()
        while time.time() - start_time < max_wait_seconds:
            running_processes = []
            for proc in psutil.process_iter(['name', 'exe', 'pid']):
                try:
                    exe_path = proc.info['exe']
                    if exe_path and os.path.abspath(
                            exe_path).lower().startswith(builddir_abs):
                        running_processes.append(proc.info['pid'])
                except (
                        psutil.NoSuchProcess,
                        psutil.AccessDenied,
                        psutil.ZombieProcess,
                ):
                    pass
                except Exception:
                    pass
            if not running_processes:
                break
            _LOGGER.info(
                f"Still waiting for child PIDs to exit: {running_processes}..."
            )
            time.sleep(1)
        else:
            _LOGGER.warning(
                f"Timed out waiting for child processes to exit after "
                f"{max_wait_seconds}s: {running_processes}")

    if args.skip_profdata or args.dry_run:
        _LOGGER.info("Skipping profdata merging")

        return

    # Profile filenames vary by process; collect them from subdirectories too.
    profraw_files = glob.glob(f'{profraw_path}/**/*.profraw', recursive=True)
    if is_crossbench:
        # Deduplicate by real path: crossbench's output directory may contain
        # symlinked views of the same results, and passing the same profile to
        # llvm-profdata more than once silently multiplies its counters
        # instead of failing.
        profraw_files = sorted({os.path.realpath(p) for p in profraw_files})
    _LOGGER.debug(f"Found {len(profraw_files)} profraw files")
    if not profraw_files:
        raise RuntimeError(f'No profraw files found in {profraw_path}')

    if not args.separate_renderer_pgo:
        run_profdata_merge(profdata_path, profraw_files, args)
    else:
        # Group profraw files by the prefix before the first '-' in their
        # basename.
        # e.g., 'default-12345.profraw' -> prefix 'default'
        # e.g., 'renderer-12345.profraw' -> prefix 'renderer'
        files_by_prefix = {}
        for f in profraw_files:
            filename = os.path.basename(f)
            prefix = (filename.split('-', 1)[0]
                      if '-' in filename else filename[:-8])
            files_by_prefix.setdefault(prefix, []).append(f)

        # Process the 'default' (main browser process) files first
        default_files = files_by_prefix.pop('default', [])
        if default_files:
            run_profdata_merge(profdata_path, default_files, args)
        else:
            raise RuntimeError(f'No browser profile files found for {name}')

        # Perform one merge per remaining prefix
        if files_by_prefix:
            for prefix, prefix_files in files_by_prefix.items():
                target_profdata_path = profdata_path.replace(
                    '.profdata', f'_{prefix}.profdata')
                run_profdata_merge(target_profdata_path, prefix_files, args)
        else:
            _LOGGER.warning(f'No non-default profile files found for {name}')

    # Test merge to prevent issues like: https://crbug.com/353702041
    with tempfile.NamedTemporaryFile() as f:
        _LOGGER.debug("Testing profdata merge")

        run_profdata_merge(f.name, [profdata_path], args)


def run_benchmark_with_repeats(benchmark: Benchmark, args: OptionsNamespace):
    '''Runs the benchmark with provided args, return # of times it failed.'''
    assert args.repeats > 0, 'repeats must be at least 1'
    for idx in range(args.repeats):
        try:
            _LOGGER.info(f"Running benchmark attempt {idx + 1}/{args.repeats}")

            run_benchmark(benchmark, args)
            _LOGGER.info(f"Benchmark succeeded on attempt {idx + 1}")

            return idx
        except (subprocess.CalledProcessError, MergeError) as e:
            if isinstance(e, subprocess.CalledProcessError):
                if e.stdout:
                    _LOGGER.error(f"Stdout:\n{e.stdout}")
                if e.stderr:
                    _LOGGER.error(f"Stderr:\n{e.stderr}")
            if idx < args.repeats - 1:
                _LOGGER.warning('%s', e)
                _LOGGER.warning(f'Retry attempt {idx + 1} for '
                                f'{benchmark.ProduceBenchmarkArgs()}')
            else:
                _LOGGER.error(f'Failed {args.repeats} times')
                raise e
    # This statement can never be reached due to the above `raise e` but is here
    # to appease the typechecker.
    return args.repeats


def run_benchmarks(benchmarks: List[Benchmark], args: OptionsNamespace):
    fail_count = 0
    for benchmark in benchmarks:
        _LOGGER.info(f"Starting benchmark: {benchmark.name}")
        fail_count += run_benchmark_with_repeats(benchmark, args)
    return fail_count


def merge_profdata(
    profile_output_path: str,
    args: OptionsNamespace,
    benchmarks: Optional[List[Benchmark]] = None,
):
    _LOGGER.info(f"Merging all profdata files into: {profile_output_path}")
    all_profdata = glob.glob(f'{args.profiledir}/*.profdata')

    weights_by_name = ({
        b.name: b.weight
        for b in benchmarks if b.weight != 1
    } if benchmarks else {})

    def apply_weight(f: str) -> str:
        basename = os.path.basename(f)
        stem = (basename[:-len('.profdata')]
                if basename.endswith('.profdata') else basename)
        weight = weights_by_name.get(stem)
        if weight is None:
            for b_name in sorted(weights_by_name, key=len, reverse=True):
                if stem.startswith(b_name + '_'):
                    weight = weights_by_name[b_name]
                    break
        if weight and weight != 1:
            return f'--weighted-input={weight},{f}'
        return f

    if not getattr(args, 'separate_renderer_pgo', False):
        _LOGGER.debug(f"Found {len(all_profdata)} profdata files")
        if not all_profdata:
            raise RuntimeError(f'No profdata files found in {args.profiledir}')
        run_profdata_merge(profile_output_path,
                           [apply_weight(f) for f in all_profdata], args)
    else:
        benchmark_names = {b.name for b in benchmarks} if benchmarks else set()
        default_profdata = []
        files_by_prefix = {}
        for f in all_profdata:
            basename = os.path.basename(f)
            stem = (basename[:-len('.profdata')]
                    if basename.endswith('.profdata') else basename)
            if stem in benchmark_names:
                default_profdata.append(f)
                continue
            matched = False
            for b_name in sorted(benchmark_names, key=len, reverse=True):
                if stem.startswith(b_name + '_'):
                    prefix = stem[len(b_name) + 1:]
                    files_by_prefix.setdefault(prefix, []).append(f)
                    matched = True
                    break
            if not matched:
                if '_' in stem:
                    prefix = stem.split('_')[-1]
                    files_by_prefix.setdefault(prefix, []).append(f)
                else:
                    default_profdata.append(f)

        _LOGGER.debug(f"Found {len(default_profdata)} browser profdata files")
        if not default_profdata:
            raise RuntimeError(
                f'No browser profdata files found in {args.profiledir}')
        run_profdata_merge(
            profile_output_path,
            [apply_weight(f) for f in default_profdata],
            args,
        )

        base, ext = os.path.splitext(profile_output_path)
        for prefix, files in files_by_prefix.items():
            _LOGGER.debug(f"Found {len(files)} {prefix} profdata files")
            target_output_path = f'{base}_{prefix}{ext}'
            run_profdata_merge(target_output_path,
                               [apply_weight(f) for f in files], args)

    if args.temporal_trace_length:
        _LOGGER.info("Generating orderfile for temporal PGO")
        orderfile_cmd = [
            _PROFDATA,
            'order',
            profile_output_path,
            '-o',
            f'{args.outputdir}/orderfile.txt',
        ]
        _LOGGER.debug(f"Running command: {' '.join(orderfile_cmd)}")

        subprocess.run(orderfile_cmd, check=True)


def write_isolated_script_test_output(output_path: str,
                                      passed: bool,
                                      start_time: Optional[float] = None):
    output_dir = os.path.dirname(output_path)
    if output_dir:
        os.makedirs(output_dir, exist_ok=True)
    status = 'PASS' if passed else 'FAIL'
    results = {
        'version':
        3,
        'interrupted':
        False,
        'path_delimiter':
        '/',
        'seconds_since_epoch':
        (start_time if start_time is not None else time.time()),
        'tests': {
            'generate_profile': {
                'expected': 'PASS',
                'actual': status,
            }
        },
        'num_failures_by_type': {
            status: 1,
        },
    }
    if not passed:
        results['tests']['generate_profile']['is_unexpected'] = True
    with open(output_path, 'w') as f:
        json.dump(results, f)


def main():
    start_time = time.time()
    args = parse_args()

    handler = logging.StreamHandler()
    formatter = logging.Formatter(
        '%(levelname).1s %(relativeCreated)6d %(message)s')
    handler.setFormatter(formatter)
    _LOGGER.addHandler(handler)

    if args.verbose >= 2:
        _LOGGER.setLevel(logging.DEBUG)
    elif args.verbose == 1:
        _LOGGER.setLevel(logging.INFO)
    else:
        _LOGGER.setLevel(logging.WARN)

    if not os.path.exists(_PROFDATA):
        if args.isolated_script_test_output:
            _LOGGER.warning(f'{_PROFDATA} missing on bot, {_LLVM_DIR}:')
            for root, _, files in os.walk(_LLVM_DIR):
                for f in files:
                    _LOGGER.warning(f'> {os.path.join(root, f)}')
        else:
            _LOGGER.warning(f'{_PROFDATA} does not exist, downloading it')
            subprocess.run(
                [sys.executable, _UPDATE_PY, '--package=coverage_tools'],
                check=True,
            )
    assert os.path.exists(_PROFDATA), f'{_PROFDATA} does not exist'

    if os.path.exists(args.profiledir):
        _LOGGER.warning('Removing %s', args.profiledir)
        shutil.rmtree(args.profiledir)

    # Run the shortest benchmarks first to fail early if anything is wrong.
    # On Apple Silicon, Speedometer3 executes quickly enough that 3 repeats and
    # a 5x merge weight (see https://crbug.com/363195532) are used to match the
    # mac-arm-pgo builder configuration.
    is_mac_arm = (sys.platform == 'darwin' and args.target_arch == 'arm64')
    benchmarks: list[Benchmark] = [
        Benchmark(
            'speedometer3.crossbench',
            ['speedometer3'],
            pageset_repeat=3 if is_mac_arm else 1,
            weight=5 if is_mac_arm else 1,
        ),
    ]

    jetstream_args = ['jetstream3']
    if args.target_arch in ('arm', 'x86'):
        # Skip memory-hungry stories on 32-bit platforms to avoid OOM and
        # timeouts, see https://crbug.com/487336000 and
        # https://crbug.com/565414952.
        jetstream_args.append('--stories=default,-babylonjs-scene-es6,'
                              '-transformersjs-bert-wasm')
    benchmarks.append(Benchmark('jetstream3.crossbench', jetstream_args))

    # These benchmarks require special access permissions:
    # https://www.chromium.org/developers/telemetry/upload_to_cloud_storage/#request-access-for-google-partners
    if not args.run_public_benchmarks_only:
        platform = 'desktop'
        # GPU-dependent benchmarks (system_health, motionmark) are disabled on
        # Linux desktop until the Swarming pool GPU driver issue is resolved,
        # see https://crbug.com/444038165.
        if not sys.platform.startswith('linux'):
            benchmarks.append(
                Benchmark(
                    'system_health',
                    [
                        f'system_health.common_{platform}',
                        '--run-abridged-story-set',
                    ],
                ))

        motionmark_benchmark_args = [
            f'rendering.{platform}',
            '--also-run-disabled-tests',
            '--story-tag-filter=motionmark_fixed_2_seconds',
        ]
        if sys.platform == 'darwin':
            # MotionMark is disabled on Linux (https://crbug.com/444038165) and
            # Windows (https://crbug.com/327032629) desktop PGO builders.
            benchmarks.append(
                Benchmark('motionmark', motionmark_benchmark_args))
            # Exercise the Skia Graphite path on macOS.
            benchmarks.append(
                Benchmark(
                    'motionmark_graphite',
                    motionmark_benchmark_args,
                    enable_features=['SkiaGraphite'],
                ))

    os.makedirs(args.outputdir, exist_ok=True)
    try:
        fail_count = run_benchmarks(benchmarks, args)
        if fail_count:
            # Retried failures were recovered.
            _LOGGER.warning(
                f'Of the {len(benchmarks)} benchmarks, there were '
                f'{fail_count} failures that were resolved by repeat '
                'runs.')

        if not args.skip_profdata and not args.dry_run:
            # Bots run a separate merge step (merge_results.py) that expects
            # profraw files instead of profdata files.
            suffix = (".profraw"
                      if args.isolated_script_test_output else ".profdata")
            profile_output_path = f'{args.outputdir}/profile{suffix}'
            merge_profdata(profile_output_path, args, benchmarks)

        if args.isolated_script_test_output:
            write_isolated_script_test_output(args.isolated_script_test_output,
                                              True, start_time)
    except Exception:
        if args.isolated_script_test_output:
            write_isolated_script_test_output(args.isolated_script_test_output,
                                              False, start_time)
        raise

    if not args.keep_temps:
        _LOGGER.info('Cleaning up %s, use --keep-temps to keep it.',
                     args.profiledir)
        shutil.rmtree(args.profiledir, ignore_errors=True)


if __name__ == '__main__':
    sys.exit(main())
