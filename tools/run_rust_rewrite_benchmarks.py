#!/usr/bin/env python3
"""Measure a configured GN rewrite benchmark and preserve its provenance."""

import argparse
import ctypes
from datetime import datetime, timezone
import hashlib
import json
import os
from pathlib import Path
import platform
import re
import shutil
import subprocess
import time

ROOT = Path(__file__).resolve().parents[1]
MODULES = {
    "http-range": {
        "patterns": [("net/http", "http_range*"), ("net/http", "http_util.cc"),
                     ("net/http", "http_util.h"), ("net/http", "http_byte_range.*"),
                     ("net/base", "parse_number*")],
        "targets": ["http_range_promote_perftests", "http_range_promote_reference",
                    "parse_number_native_reference", "http_range_parser", "parse_number_core"],
    },
    "protocol-numbers": {
        "patterns": [("net/base", "parse_number*")],
        "targets": ["parse_number_native_perftests", "parse_number_native_reference",
                    "parse_number_native_parser", "parse_number_core"],
    },
    "hsts": {
        "patterns": [("net/http", "hsts*"), ("net/http", "http_security_headers*"),
                     ("net/base", "parse_number*")],
        "targets": ["hsts_perftests", "hsts_test_reference", "parse_number_test_reference",
                    "hsts_parser", "parse_number_core"],
    },
    "mime-sniffer": {
        "patterns": [("net/base", "mime_sniffer*")],
        "targets": ["mime_sniffer_rust_perftests", "mime_sniffer_reference", "mime_sniffer_parser"],
    },
    "gzip-header": {
        "patterns": [("net/filter", "gzip_header*")],
        "targets": ["gzip_header_perftests", "gzip_header_test_reference", "gzip_header_parser"],
    },
    "http-status-code": {
        "patterns": [("net/http", "http_status_code*")],
        "targets": ["http_status_code_perftests", "http_status_code_reference", "http_status_code_rust"],
    },
    "mime-type": {
        "patterns": [("net/base", "mime_type*"), ("net/base", "mime_util.cc"),
                     ("net/http", "http_util.cc"), ("net/http", "http_util.h")],
        "targets": ["mime_type_perftests", "mime_type_test_reference", "mime_type_parser", "net"],
    },
    "content-disposition": {
        "patterns": [("net/http", "content_disposition*"), ("net/http", "http_content_disposition*")],
        "targets": ["content_disposition_perftests", "content_disposition_test_reference",
                    "content_disposition_parser", "net"],
    },
    "dns-names": {
        "patterns": [("net/dns", "dns_names*"), ("net/dns", "BUILD.gn"),
                     ("base/strings", "string_util.h"), ("base/strings", "string_util_internal.h")],
        "targets": ["dns_names_perftests", "dns_names_reference", "dns_names_rust", "net"],
        "extra_flag_paths": ["obj/net/dns/dns.ninja"],
    },
    "referrer-policy": {
        "patterns": [("net/url_request", "referrer_policy*")],
        "targets": ["referrer_policy_perftests", "referrer_policy_reference", "referrer_policy_parser", "net"],
    },
    "auth-challenges": {
        "patterns": [("net/http", "auth_challenge*"),
                     ("net/http", "http_auth_challenge_tokenizer*"),
                     ("net/http", "http_auth_multi_round_parse*"),
                     ("base/strings", "string_util.h"), ("base/strings", "string_util_internal.h"),
                     ("base/strings", "string_util_impl_helpers.h"), ("base/strings", "string_util.cc")],
        "targets": ["auth_challenge_perftests", "auth_challenge_reference", "auth_challenge_rust", "net"],
    },
    "net-diagnostics-debug-formatting": {
        "patterns": [("net/base", "net_diagnostics*"), ("net/base", "load_flags*"),
                     ("net/base", "privacy_mode*")],
        "targets": ["net_diagnostics_debug_formatting_perftests",
                    "net_diagnostics_debug_formatting_reference",
                    "net_diagnostics_debug_formatting_rust", "net"],
    },
    "port-policy": {
        "patterns": [("net/base", "port_policy*"), ("net/base", "port_util.*"),
                     ("net/base", "ip_address*"), ("net/base", "ip_endpoint.*"),
                     ("net/base", "features.*"),
                     ("base/strings", "string_number_conversions*"),
                     ("base/strings", "string_split*"), ("base/strings", "string_util*")],
        "targets": ["port_policy_perftests", "port_policy_reference", "port_policy_rust", "net"],
    },
}


def digest(path):
    with path.open("rb") as stream:
        return hashlib.file_digest(stream, "sha256").hexdigest()


