"""Parent-only copied-input preparation and ordinary net_unittests execution.

This script contains no build command. The parent supplies a real, fresh build
record before copying, then runs the small existing-Python identity query before
the broad suite. Python remains an explicitly pinned external dependency.
"""
import argparse
import ast
import ctypes
from datetime import datetime, timezone
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys

ROOT = Path('D:/Rustium')
BUILD = Path('D:/RustiumBuild/src')
OUT = BUILD / 'out/Default'
INSPECTOR = Path('C:/Program Files (x86)/Microsoft Visual Studio/2022/BuildTools/VC/Tools/MSVC/14.44.35207/bin/Hostx64/x64/dumpbin.exe')
ENV_PLAN = ROOT / 'docs/rust/benchmarks/net-runtime-fixture-diagnosis-20261113T045548449756Z/parent-environment-plan.json'
FIXTURE_REPORT = ROOT / 'docs/rust/benchmarks/net-runtime-fixture-repair-parent-20261113T052241419759Z/report.json'
STATE = ROOT / '.scratch/net-wave2/parent/last-synced.json'


def sha(path):
    with Path(path).open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def load(path):
    return json.loads(Path(path).read_text())


def save(path, value):
    Path(path).write_text(json.dumps(value, indent=2) + '\n')


def preparation_resources():
    class Memory(ctypes.Structure):
        _fields_ = [('length', ctypes.c_ulong), ('load', ctypes.c_ulong),
            *[(name, ctypes.c_ulonglong) for name in ['physical_total', 'physical_available',
                'page_total', 'page_available', 'virtual_total', 'virtual_available', 'extended_available']]]
    api = ctypes.WinDLL('kernel32', use_last_error=True)
    memory = Memory(); memory.length = ctypes.sizeof(memory)
    assert api.GlobalMemoryStatusEx(ctypes.byref(memory))
    assert memory.physical_available >= 2684354560, 'Defer below2.5GiB available memory'
    api.GetCurrentProcess.restype = ctypes.c_void_p
    api.SetProcessAffinityMask.argtypes = [ctypes.c_void_p, ctypes.c_size_t]
    assert api.SetProcessAffinityMask(api.GetCurrentProcess(), 1 << 5)
    return memory.physical_available


def module(name, path):
    spec = importlib.util.spec_from_file_location(name, path)
    result = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(result)
    return result


def verify(snapshot):
    manifest = load(snapshot / 'input-manifest.json')
    for row in manifest['files']:
        path = snapshot / row['copy']
        assert path.resolve().is_relative_to(snapshot)
        assert sha(path) == row['sha256'], 'Copied input changed: ' + str(path)
    for row in manifest['directories']:
        path = snapshot / row['copy']
        assert path.is_dir(), 'Copied directory missing: ' + str(path)
        actual = sorted(p.relative_to(path).as_posix() for p in path.rglob('*') if p.is_file())
        assert actual == row['files'], 'Copied data membership changed: ' + str(path)
        actual_dirs = sorted(p.relative_to(path).as_posix() for p in path.rglob('*') if p.is_dir())
        assert actual_dirs == row['subdirectories'], 'Copied directory membership changed: ' + str(path)
    for row in manifest['external_python_inputs']:
        assert sha(row['path']) == row['sha256'], 'External Python input changed: ' + row['path']
    return manifest


