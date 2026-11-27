"""Run focused family tests on the warm laptop or qualify a fresh consumer build."""
import argparse
import contextlib
import ctypes
import datetime
import hashlib
import json
import os
from pathlib import Path, PurePosixPath
import re
import shutil
import stat
import subprocess
import sys
import time
import tomllib


def require(condition, message):
    if not condition:
        raise ValueError(message)


def sha(path):
    with Path(path).open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def write(path, value):
    path.write_text(json.dumps(value, indent=2) + '\n', encoding='utf-8')


OUTPUTS = {'out/NetworkCI', 'out/NetworkProbeCI'}
HOST_ROOT_NAMES = {'visual_studio', 'windows_sdk', 'depot_tools', 'git'}


def is_reparse_or_symlink(info):
    return (stat.S_ISLNK(info.st_mode) or
            bool(getattr(info, 'st_file_attributes', 0) & stat.FILE_ATTRIBUTE_REPARSE_POINT))


def require_ordinary_path(path):
    info = path.lstat()
    require(not is_reparse_or_symlink(info),
            'Reparse point or symlink in builder inputs: ' + str(path))
    return info


def checkout_file_link(root, path, info):
    require(stat.S_ISLNK(info.st_mode) and
            (os.name != 'nt' or info.st_reparse_tag == stat.IO_REPARSE_TAG_SYMLINK),
            'Unsupported checkout reparse point: ' + str(path))
    target = os.readlink(path)
    relative_target = Path(target)
    require(not relative_target.anchor,
            'Absolute checkout symlink target: ' + str(path))
    location = path.parent
    for part in relative_target.parts:
        location = location.parent if part == '..' else location / part
        require(location.is_relative_to(root),
                'Checkout symlink target escapes source: ' + str(path))
    # Bind the stored link spelling as well as the named input, without opening
    # through the link. Package archives may retain POSIX separators on Windows.
    return {'target': target, 'target_path': location.relative_to(root).as_posix()}


def checkout_inputs(root):
    """Record every actual checkout file, including ignored tool inputs."""
    repositories = []
    files = {}
    file_links = {}

    def visit(directory):
        for path in sorted(directory.iterdir()):
            relative = path.relative_to(root).as_posix()
            if path.name == '.git':
                require_ordinary_path(path)
                repositories.append(directory)
                continue
            if relative in OUTPUTS:
                info = require_ordinary_path(path)
                require(stat.S_ISDIR(info.st_mode), 'Build output is not a directory: ' + relative)
                continue
            info = path.lstat()
            if is_reparse_or_symlink(info):
                file_links[relative] = checkout_file_link(root, path, info)
            elif stat.S_ISDIR(info.st_mode):
                visit(path)
            else:
                require(stat.S_ISREG(info.st_mode), 'Unexpected input type: ' + relative)
                files[relative] = sha(path)

    require_ordinary_path(root)
    visit(root)
    for name, link in file_links.items():
        require(link['target_path'] in files,
                'Checkout symlink does not name an inventoried file: ' + name)
    require(root in repositories, 'Primary Git checkout not found')
    git_revisions = {}
    for repo in sorted(repositories):
        name = repo.relative_to(root).as_posix()
        top = subprocess.check_output(['git', 'rev-parse', '--show-toplevel'],
                                      cwd=repo, text=True).strip()
        require(Path(top).resolve() == repo.resolve(), 'Git root differs: ' + name)
        dirty = subprocess.check_output(
            ['git', 'status', '--porcelain=v1', '--untracked-files=normal'], cwd=repo)
        require(not dirty, 'Uncommitted inputs in ' + name + ': ' +
                dirty.decode('utf-8', errors='replace')[:4096])
        git_revisions[name] = subprocess.check_output(['git', 'rev-parse', 'HEAD'],
                                                       cwd=repo, text=True).strip()
    configuration = root.parent / '.gclient'
    require_ordinary_path(configuration)
    return {'git_revisions': git_revisions, 'files': files, 'file_links': file_links,
            'gclient_sha256': sha(configuration)}


def compare_inputs(expected, actual):
    previous = expected['files']
    current = actual['files']
    changed = sorted(name for name in previous.keys() | current.keys()
                     if previous.get(name) != current.get(name))
    require(not changed, f'{len(changed)} input files differ: {changed[:20]}')
    require(expected['file_links'] == actual['file_links'],
            'Checkout file symlink identity changed')
    require(expected['git_revisions'] == actual['git_revisions'],
            'Git repository membership or revisions changed')
    require(expected['gclient_sha256'] == actual['gclient_sha256'],
            '.gclient changed')


