"""Parent-only immutable timing delivery; prepare never builds or measures.

Uses the ordinary delivery copy contract: independent copies, original/copy
mapping, exact directory membership and before/after hashes. This is separate
from the ordinary verify-only conductor. Only the copied canonical runner
launches measurements. Lexical includes are context, never compiler deps.
"""
import argparse
import ctypes
from datetime import datetime, timezone
import hashlib
import importlib.util
import json
import math
import os
from pathlib import Path
import posixpath
import re
import shutil
import signal
import subprocess
import sys
import time


def require(value, message):
    if not value:
        raise ValueError(message)


def sha(path):
    with Path(path).open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def load(path):
    return json.loads(Path(path).read_text(encoding='utf-8-sig'))


def save(path, data):
    with Path(path).open('x', encoding='utf-8', newline='\n') as stream:
        json.dump(data, stream, indent=2)
        stream.write('\n')


def under(root, relative):
    require(not Path(relative).is_absolute(), 'Expected relative path: ' + str(relative))
    lexical = (root / relative).absolute()
    for part in [lexical, *lexical.parents]:
        require(not part.is_symlink() and not (hasattr(part, 'is_junction') and part.is_junction()),
                'Linked snapshot path: ' + str(part))
    result = lexical.resolve()
    require(result.is_relative_to(root.resolve()), 'Path escapes root: ' + str(relative))
    return result


def unlinked(path):
    path = Path(path).absolute()
    for part in [path, *path.parents]:
        require(not part.is_symlink() and not (hasattr(part, 'is_junction') and part.is_junction()),
                'Linked snapshot input: ' + str(part))
    return path.resolve(strict=True)


def binding(path):
    return {'path': str(Path(path).resolve()), 'sha256': sha(path)}


def checked(row):
    path = unlinked(row['path'])
    require(sha(path) == row['sha256'], 'Evidence/input drift: ' + str(path))
    return path


def members(path):
    return sorted(p.relative_to(path).as_posix() + ('/' if p.is_dir() else '')
                  for p in path.rglob('*'))


class Copies:
    """Same independent-copy and membership guards as regular_suite.prepare."""
    def __init__(self, root, build):
        self.root, self.build = root.resolve(), build.resolve()
        self.files, self.directories = {}, {}

    def mapped(self, path):
        if path.is_relative_to(self.build):
            return Path('configured') / path.relative_to(self.build)
        if path.is_relative_to(self.root):
            return Path('editing') / path.relative_to(self.root)
        return Path('context/external') / hashlib.sha256(str(path).casefold().encode()).hexdigest() / path.name

    def add(self, path, expected=None, copy=None):
        path = unlinked(path)
        if path.is_dir():
            require(expected is None and copy is None, 'Directory override unsupported')
            names = members(path)
            require(path not in self.directories or self.directories[path] == names,
                    'Original directory changed during freeze: ' + str(path))
            self.directories[path] = names
            for child in path.rglob('*'):
                unlinked(child)
                if child.is_file():
                    self.add(child)
            return
        digest = sha(path)
        require(expected is None or digest == expected, 'Input drift: ' + str(path))
        row = {'original': str(path), 'copy': (copy or self.mapped(path)).as_posix(), 'sha256': digest}
        require(path not in self.files or self.files[path] == row, 'Conflicting input mapping')
        self.files[path] = row

    def stable(self):
        for path, row in self.files.items():
            require(sha(unlinked(path)) == row['sha256'], 'Original changed during freeze: ' + str(path))
        for path, names in self.directories.items():
            require(members(path) == names, 'Original directory changed during freeze: ' + str(path))

    def freeze(self, dest):
        require(not dest.exists(), 'Use a new snapshot directory')
        dest.parent.mkdir(parents=True, exist_ok=True)
        require(shutil.disk_usage(dest.parent).free > sum(p.stat().st_size for p in self.files) + 1024**3,
                'Independent copies require 1GiB free headroom')
        dest.mkdir()
        for path, names in self.directories.items():
            folder = under(dest, self.mapped(path)); folder.mkdir(parents=True, exist_ok=True)
            for name in names:
                if name.endswith('/'):
                    under(folder, name).mkdir(parents=True, exist_ok=True)
        destinations = {}
        for row in self.files.values():
            target = under(dest, row['copy'])
            key = str(target).casefold()
            require(key not in destinations or destinations[key] == row['sha256'], 'Conflicting logical input versions')
            destinations[key] = row['sha256']
            target.parent.mkdir(parents=True, exist_ok=True)
            if not target.exists():
                shutil.copyfile(row['original'], target)  # never link mutable outputs
            require(not os.path.samefile(row['original'], target), 'Copy aliases original')
            require(target.stat().st_nlink == 1 and sha(target) == row['sha256'], 'Copied input mismatch')
        self.stable()
        for path, names in self.directories.items():
            require(members(under(dest, self.mapped(path))) == names,
                    'Copied directory membership differs: ' + str(path))


def config_closure(out):
    # Recursively evaluated graph, as in native_delivery.config_files.
    pending = [out / 'build.ninja', out / 'toolchain.ninja']
    files = {out / 'args.gn', out / 'environment.x64'}
    while pending:
        path = pending.pop().resolve()
        if path in files:
            continue
        require(path.is_relative_to(out), 'Ninja include escapes output')
        text = path.read_text(encoding='utf-8')
        files.add(path)
        for name in re.findall(r'(?m)^\s*(?:subninja|include)\s+(.+)$', text):
            require('$' not in name, 'Unresolved Ninja include')
            pending.append(under(out, name.strip()))
    for pattern in ('*.rsp', '*.rsp.rust', '*.rustflags.json'):
        files.update((out / 'obj').rglob(pattern))
    return files


def parse_deps(text, selected=None):
    """Read actual Ninja deps records; stale/missing selected objects fail closed."""
    result, seen, key, count, lines, valid = {}, set(), None, None, [], False
    def finish():
        if key is not None:
            require(len(lines) == count, 'Truncated compiler dependency record: ' + key)
            require(key not in seen, 'Duplicate compiler dependency record')
            seen.add(key)
            if valid and (selected is None or key in selected):
                result[key] = list(lines)
    for line in (text.splitlines() if isinstance(text, str) else text):
        line = line.rstrip('\r\n')
        match = re.fullmatch(r'(.+): #deps (\d+), deps mtime \d+ \((VALID|STALE)\)', line)
        if match:
            finish()
            key = ninja_path(match[1])
            count, lines, valid = int(match[2]), [], match[3] == 'VALID'
        elif line.startswith('    ') and key is not None:
            lines.append(line[4:])
        elif line.startswith('ninja: Entering directory ') and key is None:
            continue
        elif line.strip():
            raise ValueError('Unrecognized compiler dependency output: ' + line[:150])
    finish()
    return result


def ninja_path(name):
    return posixpath.normpath(name.replace('\\', '/'))


