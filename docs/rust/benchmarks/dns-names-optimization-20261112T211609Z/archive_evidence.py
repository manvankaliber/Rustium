from pathlib import Path
import json,hashlib,shutil,datetime,subprocess,platform,os,re
r=Path('D:/Rustium');w=r/'.scratch/net-wave1/dns-names/optimization'
def digest(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def read(name):return json.loads((w/name).read_text())
before=read('before/source-hashes.json')
all_sources={p.relative_to(r).as_posix():digest(p) for p in sorted((r/'net/dns').glob('dns_names_*')) if p.is_file()}
changed={p:h for p,h in all_sources.items() if h!=before[p]}
expected={'net/dns/'+p for p in ['dns_names_util.cc','dns_names_core.rs','dns_names_ffi.rs','dns_names_ffi.h','dns_names_test_support.h','dns_names_rust_unittest.cc']}
assert set(changed)==expected,(set(changed),expected)
for mode in ['unit','fuzz','bench']:
 b=read(mode+'-build-provenance.json');assert b['source_hashes']==all_sources
 assert b['executable_sha256']==digest(w/(mode+'.exe'))
assert read('fuzz-execution.json')['exit_code']==0
assert read('fuzz-execution.json')['executable_sha256']==digest(w/'fuzz.exe')
assert '#1000000\tDONE' in (w/'fuzz-results.log').read_text()
# The executable's generated CXX transport uses bounded caller output and scalar
# metadata, with no Vec transport. Keep the raw generation and assembly.
bridge=(w/'bridge.cc').read_text();assert 'rust::Vec' not in bridge
asm=next((w/'assembly-target/release/deps').glob('dns_names_validation-*.s'))
ll=next((w/'assembly-target/release/deps').glob('dns_names_validation-*.ll'))
static={'method':'Source, generated CXX bridge, optimized Rust assembly/LLVM IR and configured Clang object disassembly; no performance timing.',
 'observations':['Dotted conversion and validation share safe allocation-free label scanning; C++ allocates only the final returned vector.','Ordinary network input uses safe SliceReader monomorphization in a single bridge invocation, without cursor callbacks or intermediate Vec ownership.','An address-range ownership check selects the callback reader only when input overlaps the C++ cursor object. That path shares the same Rust parser and preserves post-commit label copies.','URL canonicalization writes directly into the eventual C++ std::string. Rust controls BROKEN fallback via callback; no Rust Vec or per-byte Vec push transport remains.','Length readers and conversion functions return scalar validity/length/consumption metadata.','DomainNameLess object disassembly ends in jmp net_dns_names_less, with no byte-span conversion calls or CXX Slice temporaries in this C++ adapter.','Rust comparator assembly retains nonempty raw-slice precondition checks and executes ASCII comparison in Rust.'],
 'limits':'These are structural observations, not measurements of elapsed time, allocation counts, bridge overhead or achieved speedups. Native object flags can differ from the final configured target.','files':['bridge.cc','dns_names_validation.s','dns_names_validation.ll','production-disassembly.txt']}
(w/'static-inspection.json').write_text(json.dumps(static,indent=2))
accepted=r/'docs/rust/benchmarks/dns-names-wave1-final-2026-11-12.json'
meta=json.loads(accepted.read_text())['metadata']
meta['measurement']='Same-process paired unchanged public-API benchmark, including FFI, caller-owned initialized bounded buffers, cursor adaptation, final C++ allocation/copy/destruction and external URL/IP/ICU services. The optimized ordinary network path uses one Rust call; cursor-object input aliases retain incremental callbacks. 20 randomized-order pairs with 20,000 operations per implementation, seed 20261112. Input construction, warmup, ICU initialization and JSON output remain outside timing. Canonical production and reference component DLL boundaries must be preserved.'
(w/'benchmark-descriptor.json').write_text(json.dumps(meta,indent=2))
keys=[x['key'] for group in meta['groups'] for x in group['cases']]
harness=(r/'net/dns/dns_names_perftest.cc').read_text();assert keys==re.findall(r'\{"(dns_[a-z0-9_]+)"',harness)
assert len(keys)==21 and all(len(g['cases'])<=5 for g in meta['groups'])
coverage={Path(f['filename']).name:f['summary'] for f in read('coverage.json')['data'][0]['files'] if 'dns_names_' in f['filename']}
for v in coverage.values():assert v['lines']['percent']==100 and v['regions']['percent']==100
stamp=datetime.datetime.now(datetime.timezone.utc).strftime('%Y%m%dT%H%M%SZ')
a=r/'docs/rust/benchmarks'/('dns-names-optimization-'+stamp);a.mkdir(parents=True)
for p in all_sources:
 t=a/'source-snapshot'/p;t.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(r/p,t)
shutil.copytree(w/'before',a/'before')
for d in ['unit-rsp','fuzz-rsp','bench-rsp','probe-rsp','fuzz-initial-corpus']:
 shutil.copytree(w/d,a/d)
for pattern in ['*.py','*-build-source-hashes.json','*-build-provenance.json','*-link-command.json','unit-verified-*.profraw']:
 for p in w.glob(pattern):shutil.copyfile(p,a/p.name)
for name in ['native_seams.cc','bridge.cc','static-inspection.json','production-disassembly.txt','benchmark-descriptor.json','fuzz-execution.json','fuzz-results.log','fuzz-initial-corpus-manifest.json','unit-results.log','bench-results.log','validation-execution.json','coverage.json','coverage-command.json','coverage.profdata','lint.log','rust.log','coverage.log','assembly.log','Cargo.toml','Cargo.lock','export_probe.cc','export_probe.rsp','export-probe.json']:
 shutil.copyfile(w/name,a/name)
shutil.copytree(w/'.cargo',a/'.cargo')
t=a/'generated/net/dns/dns_names_ffi.rs.h';t.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(w/'include/net/dns/dns_names_ffi.rs.h',t)
shutil.copyfile(asm,a/'dns_names_validation.s');shutil.copyfile(ll,a/'dns_names_validation.ll')
(a/'source-hashes.json').write_text(json.dumps(all_sources,indent=2))
prior=[accepted,*sorted((r/'.scratch/net-wave1/measurements/dns-names').glob('wave1-boundary-final-*'))]
prior_files={p.relative_to(r).as_posix():digest(p) for p in prior if p.is_file()}
notes='''Parent integration for DNS-name optimization

No GN source/dependency changes are needed: the existing DNS Rust target retains the same root and source files, existing DNS source-set dependency, and allow_unsafe=true. Regenerate CXX bindings and rebuild actual production through the parent-owned configured integration. The generated private bridge signatures have changed. DNS parsing/validation/comparison/result-selection policy remains in Rust. No frozen reference, benchmark loop, workload, existing original test, public C++ header, GN file, configured worktree, gallery or backlog was edited by this optimization task.

The public inline comparator remains backed by exported C++ DomainNameLess. Its new private raw Rust ABI is called within the net component; downstream users still import only DomainNameLess. Two documented raw-slice constructions in dns_names_ffi.rs normalize empty inputs before constructing any reference. All other unsafe is CXX glue; the reusable public dns_names_core module forbids unsafe.

Current module status: already integrated. This optimized source revision passed focused native validation and needs parent configured-build validation and quiet remeasurement. Do not relabel the accepted DNS implementation Candidate only or omit its recorded regressions while this new revision waits for measurement. Preserve the historical gallery runs. Use a new unique run slug, record/import all 21 cases and pass --final only for the accepted configured execution. Refresh the Svelte dashboard after canonical import. No new performance timing was performed here.

Remeasure the unchanged dns_names_perftests target through the configured runner. The new benchmark-descriptor.json updates timing-boundary prose only, preserving groups and every case. Repeat configured original and differential tests, component DLL export/import checks, and dependent net tests as required. Parent owns configured file conflict checks/sync/Ninja, recursively imported component-DLL provenance, charts, aggregate and optimization backlog.

Remaining seams are C++ ownership, cursor commits, URL/ICU hostname canonicalization, IsCanonicalizedHostCompliant, localhost and IP-address services. Ordinary network input avoids callbacks; cursor-object overlap intentionally retains them. No private C++ layout is exposed to Rust. The fixed output buffers are fully initialized before Rust receives mutable references. Conversion copies into final C++ owned results; it never returns borrowed input. Atomic length-reader outputs are committed only after the Rust borrow ends, before the cursor assignment, including cursor/output aliases.

Native scaffolding remains the prior exact service-body extraction with real configured URL/ICU libraries, plus the actual current production adapter and the unchanged parent-repaired reference source (compiled with IS_DNS_NAMES_REFERENCE_IMPL). This native executable does not establish production DLL timing or full Chromium integration. Old accepted evidence is unchanged.
'''
(w/'HANDOFF.txt').write_text(notes);(a/'HANDOFF.txt').write_text(notes)
report={
 'component':'dns-names','task':'optimize measured DNS-name utility regressions','status':'optimization_implemented_native_validated_parent_configured_remeasurement_pending',
 'created_utc':datetime.datetime.now(datetime.timezone.utc).isoformat(),
 'git_head':subprocess.check_output(['git','-C',str(r),'rev-parse','HEAD'],text=True).strip(),
 'changed_owned_files':changed,'all_component_source_hashes':all_sources,'source_hashes_file':str(a/'source-hashes.json'),'source_hashes_file_sha256':digest(a/'source-hashes.json'),
 'source_snapshot':str(a/'source-snapshot'),'before_snapshot':str(a/'before'),'durable_evidence_directory':str(a),
 'previous_accepted_evidence':prior_files,'old_evidence_modified':False,
 'unchanged_protected_files':{p:h for p,h in all_sources.items() if p not in changed},
 'changes':static['observations'],
 'safety':{'safe_core':'pub mod dns_names_core; #![forbid(unsafe_code)]','raw_ffi':'Private net_dns_names_less has two documented from_raw_parts sites. Each is limited to a nonempty readable string_view range <= isize::MAX; zero lengths map to &[] without touching the pointer. Inputs can alias; no reference retained or mutation. Public C++ API/export unchanged.','cxx':'Shared buffer layout generated by CXX. C++ initializes NameBuffer and LabelChunk before borrowed Rust references. CanonicalName is a stack-only raw_ref to a fresh result std::string.','cursor':'The ordinary path borrows disjoint input/output, computes consumed count, ends Rust borrow and commits Skip once. If input bytes overlap any part of the SpanReader object, the same Rust parser uses callback-scoped views and post-commit copied label bytes. No C++ DNS parser or private Rust view of SpanReader exists.','packed_returns':'Conversion metadata packs consumed length in the upper 32 bits and output-length-plus-one in the lower 32 bits (zero invalid). DNS limits bound consumption below 448 and output at most 255. Length reads encode width in low two bits and U16 payload length above them, with zero invalid. Policy executes in Rust; C++ only decodes transport.','ownership':'Final outputs remain independently owned C++ string/vector/span views according to the existing API; no Rust-owned intermediate Vec is transferred or retained. Aliased length output still commits before reader assignment.'},
 'validation':{
  'native_tests':{'passed':55,'unchanged_original_tests':44,'differential_tests':11,'evidence':'unit-results.log','new_tests':['CursorInsideLabelPayload: label spanning the entire mutable reader representation, rooted/truncated and both modes.','LengthOutputAliasesCursorSpan: success/failure with output pointing at the reader span subobject.','ComparatorPrefixesAndAliasedViews: long case-folded prefixes, NUL/high bytes, unequal lengths, identical aliases and null-empty views.'],'expanded_differential_fuzz_assertions':'Existing Check now asserts the span NetworkToDottedName overload as well as cursor overload for both modes. Exhaustive byte/label/U16/cursor and service cases retained.'},
  'executions':read('validation-execution.json'),'fuzz':{**read('fuzz-execution.json'),'executions':1000000,'max_input_bytes':1024,'initial_corpus_manifest':'fuzz-initial-corpus-manifest.json','initial_corpus_directory':'fuzz-initial-corpus','scope':'Full public APIs and frozen original references, including both network overloads. C++ libFuzzer edge/cmp instrumentation; authored Rust runtime coverage measured separately.'},
  'rust_coverage':{'method':'Chromium rustc -Cinstrument-coverage, LLVM profile merge/export from final native unit executable','authored_files':coverage,'runtime_exclusions':'None within these two authored files. Other Chromium/vendor/generated C++/platform files are outside this claim.','branch_coverage':'Not collected; zero LLVM branch records is not evidence of branch coverage.'},
  'clippy':{'passed':True,'warnings_denied':True,'command':'python .scratch/net-wave1/dns-names/optimization/build_native.py lint','method':'Full bridge/core cargo check --release under configured Chromium clippy-driver workspace wrapper; -Dwarnings.'},
  'format':{'rustfmt':'Configured Chromium rustfmt --edition 2024 --check passed','clang_format':'Configured buildtools/win-format/clang-format.exe --dry-run --Werror passed','git_diff_check':'Passed for edited tracked C++ files.'},
  'component_consumer_probe':read('export-probe.json'),'benchmark_verify_only':{'passed':True,'cases':21,'timing_performed':False}},
 'static_inspection':static,
 'native_build':{'configured_gn_execution':False,'editing_worktree':str(r),'configured_dependencies':'D:/RustiumBuild/src/out/Default','production_adapter':'Actual net/dns/dns_names_util.cc compiled directly, not a substitute benchmark adapter.','reference_defines':'IS_DNS_NAMES_REFERENCE_IMPL when compiling frozen sources locally; no baseline source changes.','scaffolding':'native_seams.cc is unchanged exact existing service-body scaffolding to avoid unrelated net.dll linkage. URL/ICU implementations and data remain configured native services.','service_scaffolding_sha256':digest(w/'native_seams.cc'),
  'commands':['python .scratch/net-wave1/dns-names/optimization/build_native.py '+mode for mode in ['rust','coverage','lint','assembly','unit','fuzz','bench','probe']],
  'cpp_flags':'Full per-mode response files and link commands archived. Configured /O2, /clang:-O2, C++23preview, /MD, COMPONENT_BUILD, DCHECK_ALWAYS_ON, NET_IMPLEMENTATION, IS_DNS_NAMES_REFERENCE_IMPL, configured plugins/sanitizers. Native fuzz adds inline-8bit-counters,pc-table,trace-cmp instrumentation.',
  'rust_flags':'Release opt-level=3/codegen-units=1; -Cdebug-assertions=yes -Ccontrol-flow-guard=yes -Cpanic=abort. Coverage adds -Cinstrument-coverage; Clippy adds -Dwarnings; separate assembly build adds --emit=asm,llvm-ir.',
  'binaries':{str(p.relative_to(w)):digest(p) for p in [w/'unit.exe',w/'fuzz.exe',w/'bench.exe',w/'target/release/dns_names_validation.lib',w/'coverage-target/release/dns_names_validation.lib']},
  'external_runtime_hashes':{p.name:digest(p) for p in [*sorted(w.glob('*.dll')),w/'icudtl.dat']},
  'tool_versions':{'rustc':subprocess.check_output(['D:/RustiumBuild/src/third_party/rust-toolchain/bin/rustc.exe','-Vv'],text=True).strip(),'clang':subprocess.check_output(['D:/RustiumBuild/src/third_party/llvm-build/Release+Asserts/bin/clang-cl.exe','--version'],text=True).strip()},
  'environment':{'platform':platform.platform(),'machine':platform.machine(),'processor':platform.processor(),'logical_cpu_count':os.cpu_count(),'fuzz_affinity_cpu':6,'memory':'Not recorded'}},
 'benchmark':{'new_timings_performed':False,'new_speedup_claim':None,'descriptor_path':str(a/'benchmark-descriptor.json'),'case_groups':[len(g['cases']) for g in meta['groups']],'test_inputs_and_timing_loops':'Unchanged; SHA-256 checked against before snapshot. Exact original inline C++ comparator remains in parent-owned reference header.','parent_action':'Rebuild configured production, validate required tests, and remeasure all 21 unchanged cases in a coordinated quiet window. Use unique run slug, preserve old evidence; import every result and refresh both dashboard surfaces.','gallery_changes_by_agent':False},
 'remaining_seams':['URL/ICU canonicalization and BROKEN classification service; Rust controls fallback.','Internet-hostname compliance, localhost and IP/URL-address services; Rust preserves validation ordering.','C++ object ownership/allocation and cursor commit callbacks for cursor-object input overlap.','Platform ABI, allocator and CXX runtime.'],
 'parent_integration':{'gn_changes_required':'None; sources and existing DNS target/dependencies unchanged. Regenerate private CXX glue and rebuild configured production. Existing allow_unsafe already covers bridge and narrowly documented raw comparator adapter.','configured_targets':['net','net_unittests','dns_names_rust_unittests','dns_names_perftests','dns_names_rust_fuzzer'],'integration_status':'DNS module was already integrated. This optimized source revision has native validation; parent sync/build/acceptance and timing remain pending.','shared_files_modified_by_agent':[],'handoff':str(a/'HANDOFF.txt')},
 'limitations':['No measured speedup, allocation count or isolated bridge-cost attribution. Static reductions do not establish the cause or amount of prior regressions.','Native linkage validates actual adapter behavior with extracted service scaffolding and local references, not the production component DLL boundary; parent owns configured validation.','C++ output copies and initialized bounded-buffer storage remain. Alias-dependent callbacks remain required for exact cursor mutation semantics.','Tiny comparator still crosses the exported C++/private Rust boundary; the original comparator is genuinely inline. A residual loss remains acceptable if validated.','Retiring URL/IP/ownership bridges will not guarantee algorithmic parity.'],
 'optimization_follow_up':[{'workload':'All 21 unchanged cases','action':'Measure actual configured medians/uncertainty after integration; retain both favorable and unfavorable results and record exact residual debt.'},{'workload':'Network conversion including root/short/max/failure','action':'Profile remaining caller-buffer initialization, final C++ ownership copy and public ABI costs; do not remove alias callbacks or alter failure consumption.'},{'workload':'Canonicalization and comparator','action':'Profile URL-service callback/result ownership and private raw comparator after configured remeasurement. Keep full URL semantics and real inline baseline.'}],
 'archive_excludes':['Compiled executables, Cargo caches and copied runtime DLL/data bytes remain in optimization scratch; hashes, source snapshots, exact flags, scripts, generated code, assembly/IR, profiles, test/fuzz logs and initial corpus are durable.']}
for p in [w/'report.json',a/'report.json']:p.write_text(json.dumps(report,indent=2))
# Bind all archived source and validation artifacts without self-referential hash.
manifest={p.relative_to(a).as_posix():digest(p) for p in sorted(a.rglob('*')) if p.is_file()}
(a/'evidence-manifest.json').write_text(json.dumps(manifest,indent=2))
print(str(a));print(str(w/'report.json'))
