"""Staged parent-only ordinary native delivery. No build or timing command.

plan/self-tests are safe before build. freshness/prepare/qualify/run are parent
steps after a completed successful build, with ABI consumers rebuilt first.
The old sealed conductor is imported and its copy engine extended at checked
anchors, rather than forked. No canonical or configured inputs are written.
"""
import argparse
import ast
import ctypes
import hashlib
import importlib.util
import inspect
import json
import os
from pathlib import Path
import re
import subprocess
import sys

sys.dont_write_bytecode = True
ROOT = Path('D:/Rustium')
BUILD = Path('D:/RustiumBuild/src')
OUT = BUILD / 'out/Default'
DELIVERY = ROOT / '.scratch/net-wave5/resumed-integration/native-runtime-delivery-v2'
OLD = ROOT / '.scratch/net-wave4/regular-net-suite-prep/regular_suite.py'
FAMILIES = ('http2', 'connection-jobs', 'dns-resolution', 'cookie-storage')
RUST_COUNTS = {'http2_family_rust_policy_unittests': 28,
    'connection_jobs_rust_core_unittests': 23, 'dns_resolution_policy_unittests': 36,
    'cookie_storage_family_rust_policy_unittests': 54}


def require(condition, message):
    if not condition:
        raise ValueError(message)


def sha(path):
    with Path(path).open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def load(path):
    return json.loads(Path(path).read_text(encoding='utf-8-sig'))


def owned(path):
    path = Path(path).resolve()
    require(path.is_relative_to(DELIVERY.resolve()), 'Output outside exclusive delivery directory')
    return path


def save_new(path, value):
    path = owned(path)
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open('x', encoding='utf-8') as stream:
        json.dump(value, stream, indent=2)
        stream.write('\n')


def import_module(name, path):
    spec = importlib.util.spec_from_file_location(name, path)
    result = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(result)
    return result


def reused():
    # The copied driver finds the independently copied original conductor.
    frozen = next((p for p in Path(__file__).resolve().parents
                   if (p / 'input-manifest.json').is_file()), None)
    path = frozen / 'context/reused_regular_suite.py' if frozen else OLD
    base = import_module('reused_regular_suite', path)
    base.original_module_path = path.resolve()
    base.__file__ = str(Path(__file__).resolve())
    return base


def bound(path):
    return {'path': str(Path(path).resolve()), 'sha256': sha(path)}


def checked_binding(row):
    require(sha(row['path']) == row['sha256'], 'Evidence changed: ' + row['path'])
    return Path(row['path'])


def gn_block(text, target):
    match = re.search(r'(?m)^\s*(?:test|executable)\("' + re.escape(target) + r'"\)\s*\{', text)
    require(match, 'Actual GN executable target absent: ' + target)
    # All selected top-level targets close at column zero. This is source
    # inventory only, never a replacement for evaluated GN/build freshness.
    end = text.index('\n}', match.end())
    return text[match.start():end]


def descriptor_cases(path):
    data = load(path)
    return [case['key'] for group in data['groups'] for case in group['cases']]


def array_cases(text):
    match = re.search(r'\bkCases\s*(?:=\s*)?\{([^;]+)\};', text)
    if match:
        return re.findall(r'"([^"\n]+)"', match[1])
    # The retained coalescer/ALPS benchmarks construct their fixture vectors.
    # Keep their reviewed case IDs explicit and assert each is in the source.
    if 'HeaderCoalescer' in text:
        ids = ['request', 'duplicates', 'cookies', 'large_value', 'obs_text',
               'invalid_name', 'uppercase', 'ordering', 'size_limit', 'invalid_value']
    elif 'AlpsDecoder' in text:
        ids = ['empty', 'settings_duplicates', 'accept_bytes', 'accept_many', 'malformed_tail',
               'disabled', 'forbidden', 'settings_ack', 'invalid_stream', 'partial_frame']
    else:
        raise ValueError('Cannot identify retained workload table')
    require(all('"' + key + '"' in text for key in ids), 'Retained fixtures drift')
    return ids