def parse_depfile(stream):
    """Retained Make-style Ninja depfiles; paths, not a regex include scan.

    Handles escaped whitespace/#/:/backslash, $$, drive colons, continuations,
    multiple outputs and empty phony header rules. Unknown grammar fails closed.
    """
    result, targets, dependencies, token, colon = {}, [], [], [], False
    def word():
        if token:
            (dependencies if colon else targets).append(ninja_path(''.join(token)))
            token.clear()
    def finish():
        nonlocal targets, dependencies, colon
        word()
        if targets or dependencies or colon:
            require(targets and colon, 'Malformed retained depfile rule')
            for target in targets:
                require(target not in result, 'Duplicate retained depfile target')
                result[target] = list(dependencies)
        targets, dependencies, colon = [], [], False
    for number, raw in enumerate(stream, 1):
        require(number <= 1000000 and len(raw) <= 1024**2, 'Retained depfile parsing bound exceeded')
        line = raw.replace('\r\n', '\n')
        i, continued = 0, False
        while i < len(line):
            char = line[i]
            if char == '\\':
                require(i + 1 < len(line), 'Truncated retained depfile escape')
                following = line[i + 1]
                if following == '\n':
                    word(); continued = True; i += 2; continue
                if following in ' \t#:\\':
                    token.append(following); i += 2; continue
                token.append(char); i += 1; continue  # Windows path separator
            if char == '$':
                require(i + 1 < len(line) and line[i + 1] == '$', 'Unresolved retained depfile variable')
                token.append('$'); i += 2; continue
            if char == '#':
                break
            if char in ' \t\n':
                word(); i += 1; continue
            if char == ':':
                drive = len(token) == 1 and token[0].isalpha() and i + 1 < len(line) and line[i + 1] in '/\\'
                if drive:
                    token.append(char)
                else:
                    require(not colon, 'Unexpected retained depfile colon')
                    word(); colon = True
                i += 1; continue
            require(char not in '\r\x00', 'Invalid retained depfile character')
            token.append(char); i += 1
        if not continued:
            finish()
    require(not continued if 'continued' in locals() else True, 'Truncated retained depfile continuation')
    finish()
    return result


def dot_string(value):
    # Ninja's DOT printer quotes paths and escapes quotes/backslashes. Reject
    # other GraphViz substitutions rather than interpreting them as path bytes.
    require(re.fullmatch(r'(?:[^\\]|\\[\\"nt])*', value[1:-1]), 'Unsupported DOT path escape')
    result = json.loads(value)
    require(not any(c in result for c in '\r\n\x00'), 'Invalid graph path')
    return result


def parse_graph(stream):
    """Ninja -t graph output, including single-input compact rule arrows.

    DOT is evidence from Ninja's evaluated graph, never reconstructed from
    lexical build-line regexes. Unknown syntax, missing producers and duplicate
    identities fail closed. Per-capture limits bound Python memory.
    """
    quoted = r'"(?:[^"\\]|\\.)*"'
    node_re = re.compile(r'(' + quoted + r') \[label=(' + quoted + r')(, shape=ellipse)?\]')
    arc_re = re.compile(r'(' + quoted + r') -> (' + quoted + r')(?: \[(.*)\])?')
    nodes, arcs, opened, closed = {}, [], False, False
    for number, raw in enumerate(stream, 1):
        require(len(raw) <= 1024 * 1024 and number <= 4000000, 'Evaluated graph capture exceeds parser limit')
        line = raw.strip().removesuffix(';')
        if line == 'digraph ninja {':
            require(not opened, 'Duplicate graph header'); opened = True; continue
        require(opened and not closed, 'Malformed evaluated graph framing')
        if line == '}':
            closed = True; continue
        if line in ('rankdir="LR"', 'node [fontsize=10, shape=box, height=0.25]', 'edge [fontsize=10]'):
            continue
        match = node_re.fullmatch(line)
        if match:
            key, label, ellipse = match.groups()
            require(key not in nodes, 'Duplicate graph node')
            nodes[key] = (dot_string(label), bool(ellipse))
            require(len(nodes) <= 750000, 'Evaluated graph node limit exceeded')
            continue
        match = arc_re.fullmatch(line)
        require(match is not None, 'Unrecognized evaluated graph line: ' + line[:160])
        source, dest, attrs = match.groups()
        require(attrs is None or attrs in ('arrowhead=none', 'arrowhead=none style=dotted') or
                re.fullmatch(r'label=' + quoted + r'(?: style=dotted)?', attrs), 'Unrecognized graph edge attributes')
        arcs.append((source, dest, attrs))
        require(len(arcs) <= 3000000, 'Evaluated graph edge limit exceeded')
    require(closed and nodes, 'Truncated/empty evaluated graph')
    inputs, explicit, outputs, edges = {}, {}, {}, {}
    for source, dest, attrs in arcs:
        require(source in nodes and dest in nodes, 'Unresolved evaluated graph node')
        if nodes[dest][1]:
            require(not nodes[source][1], 'Rule-to-rule graph edge')
            inputs.setdefault(dest, []).append(ninja_path(nodes[source][0]))
            if attrs == 'arrowhead=none':
                explicit.setdefault(dest, []).append(ninja_path(nodes[source][0]))
        elif nodes[source][1]:
            outputs.setdefault(source, []).append(ninja_path(nodes[dest][0]))
        else:
            require(attrs and attrs.startswith('label='), 'Unresolved compact graph rule')
            rule = dot_string(re.fullmatch(r'label=(' + quoted + r')(?: style=dotted)?', attrs)[1]).strip()
            output = ninja_path(nodes[dest][0])
            require(output not in edges, 'Duplicate graph producer')
            edges[output] = {'rule': rule, 'outputs': [output], 'inputs': [ninja_path(nodes[source][0])],
                             'explicit_inputs': [] if 'style=dotted' in attrs else [ninja_path(nodes[source][0])]}
    for key, (rule, ellipse) in nodes.items():
        if not ellipse:
            continue
        require(key in outputs, 'Evaluated rule without outputs')
        edge = {'rule': rule, 'outputs': outputs[key], 'inputs': inputs.get(key, []),
                'explicit_inputs': explicit.get(key, [])}
        for output in outputs[key]:
            require(output not in edges, 'Duplicate evaluated producer')
            edges[output] = edge
    return edges


def capture(command, args, evidence, name, max_bytes=512 * 1024**2):
    """Parent-only acquisition. Stream to retained files, never capture_output.

    The finite Ninja read-only tools write to disk; check the capture size before
    parsing. On failure/timeout the acquisition directory remains for diagnosis.
    """
    stdout, stderr = evidence / (name + '.stdout'), evidence / (name + '.stderr')
    with stdout.open('xb') as output, stderr.open('xb') as errors:
        result = subprocess.run(command, cwd=args.build_root, stdout=output, stderr=errors,
                                timeout=600, check=False)
    require(stdout.stat().st_size <= max_bytes and stderr.stat().st_size <= 16 * 1024**2,
            'Ninja capture exceeds retained-evidence parsing limit: ' + name)
    row = {'command': command, 'exit_code': result.returncode,
           'stdout': binding(stdout), 'stderr': binding(stderr)}
    save(evidence / (name + '.json'), row)
    require(result.returncode == 0, 'Evaluated graph/dependency capture failed: ' + name)
    return stdout, row


