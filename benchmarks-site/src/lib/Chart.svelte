<script>
  import { untrack, tick, onDestroy } from 'svelte';
  import { engine, facade, defaultOptions, controlsFor, screenChart, archiveFor, evidenceURL, bridgeFor, average, dateFor, metricForRun } from './model.js';

  let { metric, initialRun, eager = false, instance = metric.id } = $props();
  let options = $state(untrack(() => defaultOptions(metric, initialRun)));
  let configOpen = $state(false);
  let active = $state(untrack(() => eager));
  let width = $state(760);
  let card;
  let configButton;
  let busy = $state(false);
  let status = $state('');
  let controller;
  let uid = $derived(`chart-${instance.replace(/[^a-zA-Z0-9-]/g, '-')}`);
  let controls = $derived(controlsFor(metric, options));
  let rows = $derived(metric.overview ? average(options.scope, options.average).rows : engine.selectedRows(metric, facade(options)));
  let bridge = $derived(bridgeFor(metric, options.run));
  let currentMetric = $derived(metricForRun(metric, options.run));
  let date = $derived(metric.overview ? null : dateFor(metric, options.run));
  let svg = $derived(active ? screenChart(metric, options, width) : '');
  let summaryView = $derived(options.view === 'saved' ? 'saved' : 'delta');
  let result = $derived(metric.overview ? average(options.scope, options.average) : null);

  // Integrate the canonical SVG renderer through a Svelte attachment. Its text
  // and attributes are escaped by the renderer; options never contain HTML.
  function drawSvg(element) {
    element.innerHTML = svg;
    return () => element.replaceChildren();
  }

  function measure(element) {
    const resize = new ResizeObserver(([entry]) => { width = Math.max(280, Math.round(entry.contentRect.width)); });
    resize.observe(element);
    const visible = new IntersectionObserver(([entry]) => {
      if (entry.isIntersecting) { active = true; visible.disconnect(); }
    }, { rootMargin: '240px' });
    visible.observe(element);
    return () => { resize.disconnect(); visible.disconnect(); };
  }
  async function toggleConfig() {
    configOpen = !configOpen;
    if (configOpen) { await tick(); card.querySelector('.configuration select')?.focus(); }
    else configButton?.focus();
  }
  function escapeConfig(event) {
    if (event.key === 'Escape' && configOpen && card?.contains(document.activeElement)) {
      configOpen = false;
      configButton?.focus();
    }
  }
  function changeOption(key, value) {
    options[key] = value;
    if (key === 'run') options.input = 'all';
    status = '';
  }
  async function download(format) {
    if (busy) return;
    busy = true;
    status = `Preparing ${format.toUpperCase()}…`;
    controller = new AbortController();
    const started = performance.now();
    try {
      await document.fonts.ready;
      const markup = engine.exportSvg(metric, facade(options));
      const canvas = document.createElement('canvas');
      const size = format === 'mp4' ? 1080 : 1600;
      canvas.width = canvas.height = size;
      const context = canvas.getContext('2d');
      if (!context) throw Error('Canvas export is unavailable in this browser.');
      context.scale(size / 1600, size / 1600);
      const feedback = { set textContent(value) { status = value; } };
      let blob;
      if (format === 'mp4') blob = await engine.generateVideo(markup, canvas, Number(options.duration), feedback, controller.signal);
      else {
        context.drawImage(await engine.svgImage(markup), 0, 0);
        blob = await new Promise((resolve, reject) => canvas.toBlob(value => value ? resolve(value) : reject(Error('PNG export failed.')), 'image/png'));
      }
      if (controller.signal.aborted) return;
      const selected = Object.entries(options).filter(([key]) => key !== 'duration' && (metric.overview ? ['scope','average','legend','details'].includes(key) : !['scope','average'].includes(key))).map(([,value]) => value).filter(Boolean).join('-').replace(/[^a-z0-9-]+/gi, '-');
      engine.saveBlob(blob, `rustium-${metric.id}-${selected}${format === 'mp4' ? `-${options.duration}s` : ''}.${format}`);
      status = format === 'mp4' ? `MP4 downloaded. Generated in ${engine.number((performance.now() - started) / 1000, 1)} s.` : 'PNG downloaded.';
    } catch (error) { status = error.message; }
    finally { busy = false; controller = undefined; }
  }
  onDestroy(() => controller?.abort());
</script>

<svelte:window onkeydown={escapeConfig} />

