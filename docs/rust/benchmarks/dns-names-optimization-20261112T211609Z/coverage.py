from pathlib import Path
import subprocess,json
w=Path('D:/Rustium/.scratch/net-wave1/dns-names/optimization');tools=Path('D:/Rustium/.scratch/gzip-header/llvm-tools/bin')
raw=list(w.glob('unit-verified-*.profraw'));assert raw
cmd=[str(tools/'llvm-profdata.exe'),'merge','-sparse',*map(str,raw),'-o',str(w/'coverage.profdata')];subprocess.run(cmd,check=True)
cmd=[str(tools/'llvm-cov.exe'),'export',str(w/'unit.exe'),'-instr-profile='+str(w/'coverage.profdata'),'-skip-functions','-ignore-filename-regex=third_party']
p=subprocess.run(cmd,capture_output=True,check=True);(w/'coverage.json').write_bytes(p.stdout);d=json.loads(p.stdout)
for f in d['data'][0]['files']:
 if 'dns_names' in f['filename']:
  print(f['filename'],json.dumps(f['summary']))
  print('Zero count segments', [s for s in f['segments'] if s[2]==0 and s[3]])
(w/'coverage-command.json').write_text(json.dumps(cmd,indent=2))
