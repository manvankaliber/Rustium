"""Parent ordinary validation proposal; no compiler or configured source writes."""
import argparse
import datetime
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import xml.etree.ElementTree as ET

ROOT = Path('D:/Rustium')
BUILD = Path('D:/RustiumBuild/src')
OUT = BUILD / 'out/Default'
OWNED = ROOT / '.scratch/base-ordinary-delivery'
FILTER = '*SplitString*'

def require(ok, message):
    if not ok:
        raise ValueError(message)

def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()

def module(name, path):
    spec = importlib.util.spec_from_file_location(name, path)
    result = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(result)
    return result

def inventory(directory):
    result = {}
    for p in directory.rglob('*'):
        require(not p.is_symlink() and not (hasattr(p, 'is_junction') and p.is_junction()), 'Linked snapshot member')
        result[p.relative_to(directory).as_posix()] = sha(p) if p.is_file() else 'directory'
    return result

def run(command, cwd, env, log):
    with log.open('wb') as stream:
        result = subprocess.run([str(x) for x in command], cwd=cwd, env=env, stdout=stream, stderr=subprocess.STDOUT)
    return result.returncode

def expected_membership(summary):
    # all_tests is the complete launcher enumeration, before the gtest filter.
    names = summary['all_tests']; disabled = summary['disabled_tests']
    require(names and len(names) == len(set(names)), 'Missing/duplicate test enumeration')
    require(len(disabled) == len(set(disabled)) and set(disabled) <= set(names), 'Disabled membership')
    selected = {n for n in names if 'SplitString' in n}
    expected = selected - set(disabled)
    require(expected, 'Empty selected suite')
    require(not any(n.split('.', 1)[-1].startswith('PRE_') for n in expected), 'PRE fixture qualification required')
    require(len(summary['per_iteration_data']) == 1, 'Exactly one iteration required')
    actual = summary['per_iteration_data'][0]
    require(set(actual) == expected, 'Missing or extra selected test attempts')
    require(all(len(attempts) == 1 and isinstance(attempts[0], dict) and attempts[0].get('status') for attempts in actual.values()), 'Repeated/missing attempt')
    return expected, len(selected & set(disabled))