def make_plan():
    gn = (ROOT / 'net/BUILD.gn').read_text()
    dns_gn = (ROOT / 'net/dns/BUILD.gn').read_text()
    cases = [{'family': 'broad-net', 'target': 'net_unittests', 'label': '//net:net_unittests',
              'kind': 'gtest', 'arguments': []}]
    inputs = [bound(DELIVERY / 'cookie-clock-contract.json'), bound(OLD), bound(ROOT / 'net/BUILD.gn'), bound(ROOT / 'net/dns/BUILD.gn'),
              bound(ROOT / 'tools/run_rust_rewrite_benchmarks.py'),
              bound(ROOT / '.scratch/net-wave3/configured_validate.py')]
    for family in FAMILIES:
        latest = ROOT / '.scratch/net-wave5/subsystem-delivery' / family / 'LATEST-COMPOSITION.json'
        manifest = ROOT / load(latest)['path'] / 'complete-family-input.json'
        data = load(manifest)
        inputs.extend([bound(latest), bound(manifest)])
        descriptors = {row['target']: row for row in data['whole_family_benchmarks']}
        for label in data['regular_targets']:
            target = label.split(':')[1]
            row = {'family': family, 'target': target, 'label': label,
                   'kind': 'gtest', 'arguments': []}
            if target in RUST_COUNTS:
                source = dns_gn if label.startswith('//net/dns:') else gn
                require('unit_test_target = "' + target + '"' in source, 'Rust GN test target absent')
                row.update(kind='rust', arguments=['--test-threads=1'], expected_tests=RUST_COUNTS[target],
                           count_basis='Historical source-bound suite count; execution must confirm unchanged scope')
            else:
                block = gn_block(gn, target)
                if 'perftests' in target:
                    sources = re.findall(r'"([^"\n]*perftest\.cc)"', block)
                    require(len(sources) == 1, 'Require one actual verifier entrypoint: ' + target)
                    path = ROOT / 'net' / sources[0]
                    text = path.read_text()
                    require('--verify-only' in text, 'Clock-free option missing: ' + target)
                    row.update(kind='workload', arguments=['--verify-only'], entrypoint=bound(path),
                               measurement_clocks_requested=False)
                    inputs.append(bound(path))
                    if label in descriptors:
                        meta = descriptors[label]
                        desc = ROOT / meta['descriptor']
                        ids = descriptor_cases(desc)
                        require(len(ids) == meta['cases'] and len(ids) == len(set(ids)), 'Incomplete descriptor')
                        row.update(case_ids=ids, descriptor=bound(desc))
                        inputs.append(bound(desc))
                        if target == 'spdy_buffer_rust_perftests':
                            fragment = ROOT / '.scratch/net-wave5/spdy-buffer/support/runner-module-fragment.json'
                            registration = load(fragment)
                            require(registration['verification_arguments'] == ['--verify-only'] and
                                    registration['cases'] == ids, 'Buffer verifier/descriptor registration drift')
                            row['runner_module_fragment'] = bound(fragment)
                            inputs.append(bound(fragment))
                    else:
                        row['case_ids'] = array_cases(text)
                    row['expected_workloads'] = len(row['case_ids'])
                    row['completion'] = completion_contract(target)
                    row['clock_scope'] = 'No harness measurement clock; normal native/mock task clocks are not instrumented'
                    row['blockers'] = []
                    if target == 'cookie_storage_family_store_perftests':
                        row['blockers'] = cookie_clock_contract(text)
                    if target in {'dns_resolution_family_rust_perftests', 'dns_request_family_async_perftests', 'host_resolver_cache_rust_perftests'}:
                        row['metadata_correction'] = 'Actual source uses --benchmark for timing; only --verify-only is permitted here'
            cases.append(row)
    require(len({c['target'] for c in cases}) == len(cases), 'Duplicate target')
    return {'schema_version': 1, 'scope': 'Installed four-family ordinary native tests and every prepared paired workload; broad net suite included',
            'cases': cases, 'source_inventory': inputs, 'reused_conductor': bound(OLD),
            'native_tests_executed': False, 'performance_acceptance': False,
            'deferred': ['Sanitizer/fuzz/security campaigns remain off by user instruction',
                         'Performance sampling/publication', 'Broad services/content/Chrome runtime suites and platform gaps; parent evaluates ABI consumer rebuild closure',
                         'No source-only inventory proves configured correctness or baseline independence']}


def completion_contract(target):
    messages = {
        'spdy_buffer_rust_perftests': 'Verified 12 paired SpdyBuffer workloads; no timing.',
        'http2_stream_family_rust_perftests': 'Verified five isolated public stream-control paths; no timing',
        'http2_stream_family_io_perftests': 'Verified five complete scripted public HTTP2 paths; no timing',
        'http2_read_queue_rust_perftests': 'Verified seven public-API workloads; no timing.',
        'connect_job_rust_perftests': 'Verified15 complete ConnectJob BASE lifecycle workloads; no timing',
        'connection_jobs_family_rust_perftests': 'Verified sixteen full-path fixtures; no timing.',
        'connection_jobs_upper_rust_perftests': 'Verified21 upper-family public lifecycle workloads; no measurement clock reads',
        'connection_jobs_upper_rust_jobs_perftests': 'Verified16 connected job lifecycles; no measurement clock reads',
        'cookie_partition_key_rust_perftests': 'Prepared 15 public-API workload checksums match; no clock read.'}
    if target in messages:
        return {'method': 'exact-completion-message', 'message': messages[target]}
    if target in {'cookie_storage_family_perftests', 'cookie_storage_family_store_perftests'}:
        return {'method': 'json-cases', 'key': 'verify_case'}
    if target in {'http2_priority_rust_perftests', 'header_coalescer_rust_perftests', 'alps_decoder_rust_perftests'}:
        return {'method': 'json-cases', 'key': 'case', 'verified_required': True}
    require(target in {'http2_write_queue_rust_perftests', 'host_resolver_cache_rust_perftests',
                       'dns_resolution_family_rust_perftests', 'dns_request_family_async_perftests'},
            'No reviewed completion contract for ' + target)
    return {'method': 'silent-checked-loop',
            'limitation': 'Source-bound loop covers all cases with CHECK comparisons; executable emits no per-case completion proof. Exit zero alone cannot independently enumerate runtime case coverage.'}


