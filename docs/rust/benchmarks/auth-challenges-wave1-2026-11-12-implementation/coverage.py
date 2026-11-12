from pathlib import Path
import json,subprocess
work=Path('D:/Rustium/.scratch/net-wave1/auth-challenges'); tool=Path('D:/Rustium/.scratch/gzip-header/llvm-tools/bin');profiles=[max(work.glob('auth-*.profraw'), key=lambda p: p.stat().st_mtime_ns)]
subprocess.run([str(tool/'llvm-profdata.exe'),'merge','-sparse',*map(str,profiles),'-o',str(work/'unit.profdata')],check=True)
data=json.loads(subprocess.check_output([str(tool/'llvm-cov.exe'),'export',str(work/'unit.exe'),'-instr-profile='+str(work/'unit.profdata')]))
(work/'coverage-raw.json').write_text(json.dumps(data)+'\n');files=[f for d in data['data'] for f in d['files'] if f['filename'].endswith(('auth_challenge_core.rs','auth_challenge_ffi.rs'))];summary={f['filename']:f['summary'] for f in files};summary['method']='LLVM source instrumentation; native original and new unit suites; authored Rust core and adapter; no branch coverage collected';summary['profiles']=[str(p) for p in profiles];(work/'coverage-summary.json').write_text(json.dumps(summary,indent=2)+'\n');print(json.dumps(summary,indent=2))