def main(args):
    output = args.output.resolve()
    require(output.is_relative_to(OWNED.resolve()) and not output.exists(), 'Unique owned output required')
    require(not any(p.is_symlink() or (hasattr(p, 'is_junction') and p.is_junction()) for p in [output, *output.parents]), 'Linked output ancestor')
    info = json.loads(args.intake.read_text()); built = json.loads((Path(info['run_directory']) / 'build-result.json').read_text())
    require(built['passed'] and built['input_identity_before_after_matched'], 'Successful stable consumer build required')
    source_manifest = Path(info['source_manifest']); require(sha(source_manifest) == info['source_manifest_sha256'] == built['source_manifest_sha256'], 'Build source identity mismatch')
    sources = json.loads(source_manifest.read_text())
    def sources_unchanged():
        require(sha(source_manifest) == info['source_manifest_sha256'], 'Manifest drift')
        for rel, digest in sources.items():
            require(sha(ROOT / rel) == digest == sha(BUILD / rel), 'Source drift: ' + rel)
    sources_unchanged()
    # The complete original split tests must retain their independent source.
    original = ROOT / 'base/strings/string_split_unittest.cc'
    frozen = ROOT / 'docs/rust/migration/drafts/foundation-strings/reference/base/strings/string_split_unittest.cc'
    require(original.read_bytes() == frozen.read_bytes() == (BUILD / 'base/strings/string_split_unittest.cc').read_bytes(), 'Original assertion source changed')
    helper_path = ROOT / '.scratch/net-wave5/resumed-integration/native-runtime-delivery-v2/native_delivery.py'
    helper = module('ordinary_runtime_collector', helper_path)
    reused = helper.reused()
    registry_path = ROOT / 'tools/run_rust_rewrite_benchmarks.py'
    registry = module('ordinary_dll_registry', registry_path)
    helper_paths = [Path(__file__).resolve(), helper_path, registry_path, ROOT / '.scratch/net-wave4/regular-net-suite-prep/regular_suite.py', Path(reused.INSPECTOR)]
    helper_pins = {str(p): sha(p) for p in helper_paths}
    plan = {'cases': [{'target': 'base_unittests'}]}
    runtime = helper.collect_runtime(plan, reused.INSPECTOR, registry)
    output.mkdir(parents=True)
    command = [BUILD/'third_party/ninja/ninja.exe', '-C', OUT, '-n', 'base_unittests', 'net_unittests']
    require(run(command, BUILD, os.environ.copy(), output/'freshness.log') == 0 and 'no work to do' in (output/'freshness.log').read_text().lower(), 'Consumers not fresh')
    require(runtime == helper.collect_runtime(plan, reused.INSPECTOR, registry), 'Runtime drift across freshness')
    snapshot = output/'snapshot'; snapshot.mkdir()
    for filename, digest in runtime['files'].items():
        source = Path(filename); destination = snapshot/'build'/source.relative_to(BUILD)
        destination.parent.mkdir(parents=True, exist_ok=True); shutil.copyfile(source, destination)
        require(sha(destination) == digest and destination.stat().st_nlink == 1, 'Copy identity/hardlink')
    evidence = output/'execution'; evidence.mkdir()
    temporary = evidence/'t'; temporary.mkdir(); require(len(str(temporary)) <= 96, 'Temporary root too long')
    env = os.environ.copy()
    require(not any(k.startswith('GTEST_') or k in ['TEST_TOTAL_SHARDS','TEST_SHARD_INDEX','GTEST_SHARD_STATUS_FILE'] for k in env), 'Inherited test selector environment')
    env.update(TMP=str(temporary), TEMP=str(temporary), GTEST_COLOR='no')
    exe = snapshot/'build/out/Default/base_unittests.exe'
    command = [exe, '--gtest_filter='+FILTER, '--test-launcher-jobs=1', '--test-launcher-batch-limit=1', '--test-launcher-retry-limit=0', '--test-launcher-bot-mode', '--test-launcher-print-test-stdio=always', '--test-launcher-summary-output='+str(evidence/'summary.json'), '--gtest_output=xml:'+str(evidence/'gtest.xml')]
    sealed = inventory(snapshot)
    metadata = {'created_utc':datetime.datetime.now(datetime.timezone.utc).isoformat(),'source_manifest_sha256':info['source_manifest_sha256'],'runtime':runtime,'snapshot_inventory':sealed,'helpers':helper_pins,'original_test_sha256':sha(original),'command':[str(x) for x in command],'filter':FILTER,'temporary':str(temporary),'scope':'Selected original split family plus native bridge cases, no other suite/batch/performance acceptance','host_limits':'System DLLs and installed controller Python are host providers; no cross-platform proof'}
    (output/'input-manifest.json').write_text(json.dumps(metadata,indent=2)+'\n')
    code = run(command, evidence, env, evidence/'output.log')
    identity = inventory(snapshot) == sealed and helper.collect_runtime(plan, reused.INSPECTOR, registry) == runtime and all(sha(Path(p)) == h for p,h in helper_pins.items())
    sources_unchanged()
    summary = json.loads((evidence/'summary.json').read_text()); expected, disabled = expected_membership(summary)
    attempts = summary['per_iteration_data'][0]
    xml = ET.parse(evidence/'gtest.xml').getroot()
    cases = list(xml.iter('testcase')); names = [n.attrib['classname']+'.'+n.attrib['name'] for n in cases]
    require(len(names) == len(set(names)) and set(names) == expected, 'XML membership mismatch')
    skips = {name for name,node in zip(names,cases) if node.find('skipped') is not None}
    failures = {name: rows[0]['status'] for name,rows in attempts.items() if rows[0]['status'] != 'SUCCESS'}
    result = {'passed':code == 0 and identity and not failures,'exit_code':code,'identity_before_after_matched':identity,'expected_enabled':len(expected),'disabled':disabled,'passed_tests':len(expected)-len(skips)-len(failures),'native_skips':sorted(skips),'failures':failures,'finished_utc':datetime.datetime.now(datetime.timezone.utc).isoformat(),'snapshot_manifest_sha256':sha(output/'input-manifest.json'),'source_manifest_sha256':info['source_manifest_sha256']}
    (output/'result.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result));return 0 if result['passed'] else 1

if __name__ == '__main__':
    parser=argparse.ArgumentParser();parser.add_argument('--intake',type=Path,required=True);parser.add_argument('--output',type=Path,required=True)
    sys.exit(main(parser.parse_args()))
