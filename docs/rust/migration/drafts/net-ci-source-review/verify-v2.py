import collections, hashlib, io, json, pathlib, subprocess, zipfile
C = '5719c09eb5483dce2ec0562b6ccd10e082e504a5'
P = 'fe18b237de48d2fbc0026a534c61ae8281dbddef'
BASE = pathlib.Path(__file__).resolve().parent
E = 'docs/rust/migration/evidence/'
def blob(path, rev=C):
    return subprocess.check_output(['git', 'show', rev + ':' + path])
def sha(data): return hashlib.sha256(data).hexdigest()
def jblob(path, rev=C): return json.loads(blob(path, rev))
errors = collections.defaultdict(list)
r = jblob(E + 'network-ci-source-repair-v2.json')
a = jblob(E + 'network-ci-source-assembly-v1.json')
inv = jblob('tools/ci/network-validation-inventory.json')
plan_path = pathlib.Path('D:/Rustium/.scratch/net-wave5/resumed-integration/native-runtime-delivery-v2/network-focus-20261117-plan.json')
plan_bytes = plan_path.read_bytes(); plan = json.loads(plan_bytes)
fields = ['family','target','label','kind','arguments','case_ids','expected_workloads','expected_tests','completion','clock_scope','measurement_clocks_requested','blockers']
if len(inv['cases']) != len(plan['cases']): errors['case_count'].append('different')
for i, (x, y) in enumerate(zip(inv['cases'], plan['cases'])):
    for field in fields:
        if x.get(field) != y.get(field): errors['case_fields'].append([i,field])
oldinv = jblob('tools/ci/network-validation-inventory.json', P)
if inv['build_roots'] != oldinv['build_roots']: errors['roots_changed'].append(True)
entrypoints=[]
for case in inv['cases']:
    if 'entrypoint' in case:
        pin=case['entrypoint']; actual=sha(blob(pin['path'])); entrypoints.append({'path':pin['path'],'expected':pin['sha256'],'actual':actual})
        if actual != pin['sha256']: errors['entrypoints'].append(pin['path'])
raw=blob(E+'network-ci-raw-source-inputs.zip'); z=zipfile.ZipFile(io.BytesIO(raw)); names=z.namelist(); idx=json.loads(z.read('input-index.json'))
if len(names)!=len(set(names)): errors['duplicate_zip_names'].append(True)
if z.testzip(): errors['zip_crc'].append(True)
if sha(raw)!=r['raw_source_archive_sha256'] or sha(raw)!=inv['source_basis']['raw_inputs_archive_sha256']: errors['archive_sha'].append(True)
for key,filename in [('cohort_manifest_sha256','cohort-manifest.json'),('auth_manifest_sha256','auth-manifest.json')]:
    if sha(z.read(filename))!=idx[key]: errors['manifest_sha'].append(filename)
cohort=json.loads(z.read('cohort-manifest.json')); auth=json.loads(z.read('auth-manifest.json'))
if cohort != idx['groups']['cohort']: errors['cohort_index'].append(True)
if {v['path']:v['after_sha256'] for v in auth['changes']} != idx['groups']['auth_proposals']: errors['auth_index'].append(True)
used=set()
for group, rows in idx['groups'].items():
    for path,digest in rows.items():
        used.add(digest); name='objects/sha256/'+digest+'.bin'
        if name not in names or sha(z.read(name))!=digest: errors['object_hash'].append([group,path])
        record=a['paths'].get('net/http/hsts_fuzzer_corpus/embedded-nul.header' if path.endswith('/hsts_fuzzer_corpus/nul.header') else path)
        if not record: errors['index_assembly_missing'].append([group,path]); continue
        expected=record.get('proposal_raw_sha256') if group=='auth_proposals' else (record.get('before_raw_sha256',record.get('root_raw_sha256')) if group=='cohort' else record.get('root_raw_sha256'))
        if digest != expected: errors['index_assembly_hash'].append([group,path,digest,expected])
objects={x[len('objects/sha256/'):-4] for x in names if x.startswith('objects/sha256/') and x.endswith('.bin')}
if used!=objects: errors['object_set'].append({'missing':sorted(used-objects),'extra':sorted(objects-used)})
for row in r['source_whitespace_repairs']:
    if sha(blob(row['path'],P))!=row['before_sha256'] or sha(blob(row['path']))!=row['after_sha256']: errors['repair_hash'].append(row['path'])
scratch=BASE/'evidence/v2-inputs'
scratch_map={'actionlint.yaml':'.github/actionlint.yaml','build_network_consumers.py':'tools/ci/build_network_consumers.py','network-consumer-build.yml':'.github/workflows/network-consumer-build.yml','test_network_build_planning.py':'tools/ci/test_network_build_planning.py'}
for name,path in scratch_map.items():
    if (scratch/name).read_bytes()!=blob(path): errors['scratch_bytes'].append(name)
result={'candidate':C,'historical_parent':P,'plan_sha256':sha(plan_bytes),'archive_sha256':sha(raw),'archive_members':len(names),'archive_object_count':len(objects),'index_groups':{k:len(v) for k,v in idx['groups'].items()},'programs':len(inv['cases']),'workload_ids':sum(len(c.get('case_ids',[])) for c in inv['cases']),'roots':len(inv['build_roots']),'unique_roots':len(set(inv['build_roots'])),'rust_counts':[c['expected_tests'] for c in inv['cases'] if c.get('expected_tests') is not None],'entrypoints':entrypoints,'repair_count':len(r['source_whitespace_repairs']),'failures':dict(errors)}
(BASE/'evidence/v2-checks.json').write_text(json.dumps(result,indent=2)+'\n',encoding='utf-8')
print(json.dumps({k:v for k,v in result.items() if k!='entrypoints'},indent=2))