def rule_definitions(graph_files):
    """Bind policy to exact GN-emitted rule blocks, not inferred file suffixes.

    Graph traversal itself is delegated to Ninja. GN rule blocks have literal
    names. Reject ambiguous rule scopes and any edge-local deps/depfile override
    until parent supplies an explicitly supported evaluator for that shape.
    """
    result = {}
    for path in sorted(graph_files):
        if path.suffix != '.ninja':
            continue
        block, name, context = [], None, None
        def finish():
            if name is not None:
                require(name not in result, 'Ambiguous scoped Ninja rule: ' + name)
                text = ''.join(block)
                result[name] = {'path': str(path), 'sha256': sha(path),
                    'block_sha256': hashlib.sha256(text.encode()).hexdigest(),
                    'deps': dict(re.findall(r'(?m)^  (deps|depfile) = (.*)$', text)).get('deps', ''),
                    'has_depfile': bool(re.search(r'(?m)^  depfile = ', text))}
        with path.open(encoding='utf-8') as stream:
            for line in stream:
                if line.startswith('rule '):
                    finish(); name = line[5:].strip(); block = [line]; context = 'rule'
                    require(re.fullmatch(r'[\w.-]+', name), 'Unresolved Ninja rule name')
                elif line[:1].isspace() and name is not None:
                    block.append(line)
                elif line.strip() and not line.startswith('#'):
                    finish(); name = None; block = []
                    if not line[:1].isspace():
                        context = 'build' if line.startswith('build ') else None
                    require(not re.match(r'\s*(deps|depfile)\s*=', line),
                            'Unsupported edge-local dependency override or inherited dependency binding')
            finish()
    return result


def evaluated_inputs(args, copies, graph_files, runtime_roots, evidence):
    """Actual target-rooted transitive producer closure and applicable deps.

    Supplementary rule/command inputs (assembler includes, action tools, implicit
    linker libraries, etc.) must be explicitly reviewed and pinned by parent.
    This is a fail-closed requirement, not a claim that Ninja records undeclared
    command inputs. Rust gcc-deps and module msvc-deps use their ACTUAL primary
    output key, never a fabricated C++ .obj record.
    """
    policy_path = getattr(args, 'rule_input_policy', None)
    require(policy_path is not None, 'Parent-reviewed rule/command input policy required')
    policy = load(policy_path)
    require(policy['schema_version'] == 1 and policy['source_manifest_sha256'] == sha(args.source_manifest),
            'Rule input policy source mismatch')
    copies.add(policy_path)
    definitions = rule_definitions(graph_files)
    out = args.executable.parent
    ninja = str(args.build_root / 'third_party/ninja/ninja.exe')
    prefix = [ninja, '-C', str(out), '-t']
    producers, requested, processed, dependency_records, dependency_methods = {}, set(), set(), {}, {}
    pending = {ninja_path(p.relative_to(out).as_posix()) for p in runtime_roots}
    command_output, command_record = capture([*prefix, 'commands', *sorted(pending)],
                                            args, evidence, 'evaluated-commands')
    require(sha(command_output) == policy['evaluated_commands_sha256'], 'Reviewed evaluated command identity differs')
    captures = [command_record]
    external = load(args.external_compile_inputs) if args.external_compile_inputs else {}
    if args.external_compile_inputs:
        copies.add(args.external_compile_inputs)
    prebuilt = policy.get('prebuilt_inputs', {})
    # These files are produced by GN, not Ninja build edges. Their exact bytes
    # are already part of the evaluated configuration closure and freshness.
    metadata = {ninja_path(p.relative_to(out).as_posix()) for p in graph_files
                if p.suffix == '.ninja' or p.name in ('args.gn', 'environment.x64')}
    def add_file(name):
        path = unlinked(out / name)
        if path.is_relative_to(args.build_root):
            copies.add(path)
        else:
            require(str(path) in external, 'Compiler dependency outside configured tree; classify explicitly: ' + str(path))
            copies.add(path, external[str(path)])
        return path
    def batch(names):
        chunk, size = [], 0
        for name in sorted(names):
            require(len(name) < 12000, 'Ninja target exceeds command-line bound')
            if chunk and (size + len(name) > 16000 or len(chunk) >= 64):
                yield chunk; chunk, size = [], 0
            chunk.append(name); size += len(name) + 3
        if chunk:
            yield chunk
    while pending:
        require(len(requested) < 750000, 'Producer closure exceeds target bound')
        roots = pending - requested
        pending = set()
        sidecars = roots & prebuilt.keys()
        for name in sidecars:
            row = prebuilt[name]
            require(row.get('kind') == 'rule-sidecar' and row.get('producer') in producers and
                    (name.endswith(('.rsp', '.rsp.rust', '.rustflags.json'))) and row.get('reason'),
                    'Only proven rule-internal sidecars may lack a producer: ' + name)
            copies.add(out / name, row['sha256'])
            requested.add(name)
        requested.update(roots & metadata)
        for chunk in batch(roots - sidecars - metadata):
            raw, record = capture([*prefix, 'graph', *chunk], args, evidence, 'graph-' + str(len(captures)))
            captures.append(record); requested.update(chunk)
            with raw.open(encoding='utf-8') as stream:
                more = parse_graph(stream)
            for output, edge in more.items():
                require(output not in producers or producers[output] == edge, 'Evaluated producer drift')
                producers[output] = edge
        queue, visited, need_deps = list(roots), set(), set()
        while queue:
            name = queue.pop()
            if name in visited:
                continue
            visited.add(name)
            edge = producers.get(name)
            if edge is None:
                path = out / name
                if path.resolve().is_relative_to(out):
                    require(name in sidecars or name in metadata, 'Unresolved producer closure: ' + name)
                    copies.add(path, prebuilt[name]['sha256'] if name in sidecars else None)
                else:
                    add_file(name)
                continue
            queue.extend(edge['inputs'])
            key = edge['outputs'][0]
            if key in processed:
                continue
            processed.add(key)
            if edge['rule'] == 'phony':
                continue
            definition = definitions.get(edge['rule'])
            require(definition is not None, 'Unresolved producer rule: ' + edge['rule'])
            rule = policy['rules'].get(edge['rule'])
            require(rule and rule['definition'] == definition and rule.get('reviewed_complete_command_inputs') is True,
                    'Unreviewed rule/command inputs: ' + edge['rule'])
            require(rule.get('tool_inputs'), 'Rule tools not pinned: ' + edge['rule'])
            for row in rule['tool_inputs']:
                copies.add(checked(row))
            mode = definition['deps']
            supplement = policy['edges'].get(key)
            if supplement is not None:
                require(supplement.get('reviewed_complete') is True and supplement.get('reason'),
                        'Unreviewed supplemental producer inputs: ' + key)
                for row in supplement['inputs']:
                    path = checked(row); copies.add(path)
                    if path.is_relative_to(out):
                        pending.add(ninja_path(path.relative_to(out).as_posix()))
            if mode:
                require(mode in ('msvc', 'gcc') and rule['dependency_method'] == 'ninja-deps',
                        'Unsupported actual rule dependency method')
                need_deps.add(key)
                dependency_methods[key] = 'ninja-deps-' + mode
            elif definition['has_depfile']:
                require(rule['dependency_method'] == 'retained-depfile' and supplement,
                        'Rule requires retained depfile evidence')
                depfile = checked(supplement['depfile'])
                require(depfile.is_relative_to(out) and depfile.stat().st_size <= 64 * 1024**2,
                        'Retained depfile outside output or exceeds parsing bound')
                copies.add(depfile, supplement['depfile']['sha256'])
                with depfile.open(encoding='utf-8') as stream:
                    records = parse_depfile(stream)
                require(key in records and records[key], 'Retained depfile lacks selected producer dependencies')
                direct = {n for n in edge['explicit_inputs'] if Path(n).suffix.lower() in
                          ('.proto', '.mojom', '.idl', '.cc', '.cpp', '.c', '.rs', '.s', '.asm')}
                require(direct <= set(records[key]), 'Retained depfile omits direct source/schema input')
                dependency_records[key] = records[key]
                dependency_methods[key] = 'retained-depfile'
                for name in records[key]:
                    path = add_file(name)
                    if path.is_relative_to(out):
                        pending.add(ninja_path(path.relative_to(out).as_posix()))
            else:
                # Assembly/action/link rules without deps logs need a reviewed
                # per-output input inventory. No imaginary C++ record required.
                require(rule['dependency_method'] == 'reviewed-explicit-inputs',
                        'Rule requires reviewed explicit input evidence')
                require(supplement and supplement.get('reviewed_complete') is True and supplement.get('reason'),
                        'Unresolved non-deps producer inputs: ' + key)
            for output in edge['outputs']:
                add_file(output)
        for chunk in batch(need_deps):
            raw, record = capture([*prefix, 'deps', *chunk], args, evidence, 'deps-' + str(len(captures)))
            captures.append(record)
            with raw.open(encoding='utf-8') as stream:
                dependencies = parse_deps(stream, set(chunk))
            require(set(chunk) <= dependencies.keys(), 'Missing/stale compiler dependency closure: ' + str(sorted(set(chunk) - dependencies.keys())))
            for key, names in dependencies.items():
                require(names, 'Empty compiler dependency record')
                # The compiler's direct source input must actually occur in its
                # record; counts alone cannot make a wrong record complete.
                edge = producers[key]
                direct = {n for n in edge['explicit_inputs'] if Path(n).suffix.lower() in (
                    '.cc', '.cpp', '.c', '.cxx', '.cu', '.m', '.mm', '.rs', '.cppm', '.ixx', '.modulemap', '.rc', '.s', '.asm', '.h', '.hpp')}
                require(direct or Path(key).suffix.lower() not in ('.obj', '.o', '.pcm', '.pch', '.rlib', '.rmeta', '.res'),
                        'Compiler producer has no applicable direct source')
                require(direct <= {ninja_path(n) for n in names}, 'Compiler dependency record omits direct source')
                dependency_records[key] = names
                for name in names:
                    path = add_file(name)
                    if path.is_relative_to(out):
                        pending.add(ninja_path(path.relative_to(out).as_posix()))
    return {'captures': captures, 'producers': producers, 'selected_objects': sorted(dependency_records),
            'dependencies': dependency_records, 'dependency_methods': dependency_methods,
            'rule_input_policy': binding(policy_path),
            'scope': 'Evaluated target/runtime producer closure and actual applicable compiler records; explicit reviewed non-deps inputs'}


