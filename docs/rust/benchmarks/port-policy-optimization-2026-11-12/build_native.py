from pathlib import Path
import os,subprocess,re,sys,shutil,json,hashlib
root=Path('D:/Rustium');full=Path('D:/RustiumBuild/src');out=full/'out/Default';work=root/'.scratch/net-wave1/port-policy/optimization'
env=os.environ.copy()
for item in (out/'environment.x64').read_bytes().decode().split('\0'):
 k,s,v=item.partition('=')
 if s:env[k]=v
env.update({'CXX':str(full/'third_party/llvm-build/Release+Asserts/bin/clang-cl.exe'),'CXXFLAGS':'/std:c++20 /ID:/Rustium'})
mode=sys.argv[1] if len(sys.argv)>1 else 'rust'
owned=sorted((root/'net/base').glob('port_policy_*')) + [root/'net/base/port_util.cc', root/'net/base/port_util.h']
source_hashes={p.relative_to(root).as_posix():hashlib.sha256(p.read_bytes()).hexdigest() for p in owned if p.is_file()}
(work/(mode+'-build-source-hashes.json')).write_text(json.dumps(source_hashes,indent=2))
tool=full/'third_party/rust-toolchain/bin'
env.update({'RUSTC':str(tool/'rustc.exe'),'RUSTDOC':str(tool/'rustdoc.exe'),'CARGO_TARGET_DIR':str(work/('coverage-target' if mode=='coverage' else 'target')),'RUSTFLAGS':'-Cdebug-assertions=yes -Ccontrol-flow-guard=yes -Cpanic=abort'+(' -Cinstrument-coverage' if mode=='coverage' else ''),'CARGO_TARGET_X86_64_PC_WINDOWS_MSVC_LINKER':str(full/'third_party/llvm-build/Release+Asserts/bin/lld-link.exe')})
if mode in ('rust','coverage','lint'):
 (work/'.cargo').mkdir(exist_ok=True);shutil.copyfile(root/'.scratch/data-pack/.cargo/config.toml',work/'.cargo/config.toml')
 cargo=(root/'.scratch/data-pack/Cargo.toml').read_text().replace('data-pack-validation','port-policy-validation').replace('../../ui/base/resource/data_pack_parser_ffi.rs',(root/'net/base/port_policy_ffi.rs').as_posix())
 (work/'Cargo.toml').write_text(cargo)
 cmd=['C:/Users/jackv/.cargo/bin/cargo.exe','check' if mode=='lint' else 'build','--release']
 if mode=='lint':
  env['RUSTC_WORKSPACE_WRAPPER']=str(tool/'clippy-driver.exe')
  env['RUSTFLAGS']+=' -Dwarnings'
  env['CARGO_TARGET_DIR']=str(work/'lint-target')
 p=subprocess.run(cmd,cwd=work,env=env,capture_output=True,text=True);(work/(mode+'.log')).write_text(p.stdout+p.stderr);print(p.stdout,p.stderr);p.check_returncode()
 bridge=out/'win_clang_x64_for_rust_host_build_tools/cxxbridge.exe';source=root/'net/base/port_policy_ffi.rs'
 h=work/'include/net/base/port_policy_ffi.rs.h';h.parent.mkdir(parents=True,exist_ok=True);h.write_bytes(subprocess.check_output([str(bridge),str(source),'--header']));(work/'bridge.cc').write_bytes(subprocess.check_output([str(bridge),str(source)]));sys.exit()
variables=dict(re.findall(r'^(\w+) = (.*)$',(out/'obj/net/gzip_header_unittests.ninja').read_text(),re.M))
template=(out/'toolchain.ninja').read_text().split('rule cxx\n',1)[1].splitlines()[0].strip().removeprefix('command = ')
def compile(source,name,extra=''):
 values=dict(variables);values.update({'in':str(source),'source_name_part':name})
 command=re.sub(r'\$\{(\w+)\}',lambda m:values.get(m[1],''),template).replace('$:',':')
 command=command.replace('-I../.. ','-I'+str(work/'include')+' -I'+str(root)+' -I../.. ')
 command=re.sub(r'/Fo\S+',lambda _: '/Fo'+str(work/(name+'.obj')),command)
 command=re.sub(r'/Fd"[^"]+"',lambda _: '/Fd"'+str(work/(name+'.pdb'))+'"',command)
 command+=' /DNET_IMPLEMENTATION /DIS_PORT_POLICY_REFERENCE_IMPL '+extra
 if mode=='fuzz':command+=' -fsanitize-coverage=inline-8bit-counters,pc-table,trace-cmp -fno-sanitize-link-runtime'
 compiler,args=command.split(' ',1);rsp=work/(name+'.rsp');rsp.write_text(args)
 rspdir=work/(mode+'-rsp');rspdir.mkdir(exist_ok=True);(rspdir/(name+'.rsp')).write_text(args)
 p=subprocess.run([str((out/compiler).resolve()),'@'+str(rsp)],cwd=out,env=env,capture_output=True,text=True);(work/(name+'-compile.log')).write_text(p.stdout+p.stderr)
 if p.returncode:print('\n'.join(l for l in (p.stdout+p.stderr).splitlines() if 'including file:' not in l));p.check_returncode()
