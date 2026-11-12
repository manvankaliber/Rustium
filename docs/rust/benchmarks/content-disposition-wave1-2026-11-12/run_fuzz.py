import subprocess,os,ctypes,json
from pathlib import Path
w=Path('D:/Rustium/.scratch/net-wave1/content-disposition');c=w/'corpus';c.mkdir(exist_ok=True)
seeds=[b'',b'attachment; filename=x',b'filename*="utf-8\'\'foo',b'=?UTF-8?Q?=ff?=',b'abc%CC%88',b'YQ==',b'=?UTF-8?Q?foo?= =?UTF-8?Q?bar?=',b'filename="x\\"',b'filename*=utf-8\'\'%ef%b7%90',b'inline, attachment',b'\x00\xff\xc3\xa9']
for i,s in enumerate(seeds):
 for mode in range(6): (c/f'{i}-{mode}').write_bytes(bytes([mode*8])+s)
env=os.environ.copy();env['PATH']='D:/RustiumBuild/src/out/Default;'+env['PATH'];cmd=[str(w/'fuzz.exe'),str(c),'-runs=1000000','-max_len=512','-seed=20261112','-print_final_stats=1','-artifact_prefix='+str(w)+'/']
with (w/'fuzz-run.log').open('wb') as log:
 p=subprocess.Popen(cmd,cwd=w,env=env,stdout=log,stderr=log)
 mask=2
 if not ctypes.windll.kernel32.SetProcessAffinityMask(int(p._handle),mask):raise RuntimeError('affinity failed')
 code=p.wait()
(w/'fuzz-execution.json').write_text(json.dumps({'command':cmd,'exit_code':code,'affinity_mask':mask,'max_len':512,'seed':20261112},indent=2)+'\n')
print('fuzzer exit',code);print((w/'fuzz-run.log').read_text(errors='replace')[-5000:])
raise SystemExit(code)
