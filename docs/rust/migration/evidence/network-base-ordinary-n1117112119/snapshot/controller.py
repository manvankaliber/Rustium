"""Separate parent-only frozen ordinary delivery. Pending qualification."""
import argparse
import base64
import ctypes
import datetime
import fnmatch
import hashlib
import importlib.util
import json
import os
import re
from pathlib import Path
import shutil
import subprocess
import sys
import windows_job

sys.dont_write_bytecode = True
ROOT = Path('D:/Rustium')
BUILD = Path('D:/RustiumBuild/src')
OUT = BUILD/'out/Default'
OWNED = ROOT/'.scratch/base-ordinary-delivery'
PATTERNS = ['*SplitString*', 'StringSplitTest.*', 'StringSplitRustBridgeTest.*']
BRIDGE = {'StringSplitRustBridgeTest.'+name for name in [
    'PartialViewsRetainNullAndBorrowedEmptyIdentity', 'NullEmptyAndEmbeddedNulInputs',
    'TypedUtf16WireUsesCodeUnitsAndOffsets', 'OnceOverloadsPreserveDirectionAndSeparatorSets',
    'DiagnosticsAndCallbacksKeepNativeSegmentOrder']}
DIAGNOSTIC = 'StringSplitRustBridgeTest.DiagnosticsAndCallbacksKeepNativeSegmentOrder'
REQUIRED = {'base/BUILD.gn','base/strings/string_split.cc','base/strings/string_split_rust.rs',
    'base/strings/string_split_rust_policy.rs','base/strings/string_split_rust_bridge_unittest.cc',
    'net/BUILD.gn','net/socket/connection_baseline_original_string_split.h'}

def require(ok, message):
    if not ok:
        raise ValueError(message)

def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()

def write(path, value):
    path.write_text(json.dumps(value, indent=2)+'\n')

def load(path):
    return json.loads(Path(path).read_text())

def module(name, path):
    spec = importlib.util.spec_from_file_location(name, path)
    value = importlib.util.module_from_spec(spec);spec.loader.exec_module(value)
    return value

def linked(path):
    return path.is_symlink() or (hasattr(path,'is_junction') and path.is_junction())

def inventory(directory):
    result = {}
    for path in directory.rglob('*'):
        require(not linked(path), 'Linked snapshot member')
        rel = path.relative_to(directory).as_posix()
        if rel != 'input-manifest.json':
            result[rel] = sha(path) if path.is_file() else 'directory'
    return result

def copy(source, destination):
    require(source.is_file() and not linked(source), 'Missing/linked source')
    destination.parent.mkdir(parents=True, exist_ok=True);shutil.copyfile(source,destination)
    require(sha(source)==sha(destination) and destination.stat().st_nlink==1,'Copy drift/hardlink')

def membership(summary):
    names=summary['all_tests'];disabled=summary['disabled_tests'];iterations=summary['per_iteration_data']
    require(names and len(names)==len(set(names)), 'Missing/duplicate enumeration')
    require(len(disabled)==len(set(disabled)) and set(disabled)<=set(names),'Disabled membership')
    selected={name for name in names if any(fnmatch.fnmatchcase(name,p) for p in PATTERNS)}
    expected=selected-set(disabled)
    require(BRIDGE<=expected,'Required bridge case absent/disabled')
    require(not any(name.split('.',1)[-1].startswith('PRE_') for name in expected),'PRE contract unqualified')
    require(len(iterations)==1 and set(iterations[0])==expected,'Missing/extra iteration attempts')
    require(all(isinstance(v,list) and len(v)==1 and v[0].get('status') for v in iterations[0].values()),'Repeated/missing attempt')
    return expected, sorted(selected & set(disabled))