if mode=='probe':
 probe=work/'export_probe.cc';probe.write_text('#include "net/dns/dns_names_util.h"\nbool DnsNamesExportProbe(std::string_view a, std::string_view b) { return net::dns_names_util::DomainNameComparator()(a,b); }\n')
 compile(probe,'export_probe','/UNET_IMPLEMENTATION')
 data=(work/'export_probe.obj').read_bytes()
 assert b'__imp_?DomainNameLess' in data and b'dns_names_internal' not in data
 (work/'export-probe.json').write_text(json.dumps({'passed':True,'public_cpp_import':'DomainNameLess','raw_rust_symbol_references':False,'object_sha256':hashlib.sha256(data).hexdigest()},indent=2))
 print('Component consumer imports public C++ DomainNameLess only.');sys.exit()
objects=[]
def add(source,name,extra=''):
 compile(source,name,extra);objects.append(str(work/(name+'.obj')))
add(work/'port_policy_original_parse_number.cc','native_number');add(root/'net/base/ip_address_util.cc','ip_address_util');add(root/'net/base/ip_address.cc','ip_address');add(root/'net/base/ip_endpoint.cc','ip_endpoint');add(work/'native_features.cc','native_features');add(root/'net/base/port_util.cc','production');add(root/'net/base/port_policy_reference.cc','reference');add(root/'net/base/port_policy_reference_number.cc','reference_number');add(work/'bridge.cc','bridge','/FIthird_party/rust/cxx/v1/cxx.h')
if mode=='unit':
 add(root/'net/base/port_util_unittest.cc','original_unit');add(root/'net/base/port_policy_rust_unittest.cc','unit')
 main=work/'unit_main.cc';main.write_text('#include "testing/gtest/include/gtest/gtest.h"\nint main(int argc,char** argv){::testing::InitGoogleTest(&argc,argv);return RUN_ALL_TESTS();}\n');add(main,'unit_main')
else:
 add(root/'net/base'/('port_policy_fuzzer.cc' if mode=='fuzz' else 'port_policy_perftest.cc'),'driver')
deps=[str(out/n) for n in ['third_party_abseil-cpp_absl.dll.lib','net.dll.lib','base_i18n.dll.lib','url_lib.dll.lib','base.dll.lib','libc++.dll.lib','base_allocator_partition_allocator_src_partition_alloc_allocator_base.dll.lib','base_allocator_partition_allocator_src_partition_alloc_allocator_core.dll.lib','base_allocator_partition_allocator_src_partition_alloc_allocator_shim.dll.lib','base_allocator_partition_allocator_src_partition_alloc_raw_ptr.dll.lib']]
libs=['iphlpapi.lib','advapi32.lib','userenv.lib','ws2_32.lib','bcrypt.lib','ntdll.lib','dbghelp.lib','synchronization.lib','shlwapi.lib','shell32.lib']
cmd=[str(full/'third_party/llvm-build/Release+Asserts/bin/lld-link.exe'),'/OUT:'+str(work/(mode+'.exe')),'/SUBSYSTEM:CONSOLE','/INCREMENTAL:NO','/OPT:REF',*objects,str(work/('coverage-target' if mode=='unit' else 'target')/'release/port_policy_validation.lib'),*deps,*libs]
if mode=='unit':
 cmd += [str(out/'obj/base/test/test_support/histogram_tester.obj'), str(out/'obj/base/test/test_support/scoped_feature_list.obj'), str(out/'obj/base/test/test_support/task_environment.obj'), str(out/'obj/base/test/test_support/scoped_run_loop_timeout.obj'), str(out/'obj/base/test/test_config/test_timeouts.obj'), str(out/'obj/base/test/test_config/test_switches.obj'), *map(str,(out/'obj/third_party/googletest/gtest').glob('*.obj')),str(out/'third_party_abseil-cpp_absl.dll.lib'),str(out/'third_party_boringssl.dll.lib'),str(out/'obj/third_party/re2/re2.lib')]
 cmd += list(map(str,(tool.parent/'lib/rustlib/x86_64-pc-windows-msvc/lib').glob('libprofiler_builtins-*.rlib')))
if mode=='fuzz':
 cmd+=['/NODEFAULTLIB:clang_rt.ubsan_standalone-x86_64.lib']
 rt=full/'third_party/llvm-build/Release+Asserts/lib/clang/24/lib/windows';cmd +=[str(rt/'clang_rt.ubsan_standalone-x86_64.lib'),str(rt/'clang_rt.ubsan_standalone_cxx-x86_64.lib'),*map(str,(root/'.scratch/data-pack/fuzzer-engine').glob('*.obj'))]
for d in ['C:/Program Files (x86)/Microsoft Visual Studio/2022/BuildTools/VC/Tools/MSVC/14.44.35207/lib/x64','C:/Program Files (x86)/Windows Kits/10/Lib/10.0.26100.0/um/x64','C:/Program Files (x86)/Windows Kits/10/Lib/10.0.26100.0/ucrt/x64']:cmd+=['/LIBPATH:'+d]
(work/(mode+'-link-command.json')).write_text(json.dumps(cmd,indent=2));p=subprocess.run(cmd,cwd=out,env=env,capture_output=True,text=True);(work/(mode+'-link.log')).write_text(p.stdout+p.stderr);print(p.stdout,p.stderr);p.check_returncode();print('Built',mode)
(work/(mode+'-build-provenance.json')).write_text(json.dumps({'mode':mode,'source_hashes':source_hashes,'executable_sha256':hashlib.sha256((work/(mode+'.exe')).read_bytes()).hexdigest(),'rust_library_sha256':hashlib.sha256((work/('coverage-target' if mode=='unit' else 'target')/'release/port_policy_validation.lib').read_bytes()).hexdigest(),'compile_response_files':{p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in work.glob('*.rsp')},'link_command_file':mode+'-link-command.json','configured_build':False},indent=2))










