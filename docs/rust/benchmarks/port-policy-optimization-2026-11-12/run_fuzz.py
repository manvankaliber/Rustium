from pathlib import Path
import subprocess,os,ctypes,json,time,hashlib,shutil
w=Path('D:/Rustium/.scratch/net-wave1/port-policy/optimization');corpus=w/'corpus';corpus.mkdir(exist_ok=True)
seeds=[b'\0\x16',b'\0\x16\0\x16',b'\xff\xff',bytes(range(48))]
for i,b in enumerate(seeds):
 if b:(corpus/str(i)).write_bytes(b)
initial=w/'fuzz-initial-corpus';initial.mkdir(exist_ok=True)
manifest={}
for path in corpus.iterdir():
 if path.is_file():
  shutil.copyfile(path,initial/path.name);manifest[path.name]=hashlib.sha256(path.read_bytes()).hexdigest()
(w/'fuzz-initial-corpus-manifest.json').write_text(json.dumps(manifest,indent=2))
cmd=[str(w/'fuzz.exe'),'-runs=1000000','-max_len=48','-seed=20261112',str(corpus)]
env=os.environ.copy();env['PATH']='D:/RustiumBuild/src/out/Default;'+env['PATH'];log=open(w/'fuzz-results.log','wb')
p=subprocess.Popen(cmd,cwd=w,env=env,stdout=log,stderr=subprocess.STDOUT,creationflags=4)
k=ctypes.WinDLL('kernel32',use_last_error=True);k.SetProcessAffinityMask.argtypes=[ctypes.c_void_p,ctypes.c_size_t];k.SetProcessAffinityMask.restype=ctypes.c_int
assert k.SetProcessAffinityMask(int(p._handle),1<<6),ctypes.get_last_error()
n=ctypes.WinDLL('ntdll');n.NtResumeProcess.argtypes=[ctypes.c_void_p];assert n.NtResumeProcess(int(p._handle))==0
rc=p.wait();log.close();(w/'fuzz-execution.json').write_text(json.dumps({'command':cmd,'cpu_affinity_mask':64,'cpu':6,'exit_code':rc,'executable_sha256':hashlib.sha256((w/'fuzz.exe').read_bytes()).hexdigest()},indent=2));print('Fuzz exit',rc)
print((w/'fuzz-results.log').read_text(errors='replace')[-2000:])