def prepare(args):
    dest = args.snapshot.resolve()
    assert not dest.exists(), 'Choose a new snapshot directory'
    proof = load(args.freshness_record)
    assert proof['exit_code'] == 0 and '-n' in proof['command'] and 'net_unittests' in proof['command']
    directory_index = proof['command'].index('-C') + 1
    assert (Path(proof['cwd']) / proof['command'][directory_index]).resolve() == OUT
    assert proof['source_manifest_sha256'] == sha(STATE)
    assert proof['configured_args_sha256'] == sha(OUT / 'args.gn')
    assert proof['executable_sha256'] == sha(OUT / 'net_unittests.exe')
    assert 'no work to do' in Path(proof['output_log']).read_text(errors='replace')
    state = load(STATE)
    originals = set()
    directories = []
    copy_overrides = {}
    editing_sources = []

    def add(path):
        path = path.resolve()
        if path.is_dir():
            members = sorted(p.resolve() for p in path.rglob('*') if p.is_file())
            children = sorted(p.relative_to(path).as_posix() for p in path.rglob('*') if p.is_dir())
            directories.append((path, members, children))
            originals.update(members)
        else:
            assert path.is_file(), 'Required input missing: ' + str(path)
            originals.add(path)

    for rel, digest in state.items():
        assert sha(BUILD / rel) == digest, 'Configured source drift: ' + rel
        editing = (args.built_source_snapshot / rel).resolve() if args.built_source_snapshot else (ROOT / rel).resolve()
        assert sha(editing) == digest, 'Built editing-source identity mismatch: ' + rel
        add(editing)
        copy_overrides[editing] = Path('editing') / rel
        editing_sources.append({'logical_original': str(ROOT / rel), 'actual_copy_source': str(editing),
                               'sha256': digest, 'claim': 'Built identity only; later editing version is not validated'})
        add(BUILD / rel)
    runtime_list = OUT / 'net_unittests.runtime_deps'
    for line in runtime_list.read_text().splitlines():
        path = (OUT / line.strip()).resolve()
        assert path.is_relative_to(BUILD), 'Unclassified runtime path: ' + str(path)
        add(path)
    registry = module('provenance', ROOT / 'tools/run_rust_rewrite_benchmarks.py')
    adjacent, system = registry.local_dll_dependencies(OUT / 'net_unittests.exe', INSPECTOR)
    for name in adjacent:
        add(OUT / name)
    for rel in ['out/Default/args.gn', 'out/Default/net_unittests.runtime_deps',
                'out/Default/build.ninja', 'out/Default/toolchain.ninja', 'out/Default/environment.x64',
                'base/base_paths.cc', 'base/base_paths_win.cc', 'net/test/python_utils.cc',
                'third_party/llvm-build/Release+Asserts/bin/clang-cl.exe',
                'third_party/rust-toolchain/bin/rustc.exe', 'third_party/ninja/ninja.exe']:
        add(BUILD / rel)
    for path in (OUT / 'obj/net').glob('*.ninja'):
        add(path)
    for path in [STATE, FIXTURE_REPORT, ENV_PLAN, INSPECTOR, Path(sys.executable),
                 ROOT / 'tools/run_rust_rewrite_benchmarks.py',
                 ROOT / '.scratch/net-wave3/configured_validate.py',
                 Path(__file__), args.freshness_record, Path(proof['output_log'])]:
        add(path)
    if args.built_source_snapshot:
        add(args.built_source_snapshot.parent / 'source-snapshot-sha256.json')
    primitive_context = []
    if args.primitive_snapshot_manifest:
        add(args.primitive_snapshot_manifest)
        for row in load(args.primitive_snapshot_manifest):
            archived = (args.primitive_snapshot_manifest.parent / row['snapshot']).resolve()
            assert sha(archived) == row['sha256'], 'Archived primitive source changed'
            add(archived)
            copy_overrides[archived] = Path('context/primitive-source') / Path(row['snapshot'])
            primitive_context.append(row)
    for row in load(FIXTURE_REPORT)['records']:
        assert sha(row['path']) == row['after_sha256'], 'Repaired fixture drift: ' + row['path']
        add(Path(row['path']))
    environment = load(ENV_PLAN)
    external = environment['pinned_existing_inputs']
    for row in external:
        assert sha(row['path']) == row['sha256']
    # Keep the external launcher identity explicit; no executable replacement.
    launcher = Path('D:/depot_tools/.cipd_bin/vpython3.exe')
    assert any(Path(row['path']) == launcher for row in external)
    def mapped(path):
        if path in copy_overrides:
            return copy_overrides[path]
        if path.is_relative_to(BUILD):
            return Path('configured') / path.relative_to(BUILD)
        if path.is_relative_to(ROOT):
            return Path('editing') / path.relative_to(ROOT)
        return Path('context/external') / hashlib.sha256(str(path).casefold().encode()).hexdigest() / path.name

    copied_bytes = sum(path.stat().st_size for path in originals)
    assert shutil.disk_usage(dest.parent).free > copied_bytes + 1024**3, 'Insufficient free space for independent copies and1GiB headroom'
    dest.mkdir(parents=True)
    records = []
    for original, _, children in directories:
        (dest / mapped(original)).mkdir(parents=True, exist_ok=True)
        for child in children:
            (dest / mapped(original) / child).mkdir(parents=True, exist_ok=True)
    for original in sorted(originals):
        relative = mapped(original)
        target = dest / relative
        digest = sha(original)
        target.parent.mkdir(parents=True, exist_ok=True)
        if target.exists():
            assert sha(target) == digest, 'Conflicting logical input versions: ' + str(target)
        else:
            shutil.copyfile(original, target)
        assert not os.path.samefile(original, target)
        assert sha(target) == digest == sha(original), 'Input changed while copying'
        records.append({'original': str(original), 'copy': relative.as_posix(), 'sha256': digest})
    # Reuse the existing parser's exact function, without importing live helpers.
    parser_path = ROOT / '.scratch/net-wave3/configured_validate.py'
    parser_source = parser_path.read_text()
    node = next(n for n in ast.parse(parser_source).body if isinstance(n, ast.FunctionDef) and n.name == 'unit_results')
    parser_function = ast.get_source_segment(parser_source, node)
    parser = dest / 'context/unit_parser.py'
    parser.parent.mkdir(exist_ok=True)
    parser.write_text('import json, re, hashlib\nimport xml.etree.ElementTree as ET\nSHA = lambda p: hashlib.sha256(p.read_bytes()).hexdigest()\n\n' + parser_function + '\n')
    records.append({'original': str(parser_path) + ':unit_results', 'copy': 'context/unit_parser.py', 'sha256': sha(parser)})
    copied_dirs = []
    for original, members, children in directories:
        now = sorted(p.resolve() for p in original.rglob('*') if p.is_file())
        assert members == now, 'Source data membership changed while copying'
        assert children == sorted(p.relative_to(original).as_posix() for p in original.rglob('*') if p.is_dir())
        copied_dirs.append({'original': str(original), 'copy': mapped(original).as_posix(),
                            'files': sorted(p.relative_to(original).as_posix() for p in members), 'subdirectories': children})
    runtime_copy = dest / 'configured/out/Default'
    copied_dirs.append({'original': str(OUT), 'copy': 'configured/out/Default',
        'files': sorted(p.relative_to(runtime_copy).as_posix() for p in runtime_copy.rglob('*') if p.is_file()),
        'subdirectories': sorted(p.relative_to(runtime_copy).as_posix() for p in runtime_copy.rglob('*') if p.is_dir()),
        'scope': 'Exact selected copied runtime/configuration closure; not the entire mutable output tree'})
    for row in records:
        if ':unit_results' not in row['original']:
            assert sha(row['original']) == row['sha256'], 'Original input changed before snapshot completed'
    manifest = {'scope': 'Ordinary complete net_unittests native/source/data snapshot',
        'created_utc': datetime.now(timezone.utc).isoformat(), 'files': records, 'directories': copied_dirs,
        'source_manifest_sha256': sha(STATE), 'runtime_deps_sha256': sha(runtime_list),
        'editing_sources': editing_sources,
        'preserved_primitive_source_context': primitive_context,
        'freshness_record_sha256': sha(args.freshness_record), 'adjacent_dlls': adjacent,
        'system_dll_names': system, 'external_python_inputs': external,
        'python_launcher': str(launcher), 'parent_path_suffix_sha256': hashlib.sha256(os.environ.get('PATH', '').encode()).hexdigest(),
        'runner_copy': mapped(Path(__file__).resolve()).as_posix(),
        'copied_with_hardlinks': False, 'copied_bytes': copied_bytes, 'test_execution_claimed': False,
        'limitations': ['System DLLs remain named machine dependencies.',
            'vpython launcher and selected Python environment remain external; qualify actual interpreter and DLL before running.',
            'Static adjacent imports are captured; undeclared dynamic services remain environment dependencies.',
            'Source context comprises synchronized rewrite inputs and supplied immutable primitive snapshots, not the entire transitive Chromium checkout.',
            'Runtime-dependency declarations are not proof that the relocated suite passed.']}
    save(dest / 'input-manifest.json', manifest)
    verify(dest)
    print(json.dumps({'snapshot': str(dest), 'manifest_sha256': sha(dest / 'input-manifest.json'),
                      'files': len(records), 'python_qualification': 'required', 'tests_run': False}))