def inventory_hash(value):
    return hashlib.sha256(json.dumps(value, sort_keys=True, separators=(',', ':')).encode()).hexdigest()


def select_plan(full, families):
    require(families and set(families) <= set(FAMILIES), 'Explicit complete families required')
    selected = [c for c in full['cases'] if c['family'] in set(families) | {'broad-net'}]
    omitted = [c for c in full['cases'] if c not in selected]
    excluded_paths = set()
    for c in omitted:
        for key in ['entrypoint', 'descriptor', 'runner_module_fragment']:
            if key in c: excluded_paths.add(c[key]['path'])
    inputs = [r for r in full['source_inventory'] if r['path'] not in excluded_paths and
              not any('/subsystem-delivery/' + f + '/' in r['path'].replace('\\', '/')
                      for f in FAMILIES if f not in families)]
    return {**full, 'cases': selected, 'source_inventory': inputs,
            'full_inventory': full, 'full_inventory_sha256': inventory_hash(full),
            'selected_families': sorted(set(families)), 'deferred_cases': omitted,
            'whole_goal_acceptance': False, 'whole_batch_acceptance': False,
            'publication_gate_changed': False}


def validate_plan(plan):
    full = plan['full_inventory']
    require(inventory_hash(full) == plan['full_inventory_sha256'], 'Immutable inventory drift')
    require(plan == select_plan(full, plan['selected_families']), 'Selection drift or incomplete family')
    require(len(full['cases']) == 36 and sum(c.get('expected_workloads', 0) for c in full['cases']) == 232,
            'Full inventory scope changed; parent review required')
    for item in plan['source_inventory']: checked_binding(item)
    require(sha(OLD) == plan['reused_conductor']['sha256'], 'Original conductor drift')
    for row in plan['cases']:
        if row['kind'] == 'workload':
            require(row['arguments'] == ['--verify-only'], 'Timing option forbidden')
            if row['target'] == 'cookie_storage_family_store_perftests':
                require(row['blockers'] == cookie_clock_contract(Path(row['entrypoint']['path']).read_text()),
                        'Cookie clock reachability contract drift')


def cookie_clock_contract(text):
    # Exact reviewed setup bytes, not a blanket token exemption: Sample clocks
    # remain present and must stay behind the existing early-return branch.
    contract = load(DELIVERY / 'cookie-clock-contract.json')
    if hashlib.sha256(text.replace('\r\n', '\n').encode()).hexdigest() == contract['staged_normalized_sha256']:
        return []
    return ['Unreviewed cookie business-clock setup: apply staged fixture patch, rebuild and refreeze; duration clocks are allowed only behind timing branch']


def under(root, relative):
    require(not Path(relative).is_absolute(), 'Expected relative path')
    result = (root / relative).resolve()
    require(result.is_relative_to(root.resolve()), 'Path escapes root: ' + str(relative))
    return result


def collect_runtime(plan, inspector, registry):
    """Same recursive adjacent-import helper as canonical configured runner."""
    files, dirs, closures = set(), {}, {}
    def add(path):
        path = path.resolve()
        require(path.is_relative_to(BUILD.resolve()), 'Unclassified runtime path: ' + str(path))
        require(path.exists() and not path.is_symlink(), 'Missing/symlink runtime input: ' + str(path))
        if path.is_dir():
            children = list(path.rglob('*'))
            require(not any(p.is_symlink() or (hasattr(p, 'is_junction') and p.is_junction()) for p in [path, *children]), 'Linked runtime directory')
            dirs[str(path)] = sorted(p.relative_to(path).as_posix() + ('/' if p.is_dir() else '') for p in children)
            files.update(p.resolve() for p in children if p.is_file())
        else:
            files.add(path)
    for case in plan['cases']:
        exe = OUT / (case['target'] + '.exe')
        deps = OUT / (case['target'] + '.runtime_deps')
        add(exe); add(deps)
        lines = [line.strip() for line in deps.read_text().splitlines() if line.strip()]
        require(lines, 'Empty runtime_deps: ' + str(deps))
        for line in lines:
            require(not Path(line).is_absolute(), 'Absolute runtime_deps path')
            add(OUT / line)
        adjacent, external = registry.local_dll_dependencies(exe, inspector)
        for name, digest in adjacent.items():
            add(OUT / name)
            require(sha(OUT / name) == digest, 'DLL drift during inspection')
        closures[case['target']] = {'executable_sha256': sha(exe), 'runtime_deps_sha256': sha(deps),
            'adjacent_dlls': adjacent, 'system_or_nonadjacent_import_names': external}
    return {'files': {str(p): sha(p) for p in sorted(files)}, 'directories': dirs, 'targets': closures}


def check_sources(manifest_path, source_snapshot):
    state = load(manifest_path)
    require(state and all(isinstance(v, str) and re.fullmatch('[0-9a-f]{64}', v) for v in state.values()), 'Require complete flat synchronized source digest map')
    require({'net/BUILD.gn', 'net/dns/BUILD.gn'} <= set(state), 'Both GN owners required')
    for rel, digest in state.items():
        require(sha(under(BUILD, rel)) == digest, 'Configured source drift: ' + rel)
        require(sha(under(source_snapshot, rel)) == digest, 'Built source snapshot mismatch: ' + rel)
    return state