<article class={['metric', metric.overview && 'overview']} {@attach node => { card = node; return () => { card = undefined; }; }} data-metric={metric.id} data-run={options.run} data-case-count={rows.length} data-speedup={result?.percent}>
  <header class="metric-header">
    <div class="min-w-0">
      <h3>{metric.title}</h3>
      <div class="metric-context flex flex-wrap items-center gap-x-3 gap-y-1">
        {#if metric.overview}<span>Benchmark aggregate · equal case weights</span>
        {:else}<time datetime={date || undefined}>{date || 'Date not recorded'}</time><span>{metric.runs?.find(([id]) => id === options.run)?.[1] || 'Recorded snapshot'}</span>{/if}
      </div>
    </div>
    <button class="config-button" {@attach node => { configButton = node; return () => { configButton = undefined; }; }} onclick={toggleConfig} aria-label={`Configure ${metric.title}`} aria-expanded={configOpen} aria-controls={`${uid}-config`}>
      <svg width="19" height="19" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="1.8" aria-hidden="true"><path d="M4 7h16M4 17h16M8 4v6M16 14v6" /></svg>
      <span>Configure</span>
    </button>
  </header>

  <div class="configuration" id={`${uid}-config`} hidden={!configOpen} role="region" aria-label={`Configuration for ${metric.title}`}>
    <div class="control-grid">
      {#each controls as [key, label, choices] (key)}
        <label for={`${uid}-${key}`}>
          <span>{label}</span>
          <select id={`${uid}-${key}`} data-option={key} value={options[key]} onchange={event => changeOption(key, event.currentTarget.value)} disabled={busy}>
            {#each choices as [value, title] (value)}<option {value}>{title}</option>{/each}
          </select>
        </label>
      {/each}
    </div>
    <div class="config-footer"><p>Exports reflect these settings. 61 s avoids X’s documented short-video loop behavior; player settings still apply.</p><button onclick={toggleConfig}>Done</button></div>
  </div>

  {#if !metric.overview}
    <div class="badges flex flex-wrap gap-2">
      {#if currentMetric.include_in_average === false}<span class="badge candidate">Candidate only · excluded from average</span>{/if}
      {#if bridge}<details class="bridge"><summary class="badge">{bridge.label}<span aria-hidden="true"> ⓘ</span></summary><div><p>{bridge.context}</p><a href={evidenceURL(bridge.archive)}>Recorded scope and provenance</a><p class="source-reference">Source: {bridge.source}</p></div></details>{/if}
    </div>
  {/if}

  <div class="chart" {@attach measure} aria-busy={!active}>
    <p class="sr-only">{metric.overview ? `${engine.signed(result.percent, 2)}% ${result.method.toLowerCase()} benchmark speedup across ${rows.length} cases. Not whole-browser performance.` : `Rust VS C++. ${rows.length} selected cases. Exact median times and confidence intervals are in the expandable table below.`}</p>
    {#if active}<div class="svg-output" {@attach drawSvg}></div>{:else}<div class="chart-placeholder"><span>Rust VS C++</span><p>Chart loads as it approaches the viewport.</p><button onclick={() => active = true}>Load chart</button></div>{/if}
  </div>

  <div class="actions flex flex-wrap items-center justify-between gap-3">
    <span class="export-note">White share media · square safe margins</span>
    <div class="export-buttons flex flex-wrap gap-2">
      <button onclick={() => download('png')} disabled={busy} aria-label={`Download ${metric.title} as PNG`}>Download PNG</button>
      <button onclick={() => download('mp4')} disabled={busy} aria-label={`Download ${metric.title} as MP4`}>Download MP4</button>
      {#if busy}<button onclick={() => controller?.abort()}>Cancel export</button>{/if}
    </div>
  </div>
  <p class="status" role="status" aria-live="polite">{status}</p>

  <details class="exact-values">
    <summary>{metric.overview ? 'Calculation & contributing cases' : 'Exact values, confidence intervals & evidence'}</summary>
    {#if metric.overview}
      <p>{result.method} of unitless C++ / Rust median-time ratios. Equal weight per latest unique case; final runs preferred. Candidate exclusions follow the canonical data. An aggregate confidence interval is not estimated. This is not whole-browser performance.</p>
    {:else}
      <p>{currentMetric.scope}. {currentMetric.baseline || 'C++ is the elapsed-time baseline.'} Original intervals describe Rust time change; speedup and time-saved bounds are transformed in reverse order. An interval overlapping zero means no measurable difference here, not equivalence.</p>
    {/if}
    <!-- svelte-ignore a11y_no_noninteractive_tabindex (Scrollable measurements need keyboard access.) -->
    <div class="table-wrap" tabindex="0" role="region" aria-label={`Exact measurements for ${metric.title}`}>
      <table>
        <thead><tr><th scope="col">Case</th><th scope="col">C++ (ns)</th><th scope="col">Rust (ns)</th><th scope="col">{summaryView === 'saved' ? 'Time saved' : 'Rust speedup'}</th>{#if !metric.overview}<th scope="col">Bootstrap 95% CI</th><th scope="col">Original time-change 95% CI</th>{/if}</tr></thead>
        <tbody>
          {#each rows as row (JSON.stringify([row.metricTitle, row.pack, row.key]))}
            <tr><th scope="row">{metric.overview ? `${row.metricTitle} / ` : ''}{row.pack ? `${row.pack} / ` : ''}{row.label}</th><td>{row.baseline_median_ns}</td><td>{row.rust_median_ns}</td><td>{engine.signed(engine.relativeValue(row, summaryView), 2)}%</td>{#if !metric.overview}<td>{engine.relativeInterval(row, summaryView).map(v => `${engine.signed(v, 2)}%`).join(' to ')}</td><td>{row.bootstrap_95_ci_percent.map(v => `${engine.signed(v, 2)}%`).join(' to ')}</td>{/if}</tr>
          {/each}
        </tbody>
      </table>
    </div>
    {#if !metric.overview}<p><a class="metric-evidence" href={evidenceURL(archiveFor(metric, options.run))}>Raw measurements and provenance for this run</a></p>{/if}
    <p>Speedup = (C++ time / Rust time − 1) × 100. Time saved = (1 − Rust time / C++ time) × 100. Positive values favor Rust. Exact raw medians above retain recorded precision.</p>
    {#if !metric.overview}<details class="record-details"><summary>Full recorded case metadata</summary><pre>{JSON.stringify(rows, null, 2)}</pre></details>{/if}
  </details>
</article>