def choose_output(root, run_id, reference_probe):
    require(re.fullmatch(r'[a-z0-9-]{1,48}', run_id), 'Use a short unique run ID')
    branch = 'NetworkProbeCI' if reference_probe else 'NetworkCI'
    relative = PurePosixPath('out', branch, run_id)
    for parent in (root / 'out', root / 'out' / branch):
        parent.mkdir(exist_ok=True)
        require(stat.S_ISDIR(require_ordinary_path(parent).st_mode),
                'Unexpected output parent: ' + str(parent))
    # An existing output may contain cached objects of unknown provenance.
    (root / relative).mkdir(exist_ok=False)
    return relative


def host_input_tree(root):
    files = {}
    pending = [root]
    while pending:
        current = pending.pop()
        for path in sorted(current.iterdir()):
            info = require_ordinary_path(path)
            if path.name == '.git':
                continue
            if stat.S_ISDIR(info.st_mode):
                pending.append(path)
            else:
                require(stat.S_ISREG(info.st_mode), 'Unexpected host input: ' + str(path))
                files[path.relative_to(root).as_posix()] = sha(path)
    require(files, 'Empty host input path: ' + str(root))
    return files


def host_configuration(root):
    path = root.parent / 'build-host.json'
    require_ordinary_path(path)
    raw = path.read_bytes()
    config = json.loads(raw)
    require(set(config) == HOST_ROOT_NAMES, 'Expected four explicit host tool roots')
    roots = {name: Path(value) for name, value in config.items()}
    for name, directory in roots.items():
        require(directory.is_absolute() and directory == directory.resolve() and
                directory != Path(directory.anchor) and
                stat.S_ISDIR(require_ordinary_path(directory).st_mode),
                'Invalid host installation path: ' + name)
    require((roots['visual_studio'] / 'VC/Auxiliary/Build/vcvarsall.bat').is_file(),
            'Missing Visual Studio C++ toolchain')
    require((roots['windows_sdk'] / 'Include').is_dir(), 'Missing SDK headers')
    require((roots['depot_tools'] / 'gclient.bat').is_file(), 'Missing depot_tools')
    require((roots['git'] / 'cmd/git.exe').is_file(), 'Missing Git for Windows')
    python_root = Path(sys.base_prefix).resolve()
    require(Path(sys.executable).resolve().is_relative_to(python_root),
            'Use the qualified depot_tools Python installation directly')
    roots['python'] = python_root
    windows = Path(os.environ['SystemRoot'])
    os.environ.update({
        'PATH': os.pathsep.join(str(directory) for directory in
                                (roots['depot_tools'], roots['git'] / 'cmd',
                                 python_root, windows / 'System32', windows)),
        'GYP_MSVS_OVERRIDE_PATH': str(roots['visual_studio']),
        'vs2026_install': str(roots['visual_studio']),
        'WINDOWSSDKDIR': str(roots['windows_sdk']),
        'DEPOT_TOOLS_UPDATE': '0',
        'DEPOT_TOOLS_WIN_TOOLCHAIN': '0',
    })
    for name in ('CL', '_CL_', 'LINK', '_LINK_', 'INCLUDE', 'LIB', 'LIBPATH',
                 'PYTHONHOME', 'PYTHONPATH', 'VSINSTALLDIR', 'VCINSTALLDIR',
                 'VCToolsInstallDir', 'WindowsSDKVersion'):
        os.environ.pop(name, None)
    return path, hashlib.sha256(raw).hexdigest(), roots


def host_inputs(configuration):
    path, expected_digest, roots = configuration
    require(sha(path) == expected_digest, 'Host tool configuration changed')
    tools = {name: {'path': str(directory), 'files': host_input_tree(directory)}
             for name, directory in sorted(roots.items())}
    environment_keys = ('PATH', 'SystemRoot', 'COMSPEC', 'TEMP', 'TMP',
                        'GYP_MSVS_OVERRIDE_PATH', 'vs2026_install', 'WINDOWSSDKDIR',
                        'DEPOT_TOOLS_WIN_TOOLCHAIN', 'DEPOT_TOOLS_UPDATE')
    return {'configuration_sha256': expected_digest, 'installations': tools,
            'windows_version': list(sys.getwindowsversion().platform_version),
            'interpreter': sys.executable,
            'environment': {key: os.environ.get(key) for key in environment_keys}}