def config_files():
    # Preserve evaluated Ninja graph, not only a source-only GN descriptor.
    pending = [OUT / 'build.ninja', OUT / 'toolchain.ninja']
    files = {OUT / 'args.gn', OUT / 'environment.x64'}
    while pending:
        path = pending.pop().resolve()
        if path in files:
            continue
        require(path.is_relative_to(OUT.resolve()), 'Ninja include escapes configured output')
        text = path.read_text(errors='replace')
        files.add(path)
        for match in re.finditer(r'(?m)^\s*(?:subninja|include)\s+(.+)$', text):
            name = match[1].strip()
            require('$' not in name, 'Unresolved Ninja include; parent must extend config closure explicitly')
            pending.append(OUT / name)
    require((OUT / 'gen/net').is_dir(), 'Generated net configuration missing')
    files.update(p for p in (OUT / 'gen/net').rglob('*') if p.is_file())
    for pattern in ['*.rsp', '*.rsp.rust', '*.rustflags.json']:
        files.update((OUT / 'obj/net').rglob(pattern))
    return {str(p.resolve()): sha(p) for p in sorted(files)}


def successful_records(paths, required, source_hash):
    covered = set()
    rows = []
    for path in paths:
        record = load(path)
        require(record.get('exit_code') == 0 and record.get('finished_utc') and
                record.get('source_inputs_stable') is True and not record.get('source_drift'), 'Completed successful stable build record required')
        require(record['source_manifest_sha256'] == source_hash and record['configured_args_sha256'] == sha(OUT / 'args.gn'), 'Build record source/config mismatch')
        command = record['command']
        require('-n' not in command and Path(command[0]).resolve() == (BUILD / 'third_party/ninja/ninja.exe').resolve(), 'Actual configured Ninja build required')
        require('-C' in command, 'Configured output missing')
        cwd = Path(record.get('cwd', BUILD))
        require((cwd / command[command.index('-C') + 1]).resolve() == OUT.resolve(), 'Wrong configured output')
        log = Path(record.get('output_log', Path(path).parent / 'build.log'))
        require(log.is_file() and log.stat().st_size > 0, 'Actual build log required')
        covered.update(command)
        rows.extend([bound(path), bound(log)])
    require(set(required) <= covered, 'Missing successful target/ABI consumer build evidence: ' + str(sorted(set(required) - covered)))
    return rows


def freshness(args):
    require(args.parent_authorized_after_build, 'Parent must explicitly authorize after verified successful build')
    plan = load(args.plan); validate_plan(plan)
    state = check_sources(args.source_manifest, args.built_source_snapshot)
    for item in plan['source_inventory']:
        path = Path(item['path']).resolve()
        if path.is_relative_to(ROOT.resolve()):
            rel = path.relative_to(ROOT.resolve()).as_posix()
            if rel in state:
                require(item['sha256'] == state[rel], 'Plan describes a different built source identity: ' + rel)
    # Source inventory alone is insufficient: every entrypoint and declared GN
    # source in selected unit/workload blocks must be in the bound build inputs.
    for case in plan['cases']:
        if case['kind'] == 'rust' or case['target'] == 'net_unittests':
            continue
        block = gn_block((ROOT / 'net/BUILD.gn').read_text(), case['target'])
        for rel in re.findall(r'"([^"\n]+\.(?:cc|h|rs))"', block):
            require('net/' + rel in state, 'Source manifest omits selected native input: net/' + rel)
    abi = load(args.abi_consumer_record)
    require(abi.get('reviewed') is True and abi.get('source_manifest_sha256') == sha(args.source_manifest), 'Current evaluated ABI consumer review required')
    require(abi.get('targets') and abi.get('refs_evidence'), 'ABI consumer target closure and evaluated graph evidence required')
    for item in abi['refs_evidence']:
        checked_binding(item)
    targets = [case['label'].removeprefix('//') for case in plan['cases']]
    targets = sorted(set(targets + abi['targets']))
    records = successful_records(args.build_record, targets, sha(args.source_manifest))
    gn = load(args.gn_record)
    require(gn.get('all_exit_zero') is True and gn['source_manifest_sha256'] == sha(args.source_manifest) and
            gn['configured_args_sha256'] == sha(OUT / 'args.gn'), 'Fresh successful configured GN/check evidence required')
    base = reused()
    registry = import_module('dll_registry', ROOT / 'tools/run_rust_rewrite_benchmarks.py')
    before = collect_runtime(plan, base.INSPECTOR, registry)
    config = config_files()
    command = [str(BUILD / 'third_party/ninja/ninja.exe'), '-C', str(OUT), '-n', *targets]
    output = owned(args.output); require(not output.exists(), 'Choose new freshness output')
    output.mkdir(parents=True)
    # This parent-invoked step only asks for freshness. It never rebuilds.
    result = subprocess.run(command, cwd=BUILD, capture_output=True, text=True)
    log = output / 'up-to-date.log'; log.write_text(result.stdout + result.stderr)
    require(result.returncode == 0 and 'no work to do' in result.stdout.lower(), 'Every target must be fresh; retain dry-run evidence')
    require(before == collect_runtime(plan, base.INSPECTOR, registry), 'Runtime changed across dry-run')
    require(config == config_files(), 'Configuration changed across dry-run')
    check_sources(args.source_manifest, args.built_source_snapshot)
    proof = {'exit_code': 0, 'command': command, 'cwd': str(BUILD), 'output_log': str(log),
        'source_manifest_sha256': sha(args.source_manifest), 'configured_args_sha256': sha(OUT / 'args.gn'),
        'executable_sha256': sha(OUT / 'net_unittests.exe'), 'runtime_identity': before, 'configuration_files': config,
        'plan': bound(args.plan), 'source_manifest': bound(args.source_manifest), 'built_source_snapshot': str(args.built_source_snapshot.resolve()),
        'provenance': [*records, bound(args.gn_record), bound(args.abi_consumer_record), *abi['refs_evidence'], bound(log)],
        'all_selected_targets_fresh': True, 'abi_consumers_rebuilt': True, 'test_execution': False}
    save_new(output / 'freshness.json', proof)


