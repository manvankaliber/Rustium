from pathlib import Path
import subprocess,json
w=Path('D:/Rustium/.scratch/net-wave1/port-policy/optimization');tools=Path('D:/Rustium/.scratch/gzip-header/llvm-tools/bin');raw=[*w.glob('original-optimized-final-*.profraw'),*w.glob('differential-optimized-final-*.profraw')]
subprocess.run([str(tools/'llvm-profdata.exe'),'merge','-sparse',*map(str,raw),'-o',str(w/'coverage.profdata')],check=True)
cmd=[str(tools/'llvm-cov.exe'),'export',str(w/'unit.exe'),'-instr-profile='+str(w/'coverage.profdata'),'-skip-functions','-ignore-filename-regex=third_party']
p=subprocess.run(cmd,capture_output=True,check=True);(w/'coverage.json').write_bytes(p.stdout)
for f in json.loads(p.stdout)['data'][0]['files']:
 if 'port_policy_' in f['filename']: print(f['filename'],json.dumps(f['summary']))
(w/'coverage-command.json').write_text(json.dumps(cmd,indent=2))


