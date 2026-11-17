import collections, hashlib, json, pathlib, re, subprocess
ROOT=pathlib.Path('D:/Rustium'); REVIEW=pathlib.Path('D:/RustiumReview'); AS=ROOT/'.rustium-runtime/network-pr-source-20261117T173415247963Z'; AUTH=ROOT/'docs/rust/migration/drafts/net-auth-graph-intake'; SH=ROOT/'.scratch/shared-strings-connection/20261117T150542774256Z'; PLAN=ROOT/'.scratch/net-wave5/resumed-integration/native-runtime-delivery-v2/network-focus-20261117-plan.json'; C='fe18b237de48d2fbc0026a534c61ae8281dbddef'; P='4485cc56f5f0e151d8433886976cdf679355e872'
def sha(b): return hashlib.sha256(b).hexdigest()
def read(p): return pathlib.Path(p).read_bytes()
def git(*args): return subprocess.check_output(['git',*args],cwd=REVIEW)
def obj(c,p): return git('show',f'{c}:{p}')
a=json.loads(read(AS/'assembly.json')); m=json.loads(read(SH/'source-manifest.json')); auth=json.loads(read(AUTH/'manifest.json')); plan=json.loads(read(PLAN)); inv=json.loads(obj(C,'tools/ci/network-validation-inventory.json')); graph=json.loads(read(SH/'current-graph-catalogue/result.json')); deps=json.loads(read(SH/'current-graph-catalogue/gn-deps.log'))
tree={line.split('\t',1)[1]:line.split()[2] for line in git('ls-tree','-r',C).decode().splitlines()}; changed=git('diff','--name-only',P,C).decode().splitlines(); paths=list(a['paths']); proc=subprocess.Popen(['git','cat-file','--batch'],cwd=REVIEW,stdin=subprocess.PIPE,stdout=subprocess.PIPE); stream,_=proc.communicate((''.join(tree[p]+'\n' for p in paths)).encode()); pos=0; blobs={}
for path in paths:
 end=stream.index(b'\n',pos); header=stream[pos:end].split(); size=int(header[2]); pos=end+1; blobs[path]=stream[pos:pos+size]; pos+=size+1
fails=collections.defaultdict(list); observed=collections.Counter(); exceptions={'base/BUILD.gn','third_party/rust/chromium_crates_io/Cargo.lock','third_party/rust/chromium_crates_io/gnrt_config.toml'}
for p,v in a['paths'].items():
 b=blobs[p]; kind=v['kind']; observed[kind]+=1
 if sha(b)!=v['assembled_sha256'] or tree[p]!=v['git_blob']: fails['blob'].append(p)
 if (AS/p).is_file() and read(AS/p)!=b: fails['retained_copy'].append(p)
 if kind.startswith('Complete auth'):
  q=AUTH/'proposal'/p
  if not q.is_file() or sha(read(q))!=v['proposal_raw_sha256']: fails['proposal_hash'].append(p); continue
  raw=read(q)
 elif 'root_raw_sha256' in v:
  q=ROOT/v.get('original_path',p)
  if not q.is_file() or sha(read(q))!=v['root_raw_sha256']: fails['root_raw'].append(p); continue
  raw=read(q)
 else: continue
 if p in exceptions: continue
 expected=raw.replace(b'\r\n',b'\n') if v['text_crlf_normalized'] else raw
 if expected!=b: fails['transformation'].append(p)
 if v['text_crlf_normalized'] and pathlib.PurePosixPath(p).suffix not in {'.cc','.h','.rs','.gn','.gni','.toml','.py','.md','.json','.txt','.nc','.ps1','.yml','.yaml','.inc'}: fails['normalization_extension'].append(p)
for p,h in m.items():
 key='net/http/hsts_fuzzer_corpus/embedded-nul.header' if p.endswith('/hsts_fuzzer_corpus/nul.header') else p
 v=a['paths'].get(key)
 if not v: fails['authority_not_assembled'].append(p)
 elif v.get('before_raw_sha256',v.get('root_raw_sha256'))!=h: fails['authority_hash'].append(p)