def native_outcome(summary, parsed, output):
    expected,disabled=membership(summary)
    require(parsed['passed']+parsed['skipped']==len(expected) and not parsed['unexpected_statuses'],'Unreconciled native statuses')
    skips=set(parsed['native_skip_names']);require(skips<=expected,'Unexpected native skip')
    output=re.sub(r'\x1b\[[0-9;]*m','',output)
    terminal={};active=None
    for line in output.splitlines():
        start=re.fullmatch(r'\[\s*RUN\s*\]\s+(\S+)\s*',line)
        end=re.fullmatch(r'\[\s*(OK|SKIPPED|FAILED)\s*\]\s+(\S+)\s+\(\d+ ms\)\s*',line)
        if start:
            name=start.group(1)
            require(name in expected and name not in terminal and active is None,'Unselected/duplicate/overlapping native run')
            active=name
        elif end:
            event,name=end.groups()
            require(name in expected and active==name and name not in terminal,'Orphaned/reordered/mismatched native terminal')
            terminal[name]=[event];active=None
    require(active is None and set(terminal)==expected,'Missing/truncated native attempt')
    native_skips=set()
    for name,events in terminal.items():
        status=summary['per_iteration_data'][0][name][0]['status'];event=events[0]
        require((event=='OK' and status=='SUCCESS') or (event=='SKIPPED' and status in ['SUCCESS','SKIPPED']),'Contradictory native/launcher outcome')
        if event=='SKIPPED':native_skips.add(name)
    require(native_skips==skips,'Native skip parser disagreement')
    diagnostic=terminal[DIAGNOSTIC]==['OK']
    return {'expected_enabled':len(expected),'disabled':disabled,'diagnostic_contract_passed':diagnostic,'native_skip_names':sorted(native_skips)}


def xml_outcome(nodes, expected, native_skips):
    names=[node.attrib['classname']+'.'+node.attrib['name'] for node in nodes]
    require(len(names)==len(set(names)) and set(names)==expected,'XML membership mismatch')
    for node,name in zip(nodes,names):
        require(node.find('failure') is None and node.find('error') is None,'XML failure/error contradicts success')
        status=node.attrib.get('status','run');result=node.attrib.get('result','completed')
        skipped=node.find('skipped') is not None or result=='skipped'
        require(status=='run' and result in ['completed','skipped'],'Explicit XML non-execution')
        require(not skipped or name in native_skips,'XML skip contradicts native OK')

def source_check(info):
    manifest=Path(info['source_manifest']);require(sha(manifest)==info['source_manifest_sha256'],'Authority manifest drift')
    require(sha(ROOT/'.scratch/net-wave2/parent/last-synced.json')==sha(manifest),'Synchronized authority drift')
    pins=load(manifest);require(len(pins)==1260 and REQUIRED<=set(pins),'Incomplete applied source membership')
    for rel,digest in pins.items():
        require(sha(ROOT/rel)==digest==sha(BUILD/rel),'Source drift: '+rel)
    return pins

def runtime_modules(snapshot=None):
    context=snapshot/'context' if snapshot else None
    native=module('frozen_native',context/'native_delivery.py' if context else ROOT/'.scratch/net-wave5/resumed-integration/native-runtime-delivery-v2/native_delivery.py')
    registry=module('frozen_registry',context/'dll_registry.py' if context else ROOT/'tools/run_rust_rewrite_benchmarks.py')
    # Same recursive adjacent-import algorithm, with bounded probe processes.
    registry.local_dll_dependencies=bounded_dll_dependencies
    inspector=context/'inspector/dumpbin.exe' if context else Path(native.reused().INSPECTOR)
    return native,registry,inspector

def runtime(native,registry,inspector,targets=('base_unittests',)):
    return native.collect_runtime({'cases':[{'target':target} for target in targets]},inspector,registry)

def configuration_identity(native):
    result=native.config_files()
    for path in (OUT/'gen/base').rglob('*'):
        if path.is_file():result[str(path.resolve())]=sha(path)
    for pattern in ['*.rsp','*.rsp.rust','*.rustflags.json']:
        for path in (OUT/'obj/base').rglob(pattern):result[str(path.resolve())]=sha(path)
    return result

def expected_build_commands():
    gn=str(BUILD/'buildtools/win/gn.exe')
    return [[gn,'gen','out/Default'],[gn,'check','out/Default','//base:base_unittests'],
        [gn,'check','out/Default','//net:*'],
        [str(BUILD/'third_party/ninja/ninja.exe'),'-C',str(OUT),'-j1','base_unittests','net_unittests']]

