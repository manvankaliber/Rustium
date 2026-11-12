import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';
import { createHash } from 'node:crypto';
import { metrics, history, categories, aggregate, engine, facade, archiveFor, bridgeFor, average, scopes, preferredRun, metricForRun, dateFor, screenChart } from '../src/lib/model.js';
import manifest from '../src/lib/generated/manifest.json' with { type: 'json' };
import { createEngine } from '../src/lib/generated/engine.js';

const site = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '..');
const scratch = path.resolve(site, '../.scratch/benchmarks-site');
const root = path.resolve(site, '..');
const median = values => { const a = [...values].sort((a,b)=>a-b); assert(a.length > 1); return a.length % 2 ? a[a.length >> 1] : (a[a.length/2-1]+a[a.length/2])/2; };
const near = (actual,wanted) => assert(Math.abs(actual-wanted) < 1e-9*Math.max(1,Math.abs(wanted)), `${actual} != ${wanted}`);
const archives = new Map();
let checked = 0;
for (const metric of metrics) {
  for (const row of metric.rows) {
    const file = archiveFor(metric, row.run);
    if (!archives.has(file)) archives.set(file, JSON.parse(fs.readFileSync(path.join(root, file), 'utf8')));
    const a = archives.get(file);
    let raw, cpp, rust;
    if (a.reports) {
      if (metric.source === 'load') {
        raw = a.reports.load.cases.find(r => r.case === row.key);
        cpp = median(raw.samples_ns_per_load.legacy); rust = median(raw.samples_ns_per_load.rust);
      } else {
        raw = a.reports.lookup.cases.find(r => r.pack === row.pack && r.method === row.method && r.pattern === row.pattern);
        cpp = median(raw.samples.map(s=>s.baseline_ns)); rust = median(raw.samples.map(s=>s.rust_ns));
      }
    } else {
      raw = a.cases.find(r=>r.key === row.key);
      const samples = a.samples.filter(s => (s.case ?? s.key) === row.key);
      cpp = median(samples.map(s=>s.baseline_ns)); rust = median(samples.map(s=>s.rust_ns));
    }
    near(row.baseline_median_ns, cpp); near(row.rust_median_ns, rust);
    assert.deepEqual(row.bootstrap_95_ci_percent, raw.bootstrap_95_ci_percent);
    near(row.change_percent, (rust/cpp-1)*100);
    near(engine.relativeValue(row,'delta'), (cpp/rust-1)*100);
    near(engine.relativeValue(row,'saved'), (1-rust/cpp)*100);
    for (const view of ['delta','saved']) {
      const [lo,hi] = row.bootstrap_95_ci_percent;
      const transform = v => view === 'saved' ? -v : -100*v/(100+v);
      assert.deepEqual(engine.relativeInterval(row,view), [transform(hi),transform(lo)]);
    }
    checked++;
  }
  const bridge = bridgeFor(metric, preferredRun(metric));
  assert(bridge.source && bridge.context);
  for (const [run] of metric.runs || [['']]) {
    const context = metricForRun(metric, run);
    assert.deepEqual(context, { ...metric, ...metric.run_context?.[run] });
    const archive = JSON.parse(fs.readFileSync(path.join(root, archiveFor(metric, run)), 'utf8'));
    const dateField = ['timestamp', 'recorded_at', 'date', 'snapshot_date'].find(key => typeof archive[key] === 'string' && /^\d{4}-\d{2}-\d{2}(?:$|T| )/.test(archive[key]));
    assert.equal(dateFor(metric, run), dateField ? archive[dateField] : null);
    const recordedBridge = context.bridge;
    if (recordedBridge) {
      assert(bridgeFor(metric, run).context.includes(recordedBridge.kind || 'kind not recorded'));
      if (manifest.evidence[recordedBridge.evidence]) assert.equal(bridgeFor(metric, run).archive, recordedBridge.evidence);
    }
    // Only screen colors may differ from the canonical renderer, never layout.
    const options = { run, input:'all', pack:metric.source==='lookup'?'resources.pak':'', view:'delta', layout:'vertical', legend:'show', details:'show' };
    const canonical = engine.chart(metric, facade(options), 280, 490);
    const screen = screenChart(metric, options, 280);
    const structure = svg => svg.replace(/#[0-9a-f]{3,8}/gi, '#color');
    assert.equal(structure(screen), structure(canonical));
  }
}
assert.equal(new Set(metrics.map(m=>m.id)).size, metrics.length);
assert.equal(history.length, metrics.reduce((n,m)=>n+(m.runs?.length || 1),0));
const historical = metrics.find(m=>m.runs?.length > 1 && m.include_in_average !== false);
if (historical) {
  const accepted = historical.runs.at(-1)[0];
  const fixture = {...historical, final_run: accepted};
  assert.equal(preferredRun(fixture), accepted);
  const fixtureEngine = createEngine({load:[],lookup:[],additional_metrics:[fixture]});
  const fixtureRows = fixtureEngine.latestCases('all');
  assert(fixtureRows.length > 0 && fixtureRows.every(r=>r.run === accepted), 'Explicit final_run must override the final slug');
}
const bridgeFixture = bridgeFor({...metrics[0],bridge:{present:true,included_in_timing:false,separately_measured:false,kind:'test classification',evidence:'fixture'}},'');
assert.equal(bridgeFixture.label, 'FFI bridge · outside timing');
assert.equal(bridgeFixture.included,false);
for (let i=1;i<history.length;i++) assert((history[i-1].date || '') >= (history[i].date || ''), 'History order');
for (const [scope] of scopes) {
  const eligible = metrics.filter(m=>m.include_in_average !== false && (scope==='all' || (['load','lookup'].includes(m.source)?'datapack':m.source)===scope));
  const unique = new Map();
  for (const m of eligible) for (const r of m.rows.filter(r=>!preferredRun(m) || r.run===preferredRun(m))) unique.set(JSON.stringify([m.id,r.pack||'',r.key]),r);
  const ratios = [...unique.values()].map(r=>r.baseline_median_ns/r.rust_median_ns);
  for (const method of ['geometric','arithmetic']) {
    const expected = method==='geometric' ? Math.exp(ratios.reduce((s,r)=>s+Math.log(r),0)/ratios.length) : ratios.reduce((s,r)=>s+r,0)/ratios.length;
    near(average(scope,method).ratio,expected); assert.equal(average(scope,method).rows.length,unique.size);
  }
}
for (const [relative, hash] of Object.entries(manifest.evidence)) {
  const copy = fs.readFileSync(path.join(site,'static/evidence',relative));
  assert.equal(createHash('sha256').update(copy).digest('hex'),hash);
}
const result = { checked_rows: checked, metric_groups: metrics.length, history_entries: history.length, categories, eligible_cases: aggregate.rows.length, geometric_speedup_percent: aggregate.percent, faster: aggregate.faster, slower: aggregate.slower, uncertain: aggregate.uncertain, evidence_files: Object.keys(manifest.evidence).length, source_hashes: { html: manifest.canonical_sha256, data: manifest.benchmark_data_sha256, engine: manifest.engine_sha256 }, result: 'pass' };
fs.mkdirSync(scratch,{recursive:true}); fs.writeFileSync(path.join(scratch,'data-validation.json'),JSON.stringify(result,null,2));
console.log(JSON.stringify(result,null,2));
