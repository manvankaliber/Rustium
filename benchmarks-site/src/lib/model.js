import data from './generated/data.json' with { type: 'json' };
import manifest from './generated/context.json' with { type: 'json' };
import { createEngine } from './generated/engine.js';

export { data, manifest };
export const engine = createEngine(data);
export const metrics = engine.metrics;
export const overview = { id: 'average', title: 'Average benchmark speedup', overview: true };
export const scopeOf = metric => ['load', 'lookup'].includes(metric.source) ? 'datapack' : metric.source;
export const sourceLabel = metric => metric.source_label || ({ datapack: 'DataPack', gzip: 'Gzip headers' }[scopeOf(metric)]) || metric.operation || metric.title;
export function categoryOf(metric) {
  if (typeof metric.category === 'string' && metric.category.trim()) return metric.category.trim();
  const source = scopeOf(metric);
  if (source === 'datapack') return 'Resources / DataPack';
  if (/gzip|compress|brotli|deflate/i.test(source)) return 'Compression';
  if (/hsts|security|auth|certificate|ssl|tls/i.test(source)) return 'Security';
  if (/dns|host.resolv/i.test(source)) return 'DNS';
  if (/protocol|number|port|ip.address|status.code|referrer/i.test(source)) return 'Protocol utilities';
  if (/http|mime|header|content.disposition/i.test(source)) return 'HTTP parsing';
  return metric.source_label || metric.source || 'Other measurements';
}
export const categories = [...new Set(metrics.map(categoryOf))].sort();
export const scopes = [['all', 'All latest cases'], ...new Map(metrics.filter(m => m.include_in_average !== false).map(m => [scopeOf(m), sourceLabel(m)]))];
export const evidenceURL = file => file ? `./evidence/${file}` : './evidence/manifest.json';
export const archiveFor = (metric, run) => metric.archives?.[run] || metric.archive || data.archive;
export const dateFor = (metric, run) => manifest.archives[archiveFor(metric, run)]?.date || null;
export const preferredRun = metric => metric.final_run || metric.runs?.find(([id]) => id === 'final')?.[0] || metric.runs?.[0]?.[0] || '';
export const metricForRun = (metric, run) => ({ ...metric, ...metric.run_context?.[run] });
export const facade = options => ({ querySelector: selector => {
  const option = selector.match(/data-option="([^"]+)"/)?.[1];
  return option && options[option] !== undefined ? { value: options[option] } : null;
} });
export const average = (scope = 'all', method = 'geometric') => engine.averageResult(facade({ scope, average: method }));
export const aggregate = average();
export const latestCaseCount = metrics.reduce((n,m) => n + new Set(m.rows.filter(r => !preferredRun(m) || r.run === preferredRun(m)).map(r => JSON.stringify([r.pack || '', r.key]))).size, 0);
export const history = metrics.flatMap(metric => (metric.runs || [['', 'Recorded snapshot']]).map(([run, label], runIndex) => ({
  id: `${metric.id}:${run}`, metric, run, label, date: dateFor(metric, run),
  category: categoryOf(metric), runIndex,
  preferred: run === preferredRun(metric)
}))).sort((a,b) => (b.date || '').localeCompare(a.date || '') || Number(b.preferred) - Number(a.preferred) || a.runIndex - b.runIndex || a.metric.id.localeCompare(b.metric.id) || a.run.localeCompare(b.run));

