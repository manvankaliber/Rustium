from pathlib import Path
import subprocess,json
root=Path('D:/Rustium');work=root/'.scratch/net-wave1/auth-challenges'; rust=Path('D:/RustiumBuild/src/third_party/rust-toolchain/bin');fmt=root/'.scratch/clang-format/clang_format/data/bin/clang-format.exe'
files=['http_auth_challenge_tokenizer.cc','http_auth_multi_round_parse.cc','auth_challenge_ffi.h','auth_challenge_reference.cc','auth_challenge_reference.h','auth_challenge_test_support.h','auth_challenge_rust_unittest.cc','auth_challenge_fuzzer.cc','auth_challenge_perftest.cc']
commands={'clippy':[str(rust/'clippy-driver.exe'),'--edition','2024','--crate-type','rlib',str(root/'net/http/auth_challenge_ffi.rs'),'--emit','metadata','-o',str(work/'clippy.rmeta'),'-D','warnings'],'rustfmt':[str(rust/'rustfmt.exe'),'--edition','2024','--check',str(root/'net/http/auth_challenge_ffi.rs'),str(root/'net/http/auth_challenge_core.rs')],'clang-format':[str(fmt),'--dry-run','--Werror',*[str(root/'net/http'/x) for x in files]]}
result={}
for name,cmd in commands.items():
 p=subprocess.run(cmd,cwd=root,text=True,capture_output=True);(work/(name+'.log')).write_text(p.stdout+p.stderr);result[name]={'command':cmd,'exit_code':p.returncode}
(work/'checks.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result,indent=2));assert all(x['exit_code']==0 for x in result.values())