def local_dll_dependencies(executable, inspector):
    """Bind component builds to their actual adjacent DLLs, recursively.

    System DLLs remain environment dependencies; no claim is made to capture
    dynamically loaded plugins. These isolated harnesses use static imports.
    """
    pending, visited, hashes, external = [executable], set(), {}, set()
    while pending:
        binary = pending.pop()
        if binary in visited:
            continue
        visited.add(binary)
        output = subprocess.check_output(
            [str(inspector), "/nologo", "/dependents", str(binary)], text=True)
        names = re.findall(r"^\s+([^\s/\\:]+\.dll)\s*$", output, re.M | re.I)
        for name in names:
            dependency = (executable.parent / name).resolve()
            if dependency.is_file():
                hashes[dependency.name] = digest(dependency)
                pending.append(dependency)
            else:
                external.add(name.lower())
    return hashes, sorted(external)


def method_tokens(source, signature):
    """Compare C++ function tokens while ignoring formatting and comments."""
    source = source[source.index(signature):]
    tokens = re.findall(r'//[^\n]*|/\*[\s\S]*?\*/|"(?:\\.|[^"\\])*"|'
                        r"'(?:\\.|[^'\\])*'|\w+|[^\s\w]", source)
    result, depth, opened = [], 0, False
    for token in tokens:
        if token.startswith(('//', '/*')):
            continue
        result.append(token)
        if token == '{':
            opened, depth = True, depth + 1
        elif token == '}':
            depth -= 1
            if opened and depth == 0:
                return result
    raise ValueError('Unterminated benchmark method: ' + signature)


