"""Build the recorded Network consumers on the dedicated Windows builder.

This records selected-consumer build evidence. It does not run or accept the
ordinary test batch, ABI qualification, performance, or subsystem cutover.
"""
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


def require_ordinary_path(path):
    info = path.lstat()
    is_link = (bool(info.st_file_attributes & stat.FILE_ATTRIBUTE_REPARSE_POINT)
               if os.name == 'nt' else path.is_symlink())
    require(not is_link,
            'Reparse point or symlink in builder inputs: ' + str(path))
    return info


def checkout_inputs(root):
    """Record every actual checkout file, including ignored tool inputs."""
    repositories = []
    files = {}

    def visit(directory):
        for path in sorted(directory.iterdir()):
            relative = path.relative_to(root).as_posix()
            if path.name == '.git':
                require_ordinary_path(path)
                repositories.append(directory)
                continue
            info = require_ordinary_path(path)
            if relative in OUTPUTS:
                require(stat.S_ISDIR(info.st_mode), 'Build output is not a directory: ' + relative)
                continue
            if stat.S_ISDIR(info.st_mode):
                visit(path)
            else:
                require(stat.S_ISREG(info.st_mode), 'Unexpected input type: ' + relative)
                files[relative] = sha(path)

    require_ordinary_path(root)
    visit(root)
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
    return {'git_revisions': git_revisions, 'files': files,
            'gclient_sha256': sha(configuration)}


def compare_inputs(expected, actual):
    previous = expected['files']
    current = actual['files']
    changed = sorted(name for name in previous.keys() | current.keys()
                     if previous.get(name) != current.get(name))
    require(not changed, f'{len(changed)} input files differ: {changed[:20]}')
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
    raw = subprocess.check_output(['powershell.exe', '-NoProfile', '-Command', script])
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
    return {'total_bytes': value.total, 'available_bytes': value.available}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--revision', required=True)
    parser.add_argument('--evidence', required=True, type=Path)
    parser.add_argument('--source', type=Path,
                        default=Path(os.environ.get('USERPROFILE', '.')) / 'rb/src')
    input_mode = parser.add_mutually_exclusive_group(required=True)
    input_mode.add_argument('--record-inputs', action='store_true')
    input_mode.add_argument('--approved-inputs-sha256')
    parser.add_argument('--jobs', choices=(1, 2), type=int, default=1)
    parser.add_argument('--reference-probe', action='store_true')
    parser.add_argument('--run-id', help='Unique ID for an empty qualification output')
    args = parser.parse_args()
    require(os.name == 'nt' and os.environ.get('COMPUTERNAME', '').upper() ==
            'HQC-VAN_KALIBER', 'Compilation is assigned to the linked laptop')
    require(len(args.revision) == 40 and
            all(c in '0123456789abcdef' for c in args.revision),
            'Use an exact reviewed Git revision')
    root = args.source.absolute()
    require(root == Path(os.environ['USERPROFILE']) / 'rb/src',
            'Use the dedicated builder checkout')
    require_ordinary_path(root.parent)
    require(args.record_inputs or args.run_id, 'A build requires a unique run ID')
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

    def command(phase, argv):
        log = evidence / (phase + '.log')
        with log.open('wb') as stream:
            code = subprocess.call(argv, cwd=root, stdout=stream,
                                   stderr=subprocess.STDOUT)
        result['commands'].append({'phase': phase, 'argv': [str(x) for x in argv],
                                   'exit_code': code, 'log': log.name,
                                   'log_sha256': sha(log)})
        write(evidence / 'progress.json', result)
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
        require(shutil.disk_usage(root).free >= 8 * 1024**3, 'Low builder disk space')
        result['memory_before'] = available_memory()
        require(result['memory_before']['available_bytes'] >= 2 * 1024**3,
                'Low available builder memory')
        if args.jobs == 2:
            require(result['memory_before']['available_bytes'] >= 8 * 1024**3,
                    'Two compiler jobs require8GiB available; use one')
        profile_path = root / 'tools/ci/network-validation-inventory.json'
        profile = json.loads(profile_path.read_text(encoding='utf-8-sig'))
        validate_inventory(profile)
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
            command(phase, ['gclient.bat', 'revinfo', '--actual', '--output-json',
                            str(record)])
            revisions = json.loads(record.read_bytes())
            require(isinstance(revisions, dict) and 'src' in revisions,
                    'Missing dependency revision inventory')
            return revisions

        inputs = checkout_inputs(root)
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
        configuration = ('is_debug=false\nis_component_build=true\nsymbol_level=0\n'
                         'blink_symbol_level=0\nv8_symbol_level=0\ntarget_os="win"\n'
                         'target_cpu="x64"\nuse_remoteexec=false\nuse_siso=false\n'
                         'rustium_reference_probe=' +
                         ('true' if args.reference_probe else 'false') + '\n')
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
        write(evidence / 'inputs-after.json', after)
        compare_inputs(inputs, after)
        after['dependency_revisions'] = dependency_revisions('dependency-revisions-after')
        after['toolchain'] = tools
        after['host'] = host_inputs(host)
        write(evidence / 'inputs-after.json', after)
        require(inputs == after, 'Dependency or host input identity changed during build')
        result.update(build_passed=True, resolved_outputs=len(targets))

    def finish():
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
                      'input_record_sha256': result.get('input_record_sha256'),
                      'ordinary_tests': 'not run', 'evidence': str(evidence)}))
    return 0 if not result.get('error') and (result['build_passed'] or
                                            args.record_inputs) else 1


if __name__ == '__main__':
    sys.exit(main())
