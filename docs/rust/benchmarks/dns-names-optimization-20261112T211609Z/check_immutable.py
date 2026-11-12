from pathlib import Path
import json,hashlib
r=Path('D:/Rustium');w=r/'.scratch/net-wave1/dns-names/optimization';before=json.loads((w/'before/source-hashes.json').read_text())
for name in ['dns_names_reference.cc','dns_names_reference.h','dns_names_perftest.cc','dns_names_util_unittest.cc','dns_names_util.h','dns_names_fuzzer.cc']:
 p='net/dns/'+name;assert hashlib.sha256((r/p).read_bytes()).hexdigest()==before[p],p
print('Frozen references, benchmark harness, existing original tests, public header and fuzzer unchanged.')