def attestation_binding(receipt, authority, current_configuration, current_runtime):
    require(receipt['source_manifest_sha256']==authority and receipt['passed'] and receipt['source_inputs_stable'],'Attestation source mismatch')
    require(receipt['cwd']==str(BUILD) and receipt['commands']==expected_build_commands() and all(code==0 for code in receipt['exit_codes']),'Exact configured commands required')
    require(len(receipt['exit_codes'])==len(expected_build_commands()),'Incomplete attested command results')
    require(receipt['configuration']==current_configuration and receipt['runtime']==current_runtime,'Receipt output/graph membership differs')


def attestation_gate_binding(job, receipt, receipt_path, intake, authority, receipt_sha):
    script=Path(__file__).with_name('attest_build.py').resolve()
    command=['python','-B',str(script),'-j1','--intake',str(intake.resolve()),
        '--output',str(receipt_path.resolve().parent),'--expected-source-sha256',authority,
        '--invocation',receipt['invocation']]
    require(job['command']==command and job['cwd']==str(ROOT),'Attestation gate command/output mismatch')
    require(job['build_configuration']['invocation']==receipt['invocation'],'Unrelated attestation invocation')
    require(job['build_configuration']['receipt']==str(receipt_path.resolve()),'Attestation gate receipt destination mismatch')
    require(job['status']=='passed' and job['exit_code']==0 and job['kind']=='build' and job['task_id']=='FOUNDATION-STRINGS-BRIDGE','Completed attestation gate required')
    require(job['source_identity']['source_manifest_sha256']==authority==receipt['source_manifest_sha256'],'Attestation authority mismatch')
    require(receipt['invocation'] and re.fullmatch('[0-9a-f]{32}',receipt['invocation']),'Unique invocation required')
    times=[datetime.datetime.fromisoformat(v) for v in [job['started_utc'],receipt['started_utc'],receipt['finished_utc'],job['finished_utc']]]
    require(all(t.utcoffset() is not None for t in times) and times==sorted(times),'Receipt outside gate execution interval')
    # Coordinator stdout is retained in the same unique job directory. Its
    # completion line binds the final receipt bytes to this actual invocation.
    proof=job['completion']
    require(proof=={'invocation':receipt['invocation'],'receipt':str(receipt_path.resolve()),'receipt_sha256':receipt_sha,'passed':True},'Gate completion receipt bytes mismatch')

def host_identity():
    require(os.name=='nt','Native delivery is Windows-only')
    dll=ctypes.create_unicode_buffer(32768)
    function=ctypes.WinDLL('kernel32',use_last_error=True).GetModuleFileNameW
    function.argtypes=[ctypes.c_void_p,ctypes.c_wchar_p,ctypes.c_uint32];function.restype=ctypes.c_uint32
    length=function(ctypes.c_void_p(sys.dllhandle),dll,len(dll))
    require(0<length<len(dll),'Cannot identify actual loaded Python DLL')
    paths=[Path(sys.executable).resolve(),Path(dll.value).resolve()]
    return {'files':{str(p):sha(p) for p in paths},'version':sys.version,
        'limits':'Installed stdlib and system/nonadjacent DLL providers are not a frozen standalone runtime; dynamically loaded plugins remain outside the closure'}

def terminate(process):
    return process.terminate_tree(timeout=15)

class CommandFailure(RuntimeError):
    def __init__(self, command, error, cleanup):
        self.details={'command':[str(x) for x in command], 'failure':type(error).__name__+': '+str(error),
            'timed_out':isinstance(error,subprocess.TimeoutExpired),'cleanup':cleanup}
        partial=getattr(error,'output',None)
        if isinstance(partial,bytes):self.details['partial_output_base64']=base64.b64encode(partial).decode('ascii')
        super().__init__(json.dumps(self.details))

def command_failure(process,command,error):
    try:cleanup=terminate(process)
    except BaseException as cleanup_error:cleanup={'failed':type(cleanup_error).__name__+': '+str(cleanup_error),'live_descendants_require_reconciliation':True}
    return CommandFailure(command,error,cleanup)