def check_freshness(args):
    proof = load(args.freshness_record)
    require(proof['all_selected_targets_fresh'] is True and proof['abi_consumers_rebuilt'] is True, 'Full target freshness/ABI rebuild evidence required')
    plan = load(checked_binding(proof['plan'])); validate_plan(plan)
    checked_binding(proof['source_manifest'])
    for row in proof['provenance']:
        checked_binding(row)
    require(proof['exit_code'] == 0 and '-n' in proof['command'] and 'no work to do' in Path(proof['output_log']).read_text().lower(), 'Freshness dry-run did not pass')
    require(proof['source_manifest_sha256'] == sha(args.source_manifest), 'Source manifest differs from freshness proof')
    require(Path(proof['built_source_snapshot']).resolve() == args.built_source_snapshot.resolve(), 'Built source identity mismatch')
    check_sources(args.source_manifest, args.built_source_snapshot)
    base = reused(); registry = import_module('dll_registry', ROOT / 'tools/run_rust_rewrite_benchmarks.py')
    require(proof['runtime_identity'] == collect_runtime(plan, base.INSPECTOR, registry), 'Runtime drift since freshness proof')
    require(proof['configuration_files'] == config_files(), 'Configuration drift since freshness proof')
    return proof


def extended_prepare(args):
    """Reuse the sealed prepare body; replace only its one-target seams."""
    owned(args.snapshot)
    base = reused()
    proof = check_freshness(args)
    base.STATE = args.source_manifest.resolve()
    base.ENV_PLAN = args.environment_plan.resolve()
    base.FIXTURE_REPORT = args.fixture_report.resolve()
    source = extended_prepare_source(base)
    # AST-compile only the prepare function; no main or subprocess is executed.
    namespace = base.__dict__
    namespace.update(check_extended_freshness=check_freshness,
                     original_conductor=base.original_module_path)
    exec(compile(ast.parse(source), '<checked-regular-suite-extension>', 'exec'), namespace)
    base.prepare(args)
    # Input originals may change while large independent tool/data copies are
    # made. Recheck the complete runtime and generated configuration afterward.
    check_freshness(args)
    save_new(args.snapshot / 'freeze-complete.json', {
        'input_manifest_sha256': sha(args.snapshot / 'input-manifest.json'),
        'freshness_record_sha256': sha(args.freshness_record),
        'test_execution': False, 'before_after_originals_and_copies_matched': True})


def extended_prepare_source(base):
    """Return a narrowly transformed function, with every original anchor checked."""
    source = inspect.getsource(base.prepare)
    def replace(old, new):
        nonlocal source
        require(source.count(old) == 1, 'Reused conductor anchor drift: ' + old[:80])
        source = source.replace(old, new)
    replace("proof = load(args.freshness_record)", "proof = check_extended_freshness(args)")
    replace("assert proof['exit_code'] == 0 and '-n' in proof['command'] and 'net_unittests' in proof['command']",
            "assert proof['exit_code'] == 0 and '-n' in proof['command'] and proof['all_selected_targets_fresh']")
    start = source.index("    runtime_list = OUT / 'net_unittests.runtime_deps'")
    end = source.index("    for rel in ['out/Default/args.gn'", start)
    source = source[:start] + '''    runtime_list = OUT / 'net_unittests.runtime_deps'
    for original in proof['runtime_identity']['files']:
        add(Path(original))
    for original in proof['runtime_identity']['directories']:
        add(Path(original))
    adjacent = {target: row['adjacent_dlls'] for target, row in proof['runtime_identity']['targets'].items()}
    system = sorted({name for row in proof['runtime_identity']['targets'].values() for name in row['system_or_nonadjacent_import_names']})
    for original in proof['configuration_files']:
        add(Path(original))
    for item in proof['provenance'] + [proof['plan']]:
        add(Path(item['path']))
    add(original_conductor)
    add(Path(__file__).resolve().parent / 'cookie-clock-contract.json')
    copy_overrides[original_conductor] = Path('context/reused_regular_suite.py')
    for item in load(proof['plan']['path'])['source_inventory']:
        add(Path(item['path']))
    add(BUILD / 'vpython.toml')
    add(BUILD / 'vpython.toml.uv.lock')
    add(BUILD / 'buildtools/win/gn.exe')
    add(OUT / 'gen/net')
    # Generated compile flags and response files are retained when present;
    # absence is not reconstructed from a guessed toolchain command line.
    for pattern in ['*.rsp', '*.rsp.rust', '*.rustflags.json']:
        for path in (OUT / 'obj/net').rglob(pattern):
            add(path)
''' + source[end:]
    replace("    if args.built_source_snapshot:\n        add(args.built_source_snapshot.parent / 'source-snapshot-sha256.json')", "    # Exact source manifest is already copied; no old campaign-specific sidecar required.")
    replace("    manifest = {'scope': 'Ordinary complete net_unittests native/source/data snapshot',", "    manifest = {'scope': 'Ordinary four-family native tests and all paired workload verifier snapshot',\n        'delivery_plan_copy': mapped(Path(proof['plan']['path']).resolve()).as_posix(),\n        'target_runtime_identity': proof['runtime_identity']['targets'],\n        'configuration_identity': proof['configuration_files'],\n        'build_and_abi_evidence': proof['provenance'],")
    return source