for entry in auth['changes']:
 p=entry['path']; v=a['paths'].get(p)
 if not v or v.get('proposal_raw_sha256')!=entry['after_sha256']: fails['auth_manifest'].append(p)
for p in changed:
 if p not in a['paths']: fails['unbound_changed'].append(p)
protected=['.gn','DEPS','build/config/BUILDCONFIG.gn','build/build_config.h','net/disk_cache/disk_cache.cc','net/disk_cache/disk_cache.h']
for p in protected:
 if obj(C,p)!=obj(P,p): fails['protected'].append(p)
for label in inv['build_roots']:
 if label not in deps: fails['roots_not_in_graph'].append(label)
keys=['family','target','label','kind','arguments','case_ids','expected_workloads','expected_tests','completion','clock_scope','measurement_clocks_requested','blockers']
for i,(x,y) in enumerate(zip(inv['cases'],plan['cases'])):
 for k in keys:
  if x.get(k)!=y.get(k): fails['case_mismatch'].append([i,k,y.get(k),x.get(k)])
for x in inv['cases']:
 e=x.get('entrypoint')
 if e and sha(obj(C,e['path']))!=e['sha256']: fails['entrypoint_hash'].append({'path':e['path'],'inventory':e['sha256'],'candidate':sha(obj(C,e['path']))})
# Strict literal quote includes in changed C/C++ files, limited to project-owned namespaces.
missing=[]
for p in changed:
 if not p.endswith(('.cc','.h','.mm','.c')) or p not in tree: continue
 data=blobs.get(p) or obj(C,p)
 for n,line in enumerate(data.decode('utf-8','replace').splitlines(),1):
  match=re.match(r'\s*#\s*include\s*"((?:net|base|build|url)/[^"$]+)"',line)
  if match and match.group(1) not in tree: missing.append([p,n,match.group(1)])
auth_before={}
for base in [ROOT,pathlib.Path('D:/RustiumBuild/src')]:
 auth_before[str(base)]=sum(1 for v in auth['changes'] if (sha(read(base/v['path'])) if (base/v['path']).is_file() else None)==v['before_sha256'])
result={'auth_unchanged_before_counts':auth_before,'reviewed_commit':C,'parent':P,'input_hashes':{'latest':sha(read(ROOT/'.rustium-runtime/network-pr-source-latest.json')),'assembly':sha(read(AS/'assembly.json')),'authority':sha(read(SH/'source-manifest.json')),'auth_manifest':sha(read(AUTH/'manifest.json')),'plan':sha(read(PLAN)),'graph_result':sha(read(SH/'current-graph-catalogue/result.json'))},'binding_kinds':dict(observed),'authority_count':len(m),'auth_changes':len(auth['changes']),'changed_count':len(changed),'normalization_count':sum(v['text_crlf_normalized'] for v in a['paths'].values()),'inventory':{'programs':len(inv['cases']),'workloads':sum(len(c.get('case_ids',[])) for c in inv['cases']),'roots':len(inv['build_roots']),'graph_roots_present':len(inv['build_roots'])-len(fails['roots_not_in_graph']),'graph_direct_owners_missing':sorted(set(graph['current_direct_source_owners'])-set(inv['build_roots'])),'program_roots_missing':sorted(set(c['label'] for c in inv['cases'])-set(inv['build_roots'])),'rust_counts':[(c['target'],c.get('expected_tests')) for c in inv['cases'] if c.get('expected_tests') is not None]},'failures':dict(fails),'literal_missing_includes':missing,'corpus_renamed_sha256':sha(blobs['net/http/hsts_fuzzer_corpus/embedded-nul.header'])}
(REVIEW/'docs/rust/migration/drafts/net-ci-source-review/evidence/checks.json').write_text(json.dumps(result,indent=2)+'\n',encoding='utf-8'); print(json.dumps(result,indent=2))
