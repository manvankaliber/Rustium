import os,subprocess,pathlib,json,hashlib
w=pathlib.Path('D:/Rustium/.scratch/net-wave1/port-policy/optimization');env=os.environ.copy();env['PATH']='D:/RustiumBuild/src/out/Default;'+env['PATH'];results=[]
for name,filter in [('original','NetUtilTest.*'),('differential','PortPolicyRust.*')]:
 env['LLVM_PROFILE_FILE']=str(w/(name+'-optimized-final-%p.profraw'))
 cmd=[str(w/'unit.exe'),'--gtest_filter='+filter,'--gtest_color=no'];p=subprocess.run(cmd,cwd=w,env=env,capture_output=True);(w/(name+'-results.log')).write_bytes(p.stdout+p.stderr);print(name,p.returncode,p.stdout.decode(errors='replace')[-350:]);results.append({'command':cmd,'exit_code':p.returncode,'binary_sha256':hashlib.sha256((w/'unit.exe').read_bytes()).hexdigest()});assert p.returncode==0
(w/'validation-execution.json').write_text(json.dumps(results,indent=2))


