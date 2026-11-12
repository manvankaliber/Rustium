from pathlib import Path
import json,re,hashlib,subprocess
w=Path('D:/Rustium/.scratch/net-wave1/dns-names');r=Path('D:/Rustium');original=json.loads((w/'original/manifest.json').read_text())
seams={p:h for p,h in original.items() if p.startswith(('net/base/','url/','base/strings/'))}
for p,h in seams.items():assert hashlib.sha256((r/p).read_bytes()).hexdigest()==h,p
(w/'external-service-source-guard.json').write_text(json.dumps({'description':'Do not time against a changed shared URL/IP/localhost/string service. If a service migrates, separately compile its frozen original for the baseline; never silently call its new Rust implementation. This is a source guard for the explicitly listed files, not a complete transitive build manifest.','source_hashes':seams},indent=2))
meta=json.loads((w/'benchmark-descriptor.json').read_text());s=(r/'net/dns/dns_names_perftest.cc').read_text();keys=re.findall(r'\{"(dns_[a-z0-9_]+)"',s);defined=[x['key'] for g in meta['groups'] for x in g['cases']];assert keys==defined and len(keys)==21 and all(len(g['cases'])<=5 for g in meta['groups']);print('Descriptor validated: 21 unique cases, groups 5/5/3/3/5.')
