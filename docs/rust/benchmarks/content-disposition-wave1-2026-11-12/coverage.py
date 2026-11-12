import subprocess,json
from pathlib import Path
w=Path('D:/Rustium/.scratch/net-wave1/content-disposition');t=Path('D:/Rustium/.scratch/gzip-header/llvm-tools/bin')
subprocess.run([str(t/'llvm-profdata.exe'),'merge','-sparse',str(w/'unit.profraw'),'-o',str(w/'unit.profdata')],check=True)
with (w/'coverage.json').open('wb') as f:subprocess.run([str(t/'llvm-cov.exe'),'export',str(w/'unit.exe'),'-instr-profile='+str(w/'unit.profdata')],stdout=f,check=True)
d=json.loads((w/'coverage.json').read_text())['data'][0]; totals={}
for f in d['files']:
 if 'content_disposition_' in f['filename']:
  print(f['filename'],f['summary']);totals[f['filename']]=f['summary']
(w/'coverage-summary.json').write_text(json.dumps(totals,indent=2)+'\n')
for f in d['functions']:
 if not f['count'] and any('content_disposition' in name for name in f['filenames']):print('ZERO',f['name'],f['regions'][0])
