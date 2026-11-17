"""Build the recorded Network consumers on the dedicated Windows builder.

This records selected-consumer build evidence. It does not run or accept the
ordinary test batch, ABI qualification, performance, or subsystem cutover.
"""
import argparse
import ctypes
import datetime
import hashlib
import json
import os
from pathlib import Path, PurePosixPath
import shutil
import subprocess
import sys


def require(condition, message):
    if not condition:
        raise ValueError(message)


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def write(path, value):
    path.write_text(json.dumps(value, indent=2) + '\n', encoding='utf-8')


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
    parser.add_argument('--jobs', choices=(1, 2), type=int, default=1)
    parser.add_argument('--reference-probe', action='store_true')
    args = parser.parse_args()
    require(os.name == 'nt' and os.environ.get('COMPUTERNAME', '').upper() ==
            'HQC-VAN_KALIBER', 'Compilation is assigned to the linked laptop')
    require(len(args.revision) == 40 and
            all(c in '0123456789abcdef' for c in args.revision),
            'Use an exact reviewed Git revision')
    root = Path(__file__).resolve().parents[2]
    relative = PurePosixPath('out/NetworkProbeCI' if args.reference_probe
                             else 'out/NetworkCI')
    output = root / relative
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

    lock = None
    try:
        import msvcrt
        lock = (root.parent / '.rustium-native.lock').open('a+b')
        lock.seek(0)
        if not lock.read(1):
            lock.write(b'0'); lock.flush()
        lock.seek(0)
        msvcrt.locking(lock.fileno(), msvcrt.LK_NBLCK, 1)
        require(git('rev-parse', 'HEAD') == args.revision, 'Checkout revision drift')
        require(not git('status', '--porcelain=v1', '--untracked-files=no'),
                'Preserve dirty source; do not compile it as the reviewed commit')
        command('processes', ['powershell.exe', '-NoProfile', '-Command',
                "@(Get-CimInstance Win32_Process | Where-Object { $_.Name -match "
                "'^(ninja|clang-cl|lld-link|rustc|cargo|.*_unittests|.*_perftests)"
                "(\\.exe)?$' } | Select-Object ProcessId,Name) | ConvertTo-Json"])
        require(not (evidence / 'processes.log').read_text(encoding='utf-8-sig').strip(),
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
        command('dependency-revisions', ['gclient.bat', 'revinfo', '--actual'])
        output.mkdir(parents=True, exist_ok=True)
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
        require(git('rev-parse', 'HEAD') == args.revision and not git(
                'status', '--porcelain=v1', '--untracked-files=no'),
                'Source changed during build')
        require(sha(profile_path) == result['inventory_sha256'] and
                sha(root / 'DEPS') == result['deps_sha256'] and
                sha(output / 'args.gn') == result['args_sha256'],
                'Source or configuration identity changed')
        require(all(sha(tool_paths[key]) == row['sha256']
                    for key, row in tools.items()), 'Toolchain changed during build')
        command('dependency-revisions-after', ['gclient.bat', 'revinfo', '--actual'])
        require((evidence / 'dependency-revisions.log').read_bytes() ==
                (evidence / 'dependency-revisions-after.log').read_bytes(),
                'Dependency revisions changed during build')
        result.update(build_passed=True, resolved_outputs=len(targets))
    except Exception as error:
        result['error'] = type(error).__name__ + ': ' + str(error)
    finally:
        if lock is not None:
            lock.close()
        result['finished_utc'] = datetime.datetime.now(datetime.timezone.utc).isoformat()
        write(evidence / 'result.json', result)
    print(json.dumps({'build_passed': result['build_passed'],
                      'ordinary_tests': 'not run', 'evidence': str(evidence)}))
    return 0 if result['build_passed'] else 1


if __name__ == '__main__':
    sys.exit(main())
