#!/usr/bin/env python3
"""Run the GN-built paired http-range benchmark on one logical CPU."""
import argparse
import ctypes
import hashlib
import json
import os
import platform
import re
import subprocess
import time
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--executable", required=True, type=Path)
    parser.add_argument("--output-dir", required=True, type=Path)
    parser.add_argument("--run", required=True)
    parser.add_argument("--cpu", type=int, default=4)
    parser.add_argument("--build-record", type=Path, help="Contemporaneous native build metadata")
    parser.add_argument("--source-list", type=Path,
                        help="JSON array of workspace-relative source paths for an experimental harness")
    args = parser.parse_args()
    if not re.fullmatch(r"[a-z0-9-]+", args.run):
        parser.error("run must be a short lowercase identifier")
    if not 0 <= args.cpu < (os.cpu_count() or 1):
        parser.error("CPU index is outside this machine's logical CPUs")
    def method(source, signature):
        start=source.index(signature);pos=source.index('{',start)+1;depth=1
        while depth:
            if source[pos]=='{': depth+=1
            elif source[pos]=='}': depth-=1
            pos+=1
        return re.findall(r'\w+|[^\s\w]',re.sub(r'//[^\n]*','',source[start:pos]))
    actual=(ROOT/'net/http/http_util.cc').read_text()
    adapter=(ROOT/'net/http/http_range_benchmark_adapter.cc').read_text()
    for signature in ['bool HttpUtil::ParseRangeHeader(', 'std::optional<HttpByteRange> HttpUtil::ParseFetchSingleRange(', 'bool HttpUtil::ParseContentRangeHeaderFor206(']:
        if method(actual,signature)!=method(adapter,signature):
            raise RuntimeError('Benchmark adapter differs from production method: '+signature)
    executable = args.executable.resolve(strict=True)
    output = args.output_dir.resolve()
    output.mkdir(parents=True, exist_ok=True)
    if args.source_list:
        paths = [(ROOT / p).resolve(strict=True) for p in json.loads(args.source_list.read_text())]
        if any(not p.is_relative_to(ROOT) for p in paths):
            parser.error("source paths must stay inside the editing workspace")
    else:
        paths = sorted(p for p in (ROOT / "net/http").glob("http_range*") if p.is_file())
        paths += [ROOT / p for p in ("net/BUILD.gn", "net/http/http_util.cc", "net/http/http_byte_range.cc", "net/http/http_byte_range.h", "net/base/parse_number_parser.rs", "net/base/parse_number_test_support.cc", "net/base/parse_number_test_support.h")]
    hashes = {p.relative_to(ROOT).as_posix(): digest(p) for p in paths}
    (output / f"{args.run}-source-hashes.json").write_text(json.dumps(hashes, indent=2) + "\n")
    binary_hash = digest(executable)
    build_record=json.loads(args.build_record.read_text()) if args.build_record else None
    if build_record:
        if build_record['binary_sha256']!=binary_hash or any(digest(ROOT/p)!=sha for p,sha in build_record['source_sha256'].items()):
            raise RuntimeError('Native binary/source differs from its recorded build')
    env = os.environ.copy()
    env["PATH"] = str(executable.parent) + os.pathsep + env.get("PATH", "")
    started = time.monotonic()
    with (output / f"{args.run}-benchmark.jsonl").open("wb") as stream:
        process = subprocess.Popen([str(executable)], cwd=executable.parent, env=env, stdout=stream, stderr=subprocess.PIPE)
        try:
            if os.name == "nt":
                if args.cpu >= 64 or not ctypes.windll.kernel32.SetProcessAffinityMask(ctypes.c_void_p(int(process._handle)), ctypes.c_size_t(1 << args.cpu)):
                    raise RuntimeError("Could not set the requested CPU affinity")
            else:
                os.sched_setaffinity(process.pid, {args.cpu})
            _, errors = process.communicate()
        except BaseException:
            process.kill()
            process.wait()
            raise
    record = {"exit_code": process.returncode, "elapsed_seconds": time.monotonic() - started,
              "command": [str(executable)], "executable_sha256": binary_hash,
              "cpu": args.cpu, "affinity_mask": 1 << args.cpu,
              "environment": {"platform": platform.platform(), "processor": platform.processor(),
                              "logical_cpus": os.cpu_count()},
              "uncontrolled": ["power/thermal state", "other system processes"],
              "stderr": errors.decode(errors="replace")}
    if build_record:
        record["native_build_record"] = build_record
    for target in ("http_range_perftests", "http_range_parser", "parse_number_core"):
        ninja = executable.parent / "obj/net" / (target + ".ninja")
        if ninja.exists():
            record[target + "_flags"] = {k: v for k, v in re.findall(r"^(defines|cflags|cflags_cc|rustflags) = (.*)$", ninja.read_text(), re.M)}
    if binary_hash != digest(executable) or any(digest(ROOT / p) != sha for p, sha in hashes.items()):
        raise RuntimeError("The binary or source changed during measurement")
    (output / f"{args.run}-execution.json").write_text(json.dumps(record, indent=2) + "\n")
    print(json.dumps(record, indent=2))
    raise SystemExit(process.returncode)

if __name__ == "__main__":
    main()
