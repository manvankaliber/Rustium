from pathlib import Path
import os, subprocess, shutil, re, sys

root=Path('D:/Rustium');full=Path('D:/RustiumBuild/src');out=full/'out/Default';work=root/'.scratch/net-wave1/content-disposition'
# Windows ships another ICU DLL with this basename; pin the configured one
# adjacent to each focused executable rather than relying on PATH order.
for dependency in ('icuuc.dll','icudtl.dat'):
    if not (work/dependency).exists():shutil.copyfile(out/dependency,work/dependency)
env=os.environ.copy()
for item in (out/'environment.x64').read_bytes().decode().split('\0'):
    k,s,v=item.partition('=')
    if s:env[k]=v
mode=sys.argv[1] if len(sys.argv)>1 else 'rust'
if mode in ('rust','coverage','clippy'):
    config=work/'.cargo';config.mkdir(exist_ok=True);shutil.copyfile(root/'.scratch/data-pack/.cargo/config.toml',config/'config.toml')
    tool=full/'third_party/rust-toolchain/bin'
    env.update({'RUSTC':str(tool/'rustc.exe'),'RUSTDOC':str(tool/'rustdoc.exe'),'CARGO_TARGET_DIR':str(work/('coverage-target' if mode=='coverage' else 'chromium-target')),'RUSTFLAGS':'-Cdebug-assertions=yes -Ccontrol-flow-guard=yes -Cpanic=abort'+(' -Cinstrument-coverage' if mode=='coverage' else ''),'CXX':str(full/'third_party/llvm-build/Release+Asserts/bin/clang-cl.exe'),'CXXFLAGS':'/std:c++20 /ID:/Rustium /ID:/Rustium/.scratch/net-wave1/content-disposition/include','CARGO_TARGET_X86_64_PC_WINDOWS_MSVC_LINKER':str(full/'third_party/llvm-build/Release+Asserts/bin/lld-link.exe')})
    env['PATH']=str(tool)+';'+env['PATH']
    action = ['clippy','--release','--','-D','warnings'] if mode=='clippy' else ['build','--release']
    subprocess.run(['C:/Users/jackv/.cargo/bin/cargo.exe',*action],cwd=work,env=env,check=True)
    bridge=out/'win_clang_x64_for_rust_host_build_tools/cxxbridge.exe'
    source=root/'net/http/content_disposition_ffi.rs'
    target=work/'include/net/http/content_disposition_ffi.rs.h';target.parent.mkdir(parents=True,exist_ok=True)
    target.write_bytes(subprocess.check_output([str(bridge),str(source),'--header']))
    (work/'bridge.cc').write_bytes(subprocess.check_output([str(bridge),str(source)]))
    print('Built Chromium Rust static library and generated matching bridge.')
    sys.exit()

variables=dict(re.findall(r'^(\w+) = (.*)$',(out/'obj/net/gzip_header_perftests.ninja').read_text(),re.M))
template=(out/'toolchain.ninja').read_text().split('rule cxx\n',1)[1].splitlines()[0].strip().removeprefix('command = ')
def compile(source,name,bridge=False):
    values=dict(variables)
    if mode=='unit' and name in ('unit','original_unit','unit_main'):
        values=dict(re.findall(r'^(\w+) = (.*)$',(out/'obj/net/gzip_header_unittests.ninja').read_text(),re.M))
    values.update({'in':str(source),'source_name_part':name})
    command=re.sub(r'\$\{(\w+)\}',lambda m:values.get(m[1],''),template).replace('$:',':')
    command=command.replace('-I../.. ','-I'+str(work/'include')+' -I'+str(root)+' -I../.. ')
    command=re.sub(r'/Fo\S+',lambda _: '/Fo'+str(work/(name+'.obj')),command)
    command=re.sub(r'/Fd"[^"]+"',lambda _: '/Fd"'+str(work/(name+'.pdb'))+'"',command)
    if bridge:command+=' /FIthird_party/rust/cxx/v1/cxx.h'
    command+=' /DNET_IMPLEMENTATION -I../../third_party/icu/source/common -I../../third_party/icu/source/i18n'
    if mode=='fuzz':command+=' -fsanitize-coverage=inline-8bit-counters,pc-table,trace-cmp -fno-sanitize-link-runtime'
    compiler,args=command.split(' ',1);rsp=work/(name+'.rsp');rsp.write_text(args)
    result=subprocess.run([str((out/compiler).resolve()),'@'+str(rsp)],cwd=out,env=env,capture_output=True,text=True)
    (work/(name+'-compile.log')).write_text(result.stdout+result.stderr)
    if result.returncode:
        print('\n'.join(l for l in (result.stdout+result.stderr).splitlines() if 'including file:' not in l));result.check_returncode()