def builder_python_cache(root, depot, prepare=False):
    """Only explicit provisioning may ask vpython to create an environment."""
    cache = root.parent / 'vpython-cache'
    marker = cache / 'network-environments.json'
    launcher = depot / '.cipd_bin/vpython3.exe'
    specs = {'depot': depot / 'vpython.toml', 'chromium': root / 'vpython.toml'}
    pins = {'launcher': sha(launcher), 'specs': {
        name: {'toml': sha(spec), 'lock': sha(spec.with_suffix('.toml.uv.lock'))}
        for name, spec in specs.items()}}
    if prepare:
        cache.mkdir(exist_ok=True)
    else:
        require(marker.is_file(), 'Python cache is unprepared; run --prepare-python-cache on the laptop')
    require_ordinary_path(cache)
    os.environ['VPYTHON_VIRTUALENV_ROOT'] = str(cache)
    os.environ.pop('VPYTHON_BYPASS', None)
    if not prepare:
        os.environ['UV_OFFLINE'] = '1'
        require_ordinary_path(marker)
        saved = json.loads(marker.read_bytes())
        require(saved['pins'] == pins, 'Python launcher or spec changed; run --prepare-python-cache')
    environments = {}
    smoke = """
import importlib.metadata
import json
import sys
from packaging.specifiers import SpecifierSet

version = sys.version.split()[0]
if version not in SpecifierSet(sys.argv[1]):
    sys.exit(f'Python {version} does not satisfy {sys.argv[1]}')
packages = {name: importlib.metadata.version(name) for name in sys.argv[2:]}
print(json.dumps([sys.executable, sys.prefix, version, packages]))
"""
    for name, spec in specs.items():
        # Check the unconditional exact distribution pins, including the different
        # six and requests versions requested by depot_tools and Chromium.
        config = tomllib.loads(spec.read_text(encoding='utf-8'))
        packages = dict(m.groups() for dep in config['dependencies']
                        if (m := re.fullmatch(r'([\w.-]+)==([^;\s]+)', dep)))
        require(packages, f'No package pins to verify for {name}')
        if prepare:
            command = [launcher, '-vpython-root', cache, '-vpython-spec', spec]
        else:
            exe = Path(saved['environments'][name]['executable'])
            require(exe.is_absolute() and exe.is_relative_to(cache) and '..' not in exe.parts and
                    exe.is_file() and sha(exe) == saved['environments'][name]['sha256'],
                    f'Cached {name} interpreter is missing or changed; run --prepare-python-cache')
            command = [exe]
        probe = subprocess.run([*command, '-I', '-B', '-c', smoke, config['requires-python'], *packages],
                               cwd=spec.parent, capture_output=True, text=True,
                               timeout=1200 if prepare else 15)
        require(probe.returncode == 0,
                f'{name} Python cache check failed: {probe.stderr[-1500:]}')
        executable, prefix, version, installed = json.loads(probe.stdout)
        exe = Path(executable)
        require(Path(prefix).resolve().is_relative_to(cache.resolve()) and
                exe.is_absolute() and exe.is_relative_to(cache) and '..' not in exe.parts and
                installed == packages, f'{name} resolved outside the cache or has different packages')
        environments[name] = {'executable': str(exe), 'prefix': prefix,
                              'version': version, 'sha256': sha(exe)}
    record = {'pins': pins, 'environments': environments}
    if prepare:
        temporary = marker.with_suffix('.tmp')
        write(temporary, record)
        temporary.replace(marker)
    else:
        require(record == saved, 'Prepared Python environment changed; run --prepare-python-cache')
    return record


@contextlib.contextmanager
def native_window(root):
    import msvcrt
    path = root.parent / '.rustium-native.lock'
    with path.open('a+b') as lock:
        # Windows supports locking beyond EOF. No lock-file content is needed.
        lock.seek(0)
        msvcrt.locking(lock.fileno(), msvcrt.LK_NBLCK, 1)
        try:
            yield
        finally:
            lock.seek(0)
            msvcrt.locking(lock.fileno(), msvcrt.LK_UNLCK, 1)


def native_processes():
    script = r'''
        $ErrorActionPreference = 'Stop'
        $owners = @(Get-CimInstance Win32_Process | Where-Object {
            $_.Name -match '^(gn|ninja|siso|autoninja|cl|clang|clang-cl|lld-link|link|rustc|cargo|msbuild|.*_(unittests?|perftests?))(\.exe)?$' -or
            ($_.Name -match '^(pythonw?|vpython3?|cmd)(\.exe)?$' -and
             $_.CommandLine -match '\bgclient(?:\.py|\.bat)?\b')
        } | Select-Object ProcessId,Name)
        ConvertTo-Json -InputObject $owners -Compress
    '''
    powershell = (Path(os.environ['SystemRoot']) / 'System32/WindowsPowerShell/v1.0/powershell.exe')
    raw = subprocess.check_output([str(powershell), '-NoProfile', '-NonInteractive',
                                   '-Command', script])
    return json.loads(raw)


def resolve_targets(profile, catalogue, output_relative):
    """Keep every root, resolving nondefault toolchains to real outputs."""
    prefix = '//' + output_relative.as_posix().rstrip('/') + '/'
    resolved = {}
    for label in profile['build_roots']:
        require(label in catalogue, 'Missing evaluated GN root: ' + label)
        outputs = catalogue[label].get('outputs', [])
        require(isinstance(outputs, list), 'Invalid GN outputs: ' + label)
        values = []
        for output in outputs:
            require(isinstance(output, str) and output.startswith(prefix),
                    'GN output escapes selected directory: ' + repr(output))
            relative = PurePosixPath(output[len(prefix):])
            require(not relative.is_absolute() and relative.parts and
                    '..' not in relative.parts and ':' not in str(relative),
                    'Unsafe GN output: ' + output)
            values.append(relative.as_posix())
        if not values:
            require('(' not in label, 'Nondefault root has no output: ' + label)
            require(label.startswith('//') and ':' in label,
                    'Expected a fully qualified GN root: ' + label)
            values = [label[2:]]
        resolved[label] = values
    require(resolved, 'Empty build-root inventory')
    return resolved


