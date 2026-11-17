"""Require retained ordinary-run provenance before comparing harness records.

The director must obtain the provenance SHA-256 from trusted gate evidence.
This verifies its binding chain; it does not authenticate arbitrary caller files
or replace the native conductor's execution and independent review.
"""
import argparse
import hashlib
import json
import math
from pathlib import Path

import schema


def require(condition, message):
    schema.require(condition, message)


def sha(path):
    with Path(path).open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def read(path):
    value = schema.parse(Path(path).read_text(encoding='utf-8'))
    def finite(item):
        if isinstance(item, float):
            require(math.isfinite(item), 'non-finite JSON number')
        elif isinstance(item, dict):
            for child in item.values(): finite(child)
        elif isinstance(item, list):
            for child in item: finite(child)
    finite(value)
    return value


def bound(row):
    path = Path(row['path']).resolve()
    schema.pinned(path, row['sha256'])
    return path


def inside(root, relative):
    path = (root / relative).resolve()
    require(path.is_relative_to(root), 'snapshot path escapes root')
    return path


def verify_snapshot(root, digest):
    root = Path(root).resolve()
    schema.pinned(root / 'input-manifest.json', digest)
    manifest = read(root / 'input-manifest.json')
    require(manifest['copied_with_hardlinks'] is False, 'mutable-output hardlinks are not a sealed snapshot')
    frozen = read(root / 'freeze-complete.json')
    require(frozen['input_manifest_sha256'] == digest
            and frozen['before_after_originals_and_copies_matched'] is True,
            'snapshot freeze not completed')
    copied = {}
    for row in manifest['files']:
        require(row['copy'] not in copied, 'duplicate copied file')
        path = inside(root, row['copy'])
        require(sha(path) == row['sha256'], 'copied file changed: ' + row['copy'])
        copied[row['copy']] = row
    require(copied, 'empty snapshot')
    for row in manifest['directories']:
        path = inside(root, row['copy'])
        require(path.is_dir(), 'missing copied directory')
        require(sorted(p.relative_to(path).as_posix() for p in path.rglob('*') if p.is_file()) == row['files'],
                'copied data membership changed')
        require(sorted(p.relative_to(path).as_posix() for p in path.rglob('*') if p.is_dir()) == row['subdirectories'],
                'copied directory membership changed')
    for row in manifest['external_python_inputs']:
        bound(row)
    proof_rows = [row for row in copied.values() if row['sha256'] == manifest['freshness_record_sha256']]
    require(len(proof_rows) == 1, 'unique copied freshness proof missing')
    proof = read(inside(root, proof_rows[0]['copy']))
    require(type(proof['exit_code']) is int and proof['exit_code'] == 0 and proof['all_selected_targets_fresh'] is True
            and proof['abi_consumers_rebuilt'] is True
            and proof['source_manifest_sha256'] == manifest['source_manifest_sha256'],
            'successful current build/ABI freshness proof missing')
    for row in manifest['build_and_abi_evidence']:
        require(any(x['sha256'] == row['sha256']
                    and Path(x['original']).resolve() == Path(row['path']).resolve()
                    for x in copied.values()), 'build/ABI evidence not copied')
    require(manifest['build_and_abi_evidence'], 'empty build/ABI provenance')
    return manifest, proof, copied


