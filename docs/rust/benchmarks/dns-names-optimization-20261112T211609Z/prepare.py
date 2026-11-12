from pathlib import Path
import shutil,hashlib,json
r=Path('D:/Rustium');w=r/'.scratch/net-wave1/dns-names/optimization';b=w/'before';b.mkdir(exist_ok=True)
paths=list((r/'net/dns').glob('dns_names_*'))
m={}
for p in paths:
 if p.is_file():
  t=b/p.name;shutil.copyfile(p,t);m[p.relative_to(r).as_posix()]=hashlib.sha256(p.read_bytes()).hexdigest()
(b/'source-hashes.json').write_text(json.dumps(m,indent=2))
s=(r/'.scratch/net-wave1/dns-names/build_native.py').read_text().replace("work=root/'.scratch/net-wave1/dns-names'", "work=root/'.scratch/net-wave1/dns-names/optimization'")
s=s.replace("command+=' /DNET_IMPLEMENTATION '+extra", "command+=' /DNET_IMPLEMENTATION /DIS_DNS_NAMES_REFERENCE_IMPL '+extra")
(w/'build_native.py').write_text(s)
shutil.copyfile(r/'.scratch/net-wave1/dns-names/native_seams.cc',w/'native_seams.cc')
for name in ['run_validation.py','run_fuzz.py','coverage.py']:
 s=(r/'.scratch/net-wave1/dns-names'/name).read_text().replace("D:/Rustium/.scratch/net-wave1/dns-names'", "D:/Rustium/.scratch/net-wave1/dns-names/optimization'")
 (w/name).write_text(s)
for p in (r/'.scratch/net-wave1/dns-names').glob('*.dll'):shutil.copyfile(p,w/p.name)
shutil.copyfile(r/'.scratch/net-wave1/dns-names/icudtl.dat',w/'icudtl.dat')
shutil.copytree(r/'.scratch/net-wave1/dns-names/fuzz-initial-corpus',w/'corpus',dirs_exist_ok=True)