def graph_objects(text):
    return set(re.findall(r'(?m)^build ([^\s:]+\.obj):', text))


def flags(text):
    return dict(re.findall(r'(?m)^(defines|include_dirs|cflags|cflags_cc|rustflags) = (.*)$', text))


def python_runtime():
    runtime = {'version': sys.version, 'executable': binding(sys.executable)}
    if getattr(sys, '_base_executable', None):
        runtime['base_executable'] = binding(sys._base_executable)
    if os.name == 'nt':
        api = ctypes.WinDLL('kernel32', use_last_error=True)
        api.GetModuleFileNameW.argtypes = [ctypes.c_void_p, ctypes.c_wchar_p, ctypes.c_uint]
        api.GetModuleFileNameW.restype = ctypes.c_uint
        buffer = ctypes.create_unicode_buffer(32768)
        size = api.GetModuleFileNameW(ctypes.c_void_p(ctypes.pythonapi._handle), buffer, len(buffer))
        require(size and size < len(buffer), 'Cannot bind loaded Python DLL')
        runtime['python_dll'] = binding(buffer.value)
    return runtime


def intake(copies, args, runner, state):
    provider = load(args.provider_intake)
    require(provider['source_manifest_sha256'] == sha(args.source_manifest), 'Provider intake source identity mismatch')
    checked({'path': provider['current_source_manifest'], 'sha256': provider['source_manifest_sha256']})
    rows = load(args.executable_map)
    require(len({r['module'] for r in rows}) == len(rows), 'Duplicate mapped module')
    selected = next((r for r in rows if r['module'] == args.module), None)
    require(selected is not None, 'Missing actual executable provenance mapping')
    module = runner.MODULES[args.module]
    require(selected['executable'] == args.executable.name == module['targets'][0] + '.exe', 'Executable map mismatch')
    require(selected['timing_arguments'] == module.get('arguments', []), 'Timing arguments differ from registry')
    require(selected['case_count'] == len(set(selected['case_keys'])) > 0, 'Incomplete case map')
    descriptor = load(checked(selected['descriptor']))
    cases = [case['key'] for g in descriptor['groups'] for case in g['cases']]
    require(cases == selected['case_keys'] and descriptor['module'] == args.module, 'Descriptor case/workload mismatch')
    source_intake = descriptor.get('source_intake')
    if source_intake is not None:
        for row in [source_intake['driver'], *source_intake['fixture_sources']]:
            # Active pins only: historical previous_* pins remain preserved context.
            copies.add(under(runner.ROOT, row['path']), row['sha256'])
            copies.add(under(args.build_root, row['path']), row['sha256'])
    required = {p.relative_to(runner.ROOT).as_posix()
                for directory, pattern in module['patterns'] for p in (runner.ROOT / directory).glob(pattern) if p.is_file()}
    required.update(module.get('required_sources', []))
    require(required <= state.keys(), 'Built source manifest omits registry inputs: ' + str(sorted(required - state.keys())))
    objects, configured_flags = set(), {}
    for row in selected['flags_and_link_inputs']:
        require(row['exists'], 'Missing mapped configured flag/link input: ' + row['path'])
        path = under(args.executable.parent, row['path'])
        copies.add(path, row['sha256'])
        text = path.read_text(encoding='utf-8')
        require(flags(text) == row['flags'], 'Mapped flags differ from actual configured graph')
        configured_flags[row['path']] = flags(text)
        objects.update(graph_objects(text))
    # Mapped definitions are context only. evaluated_inputs derives the actual
    # closure, including producers outside every initially mapped graph.
    require(configured_flags, 'Missing mapped configured flags')
    quiche = args.executable.parent / 'obj/net/third_party/quiche/quiche.ninja'
    compiled = set(re.findall(r'(?m)^build [^\n]+\.obj: [^\s]+ (\.\./\.\./[^\s]+)', quiche.read_text(encoding='utf-8')))
    require(compiled == {r['ninja_input'] for r in provider['quiche_compiled_sources']}, 'Missing configured compiled-provider closure')
    for row in provider['quiche_compiled_sources']:
        copies.add(under(args.build_root, row['source']), row['configured_sha256'])
    for key in ('quiche_include_context', 'generated_outputs'):
        require(provider[key], 'Missing provider/generated closure: ' + key)
        for row in provider[key]:
            copies.add(row['path'], row['sha256'])
    for row in [selected['driver'], *selected['fixture_sources'], *selected['uncaptured_http2_shared_policy_sources']]:
        copies.add(row['path'], row['sha256'])
        path = Path(row['path']).resolve()
        if path.is_relative_to(runner.ROOT):
            copies.add(args.build_root / path.relative_to(runner.ROOT), row['sha256'])
    for row in selected['immediate_generated_includes']:
        copies.add(under(args.executable.parent, row['path']), row['sha256'])
    copies.add(checked(selected['descriptor']))
    copies.add(args.provider_intake); copies.add(args.executable_map)
    actions = load(args.provider_action_evidence)
    require(actions['generated_outputs'] and actions['generator_inputs'] and actions['generator_action_lines'],
            'Generated provider action/input provenance missing')
    require({(r['path'], r['sha256']) for r in provider['generated_outputs']} <=
            {(r['path'], r['sha256']) for r in actions['generated_outputs']}, 'Generated provider action/output mismatch')
    for row in [*actions['generator_inputs'], *actions['generated_outputs']]:
        copies.add(row['path'], row['sha256'])
    for row in actions['providers'].values():
        if row['exists']:
            path = under(args.executable.parent, row['path'])
            copies.add(path, row['sha256'])
            configured_flags[row['path']] = flags(path.read_text(encoding='utf-8'))
            objects.update(graph_objects(path.read_text(encoding='utf-8')))
    copies.add(args.provider_action_evidence)
    return selected, objects, configured_flags


