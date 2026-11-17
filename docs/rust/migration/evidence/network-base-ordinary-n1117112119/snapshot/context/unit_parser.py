import json, re, hashlib
import xml.etree.ElementTree as ET
SHA = lambda p: hashlib.sha256(p.read_bytes()).hexdigest()

def unit_results(folder):
    """Read the launcher's real result file, keeping skips and failures visible."""
    summary = folder / 'summary.json'
    if summary.is_file():
        data = json.loads(summary.read_text())
        statuses = [attempt['status']
                    for iteration in data['per_iteration_data']
                    for attempts in iteration.values() for attempt in attempts]
        counts = {status: statuses.count(status) for status in sorted(set(statuses))}
        names = {name for iteration in data['per_iteration_data'] for name in iteration}
        # This launcher's SUCCESS also covers GoogleTest's GTEST_SKIP, and its
        # aggregate XML drops skipped tags. Keep raw statuses, recovering the
        # explicit test names from native output rather than calling them passes.
        log = folder / 'output.log'
        output = re.sub(r'\x1b\[[0-9;]*m', '', log.read_text(errors='replace'))
        skipped_names = set(re.findall(r'(?m)^\[\s*SKIPPED\s*\]\s+(\S+)', output)) & names
        skipped_success = sum(attempt['status'] == 'SUCCESS'
                              for iteration in data['per_iteration_data']
                              for name, attempts in iteration.items() if name in skipped_names
                              for attempt in attempts)
        return {'format': 'Chromium test-launcher summary', 'attempt_status_counts': counts,
                'tests_with_attempts': len(names),
                'listed_tests': len(data.get('all_tests', [])),
                'listed_disabled_tests': len(data.get('disabled_tests', [])),
                'passed': counts.get('SUCCESS', 0) - skipped_success,
                'skipped': counts.get('SKIPPED', 0) + skipped_success,
                'native_skip_names': sorted(skipped_names), 'native_output_sha256': SHA(log),
                'unexpected_statuses': {k: v for k, v in counts.items() if k not in {'SUCCESS', 'SKIPPED'}},
                'result_file_sha256': SHA(summary)}
    # Some standalone GoogleTest mains do not use Chromium's launcher. Its
    # output path can contain XML despite the requested .json extension.
    path = folder / 'gtest.json'
    raw = path.read_text()
    if raw.lstrip().startswith('<'):
        tree = ET.fromstring(raw)
        data = tree.attrib
        observed_skips = sum(test.find('skipped') is not None for test in tree.findall('.//testcase'))
        method = 'GoogleTest XML'
    else:
        data = json.loads(raw)
        # GoogleTest JSON omits the root skipped count in some versions even
        # when individual tests record result=SKIPPED.
        observed_skips = sum(test.get('result') == 'SKIPPED' or bool(test.get('skipped'))
                             for suite in data.get('testsuites', [])
                             for test in suite.get('testsuite', []))
        method = 'GoogleTest JSON'
    counts = {k: int(data.get(k, 0)) for k in ['tests', 'failures', 'errors', 'disabled', 'skipped']}
    counts['skipped'] = max(counts['skipped'], observed_skips)
    return {'format': method, 'counts': counts,
            'passed': counts['tests'] - counts['failures'] - counts['errors'] - counts['disabled'] - counts['skipped'],
            'skipped': counts['skipped'],
            'unexpected_statuses': {k: counts[k] for k in ['failures', 'errors'] if counts[k]},
            'result_file_sha256': SHA(path)}
