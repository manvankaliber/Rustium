from pathlib import Path
import os, re, subprocess

root = Path("D:/Rustium")
full = Path("D:/RustiumBuild/src")
out = full / "out/Default"
work = root / ".scratch/net-wave1/debug-formatting"
env = os.environ.copy()
for item in (out / "environment.x64").read_bytes().decode().split("\0"):
    key, sep, value = item.partition("=")
    if sep:
        env[key] = value

rustc = full / "third_party/rust-toolchain/bin/rustc.exe"
staticlib = work / "net_diagnostics_debug_formatting.lib"
rust_args = [str(rustc), "--crate-name", "net_diagnostics_debug_formatting",
             "--crate-type", "staticlib", "--edition=2021", "-C", "panic=abort",
             str(root / "net/base/net_diagnostics_debug_formatting_ffi.rs"),
             "-o", str(staticlib)]
subprocess.run(rust_args, cwd=root, env=env, check=True)

variables = dict(re.findall(r"^(\w+) = (.*)$", (out / "obj/net/net.ninja").read_text(), re.M))
template = (out / "toolchain.ninja").read_text().split("rule cxx\n", 1)[1].splitlines()[0].strip().removeprefix("command = ")

def compile(source: Path, name: str):
    values = dict(variables)
    if name == "focused_unittest":
        values = dict(re.findall(r"^(\w+) = (.*)$", (out / "obj/net/net_unittests.ninja").read_text(), re.M))
    values.update({"in": str(source), "source_name_part": name})
    command = re.sub(r"\$\{(\w+)\}", lambda m: values.get(m[1], ""), template).replace("$:", ":")
    command = command.replace("-I../.. ", f"-I{work / 'include'} -I{root} -I../.. ")
    command = re.sub(r"/Fo\S+", lambda _: f"/Fo{work / (name + '.obj')}", command)
    command = re.sub(r'/Fd"[^\"]+"', lambda _: f'/Fd"{work / (name + ".pdb")}"', command)
    rsp = work / f"{name}.rsp"
    rsp.write_text(command.split(" ", 1)[1])
    compiler = command.split(" ", 1)[0]
    result = subprocess.run([str((out / compiler).resolve()), "@" + str(rsp)], cwd=out, env=env, capture_output=True, text=True)
    (work / f"{name}-compile.log").write_text(result.stdout + result.stderr)
    if result.returncode:
        print("\n".join(line for line in (result.stdout + result.stderr).splitlines() if "including file:" not in line))
        result.check_returncode()

sources = [
    (root / "net/base/load_flags_to_string.cc", "load_flags_adapter"),
    (root / "net/base/privacy_mode.cc", "privacy_mode_adapter"),
    (root / "net/base/net_diagnostics_load_flags_reference.cc", "load_flags_reference"),
    (root / "net/base/net_diagnostics_privacy_mode_reference.cc", "privacy_mode_reference"),
    (work / "native_api_main.cc", "native_api_main"),
    (root / "net/base/net_diagnostics_debug_formatting_unittest.cc", "focused_unittest"),
]
for source, name in sources:
    compile(source, name)

libraries = [
    str(out / "base.dll.lib"), str(out / "libc++.dll.lib"),
    *[str(out / f"base_allocator_partition_allocator_src_partition_alloc_allocator_{name}.dll.lib") for name in ("base", "core", "shim")],
    str(out / "base_allocator_partition_allocator_src_partition_alloc_raw_ptr.dll.lib"),
    str(staticlib), "advapi32.lib", "userenv.lib", "ws2_32.lib", "bcrypt.lib", "ntdll.lib", "dbghelp.lib", "synchronization.lib", "shlwapi.lib", "shell32.lib",
]
linker = full / "third_party/llvm-build/Release+Asserts/bin/lld-link.exe"
exe = work / "native-api-v5.exe"
objects = [str(work / f"{name}.obj") for _, name in sources if name != "focused_unittest"]
command = [str(linker), "/OUT:" + str(exe), "/SUBSYSTEM:CONSOLE", "/INCREMENTAL:NO", *objects, *libraries]
for directory in [
    "C:/Program Files (x86)/Microsoft Visual Studio/2022/BuildTools/VC/Tools/MSVC/14.44.35207/lib/x64",
    "C:/Program Files (x86)/Windows Kits/10/Lib/10.0.26100.0/um/x64",
    "C:/Program Files (x86)/Windows Kits/10/Lib/10.0.26100.0/ucrt/x64",
]:
    command.append("/LIBPATH:" + directory)
result = subprocess.run(command, cwd=out, env=env, capture_output=True, text=True)
(work / "native-api-link.log").write_text(result.stdout + result.stderr)
if result.returncode:
    print(result.stdout, result.stderr)
    result.check_returncode()
print("Built", exe)
run = subprocess.run([str(exe)], cwd=out, env=env, capture_output=True, text=True)
(work / "native-api-output.txt").write_text(run.stdout + run.stderr)
print(run.stdout, run.stderr)
run.check_returncode()