SELECTED_OUTPUT_SCOPE = 'Exact selected copied runtime/configuration closure; not the entire mutable output tree'


def ordinary_file(manifest, snapshot, *, original=None, copy=None):
    rows = [row for row in manifest['files'] if
            (original is None or Path(row['original']) == original) and
            (copy is None or row['copy'] == copy)]
    require(len(rows) == 1, 'Missing/ambiguous ordinary copied input')
    row = rows[0]
    path = unlinked(under(snapshot, row['copy']))
    require(path.stat().st_nlink == 1 and sha(path) == row['sha256'], 'Ordinary copied input drift')
    return row, path


def runtime_directories(copies, manifest, paths):
    """Compare complete runtime directories to OLD validation, not this freeze."""
    rows = manifest['directories']
    for path in paths:
        if not path.is_dir():
            continue
        matches = [row for row in rows if Path(row['original']) == path and
                   row.get('scope') != SELECTED_OUTPUT_SCOPE]
        require(len(matches) == 1, 'Complete ordinary runtime directory record required: ' + str(path))
        row = matches[0]
        require(not row.get('scope'), 'Unrecognized ordinary directory scope')
        expected = sorted([*row['files'], *(name + '/' for name in row['subdirectories'])])
        require(len(expected) == len(set(expected)) and members(path) == expected,
                'Ordinary runtime directory membership drift: ' + str(path))
        for name in row['files']:
            file = under(path, name)
            pins = [r for r in manifest['files'] if Path(r['original']) == file]
            require(len(pins) == 1, 'Runtime member lacks ordinary hash: ' + str(file))
            copies.add(file, pins[0]['sha256'])
        # Copies.stable rechecks this exact ordinary membership after copying.
        copies.add(path)
        require(copies.directories[path] == expected and members(path) == expected,
                'Ordinary runtime directory membership drift during collection: ' + str(path))


def workload_result(case, text):
    """The native_delivery.workload_results contract, with unknown methods closed."""
    rows = []
    for line in text.splitlines():
        if line.lstrip().startswith('{'):
            row = json.loads(line)
            require(not any(k.endswith('_ns') or k in {'repeat', 'pair'} for k in row), 'Timing output in verifier')
            rows.append(row)
    contract = case['completion']
    method = contract['method']
    if method == 'exact-completion-message':
        require(contract['message'] and contract['message'] in text, 'Verifier completion message missing')
    elif method == 'json-cases':
        records = [r for r in rows if contract['key'] in r]
        require(sorted(r[contract['key']] for r in records) == sorted(case['case_ids']),
                'Missing/duplicate/extra workload verification')
        if contract.get('verified_required'):
            require(all(r.get('verified') is True for r in records), 'Explicit verified:true required')
    else:
        require(method == 'silent-checked-loop' and contract.get('limitation'), 'Unknown workload completion contract')
    return {'expected_workloads': case['expected_workloads'], 'case_ids': case['case_ids'],
            'completion_contract': contract, 'json_records': rows,
            'measurement_output_absent': True, 'runtime_case_enumeration': method == 'json-cases'}


def validated_workload(args, copies, ordinary, manifest, selected):
    target = args.executable.stem
    snapshot = args.validation_input_manifest.resolve().parent
    digest = sha(args.validation_input_manifest)
    complete = load(snapshot / 'freeze-complete.json')
    require(complete['input_manifest_sha256'] == digest and
            complete['before_after_originals_and_copies_matched'] is True, 'Ordinary freeze incomplete')
    exe_row, executable = ordinary_file(manifest, snapshot, original=args.executable)
    identity = manifest['target_runtime_identity'][target]
    require(exe_row['sha256'] == identity['executable_sha256'] == sha(args.executable),
            'Validated executable differs from timing input')
    require(identity['runtime_deps_sha256'] == sha(args.executable.with_suffix('.runtime_deps')),
            'Validated runtime dependency declaration differs')
    plan_row, plan_path = ordinary_file(manifest, snapshot, copy=manifest['delivery_plan_copy'])
    plan = load(plan_path)
    cases = [c for c in plan['cases'] if c['target'] == target]
    require(len(cases) == 1, 'Selected ordinary delivery case missing/ambiguous')
    case = cases[0]
    require(case['kind'] == 'workload' and not case.get('blockers') and
            case['arguments'] == selected['verification_arguments'] == ['--verify-only'],
            'Mapped verification arguments differ from ordinary plan')
    require(case['case_ids'] == selected['case_keys'] and
            case['expected_workloads'] == selected['case_count'], 'Ordinary plan workload coverage differs')
    require(case['descriptor']['sha256'] == selected['descriptor']['sha256'], 'Ordinary descriptor differs')
    descriptor_row, descriptor_path = ordinary_file(manifest, snapshot, original=Path(case['descriptor']['path']))
    require(descriptor_row['sha256'] == case['descriptor']['sha256'], 'Ordinary plan descriptor pin differs')
    entry_row, entry_path = ordinary_file(manifest, snapshot, original=Path(case['entrypoint']['path']))
    require(entry_row['sha256'] == case['entrypoint']['sha256'] == selected['driver']['sha256'],
            'Ordinary plan driver pin differs')
    records = [r for r in ordinary['records'] if r.get('command') and
               Path(r['command'][0]).stem == target]
    require(len(records) == 1, 'Selected workload verify-only validation missing/ambiguous')
    record = records[0]
    require(record.get('passed') is True and record.get('executed') is True and
            record.get('exit_code') == 0 and not any(record.get(k) for k in
            ('timed_out', 'error', 'identity_error', 'execution_blockers', 'cancelled')) and
            record.get('manifest_sha256') == digest and record.get('kind') == 'workload' and
            record.get('family') == case['family'] and record.get('performance_measurement') is False and
            record.get('input_identity_before') == 'matched frozen manifest and external Python' and
            record.get('input_identity_after') == 'matched before identity', 'Selected ordinary workload provenance mismatch')
    require(Path(record['command'][0]).is_absolute() and Path(record['command'][0]) == executable and
            record['command'][1:] == selected['verification_arguments'], 'Selected ordinary verification command mismatch')
    log = unlinked(args.regular_validation.resolve().parent / target / 'output.log')
    result = workload_result(case, log.read_text(encoding='utf-8', errors='replace'))
    require(record.get('results') == result, 'Ordinary workload completion result differs')
    for path in (snapshot / 'freeze-complete.json', executable, plan_path, descriptor_path, entry_path, log):
        copies.add(path)
    return {'manifest_sha256': digest, 'executable': binding(executable), 'executable_mapping': exe_row,
            'plan': binding(plan_path), 'case': case, 'record': record, 'output_log': binding(log)}