def complete_snapshot(snapshot):
    complete = load(snapshot / 'freeze-complete.json')
    require(complete['input_manifest_sha256'] == sha(snapshot / 'input-manifest.json') and
            complete['before_after_originals_and_copies_matched'] is True, 'Freeze never completed or manifest drifted')


def workload_results(case, text):
    rows = []
    for line in text.splitlines():
        if line.lstrip().startswith('{'):
            row = json.loads(line)
            require(not any(key.endswith('_ns') or key in {'repeat', 'pair'} for key in row), 'Timing output in verifier')
            rows.append(row)
    contract = case['completion']
    if contract['method'] == 'exact-completion-message':
        require(contract['message'] in text, 'Verifier completion message missing')
    elif contract['method'] == 'json-cases':
        records = [row for row in rows if contract['key'] in row]
        ids = [row[contract['key']] for row in records]
        require(sorted(ids) == sorted(case['case_ids']), 'Missing/duplicate/extra workload verification')
        if contract.get('verified_required'):
            require(all(row.get('verified') is True for row in records), 'Explicit verified:true required')
    return {'expected_workloads': case['expected_workloads'], 'case_ids': case['case_ids'],
            'completion_contract': contract, 'json_records': rows,
            'measurement_output_absent': True, 'runtime_case_enumeration': contract['method'] == 'json-cases'}


def execution_temporary_root(snapshot, evidence):
    """Use the fresh, director-owned evidence directory under the existing trust contract."""
    snapshot = Path(snapshot).resolve()
    evidence = Path(evidence).resolve()
    require(not evidence.is_relative_to(snapshot) and not snapshot.is_relative_to(evidence),
            'Temporary evidence directory overlaps sealed snapshot')
    temporary = evidence / 'tmp'
    require(len(str(temporary)) <= 96, 'Use a short unique execution evidence path (TMP maximum 96 characters)')
    require(evidence.is_dir(), 'Fresh evidence directory must already exist')
    temporary.mkdir()
    require(temporary.resolve() == temporary and not temporary.resolve().is_relative_to(snapshot),
            'Temporary directory mapping changed during allocation')
    return temporary


REQUIRED_RECOVERY_TESTS = {
    'CookieMonsterTest.GetKey',
    'CookieMonsterLegacyScopeTest.DeleteAllAliasCookies',
    'SQLitePersistentCookieStoreTest.LoadingPartitionedCookies',
    'SQLitePersistentCookieStoreTest.SourcePortIsPersistent',
    'SQLitePersistentCookieStoreTest.TestInvalidVersionRecovery',
    'SQLitePersistentCookieStoreTest.TestLoadOldSessionCookies',
    'SQLitePersistentCookieStoreTest_OriginBoundCookies.DeleteCookie',
    'SQLitePersistentCookieStorev24UpgradeTest.UpgradeToSchemaVersion24/0',
    'SQLitePersistentCookieStorev24UpgradeTest.UpgradeToSchemaVersion24/4',
    'FileOperationsInitWithNVSDirTest.InitSucceedsWhenUnneededFilesUndeletable',
    'FileOperations/FileOperationsTempDeleteTest.TempSnapshotUndeletable/LegacySubdirectory',
    'SpdySessionTest.TestYieldingSlowReads',
    'SpdySessionTest.TestYieldingDuringReadData',
    'SpdySessionTest.TestYieldingDuringAsyncReadData',
}


