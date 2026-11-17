import copy
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

import qualify_records as gate

ROOT = Path(__file__).parent


def write(path, value):
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(value))
    return {'path': str(path), 'sha256': gate.sha(path)}


class CaptureBindingTest(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name).resolve()
        self.contracts = json.loads((ROOT / 'harness-contracts.json').read_text())
        # Miniature trusted-fixture sources have actual byte identities.
        self.originals = {'net/cookies/cookie_family_perftest.cc': b'before source',
                          'net/cookies/cookie_family_test_support.h': b'unchanged case table'}
        self.afters = dict(self.originals)
        self.afters['net/cookies/cookie_family_perftest.cc'] = b'reviewed after source'
        import hashlib
        self.hashes = lambda files: {k: hashlib.sha256(v).hexdigest() for k, v in files.items()}
        contract = self.contracts['cookie_family']
        contract['source_bindings'] = self.hashes(self.originals)
        self.contract_binding = write(self.root / 'contracts.json', self.contracts)
        mapping = {'changes': [{'path': key, 'before_sha256': self.hashes(self.originals)[key],
                                'after_sha256': self.hashes(self.afters)[key]} for key in self.originals]}
        self.mapping = write(self.root / 'mapping.json', mapping)
        self.review = write(self.root / 'review.json', {'scope': 'synthetic review receipt'})
        self.rows = [{'verify_case': case, 'checksum': 'abcdef0123456789'} for case in contract['cases']]
        self.provenance = {'schema_version': 1, 'mode': 'verify', 'harness': 'cookie_family',
                           'relation': 'repeatability', 'contracts': self.contract_binding,
                           'reviewed_mapping_manifest': self.mapping, 'source_review': self.review,
                           'before': self.make_capture('left'), 'after': self.make_capture('right')}

    def make_capture(self, name, sources=None):
        sources = sources or self.afters
        base = self.root / name
        snapshot = base / 'snapshot'
        inputs = self.hashes(sources)
        files = []
        def copied(relative, data):
            p = snapshot / relative
            p.parent.mkdir(parents=True, exist_ok=True)
            p.write_bytes(data)
            files.append({'copy': relative, 'original': str(p), 'sha256': gate.sha(p)})
            return p
        for rel, data in sources.items(): copied('editing/' + rel, data)
        authority = copied('editing/source.json', json.dumps(inputs).encode())
        exe = copied('configured/out/Default/cookie_storage_family_perftests.exe', b'synthetic executable bytes')
        build = copied('context/build.json', b'{"scope":"synthetic passing build receipt"}')
        target = {'cookie_storage_family_perftests': {'executable_sha256': gate.sha(exe)}}
        freshness = {'exit_code': 0, 'all_selected_targets_fresh': True, 'abi_consumers_rebuilt': True,
                     'source_manifest_sha256': gate.sha(authority), 'runtime_identity': {'targets': target}}
        fresh = copied('context/freshness.json', json.dumps(freshness).encode())
        manifest = {'files': files, 'directories': [], 'external_python_inputs': [], 'copied_with_hardlinks': False,
                    'freshness_record_sha256': gate.sha(fresh), 'source_manifest_sha256': gate.sha(authority),
                    'build_and_abi_evidence': [{'path': str(build), 'sha256': gate.sha(build)}],
                    'target_runtime_identity': target}
        pin = write(snapshot / 'input-manifest.json', manifest)['sha256']
        write(snapshot / 'freeze-complete.json', {'input_manifest_sha256': pin, 'before_after_originals_and_copies_matched': True})
        qualification = write(base / 'qualification.json', {'qualified': True, 'snapshot_manifest_sha256': pin,
                              'selected_runtime': {'executable': {'path': str(exe), 'sha256': gate.sha(exe)},
                                                   'version': 'synthetic', 'affinity_mask': 1}})
        output = base / 'execution/output.log'; output.parent.mkdir(parents=True)
        output.write_text(''.join(json.dumps(x) + '\n' for x in self.rows))
        records = base / 'records.jsonl'; records.write_bytes(output.read_bytes())
        result = {'manifest_sha256': pin, 'qualification_sha256': qualification['sha256'], 'passed': True,
                  'executed': True, 'exit_code': 0, 'kind': 'workload', 'performance_measurement': False,
                  'input_identity_before': 'matched frozen manifest and external Python',
                  'input_identity_after': 'matched before identity', 'command': [str(exe), '--verify-only'],
                  'results': {'json_records': self.rows}}
        return {'snapshot': str(snapshot), 'snapshot_manifest_sha256': pin,
                'source_manifest': {'path': str(authority), 'sha256': gate.sha(authority)},
                'records': {'path': str(records), 'sha256': gate.sha(records)},
                'raw_output': {'path': str(output), 'sha256': gate.sha(output)},
                'execution_result': write(base / 'execution/result.json', result), 'qualification': qualification}

    def qualify(self):
        pin = write(self.root / 'provenance.json', self.provenance)
        return gate.qualify(pin['path'], pin['sha256'], self.provenance['before']['records']['path'], self.provenance['after']['records']['path'])

    def test_complete_repeatability_and_reviewed_transition(self):
        self.assertTrue(self.qualify()['qualified'])
        self.provenance['relation'] = 'reviewed-source-change'
        self.provenance['before'] = self.make_capture('original', self.originals)
        self.assertTrue(self.qualify()['qualified'])

    def test_wrong_source_relation_and_mapping_fail(self):
        self.provenance['relation'] = 'reviewed-source-change'
        with self.assertRaises(ValueError): self.qualify()
        self.provenance['relation'] = 'repeatability'
        data = json.loads(Path(self.mapping['path']).read_text())
        data['changes'][0]['before_sha256'] = '0' * 64
        self.provenance['reviewed_mapping_manifest'] = write(Path(self.mapping['path']), data)
        with self.assertRaises(ValueError): self.qualify()

    def test_changed_snapshot_and_added_directory_member_fail(self):
        left = self.provenance['before']
        snapshot = Path(left['snapshot'])
        (snapshot / 'editing/net/cookies/cookie_family_perftest.cc').write_bytes(b'unbound source')
        with self.assertRaises(ValueError): self.qualify()

    def test_directory_membership_and_hardlinks_are_enforced(self):
        row = self.provenance['after']; snapshot = Path(row['snapshot'])
        p = snapshot / 'input-manifest.json'; manifest = json.loads(p.read_text())
        directory = snapshot / 'data'; directory.mkdir()
        manifest['directories'] = [{'copy': 'data', 'files': [], 'subdirectories': []}]
        row['snapshot_manifest_sha256'] = write(p, manifest)['sha256']
        write(snapshot / 'freeze-complete.json', {'input_manifest_sha256': row['snapshot_manifest_sha256'],
                                                 'before_after_originals_and_copies_matched': True})
        (directory / 'added.log').write_text('unexpected snapshot member')
        with self.assertRaisesRegex(ValueError, 'membership'): self.qualify()
        manifest['copied_with_hardlinks'] = True
        row['snapshot_manifest_sha256'] = write(p, manifest)['sha256']
        with self.assertRaisesRegex(ValueError, 'hardlinks'): self.qualify()

    def test_missing_source_binding_and_changed_review_are_rejected(self):
        contracts = copy.deepcopy(self.contracts)
        contracts['cookie_family']['source_bindings'] = {}
        self.provenance['contracts'] = write(Path(self.contract_binding['path']), contracts)
        with self.assertRaisesRegex(ValueError, 'complete original'): self.qualify()
        self.provenance['contracts'] = write(Path(self.contract_binding['path']), self.contracts)
        Path(self.review['path']).write_text('{}')
        with self.assertRaisesRegex(ValueError, 'identity mismatch'): self.qualify()

    def test_bad_or_unrelated_receipts_fail(self):
        for field, value in [('passed', False), ('executed', False), ('exit_code', True),
                             ('input_identity_after', 'unknown'), ('performance_measurement', True),
                             ('command', ['some-other.exe', '--verify-only'])]:
            with self.subTest(field=field):
                original = copy.deepcopy(self.provenance['after']['execution_result'])
                path = Path(original['path']); raw = path.read_bytes(); data = json.loads(raw)
                data[field] = value
                self.provenance['after']['execution_result'] = write(path, data)
                with self.assertRaises(ValueError): self.qualify()
                path.write_bytes(raw); self.provenance['after']['execution_result'] = original

    def test_raw_output_and_normalized_records_cannot_disagree(self):
        after = self.provenance['after']
        p = Path(after['records']['path']); rows = copy.deepcopy(self.rows)
        rows[0]['checksum'] = '0123456789abcdef'
        p.write_text(''.join(json.dumps(x) + '\n' for x in rows))
        after['records']['sha256'] = gate.sha(p)
        with self.assertRaises(ValueError): self.qualify()

    def test_missing_provenance_timing_and_overflow_json_reject(self):
        self.provenance['mode'] = 'timing'
        with self.assertRaises(ValueError): self.qualify()
        p = self.root / 'overflow.json'; p.write_text('{"x":1e400}')
        with self.assertRaises(ValueError): gate.read(p)
        command = [sys.executable, '-B', '-O', str(ROOT / 'qualify_records.py'), 'before', 'after']
        self.assertNotEqual(0, subprocess.run(command, capture_output=True).returncode)

    def test_optimized_cli_accepts_bound_capture_and_rejects_drift(self):
        pin = write(self.root / 'provenance.json', self.provenance)
        command = [sys.executable, '-B', '-O', str(ROOT / 'qualify_records.py'),
                   self.provenance['before']['records']['path'], self.provenance['after']['records']['path'],
                   '--provenance', pin['path'], '--provenance-sha256', pin['sha256']]
        result = subprocess.run(command, capture_output=True)
        self.assertEqual(0, result.returncode, result.stderr)
        Path(self.provenance['after']['records']['path']).write_text('')
        self.assertNotEqual(0, subprocess.run(command, capture_output=True).returncode)


if __name__ == '__main__': unittest.main()
