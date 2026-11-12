<script>
  import Chart from '$lib/Chart.svelte';
  import { categories, metrics, history, aggregate, overview, latestCaseCount, manifest } from '$lib/model.js';

  let category = $state('Overview');
  let query = $state('');
  let includeHistory = $state(false);
  let limit = $state(6);
  let filtered = $derived(history.filter(entry => (includeHistory || entry.preferred) && (category === 'Overview' || entry.category === category) && `${entry.metric.title} ${entry.metric.operation} ${entry.metric.scope} ${entry.label} ${entry.metric.rows.map(r => r.label).join(' ')}`.toLowerCase().includes(query.toLowerCase())));
  let visible = $derived(filtered.slice(0, limit));
  const counts = Object.fromEntries(categories.map(name => [name, metrics.filter(m => history.some(e => e.metric.id === m.id && e.category === name)).length]));
  const latestDate = history.find(entry => entry.date)?.date;
  function choose(value) { category = value; limit = 6; }
  function loadMore(element) {
    const observer = new IntersectionObserver(([entry]) => {
      if (entry.isIntersecting && limit < filtered.length) limit += 4;
    }, { rootMargin: '150px' });
    observer.observe(element);
    return () => observer.disconnect();
  }
</script>

<svelte:head>
  <title>Rustium — Rust VS C++ benchmarks</title>
  <meta name="description" content="Recorded Rust rewrite benchmarks, historical measurements, uncertainty intervals and offline share exports. Isolated operations, not whole-browser performance." />
</svelte:head>

