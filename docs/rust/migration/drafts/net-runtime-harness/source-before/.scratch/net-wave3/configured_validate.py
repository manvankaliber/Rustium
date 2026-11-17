"""Configured batch acceptance only: never collect performance timings here."""
from pathlib import Path
from datetime import datetime, timezone
import argparse
import ctypes
import hashlib
import importlib.util
import json
import os
import re
import subprocess
import xml.etree.ElementTree as ET

ROOT = Path('D:/Rustium')
BUILD = Path('D:/RustiumBuild/src')
OUT = BUILD / 'out/Default'
SHA = lambda p: hashlib.sha256(p.read_bytes()).hexdigest()
SPEC = importlib.util.spec_from_file_location('benchmark_registry', ROOT / 'tools/run_rust_rewrite_benchmarks.py')
REGISTRY = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(REGISTRY)
VERIFY = {
    'http-range': '--verify-only', 'port-policy': '--verify-only',
    'dns-query': '--verify-only', 'parsed-cookie': '--validate-only',
    'host-port': '--verify-only', 'backoff-entry': '--verify-only',
    'websocket-extensions': '--verify-only', 'net-log-values': '--verify-only',
    'request-headers': '--validate-only', 'http-vary-data': '--verify-only',
    'websocket-frames': '--validate-only', 'http-response-info': '--validate-only',
    'proxy-values': '--verify', 'proxy-matching': '--verify-only',
    'dns-response': '--verify-only', 'http-byte-range': '--verify-only',
    'cookie-inclusion-status': '--verify-only', 'http-auth-cache': '--validate-only',
    'host-mapping': '--verify-only', 'http2-write-queue': '--verify-only',
    'tcp-client-socket': '--validate-only', 'tls-session-cache': '--verify-only',
    'dns-host-cache': '--verify-only', 'disk-simple-index': '--verify-only',
    'http3-client-stream': '--verify-only',
    'http2-read-queue': '--verify-only',
    'http2-priority-dependencies': '--verify-only',
    'net-errors': '--verify-only',
    'header-coalescer': '--verify-only',
    'alps-decoder': '--verify-only',
}


def make_plan(gn_source=None):
    # Alternate input is for preparing a future plan only. Execution always
    # calls this with the actual editing-tree GN and checks synchronized bytes.
    gn = (ROOT / 'net/BUILD.gn').read_text() if gn_source is None else gn_source
    tests = set(re.findall(r'(?m)^\s*test\("([^"\n]*rust_unittests)"\)', gn))
    tests.update(['net_unittests', 'parse_number_native_unittests', 'http_range_promote_unittests',
                  'hsts_unittests', 'mime_type_unittests', 'content_disposition_unittests',
                  'net_diagnostics_debug_formatting_unittests', 'port_policy_original_unittests'])
    fuzzers = set(re.findall(r'(?m)^\s*fuzzer_test\("([^"\n]*rust_fuzzer)"\)', gn))
    fuzzers.update(['parse_number_native_fuzzer', 'http_range_promote_fuzzer', 'hsts_parser_fuzzer'])
    units = [{'target': target, 'arguments': []} for target in sorted(tests)]
    if re.search(r'unit_test_target\s*=\s*"net_error_rust_policy_unittests"', gn):
        units.append({'target': 'net_error_rust_policy_unittests',
                      'arguments': ['--test-threads=1'], 'test_format': 'rust-libtest',
                      'expected_test_count': 10})
    for crate, count in [('header_coalescer', 8), ('alps_decoder', 3)]:
        target = crate + '_rust_policy_unittests'
        if re.search(r'unit_test_target\s*=\s*"' + target + r'"', gn):
            units.append({'target': target, 'arguments': ['--test-threads=1'],
                          'test_format': 'rust-libtest', 'expected_test_count': count})
    return {
        'units': units,
        'workloads': [{'target': REGISTRY.MODULES[key]['targets'][0], 'arguments': [arg], 'module': key}
                      for key, arg in VERIFY.items()],
        'stateful': [
            {'target': 'parsed_cookie_rust_state_fuzz', 'arguments': ['1000000'],
             'method': 'one million deterministic whole-API byte/state inputs plus boundaries; independent original status policy; not coverage-guided fuzzing'},
            {'target': 'dns_host_cache_rust_perftests', 'arguments': ['--verify', '1000000'],
             'method': 'deterministic stateful comparisons; not coverage-guided fuzzing'},
            {'target': 'http3_client_stream_rust_perftests', 'arguments': ['--fuzz'],
             'method': '1,048,576 deterministic actions per implementation; not coverage-guided fuzzing'},
        ],
        'fuzz': [{'target': target, 'arguments': ['-runs=1000000', '-seed=20261113', '-max_len=256', '-print_final_stats=1']}
                 for target in sorted(fuzzers)],
    }


