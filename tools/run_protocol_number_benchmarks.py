#!/usr/bin/env python3
"""Run the GN-built paired protocol-number benchmark on one logical CPU."""
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
    parser.add_argument("--source-list", type=Path,
                        help="JSON array of workspace-relative source paths for an experimental harness")
    args = parser.parse_args()
    if not re.fullmatch(r"[a-z0-9-]+", args.run):
        parser.error("run must be a short lowercase identifier")
    if not 0 <= args.cpu < (os.cpu_count() or 1):
        parser.error("CPU index is outside this machine's logical CPUs")
    executable = args.executable.resolve(strict=True)
    output = args.output_dir.resolve()
    output.mkdir(parents=True, exist_ok=True)
    if args.source_list:
        paths = [(ROOT / p).resolve(strict=True) for p in json.loads(args.source_list.read_text())]
        if any(not p.is_relative_to(ROOT) for p in paths):
            parser.error("source paths must stay inside the editing workspace")
    else:
        paths = sorted(p for p in (ROOT / "net/base").glob("parse_number*") if p.is_file())
        paths += [ROOT / p for p in ("net/BUILD.gn",)]
    hashes = {p.relative_to(ROOT).as_posix(): digest(p) for p in paths}
    (output / f"{args.run}-source-hashes.json").write_text(json.dumps(hashes, indent=2) + "\n")
    binary_hash = digest(executable)
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
    for target in ("parse_number_perftests", "parse_number_parser", "parse_number_core"):
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