def check_range_adapter(root):
    actual = (root / 'net/http/http_util.cc').read_text()
    adapter = (root / 'net/http/http_range_benchmark_adapter.cc').read_text()
    signatures = ['bool HttpUtil::ParseRangeHeader(',
                  'std::optional<HttpByteRange> HttpUtil::ParseFetchSingleRange(',
                  'bool HttpUtil::ParseContentRangeHeaderFor206(']
    checked = []
    for signature in signatures:
        if method_tokens(actual, signature) != method_tokens(adapter, signature):
            raise ValueError('Benchmark adapter differs from production: ' + signature)
        checked.append(signature)
    return checked


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--module", choices=MODULES, required=True)
    parser.add_argument("--executable", type=Path, required=True)
    parser.add_argument("--build-root", type=Path, required=True)
    parser.add_argument("--output-dir", type=Path, required=True)
    parser.add_argument("--run", required=True)
    parser.add_argument("--cpu", type=int, default=4)
    parser.add_argument("--allow-candidate", action="store_true",
                        help="Select the preserved, unintegrated numeric CXX experiment")
    parser.add_argument("--dependency-inspector", type=Path,
                        help="MSVC dumpbin.exe; records adjacent imported DLLs in component builds")
    args = parser.parse_args()
    if args.allow_candidate and args.module != "protocol-numbers":
        parser.error("Candidate mode is only defined for the historical numeric experiment")
    if not re.fullmatch(r"[a-z][a-z0-9-]*", args.run):
        parser.error("Run must be a lowercase slug")
    if not 0 <= args.cpu < min(64, os.cpu_count() or 1):
        parser.error("CPU index is outside the supported affinity mask")
    executable = args.executable.resolve(strict=True)
    build = args.build_root.resolve(strict=True)
    if not executable.is_relative_to(build / "out"):
        parser.error("Use the configured GN executable beneath the build root's out directory")
    module = dict(MODULES[args.module])
    if args.allow_candidate:
        module["targets"] = ["parse_number_candidate_perftests", "parse_number_test_reference",
                             "parse_number_parser", "parse_number_core"]
    adapter_methods = (check_range_adapter(ROOT)
                       if args.module == 'http-range' else None)
    if executable.stem != module["targets"][0]:
        parser.error("Executable name must match the module's configured benchmark target")
    clean_command = [str(build / "third_party/ninja/ninja.exe"), "-C",
                     str(executable.parent), "-n", module["targets"][0]]
    clean_result = subprocess.run(clean_command, capture_output=True, text=True)
    if clean_result.returncode or "no work to do" not in clean_result.stdout:
        raise RuntimeError("Configured benchmark needs rebuilding:\n" +
                           clean_result.stdout + clean_result.stderr)
    files = [ROOT / "net/BUILD.gn", ROOT / "net/base/BUILD.gn"]
    for directory, pattern in module["patterns"]:
        files.extend(path for path in (ROOT / directory).glob(pattern) if path.is_file())
    files = sorted(set(files))
    hashes = {path.relative_to(ROOT).as_posix(): digest(path) for path in files}
    for relative, expected in hashes.items():
        if not (build / relative).is_file() or digest(build / relative) != expected:
            raise RuntimeError("Editing/configured source mismatch: " + relative)
    output = args.output_dir.resolve()
    output.mkdir(parents=True, exist_ok=True)
    (output / f"{args.run}-source-hashes.json").write_text(json.dumps(hashes, indent=2) + "\n")
    binary_hash = digest(executable)
    inspector = args.dependency_inspector
    if os.name == "nt" and inspector is None:
        search_path = os.environ.get("PATH", "")
        environment_file = executable.parent / "environment.x64"
        if environment_file.is_file():
            for item in environment_file.read_bytes().decode().split("\0"):
                key, _, value = item.partition("=")
                if key.upper() == "PATH":
                    search_path = value
        found = shutil.which("dumpbin.exe", path=search_path)
        if found:
            inspector = Path(found)
    if os.name == "nt" and inspector is None:
        parser.error("Provide --dependency-inspector pointing to dumpbin.exe for component provenance")
    dll_hashes, system_dlls = local_dll_dependencies(executable, inspector) if inspector else ({}, [])
    env = os.environ.copy()
    env["PATH"] = str(executable.parent) + os.pathsep + env.get("PATH", "")
    started = time.monotonic()
    started_utc = datetime.now(timezone.utc).isoformat()
    with (output / f"{args.run}-benchmark.jsonl").open("wb") as stream:
        process = subprocess.Popen([str(executable)], cwd=executable.parent, env=env,
                                   stdout=stream, stderr=subprocess.PIPE)
        try:
            if os.name == "nt":
                if not ctypes.windll.kernel32.SetProcessAffinityMask(
                        ctypes.c_void_p(int(process._handle)), ctypes.c_size_t(1 << args.cpu)):
                    raise RuntimeError("CPU affinity could not be set")
            else:
                os.sched_setaffinity(process.pid, {args.cpu})
            _, errors = process.communicate()
        except BaseException:
            process.kill()
            process.wait()
            raise
    record = {"exit_code": process.returncode, "elapsed_seconds": time.monotonic() - started,
              "command": [str(executable)], "executable_sha256": binary_hash,
              "module": args.module, "cpu": args.cpu, "affinity_mask": 1 << args.cpu,
              "production_status": "deferred" if args.allow_candidate else "integrated; acceptance validation recorded separately",
              "started_utc": started_utc, "completed_utc": datetime.now(timezone.utc).isoformat(),
              "local_dll_sha256": dll_hashes, "system_dll_imports": system_dlls,
              "dependency_inspector": str(inspector) if inspector else None,
              "environment": {"platform": platform.platform(), "processor": platform.processor(),
                              "logical_cpus": os.cpu_count()},
              "uncontrolled": ["power/thermal state", "other system processes"],
              "stderr": errors.decode(errors="replace"), "configured_flags": {}}
    record["build_clean_check"] = {"command": clean_command, "output": clean_result.stdout.strip()}
    if adapter_methods:
        record['adapter_identity_check'] = {'method_signatures': adapter_methods,
                                            'comparison': 'C++ tokens, comments and whitespace ignored'}
    for target in module["targets"]:
        ninja = executable.parent / "obj/net" / (target + ".ninja")
        if ninja.is_file():
            record["configured_flags"][target] = dict(re.findall(
                r"^(defines|cflags|cflags_cc|rustflags) = (.*)$", ninja.read_text(), re.M))
    for relative in module.get("extra_flag_paths", []):
        ninja = executable.parent / relative
        record["configured_flags"][relative] = dict(re.findall(
            r"^(defines|cflags|cflags_cc|rustflags) = (.*)$", ninja.read_text(), re.M))
    for name, relative in [("rust", "third_party/rust-toolchain/bin/rustc.exe"),
                           ("clang", "third_party/llvm-build/Release+Asserts/bin/clang-cl.exe")]:
        compiler = build / relative
        if compiler.is_file():
            record[name + "_version"] = subprocess.check_output(
                [str(compiler), "--version"], text=True).strip()
    if binary_hash != digest(executable) or any(digest(ROOT / path) != sha or
                                               digest(build / path) != sha
                                               for path, sha in hashes.items()):
        raise RuntimeError("Source or measured executable changed during the benchmark")
    if any(digest(executable.parent / name) != sha for name, sha in dll_hashes.items()):
        raise RuntimeError("Imported component DLL changed during the benchmark")
    record['raw_report_sha256'] = digest(output / f'{args.run}-benchmark.jsonl')
    record['source_hashes_file_sha256'] = digest(output / f'{args.run}-source-hashes.json')
    (output / f"{args.run}-execution.json").write_text(json.dumps(record, indent=2) + "\n")
    print(json.dumps({key: record[key] for key in ["module", "exit_code", "elapsed_seconds",
                                                   "cpu", "executable_sha256"]}, indent=2))
    raise SystemExit(process.returncode)


if __name__ == "__main__":
    main()