def gtest_membership(folder, target):
    """The launcher's all_tests is captured before its run filters/sharding."""
    data = load(Path(folder) / 'summary.json')
    listed = data.get('all_tests')
    disabled = data.get('disabled_tests')
    iterations = data.get('per_iteration_data')
    require(isinstance(listed, list) and listed and all(isinstance(x, str) for x in listed),
            'Missing complete launcher test enumeration')
    require(len(set(listed)) == len(listed), 'Duplicate listed tests')
    require(isinstance(disabled, list) and all(isinstance(x, str) for x in disabled),
            'Missing disabled-test enumeration')
    require(len(set(disabled)) == len(disabled) and set(disabled) <= set(listed),
            'Invalid disabled-test membership')
    require(isinstance(iterations, list) and len(iterations) == 1 and isinstance(iterations[0], dict),
            'Expected exactly one complete launcher iteration')
    expected = set(listed) - set(disabled)
    require(not any(name.split('.', 1)[-1].startswith('PRE_') for name in expected),
            'PRE suites require independent execution proof before this membership contract can be used')
    actual = set(iterations[0])
    require(actual == expected, 'Missing or extra enabled test attempts: ' + repr({
        'missing': sorted(expected - actual), 'extra': sorted(actual - expected)}))
    require(all(isinstance(attempts, list) and len(attempts) == 1 and
                isinstance(attempts[0], dict) and attempts[0].get('status')
                for attempts in iterations[0].values()), 'Missing or repeated test attempts')
    if target == 'net_unittests':
        require(REQUIRED_RECOVERY_TESTS <= expected,
                'Recovery regression tests omitted or disabled: ' + repr(sorted(REQUIRED_RECOVERY_TESTS - expected)))
    # Status failures and native skips remain the independent parser's concern.
    # Native skips are recorded explicitly and never counted as passed tests.
    return {'expected_enabled': len(expected), 'observed_attempts': len(actual),
            'disabled': len(disabled), 'complete': True,
            'required_recovery_cases': sorted(REQUIRED_RECOVERY_TESTS) if target == 'net_unittests' else []}


def run(args):
    base = reused(); snapshot = args.snapshot.resolve()
    complete_snapshot(snapshot)
    manifest = base.verify(snapshot)
    require(Path(__file__).resolve() == snapshot / manifest['runner_copy'], 'Run copied conductor')
    plan = load(snapshot / manifest['delivery_plan_copy'])
    qualification = load(args.qualification)
    require(qualification['qualified'] and qualification['snapshot_manifest_sha256'] == sha(snapshot / 'input-manifest.json'), 'External Python qualification required')
    def identity():
        base.verify(snapshot)
        for key in ['executable', 'python_dll', 'base_executable']:
            item = qualification['selected_runtime'].get(key)
            if item:
                checked_binding(item)
    identity()
    parser = import_module('copied_unit_parser', snapshot / 'context/unit_parser.py')
    # Rust parser is pure and copied out of the already archived validation
    # source; no live production helper or configured build execution involved.
    validator = snapshot / 'editing/.scratch/net-wave3/configured_validate.py'
    node = next(n for n in ast.parse(validator.read_text()).body if isinstance(n, ast.FunctionDef) and n.name == 'rust_unit_results')
    ns = {'re': re, 'hashlib': hashlib}
    exec(compile(ast.Module(body=[node], type_ignores=[]), str(validator), 'exec'), ns)
    evidence = owned(args.evidence)
    require(not evidence.is_relative_to(snapshot), 'Execution output must be outside sealed snapshot')
    evidence.mkdir(parents=True, exist_ok=False)
    env = base.child_env(snapshot, manifest)
    temporary = execution_temporary_root(snapshot, evidence)
    env['TMP'] = env['TEMP'] = str(temporary)
    save_new(evidence / 'execution-environment.json', {
        'TMP': env['TMP'], 'TEMP': env['TEMP'], 'temporary_parent': str(evidence),
        'namespace_contract': 'Fresh director-owned evidence namespace remains stable; no adversarial namespace/ACL isolation claimed',
        'outside_sealed_snapshot': True, 'retained_after_execution': True})
    for key in ('GTEST_FILTER', 'GTEST_TOTAL_SHARDS', 'GTEST_SHARD_INDEX', 'GTEST_REPEAT', 'GTEST_ALSO_RUN_DISABLED_TESTS', 'GTEST_FLAGFILE', 'GTEST_BRIEF'):
        require(not env.get(key), 'Test membership environment override: ' + key)
    out = snapshot / 'configured/out/Default'
    records = []
    api = ctypes.WinDLL('kernel32', use_last_error=True)
    api.SetProcessAffinityMask.argtypes = [ctypes.c_void_p, ctypes.c_size_t]
    for case in plan['cases']:
        folder = evidence / case['target']; folder.mkdir()
        command = [str(out / (case['target'] + '.exe')), *case['arguments']]
        if case['kind'] == 'gtest':
            command += ['--test-launcher-jobs=1', '--test-launcher-retry-limit=0', '--test-launcher-print-test-stdio=always',
                        '--test-launcher-summary-output=' + str(folder / 'summary.json'),
                        '--gtest_output=json:' + str(folder / 'gtest.json')]
        record = {'command': command, 'kind': case['kind'], 'family': case['family'],
            'manifest_sha256': sha(snapshot / 'input-manifest.json'), 'qualification_sha256': sha(args.qualification),
            'performance_measurement': False, 'whole_goal_acceptance': False, 'cpu_requested': args.cpu,
            'executed': False, 'temporary_directory': str(temporary)}
        if case.get('blockers'):
            record.update(passed=False, execution_blockers=case['blockers'],
                follow_up='Parent fixes source clock path, rebuilds, refreshes plan/freshness and freezes a new identity')
            save_new(folder / 'result.json', record); records.append(record)
            continue
        try:
            identity(); record['input_identity_before'] = 'matched frozen manifest and external Python'
            with (folder / 'output.log').open('wb') as log:
                process = subprocess.Popen(command, cwd=out, env=env, stdout=log, stderr=subprocess.STDOUT)
                record['executed'] = True
                try:
                    require(api.SetProcessAffinityMask(ctypes.c_void_p(int(process._handle)), 1 << args.cpu), 'Cannot set child affinity')
                    record['exit_code'] = process.wait(timeout=args.timeout)
                except subprocess.TimeoutExpired:
                    process.kill(); record.update(exit_code=process.wait(), timed_out=True)
                except BaseException:
                    process.kill(); process.wait(); raise
            text = (folder / 'output.log').read_text(errors='replace')
            if case['kind'] == 'gtest':
                result = parser.unit_results(folder)
                result['membership'] = gtest_membership(folder, case['target'])
            elif case['kind'] == 'rust':
                result = ns['rust_unit_results'](text, case['expected_tests'])
            else:
                result = workload_results(case, text)
            record.update(results=result, passed=record['exit_code'] == 0 and not record.get('timed_out') and
                          (case['kind'] == 'workload' or (result['passed'] > 0 and not result['unexpected_statuses'])))
        except Exception as error:
            record.update(passed=False, error=repr(error))
        try:
            identity(); record['input_identity_after'] = 'matched before identity'
        except Exception as error:
            record.update(passed=False, identity_error=repr(error))
        save_new(folder / 'result.json', record); records.append(record)
        save_new(evidence / ('progress-' + str(len(records)) + '.json'), {'records': records})
        if record.get('identity_error'):
            break
    save_new(evidence / 'result.json', {'records': records, 'all_selected_executed': len(records) == len(plan['cases']) and all(r['executed'] for r in records),
        'passed': len(records) == len(plan['cases']) and all(r['passed'] for r in records),
        'manifest_sha256': sha(snapshot / 'input-manifest.json'), 'deferred': plan['deferred'],
        'whole_batch_acceptance': False, 'full_inventory_sha256': plan['full_inventory_sha256'],
        'selected_families': plan['selected_families'], 'deferred_cases': plan['deferred_cases'],
        'performance_or_whole_goal_acceptance': False})
    raise SystemExit(0 if len(records) == len(plan['cases']) and all(r['passed'] for r in records) else 1)


