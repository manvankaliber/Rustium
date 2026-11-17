"""Prepared evidence-integrity regression fixtures; no native process launched."""
import argparse
import contextlib
import copy
import io
import json
from pathlib import Path
import tempfile
import unittest
from unittest import mock
import ordinary_base_v2 as delivery

def summary():
    selected=sorted(delivery.BRIDGE)+[
        'StringSplitTest.NullSeparator','StringSplitTest.StringSplitKeepWhitespace',
        'SplitStringIntoKeyValuePairsTest/0.EmptyString']
    return {'all_tests':selected+['OtherTest.Unselected','StringSplitTest.DISABLED_Example'],
        'disabled_tests':['StringSplitTest.DISABLED_Example'],
        'per_iteration_data':[{name:[{'status':'SUCCESS'}] for name in selected}]}

def transcript(data, skipped=()):
    return ''.join('[ RUN      ] '+name+'\n[  '+('SKIPPED' if name in skipped else 'OK')+' ] '+name+' (0 ms)\n'
        for name in data['per_iteration_data'][0])

class MembershipTests(unittest.TestCase):
    def test_retains_original_patterns_and_all_named_bridge_cases(self):
        data=summary();expected,disabled=delivery.membership(data)
        self.assertEqual(len(expected),8)
        self.assertIn('StringSplitTest.NullSeparator',expected)
        self.assertIn('StringSplitTest.StringSplitKeepWhitespace',expected)
        self.assertIn(delivery.DIAGNOSTIC,expected)
        self.assertNotIn('OtherTest.Unselected',expected)
        self.assertEqual(disabled,['StringSplitTest.DISABLED_Example'])

    def test_missing_or_disabled_bridge_case_fails(self):
        for disabled in [False,True]:
            data=summary();name=delivery.DIAGNOSTIC
            data['per_iteration_data'][0].pop(name)
            if disabled:data['disabled_tests'].append(name)
            else:data['all_tests'].remove(name)
            with self.assertRaises(ValueError):delivery.membership(data)

    def test_missing_extra_and_repeated_attempts_fail(self):
        for mode in ['missing','extra','repeat']:
            data=summary();attempts=data['per_iteration_data'][0]
            if mode=='missing':attempts.pop('StringSplitTest.NullSeparator')
            elif mode=='extra':attempts['OtherTest.Unselected']=[{'status':'SUCCESS'}]
            else:attempts['StringSplitTest.NullSeparator'].append({'status':'SUCCESS'})
            with self.assertRaises(ValueError):delivery.membership(data)

    def test_native_skip_cannot_validate_diagnostic_contract(self):
        parsed={'passed':7,'skipped':1,'native_skip_names':[delivery.DIAGNOSTIC],'unexpected_statuses':{}}
        result=delivery.native_outcome(summary(),parsed,transcript(summary(),[delivery.DIAGNOSTIC]))
        self.assertFalse(result['diagnostic_contract_passed'])
        parsed.update(passed=8,skipped=0,native_skip_names=[])
        self.assertTrue(delivery.native_outcome(summary(),parsed,transcript(summary()))['diagnostic_contract_passed'])
        parsed['passed']=7
        with self.assertRaises(ValueError):delivery.native_outcome(summary(),parsed,transcript(summary()))

    def test_absent_truncated_duplicate_or_contradictory_native_output_fails(self):
        data=summary();parsed={'passed':8,'skipped':0,'native_skip_names':[],'unexpected_statuses':{}}
        complete=transcript(data)
        invalid=['',complete[:complete.rfind('[  OK')],complete+complete,
            complete.replace('[  OK ] '+delivery.DIAGNOSTIC,'[  FAILED ] '+delivery.DIAGNOSTIC)]
        for output in invalid:
            with self.assertRaises(ValueError):delivery.native_outcome(data,parsed,output)