def rust_unit_results(text, expected_count):
    summaries = re.findall(
        r'^test result: (ok|FAILED)\. (\d+) passed; (\d+) failed; (\d+) ignored; '
        r'(\d+) measured; (\d+) filtered out;', text, re.MULTILINE)
    if len(summaries) != 1:
        raise ValueError('Require exactly one native Rust test completion summary')
    status, *numbers = summaries[0]
    passed, failed, ignored, measured, filtered = map(int, numbers)
    if (status != 'ok' or failed or ignored or measured or filtered
            or passed != expected_count or expected_count <= 0):
        raise ValueError('Native Rust tests failed, skipped, filtered or differed from expected scope')
    return {'format': 'Rust libtest', 'passed': passed, 'skipped': ignored,
            'unexpected_statuses': {}, 'expected_test_count': expected_count,
            'normalized_native_output_sha256': hashlib.sha256(text.encode()).hexdigest()}


def unit_results(folder):
    """Read the launcher's real result file, keeping skips and failures visible."""
    summary = folder / 'summary.json'
    if summary.is_file():
        data = json.loads(summary.read_text())
        statuses = [attempt['status']
                    for iteration in data['per_iteration_data']
                    for attempts in iteration.values() for attempt in attempts]
        counts = {status: statuses.count(status) for status in sorted(set(statuses))}
        names = {name for iteration in data['per_iteration_data'] for name in iteration}
        # This launcher's SUCCESS also covers GoogleTest's GTEST_SKIP, and its
        # aggregate XML drops skipped tags. Keep raw statuses, recovering the
        # explicit test names from native output rather than calling them passes.
        log = folder / 'output.log'
        output = re.sub(r'\x1b\[[0-9;]*m', '', log.read_text(errors='replace'))
        skipped_names = set(re.findall(r'(?m)^\[\s*SKIPPED\s*\]\s+(\S+)', output)) & names
        skipped_success = sum(attempt['status'] == 'SUCCESS'
                              for iteration in data['per_iteration_data']
                              for name, attempts in iteration.items() if name in skipped_names
                              for attempt in attempts)
        return {'format': 'Chromium test-launcher summary', 'attempt_status_counts': counts,
                'tests_with_attempts': len(names),
                'listed_tests': len(data.get('all_tests', [])),
                'listed_disabled_tests': len(data.get('disabled_tests', [])),
                'passed': counts.get('SUCCESS', 0) - skipped_success,
                'skipped': counts.get('SKIPPED', 0) + skipped_success,
                'native_skip_names': sorted(skipped_names), 'native_output_sha256': SHA(log),
                'unexpected_statuses': {k: v for k, v in counts.items() if k not in {'SUCCESS', 'SKIPPED'}},
                'result_file_sha256': SHA(summary)}
    # Some standalone GoogleTest mains do not use Chromium's launcher. Its
    # output path can contain XML despite the requested .json extension.
    path = folder / 'gtest.json'
    raw = path.read_text()
    if raw.lstrip().startswith('<'):
        tree = ET.fromstring(raw)
        data = tree.attrib
        observed_skips = sum(test.find('skipped') is not None for test in tree.findall('.//testcase'))
        method = 'GoogleTest XML'
    else:
        data = json.loads(raw)
        # GoogleTest JSON omits the root skipped count in some versions even
        # when individual tests record result=SKIPPED.
        observed_skips = sum(test.get('result') == 'SKIPPED' or bool(test.get('skipped'))
                             for suite in data.get('testsuites', [])
                             for test in suite.get('testsuite', []))
        method = 'GoogleTest JSON'
    counts = {k: int(data.get(k, 0)) for k in ['tests', 'failures', 'errors', 'disabled', 'skipped']}
    counts['skipped'] = max(counts['skipped'], observed_skips)
    return {'format': method, 'counts': counts,
            'passed': counts['tests'] - counts['failures'] - counts['errors'] - counts['disabled'] - counts['skipped'],
            'skipped': counts['skipped'],
            'unexpected_statuses': {k: counts[k] for k in ['failures', 'errors'] if counts[k]},
            'result_file_sha256': SHA(path)}