def child_env(snapshot, manifest):
    assert hashlib.sha256(os.environ.get('PATH', '').encode()).hexdigest() == manifest['parent_path_suffix_sha256'], 'Parent PATH changed; qualify the new child environment explicitly'
    env = os.environ.copy()
    out = snapshot / 'configured/out/Default'
    env['PATH'] = os.pathsep.join([str(out), 'D:/depot_tools', 'D:/depot_tools/.cipd_bin', env.get('PATH', '')])
    env['CR_SOURCE_ROOT'] = str(snapshot / 'configured')
    env['TLS_SESSION_CACHE_RUST_CERT_FILE'] = str(snapshot / 'configured/net/data/ssl/certificates/ok_cert.pem')
    env['PYTHONDONTWRITEBYTECODE'] = '1'
    return env


PYTHON_QUERY = r'''import ctypes, hashlib, json, pathlib, sys
def identity(path):
 p=pathlib.Path(path).resolve()
 return {"path":str(p),"sha256":hashlib.sha256(p.read_bytes()).hexdigest()}
k=ctypes.WinDLL("kernel32",use_last_error=True)
k.GetModuleFileNameW.argtypes=[ctypes.c_void_p,ctypes.c_wchar_p,ctypes.c_uint]
k.GetModuleFileNameW.restype=ctypes.c_uint
b=ctypes.create_unicode_buffer(32768)
n=k.GetModuleFileNameW(ctypes.c_void_p(ctypes.pythonapi._handle),b,len(b))
assert n and n<len(b),ctypes.get_last_error()
k.GetCurrentProcess.restype=ctypes.c_void_p
k.GetProcessAffinityMask.argtypes=[ctypes.c_void_p,ctypes.POINTER(ctypes.c_size_t),ctypes.POINTER(ctypes.c_size_t)]
pm,sm=ctypes.c_size_t(),ctypes.c_size_t()
assert k.GetProcessAffinityMask(k.GetCurrentProcess(),ctypes.byref(pm),ctypes.byref(sm))
r={"executable":identity(sys.executable),"python_dll":identity(b.value),"version":sys.version,"version_info":list(sys.version_info[:3]),"prefix":sys.prefix,"base_prefix":sys.base_prefix,"sys_path":sys.path,"affinity_mask":pm.value}
if getattr(sys,"_base_executable",None):r["base_executable"]=identity(sys._base_executable)
print("RUSTIUM_PYTHON_RUNTIME="+json.dumps(r,sort_keys=True))'''