def capture(command,timeout=60):
    require(os.name=='nt','Native probes are Windows-only')
    process=windows_job.ProcessTree(command,stdout=subprocess.PIPE,stderr=subprocess.STDOUT)
    try:
        output=process.communicate(timeout=timeout)[0]
        if process.returncode!=0:raise subprocess.CalledProcessError(process.returncode,command,output=output)
        return output.decode(errors='replace')
    except BaseException as error:
        raise command_failure(process,command,error) from error
    finally:
        process.close()

def bounded_dll_dependencies(executable,inspector):
    pending=[executable];visited=set();hashes={};external=set()
    while pending:
        binary=pending.pop()
        if binary in visited:continue
        visited.add(binary)
        output=capture([inspector,'/nologo','/dependents',binary],timeout=60)
        for name in re.findall(r'^\s+([^\s/\\:]+\.dll)\s*$',output,re.M|re.I):
            dependency=(executable.parent/name).resolve()
            if dependency.is_file():hashes[dependency.name]=sha(dependency);pending.append(dependency)
            else:external.add(name.lower())
    return hashes,sorted(external)

def verify(snapshot, digest):
    manifest=snapshot/'input-manifest.json';require(sha(manifest)==digest,'Snapshot manifest drift')
    data=load(manifest);require(inventory(snapshot)==data['inventory'],'Snapshot content/membership drift')
    for source, row in data['helpers'].items():
        require(sha(snapshot/row['copy'])==row['sha256'],'Copied helper drift')
    # Check the collector's complete directory lists, including empty folders.
    for directory,members in data['runtime']['directories'].items():
        copied=snapshot/'build'/Path(directory).relative_to(BUILD)
        actual=sorted(p.relative_to(copied).as_posix()+('/' if p.is_dir() else '') for p in copied.rglob('*'))
        require(actual==members,'Declared runtime directory membership drift')
    return data

def run_command(command,cwd,env,log,timeout=600):
    require(os.name=='nt','Native process-tree protocol is Windows-only')
    with log.open('wb') as stream:
        process=windows_job.ProcessTree(command,cwd=cwd,env=env,stdout=stream,stderr=subprocess.STDOUT)
        try:
            return process.wait(timeout=timeout)
        except BaseException as error:
            raise command_failure(process,command,error) from error
        finally:
            process.close()

def execute(args):
    snapshot=args.snapshot.resolve();output=args.execution.resolve()
    result={'passed':False,'started_utc':datetime.datetime.now(datetime.timezone.utc).isoformat()};data=None;host=None
    require(output.is_relative_to(OWNED.resolve()) and not output.is_relative_to(snapshot) and not output.exists(),'Unique external execution path required')
    require(not any(linked(p) for p in [output,*output.parents]),'Linked execution ancestor')
    output.mkdir(parents=True);write(output/'result.json',result)
    try:
        data=verify(snapshot,args.manifest_sha256)
        host=host_identity();require(host==data['host_identity'],'Copied controller uses different Python host')
        require(Path(__file__).resolve()==snapshot/'controller.py','Execute copied controller')
        temporary=output/'t';temporary.mkdir();require(len(str(temporary))<=96,'Temporary path too long')
        env=os.environ.copy();require(not any(k.startswith('GTEST_') or k in ['TEST_TOTAL_SHARDS','TEST_SHARD_INDEX'] for k in env),'Inherited test selector')
        env.update(TMP=str(temporary),TEMP=str(temporary),GTEST_COLOR='no')
        command=[snapshot/'build/out/Default/base_unittests.exe','--gtest_filter='+':'.join(PATTERNS),
            '--test-launcher-jobs=1','--test-launcher-batch-limit=1','--test-launcher-retry-limit=0',
            '--test-launcher-bot-mode','--test-launcher-print-test-stdio=always',
            '--test-launcher-summary-output='+str(output/'summary.json'),'--gtest_output=xml:'+str(output/'gtest.xml')]
        result['command']=[str(x) for x in command];write(output/'result.json',result)
        result['exit_code']=run_command(command,output,env,output/'output.log')
        expected,disabled=membership(load(output/'summary.json'))
        parser=module('frozen_unit_parser',snapshot/'context/unit_parser.py')
        xml=list(parser.ET.parse(output/'gtest.xml').getroot().iter('testcase'))
        parsed=parser.unit_results(output);result['native_results']=parsed
        result.update(native_outcome(load(output/'summary.json'),parsed,(output/'output.log').read_text(errors='replace')))
        xml_outcome(xml,expected,set(result['native_skip_names']))
        result['passed']=result['exit_code']==0 and result['diagnostic_contract_passed']
    except BaseException as error:
        result['exception']=type(error).__name__+': '+str(error)
        if isinstance(error,CommandFailure):result['command_failure']=error.details
    finally:
        try:require(host is not None and host_identity()==host,'Python host drift');result['host_identity_after']=True
        except BaseException as error:result.update(passed=False,host_identity_after=str(error))
        try:verify(snapshot,args.manifest_sha256);result['snapshot_identity_after']=True
        except BaseException as error:result.update(passed=False,snapshot_identity_after=str(error))
        result['artifacts']={name:(output/name).exists() for name in ['summary.json','gtest.xml','output.log']}
        result['finished_utc']=datetime.datetime.now(datetime.timezone.utc).isoformat();write(output/'result.json',result)
    return 0 if result['passed'] else 1

