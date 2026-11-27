"""Classify copied Net code and emit reproducible remaining C++ NCLOC weights."""
from collections import Counter, defaultdict
from datetime import datetime, timezone
import hashlib
import json
from pathlib import Path
import re

from code_lines import count_code_lines, code_line_numbers, self_check

HERE = Path(__file__).resolve().parent
SNAP = HERE / 'input-snapshot'
CONFIGURED = SNAP / 'configured'
GN_GRAPH = SNAP / 'editing/.scratch/net-wave4/net-percentage-parent/recursive-net-descriptions.json'
CPP = {'.cc', '.c', '.cpp', '.cxx', '.h', '.hh', '.hpp', '.hxx', '.mm', '.m', '.inc', '.inl'}
IMPLEMENTATION = {'.cc', '.c', '.cpp', '.cxx', '.mm', '.m'}
_STRINGS = re.compile(r'"((?:\\.|[^"\\])*)"')
_COMMENTS_OR_STRINGS = re.compile(r'#[^\n]*|"(?:\\.|[^"\\])*"')
_BLOCK = re.compile(r'(?m)^\s*(\w+)\("([^"\n]+)"\)\s*\{')
_GENERATED = re.compile(r'(?im)^\s*(?://|/\*|\*|#)\s*(?:@generated\b|automatically generated\b|auto-generated\b|this (?:file|header) (?:is|was) generated\b|generated\b|do not (?:edit|modify)(?:\s|!|\.|$))')
_KNOWN_GENERATED = {
    'net/base/net_error_rust_posix_constants.inc',
    'net/base/net_error_rust_win_constants.inc',
}


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def uncomment_gn(text):
    return _COMMENTS_OR_STRINGS.sub(lambda m: '' if m[0].startswith('#') else m[0], text)


def balanced_end(text, start):
    depth = 0
    for token in re.finditer(r'"(?:\\.|[^"\\])*"|[{}]', text[start:]):
        if token[0] == '{':
            depth += 1
        elif token[0] == '}':
            depth -= 1
            if depth == 0:
                return start + token.end()
    return None


def resolve_gn_string(value, directory):
    if '$' in value or '\\' in value:
        return None
    if Path(value).suffix.lower() not in CPP | {'.rs'}:
        return None
    if value.startswith('//'):
        path = value[2:]
    else:
        path = (directory / value).as_posix()
    # Normalize only lexical paths. No access outside copied source is needed.
    parts = []
    for part in path.split('/'):
        if part == '..':
            if not parts:
                return None
            parts.pop()
        elif part not in {'.', ''}:
            parts.append(part)
    result = '/'.join(parts)
    return result if result.startswith('net/') else None


def sources_in(text, directory):
    return {p for m in _STRINGS.finditer(text) if (p := resolve_gn_string(m[1], directory))}


def lexical_memberships(paths):
    production, tests, unresolved_lists = defaultdict(set), defaultdict(set), []
    for path in paths:
        if path.suffix.lower() not in {'.gn', '.gni'}:
            continue
        rel = path.relative_to(CONFIGURED)
        text = uncomment_gn(path.read_text(errors='replace'))
        blocks = []
        for match in _BLOCK.finditer(text):
            open_at = text.index('{', match.start())
            end = balanced_end(text, open_at)
            if end is None:
                continue
            template, name = match[1], match[2]
            block = text[open_at:end]
            source_bearing = template in {'component', 'source_set', 'static_library', 'shared_library', 'executable', 'test', 'fuzzer_test', 'rust_static_library', 'rust_source_set', 'rust_library', 'group'}
            if not source_bearing:
                continue
            is_test = (template in {'test', 'fuzzer_test'}
                       or bool(re.search(r'\btestonly\s*=\s*true\b', block))
                       or any(t in name for t in ('unittest', 'perftest', 'fuzzer', 'test_support', '_reference', '_test_')))
            label = '//' + rel.parent.as_posix() + ':' + name
            destination = tests if is_test else production
            for source in sources_in(block, rel.parent):
                destination[source].add(label)
            blocks.append((match.start(), end))
        # GNI arrays can hold conditional source branches. Classify their names,
        # but do not pretend this is full evaluation of arbitrary GN templates.
        if path.suffix.lower() == '.gni':
            for match in re.finditer(r'(?m)^\s*(\w+)\s*(?:\+?=)\s*\[', text):
                variable = match[1]
                start = text.index('[', match.start())
                end = text.find(']', start)
                if end < 0:
                    continue
                destination = tests if any(t in variable.lower() for t in ('test', 'fuzz', 'bench', 'mock')) else production
                for source in sources_in(text[start:end + 1], rel.parent):
                    destination[source].add(str(rel) + '#' + variable)
    return production, tests