def main():
    global OUT
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--execute', action='store_true')
    parser.add_argument('--phases', nargs='+', choices=['units', 'workloads', 'stateful', 'fuzz'],
                        default=['units', 'workloads', 'stateful'])
    parser.add_argument('--configured-out', type=Path, default=OUT,
                        help='Configured output under the build tree; fuzz requires a real engine build.')
    parser.add_argument('--output', type=Path)
    parser.add_argument('--targets', nargs='+',
                        help='Run an explicit subset; recorded as partial validation, never whole-suite acceptance.')
    parser.add_argument('--gtest-filter',
                        help='Focused unit-test screen; recorded as partial validation.')
    args = parser.parse_args()
    OUT = args.configured_out.resolve()
    if not OUT.is_relative_to(BUILD / 'out'):
        parser.error('Configured output must be beneath the build tree out directory')
    plan = make_plan()
    if args.targets:
        known = {case['target'] for phase in args.phases for case in plan[phase]}
        if set(args.targets) - known:
            parser.error('Unknown target in selected phases: ' + str(set(args.targets) - known))
        plan = {phase: [case for case in cases if case['target'] in args.targets]
                for phase, cases in plan.items()}
    if not args.execute:
        print(json.dumps(plan, indent=2))
        return
    if 'fuzz' in args.phases:
        flags = (OUT / 'args.gn').read_text()
        if not re.search(r'(?m)^\s*use_libfuzzer\s*=\s*true\s*$', flags):
            raise RuntimeError('This configuration has no real libFuzzer engine. Windows fuzzer_test groups may be no-ops; use the separate configured fuzz build.')
        if os.name == 'nt' and not re.search(r'(?m)^\s*is_asan\s*=\s*true\s*$', flags):
            raise RuntimeError('Chromium Windows libFuzzer configuration requires is_asan=true')
    stamp = datetime.now(timezone.utc).strftime('%Y%m%dT%H%M%S%fZ')
    output = args.output or ROOT / 'docs/rust/benchmarks' / ('net-configured-validation-' + stamp)
    output.mkdir(parents=True, exist_ok=False)
    (output / 'plan.json').write_text(json.dumps(plan, indent=2) + '\n')
    (output / 'configured-args.gn').write_bytes((OUT / 'args.gn').read_bytes())
    # A dry-run must report no work for every executable before any runs. This
    # runner is parent-owned and must be invoked only after GN/Ninja is idle.
    targets = sorted({c['target'] for phase in args.phases for c in plan[phase]})
    command = [str(BUILD / 'third_party/ninja/ninja.exe'), '-C', str(OUT), '-n', *targets]
    clean = subprocess.run(command, capture_output=True, text=True)
    (output / 'up-to-date.log').write_text(clean.stdout + clean.stderr)
    if clean.returncode or 'no work to do' not in clean.stdout:
        raise RuntimeError('Configured validation executables need building; see ' + str(output / 'up-to-date.log'))
    source = json.loads((ROOT / '.scratch/net-wave2/parent/last-synced.json').read_text())
    for rel, digest in source.items():
        if not (ROOT / rel).is_file() or not (BUILD / rel).is_file() or SHA(ROOT / rel) != digest or SHA(BUILD / rel) != digest:
            raise RuntimeError('Source changed or not synchronized: ' + rel)
    (output / 'source-sha256.json').write_text(json.dumps(source, indent=2) + '\n')
    inspector = Path('C:/Program Files (x86)/Microsoft Visual Studio/2022/BuildTools/VC/Tools/MSVC/14.44.35207/bin/Hostx64/x64/dumpbin.exe')
    env = os.environ.copy()
    env['PATH'] = os.pathsep.join([str(OUT), 'D:/depot_tools', 'D:/depot_tools/.cipd_bin', env.get('PATH', '')])
    env['TLS_SESSION_CACHE_RUST_CERT_FILE'] = str(BUILD / 'net/data/ssl/certificates/ok_cert.pem')
    repair = ROOT / 'docs/rust/benchmarks/net-runtime-fixture-repair-parent-20261113T052241419759Z/report.json'
    runtime_inputs = {item['path']: item['after_sha256']
                      for item in json.loads(repair.read_text())['records']}
    environment_plan = ROOT / 'docs/rust/benchmarks/net-runtime-fixture-diagnosis-20261113T045548449756Z/parent-environment-plan.json'
    qualification = json.loads(environment_plan.read_text())
    runtime_inputs.update({item['path']: item['sha256'] for item in qualification['pinned_existing_inputs']})
    runtime_inputs.update({qualification['spec']['path']: qualification['spec']['sha256'],
                           qualification['spec']['lock_path']: qualification['spec']['lock_sha256']})
    for path, digest in runtime_inputs.items():
        if SHA(Path(path)) != digest:
            raise RuntimeError('Runtime fixture or Python input changed: ' + path)
    (output / 'runtime-input-sha256.json').write_text(json.dumps(runtime_inputs, indent=2) + '\n')
    (output / 'child-environment.json').write_text(json.dumps({
        'PATH_prefix': [str(OUT), 'D:/depot_tools', 'D:/depot_tools/.cipd_bin'],
        'PATH_suffix_sha256': hashlib.sha256(os.environ.get('PATH', '').encode()).hexdigest(),
        'TLS_SESSION_CACHE_RUST_CERT_FILE': env['TLS_SESSION_CACHE_RUST_CERT_FILE'],
        'environment_plan_sha256': SHA(environment_plan),
        'scope': 'each explicit child env only; parent PATH and system settings unchanged',
        'python_runtime_qualification': 'not implied by PATH; actual test/probe required',
    }, indent=2) + '\n')
    error_mode = None
    if os.name == 'nt':
        api = ctypes.WinDLL('kernel32', use_last_error=True)
        api.GetErrorMode.argtypes = []
        api.GetErrorMode.restype = ctypes.c_uint
        api.SetErrorMode.argtypes = [ctypes.c_uint]
        api.SetErrorMode.restype = ctypes.c_uint
        # This owned validation coordinator's children inherit the mode. Keep
        # native failures visible in logs/exit codes without interactive WER UI.
        api.SetErrorMode(api.GetErrorMode() | 0x3)
        error_mode = api.GetErrorMode()
        if error_mode & 0x3 != 0x3:
            raise RuntimeError('Failed to suppress child-process error dialogs')
    records = []
    for phase in args.phases:
        for ordinal, case in enumerate(plan[phase]):
            target = case['target']
            folder = output / (phase + '-' + target)
            folder.mkdir()
            executable = OUT / (target + '.exe')
            binary = SHA(executable)
            dlls, system = REGISTRY.local_dll_dependencies(executable, inspector)
            command = [str(executable), *case['arguments']]
            if phase == 'units' and case.get('test_format') != 'rust-libtest':
                command += ['--test-launcher-jobs=1', '--test-launcher-retry-limit=0',
                            '--test-launcher-summary-output=' + str(folder / 'summary.json'),
                            '--gtest_output=json:' + str(folder / 'gtest.json')]
                if args.gtest_filter:
                    command.append('--gtest_filter=' + args.gtest_filter)
            if phase == 'fuzz':
                artifacts = folder / 'artifacts'
                artifacts.mkdir()
                command.append('-artifact_prefix=' + str(artifacts) + os.sep)
            record = {'command': command, 'phase': phase, 'started_utc': datetime.now(timezone.utc).isoformat(),
                      'configured_out': str(OUT), 'configured_args_sha256': SHA(OUT / 'args.gn'),
                      'executable_sha256': binary, 'local_dll_sha256': dlls,
                      'system_DLL_names_only': system, 'cpu_affinity_requested': 1 << 2,
                      'performance_timing': False, 'method': case.get('method')}
            record['coordinator_error_mode_at_child_creation'] = error_mode
            limit = {'units': 3600, 'workloads': 180, 'stateful': 1800, 'fuzz': 3600}[phase]
            record['timeout_seconds'] = limit
            with (folder / 'output.log').open('wb') as log:
                process = subprocess.Popen(command, cwd=OUT, env=env, stdout=log, stderr=subprocess.STDOUT)
                if not ctypes.windll.kernel32.SetProcessAffinityMask(ctypes.c_void_p(int(process._handle)), ctypes.c_size_t(1 << 2)):
                    process.kill(); process.wait()
                    raise ctypes.WinError()
                try:
                    code = process.wait(timeout=limit)
                except subprocess.TimeoutExpired:
                    process.kill()
                    code = process.wait()
                    record['timed_out'] = True
                except BaseException:
                    process.kill()
                    process.wait()
                    raise
            record.update(exit_code=code, finished_utc=datetime.now(timezone.utc).isoformat())
            if any(SHA(Path(path)) != digest for path, digest in runtime_inputs.items()):
                raise RuntimeError('Runtime inputs changed during configured validation')
            if binary != SHA(executable) or any(SHA(OUT / name) != digest for name, digest in dlls.items()):
                raise RuntimeError('Executable or DLL changed during validation: ' + target)
            text = (folder / 'output.log').read_text(errors='replace')
            if phase == 'units':
                try:
                    if case.get('test_format') == 'rust-libtest':
                        record['test_results'] = rust_unit_results(text, case['expected_test_count'])
                        record['test_results']['native_output_sha256'] = SHA(folder / 'output.log')
                    else:
                        record['test_results'] = unit_results(folder)
                    if record['test_results']['passed'] <= 0 or record['test_results']['unexpected_statuses']:
                        code = 1
                except (OSError, ValueError, KeyError, ET.ParseError) as error:
                    record['test_result_error'] = str(error)
                    code = 1
            if phase == 'workloads' and any(token in text for token in ['"baseline_ns"', '"cpp_ns"', '"rust_ns"']):
                record['unexpected_measurement_output'] = True
                code = 1
            record['accepted_execution'] = code == 0 and not record.get('timed_out', False)
            (folder / 'execution.json').write_text(json.dumps(record, indent=2) + '\n')
            records.append(record)
            (output / 'executions.json').write_text(json.dumps(records, indent=2) + '\n')
            print(phase, target, 'exit', code, flush=True)
            if not record['accepted_execution']:
                print(text[-4500:], flush=True)
                raise SystemExit(1)
    for rel, digest in source.items():
        if SHA(ROOT / rel) != digest or SHA(BUILD / rel) != digest:
            raise RuntimeError('Source changed during validation: ' + rel)
    (output / 'validation-complete.json').write_text(json.dumps({
        'executions': len(records), 'all_exit_zero': True,
        'explicit_target_filter': args.targets,
        'explicit_gtest_filter': args.gtest_filter,
        'partial_validation': args.targets is not None or args.gtest_filter is not None,
        'completed_utc': datetime.now(timezone.utc).isoformat(),
        'scope': args.phases, 'browser_acceptance': 'Requires full Chrome rebuild and separate browser checks.',
        'fuzz_acceptance': 'Not established by default validation. Genuine engine/campaign completion evidence must be audited separately; default Windows fuzzer_test groups are no-ops.',
        'benchmark_acceptance': 'No timings; still pending.',
    }, indent=2) + '\n')


if __name__ == '__main__':
    main()
