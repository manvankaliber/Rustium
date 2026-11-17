import datetime,hashlib,importlib.util,json,pathlib,subprocess,sys
r=pathlib.Path('D:/Rustium');b=pathlib.Path('D:/RustiumBuild/src');rt=r/'.rustium-runtime';sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest();info=json.loads((rt/'shared-strings-connection-latest.json').read_text());out=pathlib.Path(info['run_directory'])/'changed-object-preflight'
if sys.argv[1:]!=['-j1','run']:
 assert not (rt/'job-active.json').exists()
 obs=subprocess.check_output(['powershell','-NoProfile','-Command',"Get-CimInstance Win32_Process | Where-Object { $_.Name -match '^(ninja|clang-cl|lld-link|net_unittests|base_unittests|rustc|cargo).*' } | Select-Object ProcessId,Name,CommandLine | ConvertTo-Json"],text=True).strip();assert not obs,obs
 request={'task_id':'FOUNDATION-STRINGS-BRIDGE','kind':'build','source_identity':info,'toolchain':{'script_sha256':sha(pathlib.Path(__file__))},'build_configuration':{'scope':'Network dependency changed-object compiler preflight plus dirty-edge explanation; complete consumer build still required','compiler_jobs':1},'cwd':str(r),'process_reconciliation':{'observed_utc':datetime.datetime.now(datetime.timezone.utc).isoformat(),'compilers_native_tests':[]}}
 p=rt/'network-changed-object-preflight-request.json';p.write_text(json.dumps(request,indent=2)+'\n');token=json.loads((rt/'director.json').read_text())['token'];sys.exit(subprocess.call(['python','tools/rustium_coord.py','job','--token',token,'--manifest',str(p),'--','python','-B',__file__,'-j1','run'],cwd=r))
assert not out.exists();out.mkdir();pins=json.loads(pathlib.Path(info['source_manifest']).read_text());tools={str(p):sha(p) for p in [b/'third_party/ninja/ninja.exe',b/'third_party/llvm-build/Release+Asserts/bin/clang-cl.exe']}
def verify():
 assert sha(pathlib.Path(info['source_manifest']))==info['source_manifest_sha256']
 assert all(sha(r/p)==h==sha(b/p) for p,h in pins.items())
verify();sys.path.insert(0,str(r/'docs/rust/migration/drafts/base-ordinary-delivery'));import ordinary_base_v2 as delivery
spec=importlib.util.spec_from_file_location('configured_env',r/'.scratch/net-wave5/configure_http2_family.py');mod=importlib.util.module_from_spec(spec);spec.loader.exec_module(mod);env=mod.build_environment()
ninja=str(b/'third_party/ninja/ninja.exe');commands=[
 [ninja,'-C','out/Default','-n','-d','explain','-j1','base_unittests','net_unittests'],
 [ninja,'-C','out/Default','-j1','obj/base/test/run_all_unittests/run_all_unittests.obj','obj/base/base_unittests/string_split_rust_bridge_unittest.obj']]
record={'passed':False,'started_utc':datetime.datetime.now(datetime.timezone.utc).isoformat(),'source_manifest_sha256':info['source_manifest_sha256'],'commands':commands,'exit_codes':[],'scope':'Compiler feedback and dirty-edge evidence only; full consumer/link/native validation remains required'}
try:
 for index,cmd in enumerate(commands):
  code=delivery.run_command(cmd,b,env,out/('command-'+str(index)+'.log'),timeout=1800);record['exit_codes'].append(code)
  if code:break
 verify();assert all(sha(p)==h for p,h in tools.items());record.update(passed=record['exit_codes']==[0,0],source_inputs_stable=True,tools=tools)
except BaseException as error:
 record['exception']=type(error).__name__+': '+str(error)
 if isinstance(error,delivery.CommandFailure):record['command_failure']=error.details
finally:
 record['finished_utc']=datetime.datetime.now(datetime.timezone.utc).isoformat();(out/'result.json').write_text(json.dumps(record,indent=2)+'\n');print(json.dumps(record))
sys.exit(0 if record['passed'] else 1)