def test_name(rel):
    p = Path(rel)
    parts = set(p.parts)
    name = p.stem.lower()
    return (bool(parts & {'test', 'tests', 'test_tools', 'testing', 'testdata', 'test_data', 'fuzzing', 'benchmarks'})
            or name.startswith(('test_', 'mock_', 'fake_', 'fuzz_'))
            or bool(re.search(r'(?:^|_)(?:test|tests|unittest|unittests|perftest|perftests|fuzz|fuzzer|fuzztest|benchmark|bench)(?:_|$)', name))
            or any(t in name for t in ('test_util', 'test_support', 'test_helpers', 'test_peer')))


def reference_name(rel):
    name = Path(rel).stem.lower()
    return (('_rust_' in name and any(t in name for t in ('original', 'reference')))
            or '_test_reference' in name or '_native_reference' in name
            or name in {'hsts_test_reference', 'parse_number_candidate'})


def classify(rel, text, prod, tests, lex_prod, lex_test):
    parts = set(Path(rel).parts)
    if reference_name(rel):
        return 'frozen_reference_or_candidate', 'Intentional test-only original/candidate naming'
    if parts & {'data', 'docs', 'testdata', 'test_data'}:
        return 'data_or_documentation_code', 'Source-extension file in data/documentation tree'
    code_lines = code_line_numbers(text)
    header_end = min((code_lines[0] - 1) if code_lines else 40, 40)
    if rel in _KNOWN_GENERATED or _GENERATED.search('\n'.join(text.splitlines()[:header_end])):
        return 'generated_code', 'Leading explicit generated marker or pinned generator product declaration'
    if parts & {'test', 'tests', 'test_tools', 'testing', 'fuzzing', 'benchmarks'}:
        return 'tests_or_test_support', 'Explicit test-support directory'
    if parts & {'tools', 'examples', 'example'} or Path(rel).stem.endswith(('_bin', '_main')):
        return 'tools_or_examples', 'Tool/example directory; retained as separate closure work'
    if rel in prod:
        return 'production', 'Evaluated Windows non-test target source/header'
    if rel in lex_prod:
        return 'production', 'Lexical production target/GNI source across platform branches'
    if rel in tests or rel in lex_test or test_name(rel):
        return 'tests_or_test_support', 'Test-only target membership or explicit test/mock/benchmark name'
    return 'production_candidate_unresolved', 'Conservatively included runtime-directory code without confirmed target membership'