def prepare(args):
    output=args.output.resolve();require(output.is_relative_to(OWNED.resolve()) and not output.exists(),'Unique owned output')
    require(not any(linked(p) for p in [output,*output.parents]),'Linked output ancestor')
    output.mkdir(parents=True);result={'passed':False,'started_utc':datetime.datetime.now(datetime.timezone.utc).isoformat(),'stage':'preparing'}
    write(output/'result.json',result);info=None;observed=None;helpers={};configuration={};snapshot=output/'snapshot';digest=None;collector=None;host=None;attestation=None
    try:
        require(args.intake.resolve()==(ROOT/'.rustium-runtime/shared-strings-connection-latest.json').resolve(),'Use current applied intake')
        info=load(args.intake);require(info['source_manifest_sha256']==args.expected_source_sha256,'Explicit director authority mismatch');pins=source_check(info)
        run=Path(info['run_directory']);built=load(run/'build-result.json');job=load(args.build_job_result)
        require(job['task_id']=='FOUNDATION-STRINGS-BRIDGE' and job['kind']=='build' and job['status']=='passed' and job['exit_code']==0 and job['finished_utc'],'Completed serial build gate required')
        require(job['source_identity']['source_manifest_sha256']==info['source_manifest_sha256']==built['source_manifest_sha256'] and built['source_count']==1260 and built['passed'] and built['input_identity_before_after_matched'],'Successful exact applied build required')
        require(job['build_configuration']['targets']==['base_unittests','net_unittests'] and job['build_configuration']['compiler_jobs']==1,'Wrong consumer/configuration gate')
        require((run/'args.gn').read_bytes()==(OUT/'args.gn').read_bytes(),'Build args drift')
        require(all(c['exit_code']==0 for c in built['commands']) and {c['phase'] for c in built['commands']}=={'gn-gen','gn-check-base','gn-check-net','consumer-build'},'Missing GN/build evidence')
        command=built['commands'][-1]['command'];require(Path(command[0]).resolve()==BUILD/'third_party/ninja/ninja.exe' and '-j1' in command and '-n' not in command and command[-2:]==['base_unittests','net_unittests'],'Actual required build missing')
        for filename,h in built['tool_sha256'].items():require(sha(filename)==h,'Build tool drift')
        build_driver=ROOT/'.rustium-runtime/build_shared_strings_connection.py';require(sha(build_driver)==job['toolchain']['script_sha256'],'Build driver identity drift')
        original=ROOT/'base/strings/string_split_unittest.cc';frozen=ROOT/'docs/rust/migration/drafts/foundation-strings/reference/base/strings/string_split_unittest.cc'
        require(original.read_bytes()==frozen.read_bytes()==(BUILD/'base/strings/string_split_unittest.cc').read_bytes(),'Original test assertions changed')
        native,registry,inspector=runtime_modules();collector=(native,registry,inspector);observed=runtime(*collector)
        configuration=configuration_identity(native);host=host_identity()
        attestation=load(args.attestation);attest_job=load(args.attestation_job_result)
        lines=(args.attestation_job_result.parent/'output.log').read_text(errors='strict').splitlines()
        require(lines,'Missing gate completion output');attest_job['completion']=json.loads(lines[-1])
        attestation_gate_binding(attest_job,attestation,args.attestation,args.intake,info['source_manifest_sha256'],sha(args.attestation))
        attestation_binding(attestation,info['source_manifest_sha256'],configuration,runtime(*collector,targets=('base_unittests','net_unittests')))
        require(sha(Path(__file__).with_name('attest_build.py'))==attest_job['toolchain']['script_sha256'],'Attestation driver drift')
        require(sha(__file__)==attest_job['toolchain']['controller_sha256'] and sha(windows_job.__file__)==attest_job['toolchain']['windows_job_sha256'],'Attestation protocol source drift')
        require(all(sha(path)==h for path,h in attestation['scripts'].items()),'Attested helper/environment source drift')
        require(all(sha(path)==h for path,h in attestation['tool_sha256'].items()),'Attested compiler/linker drift')
        require(sha(OUT/'args.gn')==attestation['args_sha256'],'Attested build args mismatch')
        dry=[BUILD/'third_party/ninja/ninja.exe','-C',OUT,'-n','base_unittests','net_unittests']
        require(run_command(dry,BUILD,os.environ.copy(),output/'freshness.log')==0 and 'no work to do' in (output/'freshness.log').read_text().lower(),'Consumers not fresh')
        require(observed==runtime(*collector),'Runtime drift during freshness')
        qualified=ROOT/'.scratch/net-wave5/resumed-integration/native-runtime-delivery-v2/n17a/s/input-manifest.json'
        require(sha(qualified)=='b86e6a0d3f9ae40e6c96ac973d4d00454403a6a1cc0185b6d45cc99596dcc64f','Qualified parser snapshot manifest changed')
        parser_rows=[row for row in load(qualified)['files'] if row['copy']=='context/unit_parser.py']
        require(len(parser_rows)==1 and parser_rows[0]['sha256']=='d4e53f9f409206f83404c2cc2dbb2fede667bb1d1a30d1b948e101d7deaeb677','Qualified parser authority mismatch')
        snapshot.mkdir();context=snapshot/'context';context.mkdir()
        helper_files=[(Path(__file__).resolve(),'controller.py'),(Path(windows_job.__file__).resolve(),'windows_job.py'),(Path(native.__file__),'context/native_delivery.py'),(Path(registry.__file__),'context/dll_registry.py'),
            (ROOT/'.scratch/net-wave4/regular-net-suite-prep/regular_suite.py','context/reused_regular_suite.py'),
            (ROOT/'.scratch/net-wave5/resumed-integration/native-runtime-delivery-v2/n17a/s/context/unit_parser.py','context/unit_parser.py'),
            (inspector,'context/inspector/dumpbin.exe'),(build_driver,'context/build_driver.py'),
            (Path(__file__).with_name('attest_build.py'),'context/attest_build.py')]
        adjacent,external=registry.local_dll_dependencies(inspector,inspector)
        helper_files += [(inspector.parent/name,'context/inspector/'+name) for name in adjacent]
        for source,rel in helper_files:
            copy(source,snapshot/rel);helpers[str(source)]={'copy':rel,'sha256':sha(source)}
        require(sha(snapshot/'context/unit_parser.py')==parser_rows[0]['sha256'],'Parser differs from qualified snapshot')
        for directory,members in observed['directories'].items():
            destination=snapshot/'build'/Path(directory).relative_to(BUILD);destination.mkdir(parents=True,exist_ok=True)
            for member in members:
                if member.endswith('/'):(destination/member).mkdir(parents=True,exist_ok=True)
        for filename,h in observed['files'].items():
            source=Path(filename);copy(source,snapshot/'build'/source.relative_to(BUILD));require(sha(source)==h,'Runtime copy drift')
        for rel in pins:copy(ROOT/rel,snapshot/'editing'/rel)
        copy(original,snapshot/'editing/base/strings/string_split_unittest.cc')
        # Keep the original receipt at its runtime path; the reviewable proof
        # excludes cooperative local process identifiers, while binding raw bytes.
        proof={k:v for k,v in job.items() if k not in ['launcher_pid','child_pid']}
        proof['original_result_sha256']=sha(args.build_job_result)
        (snapshot/'provenance').mkdir();write(snapshot/'provenance/build-gate-proof.json',proof)
        attest_proof={k:v for k,v in attest_job.items() if k not in ['launcher_pid','child_pid']}
        attest_proof['original_result_sha256']=sha(args.attestation_job_result)
        write(snapshot/'provenance/attestation-gate-proof.json',attest_proof)
        for source in [run/'build-result.json',run/'args.gn',run/'application.json',Path(info['source_manifest']),output/'freshness.log',args.attestation,qualified]:
            copy(source,snapshot/'provenance'/source.name)
        for filename,h in configuration.items():
            source=Path(filename);copy(source,snapshot/'configuration'/source.relative_to(OUT));require(sha(source)==h,'Configuration copy drift')
        metadata={'info':info,'runtime':observed,'helpers':helpers,'configuration':configuration,'host_identity':host,'patterns':PATTERNS,'required_bridge_cases':sorted(BRIDGE),'inventory':inventory(snapshot),
            'dcheck_evidence':{'path':'configuration/obj/base/base_unittests.ninja','rule':'Configured Ninja defines retained in full evaluated graph; diagnostic must actually execute without native skip regardless of is_debug'},
            'host_limits':{'python_executable':str(Path(sys.executable).resolve()),'python_sha256':sha(sys.executable),'python_version':sys.version,'provider_scope':'Installed Python/stdlib and nonadjacent/system DLLs remain identified host providers; dynamic plugins not captured','inspector_external_import_names':external}}
        write(snapshot/'input-manifest.json',metadata);digest=sha(snapshot/'input-manifest.json');verify(snapshot,digest)
        result.update(stage='executing_copied_controller',manifest_sha256=digest);write(output/'result.json',result)
        code=run_command([sys.executable,'-B',snapshot/'controller.py','--run-sealed','--snapshot',snapshot,'--manifest-sha256',digest,'--execution',output/'execution'],output,os.environ.copy(),output/'controller.log',timeout=1200)
        result['controller_exit']=code;result['execution']=load(output/'execution/result.json');result['passed']=code==0 and result['execution']['passed']
    except BaseException as error:
        result['exception']=type(error).__name__+': '+str(error)
        if isinstance(error,CommandFailure):result['command_failure']=error.details
    finally:
        result['execution_artifacts']={name:(output/'execution'/name).exists() for name in ['result.json','summary.json','gtest.xml','output.log']}
        result['controller_log_available']=(output/'controller.log').exists()
        checks={}
        for name,check in [('source',lambda:source_check(info)),('runtime',lambda:require(observed==runtime(*collector),'Original runtime drift')),
                ('configuration',lambda:require(configuration and configuration_identity(collector[0])==configuration,'Original configuration content/membership drift')),
                ('host',lambda:require(host is not None and host_identity()==host,'Python interpreter/DLL drift')),
                ('helpers',lambda:require(helpers and all(sha(p)==row['sha256'] for p,row in helpers.items()),'Original helper drift')),
                ('snapshot',lambda:verify(snapshot,digest))]:
            try:check();checks[name]=True
            except BaseException as error:checks[name]=type(error).__name__+': '+str(error)
        result['identity_checks_after']=checks;result['passed'] &= all(value is True for value in checks.values())
        result['finished_utc']=datetime.datetime.now(datetime.timezone.utc).isoformat();write(output/'result.json',result);print(json.dumps(result))
    return 0 if result['passed'] else 1

if __name__=='__main__':
    parser=argparse.ArgumentParser();parser.add_argument('--run-sealed',action='store_true');parser.add_argument('--intake',type=Path);parser.add_argument('--output',type=Path)
    parser.add_argument('--expected-source-sha256');parser.add_argument('--build-job-result',type=Path);parser.add_argument('--attestation',type=Path);parser.add_argument('--attestation-job-result',type=Path);parser.add_argument('--snapshot',type=Path);parser.add_argument('--manifest-sha256');parser.add_argument('--execution',type=Path)
    args=parser.parse_args();sys.exit(execute(args) if args.run_sealed else prepare(args))