class SnapshotAndFailureTests(unittest.TestCase):
    def setUp(self):
        self.temp=tempfile.TemporaryDirectory();self.root=Path(self.temp.name)
        self.prior=delivery.OWNED;delivery.OWNED=self.root

    def tearDown(self):
        delivery.OWNED=self.prior;self.temp.cleanup()

    def sealed(self,missing_empty=False):
        snapshot=self.root/'snapshot';(snapshot/'build/fixture').mkdir(parents=True)
        if not missing_empty:(snapshot/'build/fixture/empty').mkdir()
        helper=snapshot/'controller.py';helper.write_text('# pinned fixture\n')
        data={'helpers':{'source.py':{'copy':'controller.py','sha256':delivery.sha(helper)}},
            'runtime':{'directories':{str(delivery.BUILD/'fixture'):['empty/']}},
            'inventory':delivery.inventory(snapshot)}
        manifest=snapshot/'input-manifest.json';delivery.write(manifest,data)
        return snapshot,delivery.sha(manifest)

    def test_empty_directory_membership_is_independent_of_file_inventory(self):
        snapshot,digest=self.sealed(missing_empty=True)
        with self.assertRaises(ValueError):delivery.verify(snapshot,digest)

    def test_changed_helper_or_unexpected_snapshot_member_fails(self):
        snapshot,digest=self.sealed();delivery.verify(snapshot,digest)
        (snapshot/'unexpected').mkdir()
        with self.assertRaises(ValueError):delivery.verify(snapshot,digest)
        (snapshot/'unexpected').rmdir();(snapshot/'controller.py').write_text('# changed\n')
        with self.assertRaises(ValueError):delivery.verify(snapshot,digest)

    def test_preparation_rejection_retains_durable_failed_result(self):
        output=self.root/'rejected'
        args=argparse.Namespace(output=output,intake=self.root/'wrong-intake.json')
        with contextlib.redirect_stdout(io.StringIO()):code=delivery.prepare(args)
        result=json.loads((output/'result.json').read_text())
        self.assertEqual(code,1);self.assertFalse(result['passed'])
        self.assertIn('exception',result);self.assertIn('finished_utc',result)
        self.assertEqual(set(result['identity_checks_after']),{'source','runtime','configuration','host','helpers','snapshot'})

    def test_execution_rejection_retains_failed_identity_and_artifacts(self):
        snapshot,digest=self.sealed();output=self.root/'execution'
        args=argparse.Namespace(snapshot=snapshot,execution=output,manifest_sha256='0'*64)
        self.assertEqual(delivery.execute(args),1)
        result=json.loads((output/'result.json').read_text())
        self.assertFalse(result['passed']);self.assertIn('exception',result)
        self.assertIsInstance(result['snapshot_identity_after'],str)
        self.assertFalse(result['artifacts']['summary.json'])

    def test_receipt_output_config_additions_and_wrong_destination_fail(self):
        config={'build.ninja':'a'};runtime={'base_unittests.exe':'b'}
        receipt={'source_manifest_sha256':'authority','passed':True,'source_inputs_stable':True,
            'cwd':str(delivery.BUILD),'commands':delivery.expected_build_commands(),
            'exit_codes':[0,0,0,0],'configuration':config,'runtime':runtime}
        delivery.attestation_binding(receipt,'authority',config,runtime)
        variants=[]
        item=copy.deepcopy(receipt);item['runtime']={'base_unittests.exe':'changed'};variants.append(item)
        item=copy.deepcopy(receipt);item['configuration']['unexpected.ninja']='new';variants.append(item)
        item=copy.deepcopy(receipt);item['commands'][-1][2]='other-output';variants.append(item)
        item=copy.deepcopy(receipt);item['exit_codes']=[0];variants.append(item)
        for item in variants:
            with self.assertRaises(ValueError):delivery.attestation_binding(item,'authority',config,runtime)

    def test_timeout_and_cleanup_failure_are_structured(self):
        process=mock.Mock();process.wait.side_effect=delivery.subprocess.TimeoutExpired(['fixture'],1)
        for cleanup in [{'required':True,'reaped':True},RuntimeError('cleanup failed')]:
            with mock.patch.object(delivery.subprocess,'Popen',return_value=process),mock.patch.object(delivery,'terminate') as terminate:
                if isinstance(cleanup,Exception):terminate.side_effect=cleanup
                else:terminate.return_value=cleanup
                with self.assertRaises(delivery.CommandFailure) as caught:
                    delivery.run_command(['fixture'],self.root,{},self.root/'timeout.log',timeout=1)
                self.assertTrue(caught.exception.details['timed_out']);terminate.assert_called_once_with(process)
                self.assertIn('cleanup',caught.exception.details)
                if isinstance(cleanup,Exception):self.assertTrue(caught.exception.details['cleanup']['live_descendants_require_reconciliation'])

    def test_failed_probe_preserves_raw_output_bytes(self):
        error=delivery.subprocess.CalledProcessError(1,['fixture'],output=b'\xff\x00raw')
        result=delivery.CommandFailure(['fixture'],error,{'reaped':True})
        self.assertEqual(delivery.base64.b64decode(result.details['partial_output_base64']),b'\xff\x00raw')

if __name__=='__main__':unittest.main()