def main():
    assert self_check() == 10
    sealed = json.loads((HERE / 'input-sha256.json').read_text())
    graph = json.loads(GN_GRAPH.read_text())
    prod, tests = defaultdict(set), defaultdict(set)
    outside = defaultdict(set)
    for label, item in graph.items():
        destination = tests if item.get('testonly', False) else prod
        for source in item.get('sources', []):
            if source.startswith('//net/'):
                destination[source[2:]].add(label)
        for dep in item.get('deps', []):
            plain = dep.split('(')[0]
            if not plain.startswith('//net'):
                outside[plain].add(label)
    paths = [Path(r['snapshot']) for r in sealed['files']]
    lex_prod, lex_test = lexical_memberships(paths)
    records = []
    for row in sealed['files']:
        p = Path(row['snapshot'])
        if p.suffix.lower() not in CPP | {'.rs'}:
            continue
        assert sha(p) == row['sha256']
        text = p.read_text(errors='replace')
        rel = row['path']
        category, reason = classify(rel, text, prod, tests, lex_prod, lex_test)
        family = 'net-vendor/' + Path(rel).parts[2] if rel.startswith('net/third_party/') else 'first-party/' + Path(rel).parts[1]
        if p.suffix.lower() == '.rs':
            language = 'rust'
            ncloc = None  # Rust never supplies migrated-policy credit by line count.
        else:
            language = 'cpp'
            ncloc = count_code_lines(text)
        records.append({**row, 'language': language, 'kind': 'implementation' if p.suffix.lower() in IMPLEMENTATION else 'header_or_inline', 'family': family,
            'category': category, 'classification_reason': reason, 'remaining_cpp_ncloc': ncloc,
            'evaluated_non_test_targets': sorted(prod.get(rel, [])), 'evaluated_test_targets': sorted(tests.get(rel, [])),
            'lexical_non_test_memberships': sorted(lex_prod.get(rel, [])), 'lexical_test_memberships': sorted(lex_test.get(rel, [])),
            'name_looks_test_related': test_name(rel),
            'production_weight_included': language == 'cpp' and category in {'production', 'production_candidate_unresolved'}})
    groups = defaultdict(lambda: Counter())
    families = defaultdict(lambda: Counter())
    for row in records:
        scope = 'net-vendored' if row['path'].startswith('net/third_party/') else 'first-party-net'
        for key in [scope, 'whole-net-directory']:
            groups[key][row['category'] + '_files'] += 1
            groups[key][row['category'] + '_cpp_ncloc'] += row['remaining_cpp_ncloc'] or 0
            if row['production_weight_included']:
                groups[key]['remaining_cpp_production_ncloc'] += row['remaining_cpp_ncloc']
                groups[key]['remaining_cpp_production_files'] += 1
        if row['production_weight_included']:
            families[row['family']]['remaining_cpp_ncloc'] += row['remaining_cpp_ncloc']
            families[row['family']]['files'] += 1
    (HERE / 'scope-files.json').write_text(json.dumps(records, indent=2) + '\n')
    (HERE / 'outside-net-evaluated-deps.json').write_text(json.dumps({k: sorted(v) for k, v in sorted(outside.items())}, indent=2) + '\n')
    report = {'recorded_utc': datetime.now(timezone.utc).isoformat(),
        'metric': 'Remaining handwritten production C/C++ comment-stripped nonblank physical code lines (NCLOC), including declarations/braces/preprocessor',
        'scope': 'All present configured-tree net platform source branches; Windows evaluated membership plus lexical GN and conservative runtime-directory candidates',
        'not_completion_percentage': True, 'groups': {k: dict(v) for k, v in groups.items()}, 'families': {k: dict(v) for k, v in sorted(families.items())},
        'evaluated_gn_targets': len(graph), 'all_platform_lexical_source_paths': len(lex_prod),
        'source_manifest_sha256': sha(HERE / 'input-sha256.json'), 'recursive_gn_sha256': sha(GN_GRAPH),
        'scope_files_sha256': sha(HERE / 'scope-files.json'), 'lexer_sha256': sha(HERE / 'code_lines.py'),
        'exclusions': 'Tests/support, tools/examples, data/doc code, explicit generated files and preserved test-only original/candidate references are separate records, not silently removed from eventual delivery.',
        'limitations': ['Headers include declarations and interop, not just algorithms.', 'Lexical GN is not a full all-platform evaluation.', 'Unresolved runtime candidates are included conservatively and individually listed.', 'Current C++ bodies plus retired original policy credit form a versioned source-weight metric, not a functionality/performance acceptance percentage.', 'Outside-net providers and caller/tool/test closure remain separate unfinished delivery work.'],
    }
    (HERE / 'scope-counts.json').write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps({'groups': report['groups'], 'evaluated_targets': len(graph), 'records': len(records)}, indent=2))


if __name__ == '__main__':
    main()