def main():
    require(__debug__, 'Do not use -O; reused conductor uses assertions')
    p = argparse.ArgumentParser(description=__doc__)
    sub = p.add_subparsers(dest='action', required=True)
    s = sub.add_parser('plan'); s.add_argument('--output', type=Path, required=True)
    s.add_argument('--family', choices=FAMILIES, action='append', required=True)
    s = sub.add_parser('freshness')
    for arg in ['plan', 'source-manifest', 'built-source-snapshot', 'abi-consumer-record', 'gn-record', 'output']:
        s.add_argument('--' + arg, type=Path, required=True)
    s.add_argument('--build-record', type=Path, action='append', required=True)
    s.add_argument('--parent-authorized-after-build', action='store_true')
    s = sub.add_parser('prepare')
    for arg in ['snapshot', 'freshness-record', 'source-manifest', 'built-source-snapshot']:
        s.add_argument('--' + arg, type=Path, required=True)
    s.add_argument('--primitive-snapshot-manifest', type=Path)
    s.add_argument('--environment-plan', type=Path, default=reused().ENV_PLAN)
    s.add_argument('--fixture-report', type=Path, default=reused().FIXTURE_REPORT)
    s = sub.add_parser('qualify')
    for arg in ['snapshot', 'evidence']:
        s.add_argument('--' + arg, type=Path, required=True)
    s = sub.add_parser('run')
    for arg in ['snapshot', 'qualification', 'evidence']:
        s.add_argument('--' + arg, type=Path, required=True)
    s.add_argument('--cpu', type=int, default=2); s.add_argument('--timeout', type=int, default=3600)
    args = p.parse_args()
    if args.action == 'plan':
        save_new(args.output, select_plan(make_plan(), args.family)); return
    require(os.name == 'nt', 'Windows configured runtime only')
    base = reused(); base.preparation_resources()
    if hasattr(args, 'cpu'):
        require(0 <= args.cpu < min(os.cpu_count() or 1, 64), 'Invalid CPU')
    if args.action == 'qualify':
        complete_snapshot(args.snapshot.resolve())
        require(not args.evidence.resolve().is_relative_to(args.snapshot.resolve()), 'Qualification output must be outside sealed snapshot')
        owned(args.evidence); base.qualify(args)
    else:
        {'freshness': freshness, 'prepare': extended_prepare, 'run': run}[args.action](args)


if __name__ == '__main__':
    main()