def validate_inventory(profile):
    cases = profile['cases']
    require(len(cases) == 36 and len({c['label'] for c in cases}) == 36,
            'All36 distinct program labels are required')
    workloads = [c for c in cases if c['kind'] == 'workload']
    require(sum(c['expected_workloads'] for c in workloads) == 232,
            'All232 ordinary workloads are required')
    for case in workloads:
        require(case['arguments'] == ['--verify-only'] and
                case['measurement_clocks_requested'] is False,
                'This is an ordinary inventory, without timing')
        require(len(case['case_ids']) == len(set(case['case_ids'])) ==
                case['expected_workloads'], 'Incomplete workload membership')
    roots = profile['build_roots']
    require(len(roots) == len(set(roots)) == 96, 'Preserve all96 recorded roots')
    require({c['label'] for c in cases} <= set(roots), 'Missing program build roots')


def available_memory():
    class Status(ctypes.Structure):
        _fields_ = [('length', ctypes.c_ulong), ('load', ctypes.c_ulong)] + [
            (name, ctypes.c_ulonglong) for name in
            ('total', 'available', 'page_total', 'page_available',
             'virtual_total', 'virtual_available', 'extended_available')]
    value = Status()
    value.length = ctypes.sizeof(value)
    require(ctypes.windll.kernel32.GlobalMemoryStatusEx(ctypes.byref(value)),
            'Cannot measure builder memory')
    return {'total_bytes': value.total, 'available_bytes': value.available,
            'commit_headroom_bytes': value.page_available}


def feedback_cases(profile, focus, requested, gtest_filter, rust_test=None, full_suite=False):
    """Select only named ordinary programs of the active family, in repair order."""
    family = {c['target']: c for c in profile['cases'] if c['family'] == focus['family']}
    names = [name.strip() for name in requested.split(',')]
    require(names and len(set(names)) == len(names), 'Supply distinct target names')
    for name in names:
        require(name in family, f'{name!r} is outside active family {focus["family"]}')
        require(name not in focus['separate_configuration'],
                f'{name} needs its separately qualified configuration')
    require(sum(map(bool, (gtest_filter, rust_test, full_suite))) <= 1,
            'Select exact test names or explicitly request complete suites')
    if gtest_filter:
        require(len(names) == 1 and family[names[0]]['kind'] == 'gtest' and
                all(re.fullmatch(r'[A-Za-z0-9_/]+\.[A-Za-z0-9_/]+', name)
                    for name in gtest_filter.split(':')),
                'Use exact colon-separated test names with one gtest target')
    elif rust_test:
        require(len(names) == 1 and family[names[0]]['kind'] == 'rust' and
                re.fullmatch(r'[A-Za-z0-9_]+(?:::[A-Za-z0-9_]+)*', rust_test),
                'Use one exact libtest name with one Rust test target')
    elif any(family[name]['kind'] in {'gtest', 'rust'} for name in names):
        require(full_suite, 'Name the failing case with --gtest-filter or --rust-test, '
                'or use --full-suite for all tests in the named targets')
    return [family[name] for name in names]


def normal_configuration(reference_probe=False):
    return ('is_debug=false\nis_component_build=true\nsymbol_level=0\n'
            'blink_symbol_level=0\nv8_symbol_level=0\ntarget_os="win"\n'
            'target_cpu="x64"\nuse_remoteexec=false\nuse_siso=false\n'
            'rustium_reference_probe=' + ('true' if reference_probe else 'false') + '\n')


def check_feedback_gtest(summary, gtest_filter, log_bytes=b''):
    iterations = summary['per_iteration_data']
    require(len(iterations) == 1 and iterations[0], 'gtest executed no cases')
    cases = iterations[0]
    expected = (set(gtest_filter.split(':')) if gtest_filter else
                set(summary['all_tests']) - set(summary['disabled_tests']))
    require(set(cases) == expected, 'Requested or discovered cases did not all run')
    # Some native GTEST_SKIP results appear as SUCCESS in the launcher JSON.
    skipped = {name.decode() for name in re.findall(
        rb'^\[\s*SKIPPED\s*\]\s+([\w/]+\.[\w/]+)', log_bytes, re.M)}
    for name, runs in cases.items():
        require(len(runs) == 1 and runs[0]['status'] in {'SUCCESS', 'SKIPPED'},
                f'gtest did not pass: {name}')
        if runs[0]['status'] == 'SKIPPED':
            skipped.add(name)
    skipped &= expected
    require(expected - skipped, 'No requested gtest actually passed')
    require(not gtest_filter or not skipped, 'A requested gtest was skipped')
    return {'passed': len(expected - skipped), 'skipped': len(skipped)}