def qualify(args):
    snapshot = args.snapshot.resolve()
    manifest = verify(snapshot)
    assert Path(__file__).resolve() == snapshot / manifest['runner_copy'], 'Qualify using the copied conductor'
    evidence = args.evidence.resolve(); evidence.mkdir(parents=True, exist_ok=False)
    env = child_env(snapshot, manifest)
    resolved_launcher = shutil.which('vpython3.exe', path=env['PATH'])
    assert resolved_launcher and Path(resolved_launcher).resolve() == Path(manifest['python_launcher']).resolve()
    command = ['vpython3.exe', '-u', '-c', PYTHON_QUERY]
    result = subprocess.run(command, executable=resolved_launcher, cwd=snapshot / 'configured/out/Default',
        env=env, capture_output=True, text=True, timeout=180)
    (evidence / 'stdout.txt').write_text(result.stdout)
    (evidence / 'stderr.txt').write_text(result.stderr)
    values = [line.removeprefix('RUSTIUM_PYTHON_RUNTIME=') for line in result.stdout.splitlines() if line.startswith('RUSTIUM_PYTHON_RUNTIME=')]
    assert result.returncode == 0 and len(values) == 1, 'Existing vpython identity query failed; retain logs'
    runtime = json.loads(values[0])
    assert runtime['version_info'][:2] == [3, 11], 'Interpreter does not match the pinned >=3.11,<3.12 spec'
    assert runtime['affinity_mask'] == 1 << 5, 'Identity query did not inherit CPU5 affinity'
    for key in ['executable', 'python_dll', 'base_executable']:
        if key in runtime:
            assert sha(runtime[key]['path']) == runtime[key]['sha256']
    verify(snapshot)
    save(evidence / 'qualification.json', {'qualified': True,
        'snapshot_manifest_sha256': sha(snapshot / 'input-manifest.json'), 'command': command,
        'launcher': {'path': resolved_launcher, 'sha256': sha(resolved_launcher)},
        'exit_code': result.returncode, 'selected_runtime': runtime,
        'source_spec_sha256': sha(snapshot / 'configured/vpython.toml'),
        'source_lock_sha256': sha(snapshot / 'configured/vpython.toml.uv.lock'),
        'runtime_mode': 'Explicitly pinned external environment; not copied',
        'limitation': 'Interpreter/loaded DLL identities are pinned; the full external package/cache tree is not frozen.',
        'suite_execution_claimed': False})
    print(evidence / 'qualification.json')