<a class="skip-link" href="#main">Skip to benchmarks</a>
<div class="app-shell">
  <aside class="rail">
    <a class="brand" href="#main" onclick={() => choose('Overview')} aria-label="Rustium benchmark overview"><span class="brand-mark" aria-hidden="true"><i></i><i></i><i></i></span><span>Rustium<span class="brand-sub">Benchmark console</span></span></a>
    <nav aria-label="Benchmark subsystems" class="desktop-nav">
      <button class={{ chosen: category === 'Overview' }} aria-current={category === 'Overview' ? 'page' : undefined} onclick={() => choose('Overview')}><span>Overview</span><span>{metrics.length}</span></button>
      <p class="nav-label">Measured subsystems</p>
      {#each categories as name (name)}<button class={{ chosen: category === name }} aria-current={category === name ? 'page' : undefined} onclick={() => choose(name)}><span>{name}</span><span>{counts[name]}</span></button>{/each}
    </nav>
    <label class="mobile-nav"><span>Subsystem</span><select value={category} onchange={event => choose(event.currentTarget.value)}><option>Overview</option>{#each categories as name (name)}<option>{name}</option>{/each}</select></label>
    <div class="rail-foot"><p>Measured operations.<br />Visible tradeoffs.</p><a href="./evidence/rust-performance.html">Standalone offline gallery</a><a href="./evidence/manifest.json">Evidence & source manifest</a></div>
  </aside>

  <main id="main">
    <noscript><p class="no-script">Statistics and exact tables are available below. Enable JavaScript for filtering, chart rendering and exports, or open the <a href="./evidence/rust-performance.html">standalone gallery</a>.</p></noscript>
    <header class="page-header">
      <div class="page-topline"><span>Rust rewrite / performance evidence</span><span>{latestDate ? `Latest recorded date ${latestDate}` : 'Measurement dates not recorded'}</span></div>
      <h1>{category === 'Overview' ? 'Rust rewrite benchmarks' : category}</h1>
      <p class="intro">Rust VS C++ across measured Chromium components. Explore the gains, regressions and uncertainty behind the rewrite.</p>
      <div class="implementation-key" aria-label="Implementation colors"><span><i class="cpp"></i>C++ baseline</span><span><i class="rust"></i>Rust rewrite</span><span class="scope-note">Isolated benchmarks. Whole-browser performance is not measured.</span></div>
    </header>

    <dl class="stats grid grid-cols-2 lg:grid-cols-4">
      <div><dt>Metric groups</dt><dd>{metrics.length}</dd></div>
      <div><dt>Latest unique cases</dt><dd>{latestCaseCount}</dd></div>
      <div><dt>Cases in aggregate</dt><dd>{aggregate.rows.length}</dd></div>
      <div><dt>Archived run reports</dt><dd>{Object.keys(manifest.archives).length}</dd></div>
    </dl>

    {#if category === 'Overview'}
      <section class="overview-section" aria-labelledby="overview-title">
        <div class="section-heading"><h2 id="overview-title">The measured picture</h2><span>Latest eligible cases</span></div>
        <div class="overview-layout">
          <Chart metric={overview} eager instance="overview" />
          <aside class="reading-guide">
            <h3>Read the tradeoffs</h3>
            <p>The geometric mean compares C++ / Rust median-time ratios, with equal weight per case. It favors each metric’s final run and includes eligible regressions.</p>
            <dl class="result-counts"><div><dt>Measurably faster</dt><dd>{aggregate.faster}</dd></div><div><dt>Measurably slower</dt><dd>{aggregate.slower}</dd></div><div><dt>No measurable difference</dt><dd>{aggregate.uncertain}</dd></div></dl>
            <p class="small">Counts use each case’s bootstrap 95% interval. Overlap with zero does not establish equivalence. Candidate-only groups are excluded; previous versions remain available below.</p>
            <details class="methodology"><summary>Measurement & bridge policy</summary><p>Timing boundaries, flags, environments, repetitions and exclusions belong to each archived report. A bridge badge identifies an explicit C++ / Rust boundary in the recorded scope. It does not quantify bridge overhead or explain a slowdown. Unknown boundaries stay unclassified.</p><p>Share images and videos use the established white industrial theme. PNG is 1600 × 1600; H.264 MP4 is 1080 × 1080. Video encodes a reveal once, then holds the final sample; reduced motion uses a static frame. No real-time recording or external encoder.</p></details>
          </aside>
        </div>
      </section>
    {/if}

    <section class="feed-section" aria-labelledby="feed-title">
      <div class="section-heading"><h2 id="feed-title">{category === 'Overview' ? 'Benchmark ledger' : `${category} benchmarks`}</h2><span>Newest recorded date first</span></div>
      <div class="feed-toolbar flex flex-wrap items-end justify-between gap-4">
        <label class="search"><span>Search operations and workloads</span><input type="search" bind:value={query} oninput={() => limit = 6} placeholder="e.g. gzip, alias, range" /></label>
        <label class="history-toggle"><input type="checkbox" bind:checked={includeHistory} onchange={() => limit = 6} /><span>Include earlier runs</span></label>
      </div>
      <p class="feed-count" role="status">{filtered.length} {includeHistory ? 'metric / run entries' : 'metric groups'}{query ? ` matching “${query}”` : ''}. {includeHistory ? 'Same-date ties prefer final runs, then recorded run order and metric ID.' : 'Each group opens on its preferred run; configuration contains its full history.'}</p>
      <div class="feed">
        {#each visible as entry (entry.id)}
          <Chart metric={entry.metric} initialRun={entry.run} instance={entry.id} />
        {:else}<div class="empty-state"><h3>No matching measurements</h3><p>Try another operation, workload or subsystem.</p><button onclick={() => { query = ''; choose('Overview'); }}>Clear filters</button></div>{/each}
      </div>
      {#if limit < filtered.length}<div class="load-more" {@attach loadMore}><button onclick={() => limit += 6}>Load more benchmarks</button><span>{visible.length} of {filtered.length} shown</span></div>{/if}
    </section>
    <footer class="page-footer"><p>Recorded evidence is the source of truth. Charts preserve historical versions and unfavorable results.</p><a href="./evidence/manifest.json">Verify source hashes</a><a href="./evidence/rust-performance.html">Open offline gallery</a></footer>
  </main>
</div>
