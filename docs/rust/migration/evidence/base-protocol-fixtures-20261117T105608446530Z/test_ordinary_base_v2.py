"""Prepared evidence-integrity regression fixtures; no native process launched."""
import argparse
import contextlib
import copy
import io
import json
from pathlib import Path
import tempfile
import unittest
import sys
import xml.etree.ElementTree as ET
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

    def test_terminal_before_run_and_overlapping_attempts_fail(self):
        data=summary();parsed={'passed':8,'skipped':0,'native_skip_names':[],'unexpected_statuses':{}}
        lines=transcript(data).splitlines(keepends=True)
        for indices in [[1,0,2,3],[0,2,1,3],[0,3,1,2]]:
            output=''.join(lines[i] for i in indices)+''.join(lines[4:])
            with self.assertRaises(ValueError):delivery.native_outcome(data,parsed,output)

    def test_explicit_xml_skip_and_nonrun_contradictions_fail(self):
        nodes=[ET.Element('testcase',classname=name.split('.',1)[0],name=name.split('.',1)[1]) for name in delivery.membership(summary())[0]]
        expected=delivery.membership(summary())[0]
        delivery.xml_outcome(nodes,expected,set())
        ET.SubElement(nodes[0],'skipped')
        with self.assertRaises(ValueError):delivery.xml_outcome(nodes,expected,set())
        name=nodes[0].attrib['classname']+'.'+nodes[0].attrib['name']
        delivery.xml_outcome(nodes,expected,{name})
        nodes[0].remove(nodes[0][0]);nodes[0].set('status','notrun')
        with self.assertRaises(ValueError):delivery.xml_outcome(nodes,expected,{name})

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
            with mock.patch.object(delivery.windows_job,'ProcessTree',return_value=process),mock.patch.object(delivery,'terminate') as terminate:
                if isinstance(cleanup,Exception):terminate.side_effect=cleanup
                else:terminate.return_value=cleanup
                with self.assertRaises(delivery.CommandFailure) as caught:
                    delivery.run_command(['fixture'],self.root,{},self.root/'timeout.log',timeout=1)
                self.assertTrue(caught.exception.details['timed_out']);terminate.assert_called_once_with(process)
                self.assertIn('cleanup',caught.exception.details)
                if isinstance(cleanup,Exception):self.assertTrue(caught.exception.details['cleanup']['live_descendants_require_reconciliation'])

    def test_gate_must_bind_exact_invocation_destination_bytes_and_interval(self):
        intake=delivery.ROOT/'.rustium-runtime/shared-strings-connection-latest.json'
        receipt_path=self.root/'receipt.json';invocation='a'*32
        receipt={'invocation':invocation,'source_manifest_sha256':'authority','started_utc':'2026-11-17T10:00:01+00:00','finished_utc':'2026-11-17T10:00:02+00:00'}
        command=['python','-B',str(Path(delivery.__file__).with_name('attest_build.py').resolve()),'-j1','--intake',str(intake.resolve()),'--output',str(receipt_path.resolve().parent),'--expected-source-sha256','authority','--invocation',invocation]
        job={'command':command,'cwd':str(delivery.ROOT),'build_configuration':{'invocation':invocation,'receipt':str(receipt_path.resolve())},
            'status':'passed','exit_code':0,'kind':'build','task_id':'FOUNDATION-STRINGS-BRIDGE',
            'source_identity':{'source_manifest_sha256':'authority'},'started_utc':'2026-11-17T10:00:00+00:00','finished_utc':'2026-11-17T10:00:03+00:00',
            'completion':{'invocation':invocation,'receipt':str(receipt_path.resolve()),'receipt_sha256':'receipt-bytes','passed':True}}
        delivery.attestation_gate_binding(job,receipt,receipt_path,intake,'authority','receipt-bytes')
        for mode in ['destination','stale','other-invocation','bytes','command','no-timezone']:
            bad=copy.deepcopy(job)
            if mode=='destination':bad['build_configuration']['receipt']=str(self.root/'other.json')
            elif mode=='stale':bad['started_utc']='2026-11-17T10:00:03+00:00'
            elif mode=='other-invocation':bad['build_configuration']['invocation']='b'*32
            elif mode=='bytes':bad['completion']['receipt_sha256']='different'
            elif mode=='command':bad['command'][bad['command'].index('--output')+1]=str(self.root/'other')
            else:bad['started_utc']='2026-11-17T10:00:00'
            with self.assertRaises(ValueError):delivery.attestation_gate_binding(bad,receipt,receipt_path,intake,'authority','receipt-bytes')

    def test_exited_root_with_pipe_holding_descendant_is_owned_and_killed(self):
        child='import time;time.sleep(30)'
        parent='import subprocess,sys;subprocess.Popen([sys.executable,"-c",'+repr(child)+'])'
        tree=delivery.windows_job.ProcessTree([sys.executable,'-c',parent],stdout=delivery.subprocess.PIPE,stderr=delivery.subprocess.STDOUT)
        try:
            with self.assertRaises(delivery.subprocess.TimeoutExpired):tree.communicate(timeout=1)
            self.assertEqual(tree.process.poll(),0)
            self.assertGreater(tree.active(),0)
            result=tree.terminate_tree(timeout=15)
            self.assertTrue(result['owned_job_empty']);self.assertEqual(tree.active(),0)
            tree.process.communicate(timeout=5)
        finally:
            try:tree.terminate_tree(timeout=15)
            finally:tree.close()

    def test_failed_job_termination_is_never_inferred_from_root_exit(self):
        tree=object.__new__(delivery.windows_job.ProcessTree)
        tree.kernel=mock.Mock();tree.kernel.TerminateJobObject.return_value=0
        tree.job=1;tree.process=mock.Mock();tree.process.poll.return_value=0
        with mock.patch.object(tree,'active',return_value=1):
            failure=delivery.command_failure(tree,['fixture'],RuntimeError('probe failed'))
        self.assertTrue(failure.details['cleanup']['live_descendants_require_reconciliation'])
        tree.process.wait.assert_not_called()

    def test_failed_probe_preserves_raw_output_bytes(self):
        error=delivery.subprocess.CalledProcessError(1,['fixture'],output=b'\xff\x00raw')
        result=delivery.CommandFailure(['fixture'],error,{'reaped':True})
        self.assertEqual(delivery.base64.b64decode(result.details['partial_output_base64']),b'\xff\x00raw')

if __name__=='__main__':unittest.main()