def run(args):
    snapshot = args.snapshot.resolve()
    manifest = verify(snapshot)
    assert Path(__file__).resolve() == snapshot / manifest['runner_copy'], 'Run the copied conductor'
    qualification = load(args.qualification)
    assert qualification['qualified'] and qualification['snapshot_manifest_sha256'] == sha(snapshot / 'input-manifest.json')
    runtime = qualification['selected_runtime']
    def verify_python():
        for key in ['executable', 'python_dll', 'base_executable']:
            if key in runtime:
                assert sha(runtime[key]['path']) == runtime[key]['sha256']
    verify_python()
    evidence = args.evidence.resolve(); evidence.mkdir(parents=True, exist_ok=False)
    out = snapshot / 'configured/out/Default'
    command = [str(out / 'net_unittests.exe'), '--test-launcher-jobs=1', '--test-launcher-retry-limit=0',
               '--test-launcher-summary-output=' + str(evidence / 'summary.json'),
               '--gtest_output=json:' + str(evidence / 'gtest.json')]
    env = child_env(snapshot, manifest)
    temporary = evidence / 'tmp'; temporary.mkdir()
    env['TMP'] = env['TEMP'] = str(temporary)
    timed_out = False
    with (evidence / 'output.log').open('wb') as output:
        process = subprocess.Popen(command, cwd=out, env=env, stdout=output, stderr=subprocess.STDOUT)
        api = ctypes.WinDLL('kernel32', use_last_error=True)
        api.SetProcessAffinityMask.argtypes = [ctypes.c_void_p, ctypes.c_size_t]
        if not api.SetProcessAffinityMask(ctypes.c_void_p(int(process._handle)), ctypes.c_size_t(1 << args.cpu)):
            process.kill(); process.wait(); raise ctypes.WinError()
        try:
            code = process.wait(timeout=args.timeout)
        except subprocess.TimeoutExpired:
            timed_out = True; process.kill(); code = process.wait()
        except BaseException:
            process.kill(); process.wait(); raise
    verify(snapshot); verify_python()
    parser = module('copied_unit_parser', snapshot / 'context/unit_parser.py')
    record = {'command': command, 'exit_code': code, 'timed_out': timed_out,
        'manifest_sha256': sha(snapshot / 'input-manifest.json'), 'qualification_sha256': sha(args.qualification),
        'performance_measurement': False, 'browser_or_whole_goal_acceptance': False,
        'input_bytes_stable': True, 'cpu_requested': args.cpu,
        'python_environment': 'External interpreter and loaded DLL pinned; full package/cache tree not copied',
        'CR_SOURCE_ROOT': env['CR_SOURCE_ROOT'], 'child_environment_only': True}
    try:
        tests = parser.unit_results(evidence)
        record['test_results'] = tests
        record['passed'] = code == 0 and not timed_out and tests['passed'] > 0 and not tests['unexpected_statuses']
    except Exception as error:
        record.update(passed=False, result_parse_error=str(error))
    save(evidence / 'result.json', record)
    print(json.dumps(record, indent=2))
    raise SystemExit(0 if record['passed'] else 1)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    sub = parser.add_subparsers(dest='action', required=True)
    p = sub.add_parser('prepare'); p.add_argument('--snapshot', type=Path, required=True); p.add_argument('--freshness-record', type=Path, required=True); p.add_argument('--built-source-snapshot', type=Path); p.add_argument('--primitive-snapshot-manifest', type=Path)
    p = sub.add_parser('qualify'); p.add_argument('--snapshot', type=Path, required=True); p.add_argument('--evidence', type=Path, required=True)
    p = sub.add_parser('run'); p.add_argument('--snapshot', type=Path, required=True); p.add_argument('--qualification', type=Path, required=True); p.add_argument('--evidence', type=Path, required=True); p.add_argument('--cpu', type=int, default=2); p.add_argument('--timeout', type=int, default=3600)
    args = parser.parse_args()
    preparation_resources()
    if hasattr(args, 'cpu'):
        assert 0 <= args.cpu < min(os.cpu_count() or 1, 64)
    {'prepare': prepare, 'qualify': qualify, 'run': run}[args.action](args)


if __name__ == '__main__':
    main()
