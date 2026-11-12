from pathlib import Path
import ctypes,subprocess,os,sys,json,hashlib
work=Path('D:/Rustium/.scratch/net-wave1/auth-challenges');out=Path('D:/RustiumBuild/src/out/Default');mode=sys.argv[1];env=os.environ.copy();env['PATH']=str(out)+';'+env.get('PATH','');env['LLVM_PROFILE_FILE']=str(work/'auth-%p.profraw');cmd=[str(work/(mode+'.exe'))]
if mode=='fuzz':
 corpus=work/'corpus';corpus.mkdir(exist_ok=True)
 for i,seed in enumerate([b'Negotiate',b'Negotiate Zm9vYmFy',b'NTLM ====',b'Negotiate Zh==',b'NTLM qop=auth',b'\x00\xff']): (corpus/str(i)).write_bytes(seed)
 cmd += [str(corpus),'-runs=1000000','-max_len=256','-seed=20261112','-print_final_stats=1','-artifact_prefix='+str(work/'crash-')]
if mode=='unit':cmd+=['--gtest_output=json:'+str(work/'unit-results.json')]
with (work/(mode+'-run.log')).open('w') as log:
 p=subprocess.Popen(cmd,cwd=work,env=env,stdout=log,stderr=subprocess.STDOUT)
 ctypes.windll.kernel32.SetProcessAffinityMask(ctypes.c_void_p(int(p._handle)),ctypes.c_size_t(2))
 code=p.wait()
(work/(mode+'-execution.json')).write_text(json.dumps({'command':cmd,'exit_code':code,'affinity_mask':2,'sha256':hashlib.sha256((work/(mode+'.exe')).read_bytes()).hexdigest(),'benchmark':False},indent=2)+'\n');print((work/(mode+'-run.log')).read_text());sys.exit(code)
