import os, subprocess, json
from pathlib import Path
w=Path('D:/Rustium/.scratch/net-wave1/content-disposition');env=os.environ.copy();env['PATH']='D:/RustiumBuild/src/out/Default;'+env['PATH'];env['LLVM_PROFILE_FILE']=str(w/'unit.profraw')
p=subprocess.run([str(w/'unit.exe'),'--gtest_output=json:'+str(w/'unit-results.json')],env=env,cwd='D:/RustiumBuild/src/out/Default',capture_output=True,text=True);(w/'unit-run.log').write_text(p.stdout+p.stderr); print('exit',p.returncode);print((p.stdout+p.stderr)[-8000:])

(w/'unit-execution.json').write_text(json.dumps({'exit_code':p.returncode,'command':[str(w/'unit.exe'),'--gtest_output=json:'+str(w/'unit-results.json')],'cwd':'D:/RustiumBuild/src/out/Default','LLVM_PROFILE_FILE':env['LLVM_PROFILE_FILE']},indent=2)+'\n')
p.check_returncode()
