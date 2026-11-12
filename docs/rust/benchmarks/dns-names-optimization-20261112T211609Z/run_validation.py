from pathlib import Path
import os,subprocess,json,hashlib
w=Path('D:/Rustium/.scratch/net-wave1/dns-names/optimization');env=os.environ.copy();env['PATH']='D:/RustiumBuild/src/out/Default;'+env['PATH'];env['LLVM_PROFILE_FILE']=str(w/'unit-verified-%p.profraw')
results=[]
for name,args in [('unit',[]),('bench',['--verify-only'])]:
 cmd=[str(w/(name+'.exe')),*args];p=subprocess.run(cmd,cwd=w,env=env,capture_output=True);(w/(name+'-results.log')).write_bytes(p.stdout+p.stderr);results.append({'command':cmd,'exit_code':p.returncode,'executable_sha256':hashlib.sha256((w/(name+'.exe')).read_bytes()).hexdigest(),'benchmark_timing':False});print(name,p.returncode);print(p.stdout.decode(errors='replace')[-300:]);assert p.returncode==0
(w/'validation-execution.json').write_text(json.dumps(results,indent=2))