def validate_prior(args, copies, selected):
    target = args.executable.stem
    proof = load(args.freshness_record)
    require(proof['exit_code'] == 0 and proof['all_selected_targets_fresh'] is True and proof['abi_consumers_rebuilt'] is True,
            'Successful configured build/freshness and ABI consumer proof required')
    require(proof['source_manifest_sha256'] == sha(args.source_manifest) and
            proof['configured_args_sha256'] == sha(args.executable.parent / 'args.gn'), 'Freshness source/config mismatch')
    require('-n' in proof['command'] and any(x in (target, 'net:' + target) for x in proof['command']), 'Target absent from freshness proof')
    copies.add(args.freshness_record)
    copies.add(checked(proof['source_manifest']))
    for row in proof['provenance']:
        copies.add(checked(row))
    for path, digest in proof['configuration_files'].items():
        copies.add(path, digest)
    ordinary = load(args.regular_validation)
    ordinary_inputs = load(args.validation_input_manifest)
    require(ordinary.get('passed') is True and ordinary.get('all_selected_executed') is True and
            ordinary['manifest_sha256'] == sha(args.validation_input_manifest), 'Completed ordinary native validation required')
    require(ordinary_inputs['source_manifest_sha256'] == sha(args.source_manifest), 'Ordinary validation source identity mismatch')
    workload = validated_workload(args, copies, ordinary, ordinary_inputs, selected)
    # Bind all prior configured input bytes, not just the executable or source manifest.
    for row in ordinary_inputs['files']:
        path = Path(row['original'])
        if path.is_relative_to(args.build_root):
            copies.add(path, row['sha256'])
    copies.add(args.regular_validation); copies.add(args.validation_input_manifest)
    return proof, ordinary_inputs, workload


RANGE_INPUTS = ('net/http/http_util.cc', 'net/http/http_range_benchmark_adapter.cc',
                'net/http/http_byte_range.cc', 'net/http/http_range_promote_reference.h')


def semantic_guards(module, root, runner):
    if module != 'http-range':
        return {'module': module, 'applicable': False, 'checks': {}}
    return {'module': module, 'applicable': True, 'checks': {
        'adapter_methods': runner.check_range_adapter(root),
        'range_reference': runner.check_range_reference(root)}}


