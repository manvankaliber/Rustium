from pathlib import Path
import os, subprocess, re, sys, json, hashlib
root=Path('D:/Rustium'); full=Path('D:/RustiumBuild/src'); out=full/'out/Default'; work=root/'.scratch/net-wave1/auth-challenges/optimization'; mode=sys.argv[1] if len(sys.argv)>1 else 'diff'
env=os.environ.copy()
for item in (out/'environment.x64').read_bytes().decode().split('\0'):
 k,s,v=item.partition('=')
 if s: env[k]=v
tool=full/'third_party/rust-toolchain/bin'; llvm=full/'third_party/llvm-build/Release+Asserts/bin'
rust=[str(tool/'rustc.exe'),'--edition=2024','--crate-type=staticlib','--crate-name=auth_challenge','-C','opt-level=2','-C','panic=abort','-C','target-feature=-crt-static',str(root/'net/http/auth_challenge_ffi.rs'),'-o',str(work/'auth_challenge.lib')]
if mode=='unit': rust[4:4]=['-C','instrument-coverage','-C','llvm-args=-enable-name-compression=false']
subprocess.run(rust,env=env,check=True);(work/(mode+'-rust-command.json')).write_text(json.dumps(rust,indent=2))
variables=dict(re.findall(r'^(\w+) = (.*)$',(out/('obj/net/gzip_header_unittests.ninja' if mode=='unit' else 'obj/net/net.ninja')).read_text(),re.M)); template=(out/'toolchain.ninja').read_text().split('rule cxx\n',1)[1].splitlines()[0].strip().removeprefix('command = ')
def compile(src,name):
 v=dict(variables);v.update({'in':str(src),'source_name_part':name});cmd=re.sub(r'\$\{(\w+)\}',lambda m:v.get(m[1],''),template).replace('$:',':');cmd=cmd.replace('-I../.. ','-I'+str(root)+' -I../.. ');cmd=re.sub(r'/Fo\S+',lambda _: '/Fo'+str(work/(name+'.obj')),cmd);cmd=re.sub(r'/Fd"[^"]+"',lambda _: '/Fd"'+str(work/(name+'.pdb'))+'"',cmd);cmd+=' /DNET_IMPLEMENTATION /DIS_AUTH_CHALLENGE_REFERENCE_IMPL';cmd += ' -fsanitize-coverage=inline-8bit-counters,pc-table,trace-cmp -fno-sanitize-link-runtime' if mode=='fuzz' else '';exe,args=cmd.split(' ',1);rsp=work/(mode+'-'+name+'.rsp');rsp.write_text(args);res=subprocess.run([str((out/exe).resolve()),'@'+str(rsp)],cwd=out,env=env,capture_output=True,text=True);(work/(mode+'-'+name+'.log')).write_text(res.stdout+res.stderr)
 if res.returncode:
  print('\n'.join(x for x in (res.stdout+res.stderr).splitlines() if 'including file:' not in x));res.check_returncode()
files={'tokenizer':root/'net/http/http_auth_challenge_tokenizer.cc','round':root/'net/http/http_auth_multi_round_parse.cc','reference':root/'net/http/auth_challenge_reference.cc','modp':root/'net/http/auth_challenge_reference_modp.cc'}
if mode=='unit':
 files.update({'old_token':root/'net/http/http_auth_challenge_tokenizer_unittest.cc','old_round':root/'net/http/http_auth_multi_round_parse_unittest.cc','new_test':root/'net/http/auth_challenge_rust_unittest.cc','main':work/'unit_main.cc'})
 (work/'unit_main.cc').write_text('#include "testing/gtest/include/gtest/gtest.h"\nint main(int argc,char** argv){::testing::InitGoogleTest(&argc,argv);return RUN_ALL_TESTS();}\n')
elif mode=='diff':files['main']=work/'differential_main.cc'
elif mode=='bench':files['main']=root/'net/http/auth_challenge_perftest.cc'
elif mode=='fuzz':files['main']=root/'net/http/auth_challenge_fuzzer.cc'
else:raise ValueError(mode)
for n,p in files.items():compile(p,n)
deps=['net.dll.lib','base.dll.lib','libc++.dll.lib',*[f'base_allocator_partition_allocator_src_partition_alloc_allocator_{n}.dll.lib' for n in ('base','core','shim')],'base_allocator_partition_allocator_src_partition_alloc_raw_ptr.dll.lib']
libs=['advapi32.lib','userenv.lib','ws2_32.lib','bcrypt.lib','ntdll.lib','dbghelp.lib','synchronization.lib','shlwapi.lib','shell32.lib']
cmd=[str(llvm/'lld-link.exe'),'/OUT:'+str(work/(mode+'.exe')),'/SUBSYSTEM:CONSOLE','/INCREMENTAL:NO',*[str(work/(n+'.obj')) for n in files],str(work/'auth_challenge.lib'),*[str(out/x) for x in deps],*libs]
for d in ['C:/Program Files (x86)/Microsoft Visual Studio/2022/BuildTools/VC/Tools/MSVC/14.44.35207/lib/x64','C:/Program Files (x86)/Windows Kits/10/Lib/10.0.26100.0/um/x64','C:/Program Files (x86)/Windows Kits/10/Lib/10.0.26100.0/ucrt/x64']:cmd.append('/LIBPATH:'+d)
if mode=='fuzz':
 runtime=full/'third_party/llvm-build/Release+Asserts/lib/clang/24/lib/windows'
 cmd += ['/NODEFAULTLIB:clang_rt.ubsan_standalone-x86_64.lib',str(runtime/'clang_rt.ubsan_standalone-x86_64.lib'),str(runtime/'clang_rt.ubsan_standalone_cxx-x86_64.lib'),*map(str,(root/'.scratch/data-pack/fuzzer-engine').glob('*.obj'))]
if mode=='unit':
 cmd += [*map(str,(out/'obj/third_party/googletest/gtest').glob('*.obj')),str(out/'third_party_abseil-cpp_absl.dll.lib')]
 cmd += list(map(str,(full/'third_party/rust-toolchain/lib/rustlib/x86_64-pc-windows-msvc/lib').glob('libprofiler_builtins-*.rlib')))
r=subprocess.run(cmd,cwd=out,env=env,capture_output=True,text=True);(work/(mode+'-link.log')).write_text(r.stdout+r.stderr);print(r.stdout,r.stderr);r.check_returncode()
(work/(mode+'-binary.json')).write_text(json.dumps({'sha256':hashlib.sha256((work/(mode+'.exe')).read_bytes()).hexdigest(),'link_command':cmd,'rust_command':rust},indent=2)+'\n')


