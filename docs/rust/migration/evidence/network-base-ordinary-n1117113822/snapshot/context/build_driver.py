import datetime,hashlib,importlib.util,json,pathlib,subprocess,sys,time
r=pathlib.Path('D:/Rustium');b=pathlib.Path('D:/RustiumBuild/src');rt=r/'.rustium-runtime';info=json.loads((rt/'shared-strings-connection-latest.json').read_text());out=pathlib.Path(info['run_directory']);manifest=pathlib.Path(info['source_manifest']);pins=json.loads(manifest.read_text());sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
spec=importlib.util.spec_from_file_location('env',r/'.scratch/net-wave5/configure_http2_family.py');mod=importlib.util.module_from_spec(spec);spec.loader.exec_module(mod);env=mod.build_environment()
def verify():
 assert sha(manifest)==info['source_manifest_sha256']
 for p,h in pins.items():assert sha(r/p)==h==sha(b/p),p
verify();(out/'args.gn').write_bytes((b/'out/Default/args.gn').read_bytes())
tools=[b/'buildtools/win/gn.exe',b/'third_party/ninja/ninja.exe',b/'third_party/llvm-build/Release+Asserts/bin/clang-cl.exe',b/'third_party/llvm-build/Release+Asserts/bin/lld-link.exe',b/'third_party/rust-toolchain/bin/rustc.exe'];toolpins={str(p):sha(p) for p in tools if p.exists()}
commands=[('gn-gen',[tools[0],'gen','out/Default']),('gn-check-base',[tools[0],'check','out/Default','//base:base_unittests']),('gn-check-net',[tools[0],'check','out/Default','//net:*']),('consumer-build',[tools[1],'-C','out/Default','-j1','base_unittests','net_unittests'])]
records=[];ok=True
for phase,command in commands:
 start=time.monotonic();cmd=[str(x) for x in command]
 with (out/(phase+'.log')).open('wb') as log:ret=subprocess.run(cmd,cwd=b,env=env,stdout=log,stderr=subprocess.STDOUT).returncode
 row={'phase':phase,'command':cmd,'exit_code':ret,'elapsed_seconds':time.monotonic()-start};records.append(row);print(json.dumps(row),flush=True);(out/'build-progress.json').write_text(json.dumps(records,indent=2)+'\n')
 if ret:ok=False;print((out/(phase+'.log')).read_text(errors='replace')[-6000:],flush=True);break
verify();assert all(sha(pathlib.Path(p))==h for p,h in toolpins.items())
result={'passed':ok,'source_manifest_sha256':info['source_manifest_sha256'],'source_count':len(pins),'input_identity_before_after_matched':True,'tool_sha256':toolpins,'commands':records,'native_execution':False,'scope':'Configured GN and serial base/net consumers for55 applied bindings; actual production-path native tests and remaining consumers/platform/performance still pending'};(out/'build-result.json').write_text(json.dumps(result,indent=2)+'\n');sys.exit(0 if ok else 1)