def prepare(args, runner):
    require(args.parent_authorized_timing_prepare, 'Parent authorization after ordinary correctness required')
    args.build_root = unlinked(args.build_root)
    args.executable = unlinked(args.executable)
    out = args.executable.parent
    require(out.is_relative_to(args.build_root / 'out'), 'Configured output executable required')
    root = runner.ROOT.resolve()
    snapshot = args.snapshot.resolve()
    require(not snapshot.exists(), 'Use a new snapshot directory')
    require(not snapshot.is_relative_to(out) and not out.is_relative_to(snapshot), 'Freeze outside mutable output')
    copies = Copies(root, args.build_root)
    state = load(args.source_manifest)
    require(state and all(re.fullmatch('[0-9a-f]{64}', x) for x in state.values()), 'Complete flat source manifest required')
    for relative, digest in state.items():
        copies.add(under(args.build_root, relative), digest)
        copies.add(under(args.built_source_snapshot, relative), digest, Path('editing') / relative)
    copies.add(args.source_manifest)
    selected, _mapped_object_context, configured_flags = intake(copies, args, runner, state)
    proof, ordinary_inputs, workload = validate_prior(args, copies, selected)
    graph = config_closure(out)
    remaining = {line.strip() for line in load(args.provider_action_evidence)['generator_action_lines']}
    for path in graph:
        if path.suffix == '.ninja':
            with path.open(encoding='utf-8') as stream:
                for line in stream:
                    remaining.discard(line.strip())
    require(not remaining, 'Generated provider action differs from evaluated graph')
    for path in graph:
        copies.add(path)
    # Preserve the actual dependency/build databases backing this fresh graph.
    copies.add(out / '.ninja_deps'); copies.add(out / '.ninja_log')
    runtime_list = out / (args.executable.stem + '.runtime_deps')
    copies.add(runtime_list)
    lines = [x.strip() for x in runtime_list.read_text().splitlines() if x.strip()]
    require(lines, 'Empty runtime dependency closure')
    runtime_paths = []
    for line in lines:
        require(not Path(line).is_absolute(), 'Absolute runtime dependency')
        path = unlinked(out / line)
        require(path.is_relative_to(args.build_root), 'Runtime dependency escapes configured tree')
        runtime_paths.append(path)
    runtime_directories(copies, ordinary_inputs, runtime_paths)
    for path in runtime_paths:
        # Every selected runtime file, including additions outside directories,
        # must have belonged to the previously ordinary-validated identity.
        if path.is_file():
            rows = [r for r in ordinary_inputs['files'] if Path(r['original']) == path]
            require(len(rows) == 1, 'Runtime input lacks ordinary validation pin: ' + str(path))
            copies.add(path, rows[0]['sha256'])
    dlls, systems = runner.local_dll_dependencies(args.executable, args.dependency_inspector)
    runtime_identity = ordinary_inputs['target_runtime_identity'][args.executable.stem]
    require(dlls == runtime_identity['adjacent_dlls'] and
            sorted(systems) == sorted(runtime_identity['system_or_nonadjacent_import_names']),
            'Validated DLL/import closure differs')
    require(all(re.fullmatch(r'(?:api-ms-win-[\w-]+|ext-ms-win-[\w-]+|[\w.-]+)\.dll', name, re.I) for name in systems), 'Invalid system import')
    known = load(args.system_dependencies)
    require(set(systems) <= {name.lower() for name in known['known_system_dlls']}, 'Unclassified nonadjacent DLL dependency')
    copies.add(args.system_dependencies)
    for name, digest in dlls.items():
        require(ordinary_inputs['target_runtime_identity'][args.executable.stem]['adjacent_dlls'].get(name) == digest,
                'Validated adjacent DLL identity differs')
        copies.add(out / name, digest)
    copies.add(args.executable)
    runtime = python_runtime()
    for key, row in runtime.items():
        if key != 'version':
            copies.add(checked(row))
    for path in (args.dependency_inspector, args.build_root / 'third_party/ninja/ninja.exe',
                 args.build_root / 'third_party/llvm-build/Release+Asserts/bin/clang-cl.exe',
                 args.build_root / 'third_party/rust-toolchain/bin/rustc.exe', Path(sys.executable),
                 Path(runner.__file__), Path(__file__)):
        copies.add(path)
    # Actual evaluated producer/dependency capture. Raw output is streamed to
    # retained sibling evidence, then independently copied into the snapshot.
    ninja = str(args.build_root / 'third_party/ninja/ninja.exe')
    clean_command = [ninja, '-C', str(out), '-n', args.executable.stem]
    clean = subprocess.run(clean_command, cwd=args.build_root, capture_output=True, text=True, check=False)
    require(clean.returncode == 0 and 'no work to do' in clean.stdout.lower(), 'Original compiled target is not fresh')
    evidence = snapshot.with_name(snapshot.name + '.acquisition')
    evidence.mkdir(parents=True, exist_ok=False)
    closure = evaluated_inputs(args, copies, graph, [args.executable, *(out / name for name in dlls)], evidence)
    for path in evidence.iterdir():
        copies.add(path)
    if args.module == 'http-range':
        require(set(RANGE_INPUTS) <= state.keys(), 'Range guards require built immutable source inputs')
    copies.stable()
    copies.freeze(snapshot)
    # Re-check original graph/target freshness after a potentially large copy.
    clean_after = subprocess.run(clean_command, cwd=args.build_root, capture_output=True, text=True, check=False)
    require(clean_after.returncode == 0 and 'no work to do' in clean_after.stdout.lower(), 'Original freshness changed during freeze')
    copies.stable()
    save(snapshot / 'compiler-dependencies.json', closure)
    # Guard the copied BUILT identity, after independent copying and hash checks.
    # Measurement later consumes only this sealed result and its input bindings.
    guards = semantic_guards(args.module, snapshot / 'configured', runner)
    guards.update(passed=True, source_manifest_sha256=sha(args.source_manifest), source_bindings=[
        {'source': relative, 'copy': 'configured/' + relative, 'sha256': state[relative]}
        for relative in RANGE_INPUTS] if args.module == 'http-range' else [])
    save(snapshot / 'semantic-guards.json', guards)
    save(snapshot / 'freshness.json', {'before': clean.stdout, 'after': clean_after.stdout,
         'command': clean_command, 'before_stderr': clean.stderr, 'after_stderr': clean_after.stderr,
         'source_manifest_sha256': sha(args.source_manifest), 'prior_proof': binding(args.freshness_record)})
    manifest = {'schema_version': 1, 'scope': 'Immutable configured paired benchmark identity only',
        'created_utc': datetime.now(timezone.utc).isoformat(), 'module': args.module,
        'arguments': selected['timing_arguments'], 'case_keys': selected['case_keys'],
        'original_executable': str(args.executable), 'original_build_root': str(args.build_root),
        'executable_copy': copies.mapped(args.executable).as_posix(), 'runtime_cwd': copies.mapped(out).as_posix(),
        'runner_copy': copies.mapped(Path(runner.__file__).resolve()).as_posix(),
        'helper_copy': copies.mapped(Path(__file__).resolve()).as_posix(),
        'source_manifest_copy': copies.mapped(args.source_manifest.resolve()).as_posix(),
        'descriptor_copy': copies.mapped(Path(selected['descriptor']['path']).resolve()).as_posix(),
        'source_manifest_sha256': sha(args.source_manifest), 'configured_flags': configured_flags,
        'local_dll_sha256': dlls, 'system_dll_imports': systems,
        'provider_intake': binding(args.provider_intake), 'executable_map': binding(args.executable_map),
        'provider_action_evidence': binding(args.provider_action_evidence),
        'dependency_inspector': binding(args.dependency_inspector),
        'compiler_version_text': 'Not queried; actual compiler files and configured flags are hash-bound in files.',
        'regular_validation': binding(args.regular_validation), 'freshness_proof': binding(args.freshness_record),
        'validated_workload': workload, 'semantic_guards_copy': 'semantic-guards.json',
        'python_runtime': runtime,
        'files': list(copies.files.values()), 'copied_with_hardlinks': False,
        'source_claim': 'Built frozen identity only; independently edited later sources are not validated',
        'limitations': ['System DLLs remain known named machine dependencies.',
          'Parent Python executable and loaded Python DLL are pinned; the complete external standard-library/package/cache tree is not frozen.',
          'Static adjacent imports and declared runtime data are captured; undeclared dynamic providers require parent review.',
          'Compiler deps identify recorded build inputs, not semantic baseline independence or runtime coverage.',
          'No timing, performance acceptance, publication or whole-network completion is claimed by preparation.']}
    # Exact whole snapshot membership also seals configuration/data directory layout.
    manifest['files'] += [{'original': 'generated:' + name, 'copy': name, 'sha256': sha(snapshot / name)}
                          for name in ('compiler-dependencies.json', 'freshness.json', 'semantic-guards.json')]
    manifest['directories'] = members(snapshot)
    save(snapshot / 'timing-manifest.json', manifest)
    verify(snapshot, sha(snapshot / 'timing-manifest.json'))
    return binding(snapshot / 'timing-manifest.json')


def verify(snapshot, expected):
    path = unlinked(snapshot / 'timing-manifest.json')
    require(sha(path) == expected, 'Frozen manifest drift')
    manifest = load(path)
    require(manifest['schema_version'] == 1 and manifest['copied_with_hardlinks'] is False, 'Unsupported snapshot contract')
    require(members(snapshot) == sorted([*manifest['directories'], 'timing-manifest.json']), 'Frozen membership drift')
    require({name for name in manifest['directories'] if not name.endswith('/')} ==
            {row['copy'] for row in manifest['files']}, 'Frozen snapshot contains unhashed/unmapped input')
    for row in manifest['files']:
        copied = unlinked(under(snapshot, row['copy']))
        require(copied.stat().st_nlink == 1 and sha(copied) == row['sha256'], 'Frozen input drift: ' + row['copy'])
    require(sha(under(snapshot, manifest['source_manifest_copy'])) == manifest['source_manifest_sha256'], 'Frozen source identity mismatch')
    guards = load(under(snapshot, manifest['semantic_guards_copy']))
    require(guards['passed'] is True and guards['module'] == manifest['module'] and
            guards['source_manifest_sha256'] == manifest['source_manifest_sha256'] and
            guards['applicable'] == (manifest['module'] == 'http-range'), 'Missing successful frozen semantic guards')
    if guards['applicable']:
        require({r['source'] for r in guards['source_bindings']} == set(RANGE_INPUTS) and
                len(guards['checks']['adapter_methods']) == 3 and guards['checks']['range_reference'],
                'Incomplete range semantic guards')
        for row in guards['source_bindings']:
            require(sha(under(snapshot, row['copy'])) == row['sha256'], 'Range guard source binding drift')
    return manifest