def warm_feedback(root, evidence, args, host, cases, result, command):
    # Reuse the laptop's established ordinary component output. Signoff allocates
    # a separate fresh output; an incompatible or missing cache needs preparation.
    relative = Path('out/NetworkCI/pr1-ad617-20261118a')
    output = root / relative
    for directory in (output.parent.parent, output.parent, output):
        require_ordinary_path(directory)
    args_file = output / 'args.gn'
    require(args_file.read_text(encoding='utf-8') == normal_configuration(),
            'Prepare the normal component cache with the recorded GN arguments')
    vpython = host[2]['depot_tools'] / '.cipd_bin/vpython3.exe'
    require(vpython.is_file(), 'Prepare the laptop vpython3 installation')
    temp = root.parent / 'tmp'
    temp.mkdir(exist_ok=True)
    require_ordinary_path(temp)
    os.environ.update(CR_SOURCE_ROOT=str(root), TMP=str(temp), TEMP=str(temp),
                      CHROME_LOG_FILE=str(evidence / 'chrome.log'),
                      PATH=str(vpython.parent) + os.pathsep + os.environ['PATH'])
    headroom = min(result['memory_before']['available_bytes'],
                   result['memory_before']['commit_headroom_bytes'])
    require(headroom >= 2 * 1024**3, 'Insufficient memory or commit headroom for warm feedback')
    for marker in ('build.ninja', '.ninja_log', '.ninja_deps'):
        path = output / marker
        require(path.is_file() and path.stat().st_size > 0,
                f'Warm output is missing {marker}; restore or prepare the component cache')
    result.update(warm_output=relative.as_posix(), args_sha256=sha(args_file),
                  compiler_jobs=args.jobs, targets={})
    ninja = root / 'third_party/ninja/ninja.exe'
    # Ninja consults GN's recorded generator dependencies, including BUILD.gn
    # and args.gn. Regenerate only a stale manifest before checking its link pool.
    command('refresh-graph', [ninja, '-C', output, f'-j{args.jobs}', 'build.ninja'])
    with (output / 'build.ninja').open(encoding='utf-8') as graph:
        for line in graph:
            if line.rstrip() == 'pool build_toolchain_link_pool':
                require(next(graph, '').strip() == 'depth = 1',
                        'Warm feedback requires one concurrent link')
                break
        else:
            raise ValueError('Missing generated link pool')
    for case in cases:
        name = case['target']
        row = result['targets'][name] = {'built': False, 'passed': False}
        build_log = command('build-' + name, [ninja, '-C', output, f'-j{args.jobs}', case['label'][2:]])
        with build_log.open('rb') as stream:
            row['ninja_no_work'] = any(line.strip() == b'ninja: no work to do.' for line in stream)
        exe = output / (name + '.exe')
        row.update(built=True, executable_sha256=sha(exe))
        arguments = list(case['arguments'])
        if case['kind'] == 'gtest':
            summary = evidence / (name + '-summary.json')
            arguments += ['--test-launcher-jobs=1', '--test-launcher-retry-limit=0',
                          '--test-launcher-print-test-stdio=always',
                          '--test-launcher-summary-output=' + str(summary)]
            if args.gtest_filter:
                arguments.append('--gtest_filter=' + args.gtest_filter)
        elif case['kind'] == 'rust' and args.rust_test:
            arguments.extend(['--exact', args.rust_test])
        log = command('test-' + name, [exe, *arguments])
        if case['kind'] == 'gtest':
            row['cases'] = check_feedback_gtest(json.loads(summary.read_bytes()),
                                               args.gtest_filter, log.read_bytes())
        elif case['kind'] == 'rust':
            expected = 1 if args.rust_test else case['expected_tests']
            if args.rust_test:
                require(re.search(rb'^test ' + re.escape(args.rust_test.encode()) +
                                  rb' \.\.\. ok\r?$', log.read_bytes(), re.M),
                        'The exact Rust case did not pass')
            require(re.search(fr'^test result: ok\. {expected} passed; 0 failed; 0 ignored;'.encode(),
                              log.read_bytes(), re.M), 'Rust suite differs from its recorded count')
            row['cases'] = {'passed': expected, 'skipped': 0}
        else:
            require(case['completion']['method'] == 'exact-completion-message',
                    'This workload needs a focused verifier for its completion contract')
            require(case['completion']['message'] in log.read_text(encoding='utf-8', errors='replace'),
                    'Missing workload completion message')
        row['passed'] = True
        write(evidence / 'progress.json', result)
        print(f'PASS {name} at {args.revision[:12]}', flush=True)
    result['feedback_passed'] = True