def capture(row, expected_records, contract, harness, expected_sources):
    records_path = bound(row['records'])
    require(records_path == Path(expected_records).resolve(), 'record file differs from capture binding')
    root = Path(row['snapshot']).resolve()
    manifest, freshness, copied = verify_snapshot(root, row['snapshot_manifest_sha256'])
    result = read(bound(row['execution_result']))
    qualification = read(bound(row['qualification']))
    require(qualification['qualified'] is True
            and qualification['snapshot_manifest_sha256'] == row['snapshot_manifest_sha256']
            and result['qualification_sha256'] == row['qualification']['sha256'], 'qualification mismatch')
    require(qualification['selected_runtime'], 'qualified external Python identity missing')
    for key in ('executable', 'python_dll', 'base_executable'):
        selected = qualification['selected_runtime'].get(key)
        if selected: bound(selected)
    require(result['manifest_sha256'] == row['snapshot_manifest_sha256']
            and result['passed'] is True and result['executed'] is True
            and type(result['exit_code']) is int and result['exit_code'] == 0
            and not result.get('timed_out') and not result.get('error') and not result.get('identity_error')
            and result['kind'] == 'workload' and result['performance_measurement'] is False
            and result['input_identity_before'] == 'matched frozen manifest and external Python'
            and result['input_identity_after'] == 'matched before identity', 'successful stable ordinary run missing')
    target = harness.replace('cookie_family_store', 'cookie_storage_family_store').replace('cookie_family', 'cookie_storage_family') + '_perftests'
    executable = inside(root, 'configured/out/Default/' + target + '.exe')
    require(result['command'] == [str(executable), '--verify-only'], 'unexpected capture command')
    binary = sha(executable)
    require(manifest['target_runtime_identity'][target]['executable_sha256'] == binary
            == freshness['runtime_identity']['targets'][target]['executable_sha256'], 'executable/build identity mismatch')
    inputs = read(bound(row['source_manifest']))
    require(row['source_manifest']['sha256'] == manifest['source_manifest_sha256'], 'capture source authority differs')
    for rel, digest in expected_sources.items():
        require(inputs.get(rel) == digest, 'source mapping differs: ' + rel)
        name = 'editing/' + rel
        require(name in copied and copied[name]['sha256'] == digest, 'mapped source not in snapshot: ' + rel)
    output = bound(row['raw_output'])
    require(output == bound({'path': str(Path(row['execution_result']['path']).parent / 'output.log'),
                             'sha256': row['raw_output']['sha256']}), 'output does not belong to execution record')
    extracted = [schema.parse(line) for line in output.read_text(encoding='utf-8').splitlines()
                 if line.lstrip().startswith('{')]
    records = schema.load(records_path)
    require(records == extracted == result['results']['json_records'], 'records differ from retained execution output')
    schema.validate(records, contract, 'verify')
    return {'binary_sha256': binary, 'source_manifest_sha256': manifest['source_manifest_sha256']}


def qualify(provenance_path, provenance_sha256, before, after):
    schema.pinned(provenance_path, provenance_sha256)
    provenance = read(provenance_path)
    require(provenance['schema_version'] == 1 and provenance['mode'] == 'verify',
            'only ordinary verify provenance is qualified; timing needs benchmark gate')
    harness = provenance['harness']
    contracts = read(bound(provenance['contracts']))
    contract = contracts[harness]
    proposal = read(bound(provenance['reviewed_mapping_manifest']))
    bound(provenance['source_review'])
    mapping = {row['path']: row for row in proposal['changes']}
    originals = contract['source_bindings']
    prefix = 'net/cookies/' + harness
    require(set(originals) == {prefix + '_perftest.cc', prefix + '_test_support.h'},
            'complete original harness source bindings required')
    current = {}
    for rel, before_hash in originals.items():
        if rel in mapping:
            row = mapping[rel]
            require(row['before_sha256'] == before_hash, 'original contract mapping differs')
            current[rel] = row['after_sha256']
        else:
            current[rel] = before_hash
    relation = provenance['relation']
    require(relation in ('repeatability', 'reviewed-source-change'), 'unknown comparison relation')
    left_sources = current if relation == 'repeatability' else originals
    left = capture(provenance['before'], before, contract, harness, left_sources)
    right = capture(provenance['after'], after, contract, harness, current)
    schema.compare(schema.load(before), schema.load(after), contract, 'verify')
    return {'relation': relation, 'before': left, 'after': right, 'qualified': True,
            'scope': 'Trusted ordinary capture/source/binary binding and strict record comparison only; no performance/cutover claim'}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('before'); parser.add_argument('after')
    parser.add_argument('--provenance', required=True)
    parser.add_argument('--provenance-sha256', required=True)
    args = parser.parse_args()
    try:
        print(json.dumps(qualify(args.provenance, args.provenance_sha256, args.before, args.after)))
    except (ValueError, TypeError, KeyError, OSError) as exc:
        parser.exit(1, 'capture qualification failed: ' + str(exc) + '\n')


if __name__ == '__main__': main()