def execute(args, runner):
    snapshot = args.timing_snapshot.resolve(strict=True)
    manifest_hash = args.snapshot_manifest_sha256
    require(manifest_hash and re.fullmatch('[0-9a-f]{64}', manifest_hash), 'Explicit frozen manifest SHA-256 required')
    manifest = verify(snapshot, manifest_hash)
    require(python_runtime() == manifest['python_runtime'], 'Parent interpreter/loaded Python DLL drift')
    require(Path(runner.__file__).resolve() == under(snapshot, manifest['runner_copy']), 'Launch the copied canonical runner')
    require(Path(__file__).resolve() == under(snapshot, manifest['helper_copy']), 'Launch with copied snapshot helper')
    require(not args.allow_candidate, 'Candidate snapshot mode is not defined')
    timeout = args.timeout if args.timeout is not None else 3600
    require(math.isfinite(timeout) and timeout > 0, 'Finite positive timeout required')
    require(args.module == manifest['module'] and runner.MODULES[args.module].get('arguments', []) == manifest['arguments'], 'Frozen workload/arguments mismatch')
    require(str(args.executable.resolve()) == manifest['original_executable'] and
            str(args.build_root.resolve()) == manifest['original_build_root'], 'Original-to-copy identity mismatch')
    output = args.output_dir.resolve()
    require(not output.is_relative_to(snapshot), 'Write execution evidence outside sealed inputs')
    output.mkdir(parents=True, exist_ok=True)
    paths = {key: output / (args.run + '-' + suffix) for key, suffix in
             [('raw', 'benchmark.jsonl'), ('sources', 'source-hashes.json'), ('record', 'execution.json')]}
    require(not any(p.exists() for p in paths.values()), 'Evidence exists; use a unique run slug')
    # Reserve every evidence name before launch, including failure/cancellation paths.
    for path in paths.values():
        with path.open('xb'):
            pass
    shutil.copyfile(under(snapshot, manifest['source_manifest_copy']), paths['sources'])
    executable = under(snapshot, manifest['executable_copy'])
    cwd = under(snapshot, manifest['runtime_cwd'])
    env = os.environ.copy()
    # Windows system imports resolve only from the sealed runtime and OS directories.
    system_root = Path(os.environ.get('SystemRoot', 'C:/Windows'))
    env['PATH'] = os.pathsep.join(map(str, [cwd, system_root / 'System32', system_root]))
    env['CR_SOURCE_ROOT'] = str(snapshot / 'configured')
    env['TLS_SESSION_CACHE_RUST_CERT_FILE'] = str(snapshot / 'configured/net/data/ssl/certificates/ok_cert.pem')
    temporary = output / (args.run + '-tmp')
    temporary.mkdir(exist_ok=False)
    env['TMP'] = env['TEMP'] = str(temporary)
    command = [str(executable), *manifest['arguments']]
    record = {'exit_code': -1, 'module': args.module, 'command': command, 'cwd': str(cwd),
        'executable': str(executable), 'executable_sha256': sha(executable),
        'cpu': args.cpu, 'configured_flags': manifest['configured_flags'],
        'local_dll_sha256': manifest['local_dll_sha256'], 'system_dll_imports': manifest['system_dll_imports'],
        'snapshot_manifest_sha256': manifest_hash, 'snapshot_manifest': manifest,
        'started_utc': datetime.now(timezone.utc).isoformat(), 'performance_measurement': True,
        'production_status': 'integrated; acceptance validation recorded separately',
        'source_claim': manifest['source_claim'], 'frozen_inputs_before': True,
        'python_runtime': manifest['python_runtime'], 'timeout_seconds': timeout,
        'environment': {'platform': sys.platform, 'logical_cpus': os.cpu_count(),
                        'PATH': env['PATH'], 'CR_SOURCE_ROOT': env['CR_SOURCE_ROOT']},
        'uncontrolled': ['power/thermal state', 'other system processes', 'named system DLL bytes'],
        'publication': 'held; archive every measured case using --archive-only'}
    record['environment'].update(TMP=env['TMP'], TEMP=env['TEMP'], SystemRoot=str(system_root))
    if os.name == 'nt':
        record['windows_command_line'] = subprocess.list2cmdline(command)
    started = time.monotonic()
    previous_handler = signal.getsignal(signal.SIGTERM)
    def cancel(signum, frame):
        raise KeyboardInterrupt('Timing cancelled')
    signal.signal(signal.SIGTERM, cancel)
    try:
        verify(snapshot, manifest_hash)
        with paths['raw'].open('wb') as stream:
            code, errors, affinity = runner.run_benchmark(command, cwd=cwd, env=env,
                stdout=stream, cpu=args.cpu, timeout=timeout)
        record.update(exit_code=code, stderr=errors.decode(errors='replace'), affinity=affinity,
                      affinity_mask=affinity['verified_process_mask'],
                      affinity_mask_is_verified=affinity['verified_process_mask'] is not None)
        require(affinity['verified_before_resume'] is True and affinity['verified_process_mask'] == 1 << args.cpu,
                'Immutable timing requires verified suspended-start affinity')
    except BaseException as error:
        record.update(exit_code=-1, failure=repr(error), cancelled=isinstance(error, KeyboardInterrupt),
                      timed_out=isinstance(error, subprocess.TimeoutExpired))
        if isinstance(error, subprocess.TimeoutExpired):
            record['stderr'] = (error.stderr or b'').decode(errors='replace')
        raise
    finally:
        signal.signal(signal.SIGTERM, previous_handler)
        try:
            verify(snapshot, manifest_hash)
            require(python_runtime() == manifest['python_runtime'], 'Parent Python changed during measurement')
            record['frozen_inputs_after'] = True
        except BaseException as error:
            record.update(exit_code=-1, frozen_inputs_after=False, identity_failure=repr(error))
        record.update(completed_utc=datetime.now(timezone.utc).isoformat(),
                      elapsed_seconds=time.monotonic() - started,
                      raw_report_sha256=sha(paths['raw']), source_hashes_file_sha256=sha(paths['sources']))
        paths['record'].write_text(json.dumps(record, indent=2) + '\n', encoding='utf-8')
    require(record['frozen_inputs_after'], 'Frozen inputs changed; failed execution retained')
    return record['exit_code']


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('action', choices=['prepare'])
    for name in ('module',):
        parser.add_argument('--' + name, required=True)
    for name in ('executable', 'build-root', 'snapshot', 'source-manifest', 'built-source-snapshot',
                 'provider-intake', 'provider-action-evidence', 'executable-map', 'freshness-record', 'regular-validation',
                 'validation-input-manifest', 'dependency-inspector', 'system-dependencies'):
        parser.add_argument('--' + name, type=Path, required=True)
    parser.add_argument('--parent-authorized-timing-prepare', action='store_true')
    parser.add_argument('--external-compile-inputs', type=Path,
                        help='Exact path/SHA-256 map for external SDK/compiler dependency inputs')
    parser.add_argument('--rule-input-policy', type=Path, required=True,
                        help='Parent-reviewed actual rule definitions, command tools and non-deps per-output inputs')
    args = parser.parse_args()
    spec = importlib.util.spec_from_file_location('configured_timing_runner', Path(__file__).with_name('run_rust_rewrite_benchmarks.py'))
    runner = importlib.util.module_from_spec(spec); spec.loader.exec_module(runner)
    require(args.module in runner.MODULES, 'Unknown module')
    print(json.dumps(prepare(args, runner), indent=2))


if __name__ == '__main__':
    main()