def parse_args(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--revision', required=True)
    parser.add_argument('--evidence', required=True, type=Path)
    parser.add_argument('--source', type=Path,
                        default=Path(os.environ.get('USERPROFILE', '.')) / 'rb/src')
    input_mode = parser.add_mutually_exclusive_group(required=True)
    input_mode.add_argument('--prepare-python-cache', action='store_true',
                            help='Provision the two pinned Python environments on the laptop')
    input_mode.add_argument('--record-inputs', action='store_true')
    input_mode.add_argument('--approved-inputs-sha256')
    input_mode.add_argument('--feedback-targets', help='Comma-separated active-family targets, failure first')
    selection = parser.add_mutually_exclusive_group()
    selection.add_argument('--gtest-filter', default='', help='Exact gtest case names separated by colons')
    selection.add_argument('--rust-test', help='One exact Rust libtest case')
    selection.add_argument('--full-suite', action='store_true', help='Run all tests in the named targets')
    parser.add_argument('--jobs', choices=range(4, 9), type=int, default=4,
                        help='Concurrent Ninja jobs for any build (default: 4)')
    parser.add_argument('--reference-probe', action='store_true')
    parser.add_argument('--run-id', help='Unique ID for an empty qualification output')
    return parser.parse_args(argv)


def main():
    args = parse_args()
    require(os.name == 'nt' and os.environ.get('COMPUTERNAME', '').upper() ==
            'HQC-VAN_KALIBER', 'Compilation is assigned to the linked laptop')
    require(len(args.revision) == 40 and
            all(c in '0123456789abcdef' for c in args.revision),
            'Use an exact reviewed Git revision')
    root = args.source.absolute()
    require(root == Path(os.environ['USERPROFILE']) / 'rb/src',
            'Use the dedicated builder checkout')
    require_ordinary_path(root.parent)
    require(args.feedback_targets or not (args.gtest_filter or args.rust_test or args.full_suite),
            'Test selectors require focused feedback')
    require(not args.prepare_python_cache or not (args.run_id or args.reference_probe),
            'Prepare the shared Python cache separately from a qualification build')
    require(not args.feedback_targets or not (args.reference_probe or args.run_id),
            'Warm feedback uses the ordinary output only')
    require(args.prepare_python_cache or args.record_inputs or args.feedback_targets or args.run_id,
            'A fresh build requires a unique run ID')
    evidence = args.evidence.resolve()
    require(not evidence.is_relative_to(root), 'Keep logs outside source/output')
    require(not evidence.exists(), 'Retain previous runs; use new evidence path')
    evidence.mkdir(parents=True)
    result = {'source_revision': args.revision, 'started_utc':
              datetime.datetime.now(datetime.timezone.utc).isoformat(),
              'build_passed': False, 'ordinary_tests': 'not run',
              'subsystem_acceptance': False, 'commands': [],
              'scope': 'Recorded selected-consumer build only; complete affected-'
                       'consumer, ABI, runtime and platform qualification pending'}
    write(evidence / 'progress.json', result)
    os.environ['PYTHONDONTWRITEBYTECODE'] = '1'
    os.environ['GIT_NO_LAZY_FETCH'] = '1'
    if args.feedback_targets:
        result.update(scope='Focused incremental feedback only; cached objects, nested dependencies '
                      'and external toolchain contents have not been independently qualified', feedback_passed=False,
                      ordinary_tests='Focused selection only; see per-target results')
    started = time.monotonic()

    def command(phase, argv):
        log = evidence / (phase + '.log')
        begin = time.monotonic()
        print(f'START {phase}', flush=True)
        with log.open('wb') as stream:
            code = subprocess.call(argv, cwd=root, stdout=stream,
                                   stderr=subprocess.STDOUT)
        result['commands'].append({'phase': phase, 'argv': [str(x) for x in argv],
                                   'exit_code': code, 'elapsed_seconds': round(time.monotonic() - begin, 2),
                                   'seconds_since_start': round(time.monotonic() - started, 2), 'log': log.name,
                                   'log_sha256': sha(log)})
        write(evidence / 'progress.json', result)
        print(f'{phase} exited {code} after {time.monotonic() - begin:.1f}s', flush=True)
        if code:
            with log.open('rb') as stream:
                stream.seek(max(0, log.stat().st_size - 6000))
                print(stream.read().decode('utf-8', errors='replace'), flush=True)
        require(code == 0, phase + ' failed; see retained raw log')
        return log

    def git(*argv):
        return subprocess.check_output(['git', *argv], cwd=root,
                                       text=True).strip()

    def execute():
        host = host_configuration(root)
        require(git('rev-parse', 'HEAD') == args.revision, 'Checkout revision drift')
        require(git('config', '--local', '--get', 'core.autocrlf') == 'false',
                'Qualify the dedicated checkout with core.autocrlf=false')
        competing = native_processes()
        write(evidence / 'processes.json', competing)
        require(not competing,
                'A compiler/test process already owns the substantial window')
        result['phase'] = 'python-cache'
        write(evidence / 'progress.json', result)
        cache_start = time.monotonic()
        if args.prepare_python_cache:
            builder_python_cache(root, host[2]['depot_tools'], prepare=True)
        result['python_cache'] = builder_python_cache(root, host[2]['depot_tools'])
        result['python_cache_seconds'] = round(time.monotonic() - cache_start, 2)
        result['phase'] = 'cache-ready'
        write(evidence / 'progress.json', result)
        if args.prepare_python_cache:
            result['scope'] = 'Prepared and checked pinned Python environments'
            return
        require(shutil.disk_usage(root).free >= 8 * 1024**3, 'Low builder disk space')
        result['memory_before'] = available_memory()
        require(result['memory_before']['available_bytes'] >= 2 * 1024**3,
                'Low available builder memory')
        profile_path = root / 'tools/ci/network-validation-inventory.json'
        profile = json.loads(profile_path.read_text(encoding='utf-8-sig'))
        validate_inventory(profile)
        if args.feedback_targets:
            focus = json.loads((root / 'tools/ci/network-focus.json').read_bytes())
            cases = feedback_cases(profile, focus, args.feedback_targets, args.gtest_filter,
                                   args.rust_test, args.full_suite)
            result['active_family'] = focus['family']
            result['test_selection'] = {'targets': [case['target'] for case in cases],
                                        'gtest_filter': args.gtest_filter, 'rust_test': args.rust_test,
                                        'full_suite': args.full_suite}
            def focused_inputs():
                require(git('rev-parse', 'HEAD') == args.revision and
                        not git('status', '--porcelain=v1', '--untracked-files=normal'),
                        'Prepare a clean checkout of the reviewed head')
                paths = [root / 'DEPS', root.parent / '.gclient', host[0],
                         root / 'testing/location_tags.json', root / 'build/util/LASTCHANGE.committime']
                return {str(path): sha(path) for path in paths}
            result['focused_inputs'] = focused_inputs()
            for path in (Path(__file__), profile_path, root / 'tools/ci/network-focus.json'):
                tracked = 'tools/ci/' + path.name
                require(git('hash-object', '--no-filters', '--', str(path.resolve())) ==
                        git('rev-parse', args.revision + ':' + tracked),
                        'Runner input differs from reviewed commit: ' + tracked)
            warm_feedback(root, evidence, args, host, cases, result, command)
            require(focused_inputs() == result['focused_inputs'],
                    'Source or builder configuration changed during focused feedback')
            return
        for case in profile['cases']:
            if 'entrypoint' in case:
                pin = case['entrypoint']
                require(sha(root / pin['path']) == pin['sha256'],
                        'Entrypoint differs from candidate inventory: ' + pin['path'])
        tool_paths = {
            'gn': root / 'buildtools/win/gn.exe',
            'ninja': root / 'third_party/ninja/ninja.exe',
            'clang': root / 'third_party/llvm-build/Release+Asserts/bin/clang-cl.exe',
            'linker': root / 'third_party/llvm-build/Release+Asserts/bin/lld-link.exe',
            'rustc': root / 'third_party/rust-toolchain/bin/rustc.exe'}
        tools = {key: {'path': str(path), 'sha256': sha(path)}
                 for key, path in tool_paths.items()}
        result.update(toolchain=tools, inventory_sha256=sha(profile_path),
                      deps_sha256=sha(root / 'DEPS'), compiler_jobs=args.jobs,
                      reference_probe=args.reference_probe)
        def dependency_revisions(phase):
            record = evidence / (phase + '.json')
            # gclient.bat normally bootstraps vpython before invoking gclient.py.
            # Use that script with its verified environment directly.
            python = result['python_cache']['environments']['depot']['executable']
            command(phase, [python, '-B', host[2]['depot_tools'] / 'gclient.py',
                            'revinfo', '--actual', '--output-json', str(record)])
            revisions = json.loads(record.read_bytes())
            require(isinstance(revisions, dict) and 'src' in revisions,
                    'Missing dependency revision inventory')
            return revisions

        inputs = checkout_inputs(root)
        inputs['python_cache'] = result['python_cache']
        inputs['toolchain'] = tools
        inputs['host'] = host_inputs(host)
        require(inputs['git_revisions']['.'] == args.revision,
                'Unexpected checkout revision in source snapshot')
        manifests = root.parent / 'qualified-inputs'
        if args.record_inputs:
            inputs['dependency_revisions'] = dependency_revisions('dependency-revisions')
            before = evidence / 'inputs-before.json'
            write(before, inputs)
            manifests.mkdir(exist_ok=True)
            require_ordinary_path(manifests)
            digest = sha(before)
            approved_copy = manifests / (digest + '.json')
            if approved_copy.exists():
                require_ordinary_path(approved_copy)
                require(sha(approved_copy) == digest, 'Existing input record differs')
            else:
                with approved_copy.open('xb') as stream:
                    stream.write(before.read_bytes())
            result['input_record_sha256'] = digest
            return
        digest = args.approved_inputs_sha256
        require(re.fullmatch('[0-9a-f]{64}', digest), 'Use the approved input record SHA-256')
        require_ordinary_path(manifests)
        approved = manifests / (digest + '.json')
        require_ordinary_path(approved)
        require(sha(approved) == digest, 'Approved input record digest differs')
        expected = json.loads(approved.read_bytes())
        compare_inputs(expected, inputs)
        require(expected['toolchain'] == tools, 'Toolchain identity differs from approval')
        require(expected['host'] == inputs['host'], 'Host toolchain or environment differs from approval')
        inputs['dependency_revisions'] = dependency_revisions('dependency-revisions')
        write(evidence / 'inputs-before.json', inputs)
        require(expected == inputs, 'Dependency identity differs from approval')
        result['approved_inputs_sha256'] = digest
        relative = choose_output(root, args.run_id, args.reference_probe)
        output = root / relative
        result['fresh_output'] = relative.as_posix()
        configuration = normal_configuration(args.reference_probe) + 'concurrent_links=1\n'
        (output / 'args.gn').write_text(configuration, encoding='utf-8')
        result['args_sha256'] = sha(output / 'args.gn')
        gn = str(tool_paths['gn'])
        command('gn-gen', [gn, 'gen', relative.as_posix()])
        command('gn-check-base', [gn, 'check', relative.as_posix(),
                                  '//base:base_unittests'])
        command('gn-check-net', [gn, 'check', relative.as_posix(), '//net:*'])
        catalogue_file = command('gn-outputs', [gn, 'desc', relative.as_posix(),
                                  '*', 'outputs', '--format=json'])
        catalogue = json.loads(catalogue_file.read_text(encoding='utf-8-sig'))
        resolved = resolve_targets(profile, catalogue, relative)
        targets = sorted({value for values in resolved.values() for value in values})
        write(evidence / 'resolved-targets.json', resolved)
        def graph_identity():
            return {p.relative_to(output).as_posix(): sha(p)
                    for p in output.rglob('*.ninja')}
        graph_before = graph_identity()
        require(graph_before, 'Missing generated Ninja graph')
        write(evidence / 'graph-before.json', graph_before)
        ninja = str(tool_paths['ninja'])
        command('ninja-query', [ninja, '-C', str(output), '-t', 'query', *targets])
        command('ninja-commands', [ninja, '-C', str(output), '-t', 'commands', *targets])
        command('ninja-build', [ninja, '-C', str(output), '-j' + str(args.jobs),
                                '-k0', *targets])
        graph_after = graph_identity()
        write(evidence / 'graph-after.json', graph_after)
        require(graph_before == graph_after, 'Generated graph changed during build')
        require(sha(profile_path) == result['inventory_sha256'] and
                sha(root / 'DEPS') == result['deps_sha256'] and
                sha(output / 'args.gn') == result['args_sha256'],
                'Source or configuration identity changed')
        require(all(sha(tool_paths[key]) == row['sha256']
                    for key, row in tools.items()), 'Toolchain changed during build')
        after = checkout_inputs(root)
        after['python_cache'] = builder_python_cache(root, host[2]['depot_tools'])
        write(evidence / 'inputs-after.json', after)
        compare_inputs(inputs, after)
        after['dependency_revisions'] = dependency_revisions('dependency-revisions-after')
        after['toolchain'] = tools
        after['host'] = host_inputs(host)
        write(evidence / 'inputs-after.json', after)
        require(inputs == after, 'Dependency or host input identity changed during build')
        result.update(build_passed=True, resolved_outputs=len(targets))

    def finish():
        if result.get('error') and args.feedback_targets:
            result['feedback_passed'] = False
        result['finished_utc'] = datetime.datetime.now(datetime.timezone.utc).isoformat()
        write(evidence / 'result.json', result)

    try:
        with native_window(root):
            try:
                execute()
            except Exception as error:
                result['error'] = type(error).__name__ + ': ' + str(error)
            finally:
                finish()
    except Exception as error:
        result.update(build_passed=False, error=type(error).__name__ + ': ' + str(error))
        finish()
    print(json.dumps({'build_passed': result['build_passed'],
                      'feedback_passed': result.get('feedback_passed'),
                      'input_record_sha256': result.get('input_record_sha256'),
                      'ordinary_tests': result['ordinary_tests'], 'error': result.get('error'),
                      'evidence': str(evidence)}))
    return 0 if not result.get('error') and (result['build_passed'] or
                                            args.prepare_python_cache or args.record_inputs or
                                            result.get('feedback_passed')) else 1


if __name__ == '__main__':
    sys.exit(main())