export function bridgeFor(metric, run) {
  if (metric.overview) return null;
  metric = metricForRun(metric, run);
  const archive = archiveFor(metric, run);
  if (metric.bridge) {
    const bridge = metric.bridge;
    return {
      included: bridge.present === true && bridge.included_in_timing === true,
      label: bridge.present === false ? 'No FFI bridge' : bridge.present !== true ? 'Bridge unclassified' : bridge.included_in_timing === true ? 'FFI bridge' : bridge.included_in_timing === false ? 'FFI bridge · outside timing' : 'FFI bridge · timing unclassified',
      context: `Recorded bridge metadata: ${bridge.kind || 'kind not recorded'}. Presence: ${String(bridge.present ?? 'not recorded')}. Included in timing: ${String(bridge.included_in_timing ?? 'not recorded')}. ${bridge.separately_measured === true ? 'A separate measurement is identified by the linked evidence; no overhead amount is inferred here.' : 'Bridge overhead is not separately quantified. Presence does not establish the cause of a speedup or slowdown.'}`,
      source: bridge.evidence || `rust-performance.html#benchmark-data / ${metric.id}.bridge`,
      archive: manifest.evidence[bridge.evidence] ? bridge.evidence : archive
    };
  }
  const recorded = manifest.archives[archive]?.measurement;
  const scope = recorded && /bridge|\bffi\b/i.test(recorded) ? recorded : metric.scope;
  const included = /bridge|\bffi\b/i.test(scope || '');
  return {
    included,
    label: included ? 'FFI bridge' : 'Bridge unclassified',
    context: included
      ? `A C++ / Rust boundary is included in the recorded operation scope: “${scope}”. This is not a separately quantified bridge-overhead measurement; it does not establish the cause of a speedup or slowdown.`
      : 'The available metric scope does not explicitly establish a timed C++ / Rust boundary. Bridge presence and overhead are unclassified.',
    source: recorded && /bridge|\bffi\b/i.test(recorded) ? `${archive}#metadata.measurement` : `rust-performance.html#benchmark-data / ${metric.id}.scope`,
    archive
  };
}
export function defaultOptions(metric, run) {
  return { run: run ?? preferredRun(metric), input: 'all', pack: metric.source === 'lookup' ? 'resources.pak' : '', view: 'delta', layout: 'horizontal', legend: 'hide', details: 'hide', duration: '61', scope: 'all', average: 'geometric' };
}
export function controlsFor(metric, options) {
  const common = [
    ['legend', 'Legend', [['hide', 'Hidden'], ['show', 'Visible']]],
    ['details', 'Measurement notes', [['hide', 'Hidden'], ['show', 'Visible']]],
    ['duration', 'MP4 length', [['61', '61 seconds'], ['8', '8 seconds']]]
  ];
  if (metric.overview) return [['scope', 'Benchmark scope', scopes], ['average', 'Average method', [['geometric', 'Geometric mean'], ['arithmetic', 'Arithmetic mean']]], ...common];
  const rows = metric.rows.filter(r => !options.run || r.run === options.run);
  return [
    ...(metric.runs ? [['run', 'Benchmark run', metric.runs]] : []),
    ...(metric.source === 'lookup' ? [['pack', 'Resource pack', [...new Set(metric.rows.map(r => r.pack))].map(p => [p,p])]] : []),
    ['input', 'Input / workload', [['all', 'All measured cases'], ...new Map(rows.map(r => [r.key,r.label]))]],
    ['view', 'Comparison', [['delta', 'Rust speedup (%)'], ['saved', 'Time saved (%)'], ['time', 'Elapsed time (ns)'], ['ratio', 'Elapsed time (C++ = 100)']]],
    ['layout', 'Layout', [['horizontal', 'Horizontal bars'], ['vertical', 'Vertical bars']]], ...common
  ];
}
export function screenChart(metric, options, width) {
  // The canonical renderer owns wrapping and reserves space for full notes,
  // headings and workload labels. Keep screen and export geometry in sync.
  const markup = engine.chart(metric, facade(options), width, width < 450 ? 490 : 445);
  return markup.replaceAll('#fff', '#20252b').replaceAll('#1f1f1f', '#edf0f2').replaceAll('#707070', '#b3bdc8').replaceAll('#e4e4e4', '#48525d')
    // Keep the bars' exact identity colors; brighten small colored text on dark.
    .replace(/(<text\b[^>]*fill=)"#00599c"/g, '$1"#80bde8"').replace(/(<text\b[^>]*fill=)"#ce422b"/g, '$1"#ff9d89"');
}
