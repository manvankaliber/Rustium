"""Prepared follower build receipt: execute only after the active build ends."""
import argparse
import datetime
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import sys

sys.dont_write_bytecode=True
import ordinary_base_v2 as delivery

def main(args):
    root=delivery.ROOT;build=delivery.BUILD;output=args.output.resolve()
    delivery.require(output.is_relative_to((root/'.scratch/base-attested-build').resolve()) and not output.exists(),'Unique attestation output required')
    delivery.require(not any(delivery.linked(p) for p in [output,*output.parents]),'Linked attestation ancestor')
    delivery.require(args.invocation and delivery.re.fullmatch('[0-9a-f]{32}',args.invocation),'Unique invocation required')
    output.mkdir(parents=True);result={'passed':False,'invocation':args.invocation,'started_utc':datetime.datetime.now(datetime.timezone.utc).isoformat(),'cwd':str(build),'commands':delivery.expected_build_commands(),'exit_codes':[]}
    delivery.write(output/'receipt.json',result);info=None;host=None;observed=None;collector=None;configuration=None
    try:
        info=delivery.load(args.intake);delivery.require(args.intake.resolve()==(root/'.rustium-runtime/shared-strings-connection-latest.json').resolve(),'Current applied intake required')
        delivery.require(info['source_manifest_sha256']==args.expected_source_sha256,'Director source authority mismatch');delivery.source_check(info)
        prior=delivery.load(Path(info['run_directory'])/'build-result.json')
        delivery.require(prior['passed'] and prior['input_identity_before_after_matched'] and prior['source_manifest_sha256']==info['source_manifest_sha256'],'Prior substantial build must complete successfully first')
        spec=importlib.util.spec_from_file_location('configured_env',root/'.scratch/net-wave5/configure_http2_family.py');envmod=importlib.util.module_from_spec(spec);spec.loader.exec_module(envmod);env=envmod.build_environment()
        host=delivery.host_identity();tools={str(p):delivery.sha(p) for p in [build/'buildtools/win/gn.exe',build/'third_party/ninja/ninja.exe',build/'third_party/llvm-build/Release+Asserts/bin/clang-cl.exe',build/'third_party/llvm-build/Release+Asserts/bin/lld-link.exe',build/'third_party/rust-toolchain/bin/rustc.exe']}
        for index,command in enumerate(result['commands']):
            code=delivery.run_command(command,build,env,output/('command-'+str(index)+'.log'),timeout=7200 if index==3 else 600)
            result['exit_codes'].append(code);delivery.write(output/'receipt.json',result);delivery.require(code==0,'Attested configured command failed')
        native,registry,inspector=delivery.runtime_modules();collector=(native,registry,inspector)
        # This is captured inside the successful actual build gate, not inferred
        # later from an unrelated successful receipt or a dry-run alone.
        configuration=delivery.configuration_identity(native)
        observed=delivery.runtime(*collector,targets=('base_unittests','net_unittests'))
        delivery.require(configuration==delivery.configuration_identity(native),'Configuration membership/content changed during receipt')
        delivery.require(observed==delivery.runtime(*collector,targets=('base_unittests','net_unittests')),'Consumer output identity changed during receipt')
        delivery.source_check(info);delivery.require(all(delivery.sha(p)==h for p,h in tools.items()),'Toolchain drift')
        result.update(passed=True,source_manifest_sha256=info['source_manifest_sha256'],source_inputs_stable=True,
            runtime=observed,configuration=configuration,tool_sha256=tools,args_sha256=delivery.sha(build/'out/Default/args.gn'),
            scripts={str(p.resolve()):delivery.sha(p) for p in [Path(__file__),Path(delivery.__file__),Path(delivery.windows_job.__file__),Path(envmod.__file__)]},
            host_identity=host,scope='Exact commands/configuration/graph/consumer outputs at successful serial build completion; no native test, ABI/platform/performance acceptance')
    except BaseException as error:
        result['exception']=type(error).__name__+': '+str(error)
        if isinstance(error,delivery.CommandFailure):result['command_failure']=error.details
    finally:
        checks={}
        for name,check in [('source',lambda:delivery.source_check(info)),
                ('configuration',lambda:delivery.require(configuration is not None and delivery.configuration_identity(collector[0])==configuration,'Configuration drift')),
                ('runtime',lambda:delivery.require(observed is not None and delivery.runtime(*collector,targets=('base_unittests','net_unittests'))==observed,'Output drift')),
                ('host',lambda:delivery.require(host is not None and delivery.host_identity()==host,'Host interpreter/DLL drift'))]:
            try:check();checks[name]=True
            except BaseException as error:checks[name]=type(error).__name__+': '+str(error)
        result['postchecks']=checks;result['passed'] &= all(v is True for v in checks.values());result['finished_utc']=datetime.datetime.now(datetime.timezone.utc).isoformat()
        delivery.write(output/'receipt.json',result);print(json.dumps({'invocation':args.invocation,'passed':result['passed'],'receipt':str(output/'receipt.json'),'receipt_sha256':delivery.sha(output/'receipt.json')}))
    return 0 if result['passed'] else 1

if __name__=='__main__':
    parser=argparse.ArgumentParser();parser.add_argument('--intake',type=Path,required=True);parser.add_argument('--output',type=Path,required=True);parser.add_argument('--expected-source-sha256',required=True);parser.add_argument('--invocation',required=True);parser.add_argument('-j1',action='store_true')
    sys.exit(main(parser.parse_args()))
