from pathlib import Path
import os, subprocess, shutil, re, sys

root=Path('D:/Rustium');full=Path('D:/RustiumBuild/src');out=full/'out/Default';work=root/'.scratch/net-wave1/status-codes'
env=os.environ.copy()
for item in (out/'environment.x64').read_bytes().decode().split('\0'):
    k,s,v=item.partition('=')
    if s:env[k]=v
mode=sys.argv[1] if len(sys.argv)>1 else 'differential'
variables=dict(re.findall(r'^(\w+) = (.*)$',(out/'obj/net/net.ninja').read_text(),re.M))
template=(out/'toolchain.ninja').read_text().split('rule cxx\n',1)[1].splitlines()[0].strip().removeprefix('command = ')
def compile(source,name,bridge=False):
    values=dict(variables)
    if mode=='unit' and name in ('unit','unit_main','unit_old','unit_new'):
        values=dict(re.findall(r'^(\w+) = (.*)$',(out/'obj/net/gzip_header_unittests.ninja').read_text(),re.M))
    values.update({'in':str(source),'source_name_part':name})
    command=re.sub(r'\$\{(\w+)\}',lambda m:values.get(m[1],''),template).replace('$:',':')
    command=command.replace('-I../.. ','-I'+str(work/'include')+' -I'+str(root)+' -I../.. ')
    command=re.sub(r'/Fo\S+',lambda _: '/Fo'+str(work/(name+'.obj')),command)
    command=re.sub(r'/Fd"[^"]+"',lambda _: '/Fd"'+str(work/(name+'.pdb'))+'"',command)
    if bridge:command+=' /FIthird_party/rust/cxx/v1/cxx.h'
    if mode=='unit':command+=' /DNET_IMPLEMENTATION'
    if False:command+=' -fsanitize-coverage=inline-8bit-counters,pc-table,trace-cmp -fno-sanitize-link-runtime'
    compiler,args=command.split(' ',1);rsp=work/(name+'.rsp');rsp.write_text(args)
    result=subprocess.run([str((out/compiler).resolve()),'@'+str(rsp)],cwd=out,env=env,capture_output=True,text=True)
    (work/(name+'-compile.log')).write_text(result.stdout+result.stderr)
    if result.returncode:
        print('\n'.join(l for l in (result.stdout+result.stderr).splitlines() if 'including file:' not in l));result.check_returncode()


compile(root/'net/http/http_status_code.cc','reader')
compile(root/'net/http/http_status_code_reference.cc','legacy')
if mode == 'unit':
    compile(root/'net/http/http_status_code_unittest.cc','unit_old')
    compile(root/'net/http/http_status_code_rust_unittest.cc','unit_new')
    compile(work/'unit_main.cc',mode)
else:
    compile(root/'net/http/http_status_code_perftest.cc' if mode=='bench' else work/'differential_main.cc',mode)
dependencies=[str(out/'base.dll.lib'),str(out/'libc++.dll.lib'),*[str(out/('base_allocator_partition_allocator_src_partition_alloc_allocator_'+name+'.dll.lib')) for name in ('base','core','shim')],str(out/'base_allocator_partition_allocator_src_partition_alloc_raw_ptr.dll.lib')]
libraries=['advapi32.lib','userenv.lib','ws2_32.lib','bcrypt.lib','ntdll.lib','dbghelp.lib','synchronization.lib','shlwapi.lib','shell32.lib']
command=[str(full/'third_party/llvm-build/Release+Asserts/bin/lld-link.exe'),'/OUT:'+str(work/(mode+'.exe')),'/SUBSYSTEM:CONSOLE','/INCREMENTAL:NO',*[str(work/(n+'.obj')) for n in ('reader','legacy',mode)],str(work/('http_status_code_coverage.lib' if mode=='unit' else 'http_status_code.lib')),*dependencies,*libraries]
for directory in ['C:/Program Files (x86)/Microsoft Visual Studio/2022/BuildTools/VC/Tools/MSVC/14.44.35207/lib/x64','C:/Program Files (x86)/Windows Kits/10/Lib/10.0.26100.0/um/x64','C:/Program Files (x86)/Windows Kits/10/Lib/10.0.26100.0/ucrt/x64']:
    command.append('/LIBPATH:'+directory)

if mode=='unit':
    command += [str(work/'unit_old.obj'),str(work/'unit_new.obj'),*map(str,(out/'obj/third_party/googletest/gtest').glob('*.obj')),str(out/'third_party_abseil-cpp_absl.dll.lib'),str(out/'third_party_boringssl.dll.lib'),str(out/'obj/third_party/re2/re2.lib')]
    command += list(map(str,(full/'third_party/rust-toolchain/lib/rustlib/x86_64-pc-windows-msvc/lib').glob('libprofiler_builtins-*.rlib')))
result=subprocess.run(command,cwd=out,env=env,capture_output=True,text=True)
(work/'link.log').write_text(result.stdout+result.stderr)
print(result.stdout,result.stderr);result.check_returncode()

