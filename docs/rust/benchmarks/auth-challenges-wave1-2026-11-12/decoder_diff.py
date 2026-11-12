from pathlib import Path
import os, subprocess
root=Path('D:/Rustium')
work=root/'.scratch/net-wave1/auth-challenges'
build=Path('D:/RustiumBuild/src')
env=os.environ.copy()
for item in (build/'out/Default/environment.x64').read_bytes().decode().split('\0'):
    k,s,v=item.partition('=')
    if s: env[k]=v
llvm=build/'third_party/llvm-build/Release+Asserts/bin'
src=work/'original/third_party/modp_b64/modp_b64.cc'
subprocess.run([str(llvm/'clang-cl.exe'),'/nologo','/c','/O2',str(src),'/Fo'+str(work/'modp.obj')],env=env,check=True)
subprocess.run([str(llvm/'clang-cl.exe'),'/nologo','/c','/O2',str(src.parent/'auth_reference_decode.cc'),'/Fo'+str(work/'ref.obj')],env=env,check=True)
subprocess.run([str(llvm/'lld-link.exe'),'/lib','/OUT:'+str(work/'modp.lib'),str(work/'modp.obj'),str(work/'ref.obj')],env=env,check=True)
subprocess.run(['rustc','--edition','2021','-O',str(work/'decoder_diff.rs'),'-L',str(work),'-l','static=modp','-o',str(work/'decoder_diff.exe')],env=env,check=True)
subprocess.run([str(work/'decoder_diff.exe')],env=env,check=True)