names = ['reader','services','legacy','legacy_util','legacy_services','legacy_modp','charset','bridge',mode]
compile(root/'net/http/http_content_disposition.cc','reader')
compile(root/'net/http/content_disposition_services.cc','services')
compile(root/'net/http/content_disposition_reference.cc','legacy')
compile(root/'net/http/content_disposition_http_util_reference.cc','legacy_util')
compile(root/'net/http/content_disposition_services_reference.cc','legacy_services')
compile(root/'net/http/content_disposition_modp_reference.cc','legacy_modp')
compile(root/'net/base/net_string_util_icu.cc','charset')
compile(work/'bridge.cc','bridge',True)
source=root/'net/http'/('content_disposition_perftest.cc' if mode=='bench' else 'content_disposition_unittest.cc' if mode=='unit' else 'content_disposition_fuzzer.cc')
compile(source,mode)
if mode=='unit':
    compile(root/'net/http/http_content_disposition_unittest.cc','original_unit')
    main=work/'unit_main.cc';main.write_text('#include "base/i18n/icu_util.h"\n#include "testing/gtest/include/gtest/gtest.h"\nint main(int argc,char** argv){base::i18n::InitializeICU();::testing::InitGoogleTest(&argc,argv);return RUN_ALL_TESTS();}\n')
    compile(main,'unit_main')
# Public parsing is compiled from editing-worktree sources. net.dll supplies
# the shared response-header container APIs; ICU/base supply charset services.
dependencies=[str(out/'base.dll.lib'),str(out/'net.dll.lib'),str(out/'base_i18n.dll.lib'),str(out/'icuuc.dll.lib'),str(out/'libc++.dll.lib'),*[str(out/('base_allocator_partition_allocator_src_partition_alloc_allocator_'+name+'.dll.lib')) for name in ('base','core','shim')],str(out/'base_allocator_partition_allocator_src_partition_alloc_raw_ptr.dll.lib')]
libraries=['advapi32.lib','userenv.lib','ws2_32.lib','bcrypt.lib','ntdll.lib','dbghelp.lib','synchronization.lib','shlwapi.lib','shell32.lib']
command=[str(full/'third_party/llvm-build/Release+Asserts/bin/lld-link.exe'),'/OUT:'+str(work/(mode+'.exe')),'/SUBSYSTEM:CONSOLE','/INCREMENTAL:NO',*[str(work/(n+'.obj')) for n in names],str(work/('coverage-target' if mode=='unit' else 'chromium-target')/'release/content_disposition_validation.lib'),*dependencies,*libraries]
if mode=='unit':
    command += [str(work/'unit_main.obj'),str(work/'original_unit.obj'),*map(str,(out/'obj/third_party/googletest/gtest').glob('*.obj')),str(out/'third_party_abseil-cpp_absl.dll.lib'),str(out/'third_party_boringssl.dll.lib'),str(out/'obj/third_party/re2/re2.lib')]
    command += list(map(str,(full/'third_party/rust-toolchain/lib/rustlib/x86_64-pc-windows-msvc/lib').glob('libprofiler_builtins-*.rlib')))
for directory in ['C:/Program Files (x86)/Microsoft Visual Studio/2022/BuildTools/VC/Tools/MSVC/14.44.35207/lib/x64','C:/Program Files (x86)/Windows Kits/10/Lib/10.0.26100.0/um/x64','C:/Program Files (x86)/Windows Kits/10/Lib/10.0.26100.0/ucrt/x64']:
    command.append('/LIBPATH:'+directory)
if mode=='fuzz':
    command.append('/NODEFAULTLIB:clang_rt.ubsan_standalone-x86_64.lib')
    runtime=full/'third_party/llvm-build/Release+Asserts/lib/clang/24/lib/windows'
    command.extend([str(runtime/'clang_rt.ubsan_standalone-x86_64.lib'),str(runtime/'clang_rt.ubsan_standalone_cxx-x86_64.lib')])
    command.extend(map(str,(root/'.scratch/data-pack/fuzzer-engine').glob('*.obj')))
(work/(mode+'-link-command.json')).write_text(__import__('json').dumps(command,indent=2)+'\n')
result=subprocess.run(command,cwd=out,env=env,capture_output=True,text=True)
(work/(mode+'-link.log')).write_text(result.stdout+result.stderr);print(result.stdout,result.stderr);result.check_returncode()
print('Built',mode)

evidence=work/'build-evidence'/mode;evidence.mkdir(parents=True,exist_ok=True)
for path in [*work.glob('*.rsp'),*work.glob('*-compile.log'),work/(mode+'-link-command.json'),work/(mode+'-link.log')]:shutil.copyfile(path,evidence/path.name)
paths=[root/'net/http/http_content_disposition.cc',root/'net/http/http_content_disposition.h',*sorted((root/'net/http').glob('content_disposition_*')),root/'net/base/net_string_util_icu.cc']
(evidence/'source-hashes.json').write_text(__import__('json').dumps({str(p.relative_to(root)):__import__('hashlib').sha256(p.read_bytes()).hexdigest() for p in paths if p.is_file()},indent=2)+'\n')
