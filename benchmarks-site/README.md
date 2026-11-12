# Rustium benchmark console

Native SvelteKit + Tailwind dashboard for the recorded Rust rewrite evidence. Dark screen charts, white square share media, no iframe, no chart library, no network fonts or export service. No deployment or Git commit is performed by the implementation.

## Run and validate

Use Node 22.12+ (validated with Windows Node 26.13.0) and npm. All dependencies live in this directory; exact direct versions and the transitive tree are in `package.json` and `package-lock.json`.

```powershell
cd D:/Rustium/benchmarks-site
npm ci
npm run dev
# Open the URL printed by Vite.

npm run sync          # Refresh from CURRENT ../rust-performance.html and evidence
npm run sync:check    # Read-only; nonzero exit on any stale generated file
npm run check         # Svelte and TypeScript diagnostics
npm run test:data     # Raw samples, medians, transformed intervals and aggregate
npm run build        # Sync, prerender into build/, then check for concurrent changes
npm run test:smoke    # Tests built output with installed Chrome; closes browser/server
npm run preview      # Preview the static production build
```

`test:smoke` uses Playwright's `chrome` channel. Install Google Chrome locally, or explicitly adapt the channel to your available engine. It starts an ephemeral loopback server only for its programmatic tests and closes it on completion. It writes artifacts to `../.scratch/benchmarks-site/`. No Rust benchmarking, production build, source sync to Chromium, deployment, or parent-file mutation occurs.

The static app needs HTTP(S) for module loading and WebCodecs security requirements (localhost works). The linked **Standalone offline gallery** remains a complete HTML file that opens directly from disk. Its copied evidence paths are preserved. PNG generation works without WebCodecs; unsupported MP4 encoding produces a recoverable explanation. Exports require no network calls.

## One source of truth

`scripts/sync.mjs` reads the canonical `benchmark-data` JSON block and the IIFE in `../rust-performance.html`. It copies the numerical renderer, selection math, SVG composition, canvas PNG, WebCodecs AVC encoder, three-entry encoding cache and minimal MP4 muxer into `src/lib/generated/engine.js`. Svelte owns the interface and export lifecycle; the old DOM gallery bootstrap is not executed.

Extraction fails loudly if expected boundaries or functions disappear. It changes only the legacy accepted-run selector to prefer `metric.final_run` when the canonical renderer has not yet adopted that field. No measured values, interval transformations, drawing geometry, codec settings or sample timing are substituted. Generated source, dataset and canonical-page SHA-256 hashes are recorded in `src/lib/generated/manifest.json` and `static/evidence/manifest.json`. `context.json` is a smaller runtime subset so frozen-source hash inventories are not shipped in the JavaScript bundle.

Every file under `../docs/rust/benchmarks/` is copied byte-for-byte, with relative paths preserved under `static/evidence/docs/rust/benchmarks/`. This includes raw reports, provenance, frozen source references and validation context. Each file is hashed in the complete manifest. Canonical archive references are validated to exist; no missing provenance fields are invented or filled from current source. The canonical HTML is copied verbatim to `static/evidence/rust-performance.html`, so its evidence links still work. Recorded source paths inside raw JSON remain historical provenance, not promised links to live filesystem paths.

`predev`, `precheck`, `pretest:data` and `prebuild` refresh automatically. `build` finishes with a read-only staleness check; if the parent imports data during a build, rerun it. `sync:check` also checks copied evidence, not merely the JSON dataset. Generated files and evidence copies are ignored by Git and recreated from the canonical page and archived reports; never edit them manually. Importers require no changes because the prebuild pipeline consumes their canonical output. The root `AGENTS.md` contains the dashboard and current publication policy.

## Data and design mapping

The screen follows the design in `DESIGN.md`: graphite measurement console, industrial system typography, square controls, implementation colors #00599c and #ce422b. Only small colored SVG text is brightened for dark contrast. White export SVGs and videos are untouched by screen recoloring. Structural borders organize navigation and controls; there is no divider below a chart.

Categories derive from actual metric sources. DataPack maps to Resources / DataPack; gzip/compression sources to Compression; HSTS/auth/security to Security; DNS sources to DNS; protocol/number/status utilities to Protocol utilities; HTTP/MIME/header sources to HTTP parsing. Unrecognized sources use their recorded source label. A category appears only when it has data. Category counts mean metric groups, not completed migrations.

The main stats count metric groups, latest unique metric/pack/case keys (including candidates), eligible contributing aggregate cases, and distinct linked archive reports. There are no hardcoded measurement counts or claimed codebase-completion percentages. The feed initially shows each metric's preferred run and can include all historical metric/run entries. Selection priority is `final_run`, then the `final` slug, then the first recorded run. Aggregate selection has the same precedence, respects `include_in_average: false`, and keeps eligible negative results. The default method is the geometric mean of C++/Rust median-time ratios with equal case weights. Arithmetic mean remains selectable. This is an operation-benchmark aggregate, not whole-browser speed.

Feed dates come only from linked archive `timestamp`, `recorded_at`, `date`, or `snapshot_date` fields. Missing dates display **Date not recorded** and sort last. Same-date ties prefer the accepted run, then recorded run order, metric ID and run ID. Filesystem times are never consulted. Earlier versions remain available through each run selector even when earlier-run feed entries are hidden.

Initial statistics, case tables, run labels and evidence links are prerendered. SVG construction is attached client-side and expensive charts are activated near the viewport with reserved space and an explicit Load chart fallback. The overview includes an accessible numerical summary. Feed entries load progressively with an explicit Load more fallback. Desktop has a persistent subsystem rail; mobile replaces it with a labeled native select, keeps 44px controls, and stacks the nonmodal configuration region in normal flow. Tables contain horizontal scrolling without overflowing the document.

## Feature parity

Every metric retains run, workload/input, DataPack pack, horizontal/vertical layout, speedup/time-saved/elapsed-time/time-index comparison, legend, measurement-note and clip-duration controls. A chart-named Configure button uses `aria-expanded` and `aria-controls`, focuses the first control on opening, and restores focus on Escape or Done. The region is nonmodal, so normal Tab order applies. Legends and notes default hidden; the chart subtitle is Rust VS C++.

Collapsed tables expose full-precision C++ and Rust medians, the chosen speedup/time-saved summary, transformed 95% bootstrap intervals and original recorded time-change intervals. Raw case metadata (including any equivalence evidence) is separately expandable. The scope/baseline and raw report stay accessible. Positive speedup means `(C++ / Rust - 1) * 100`; time saved is `(1 - Rust / C++) * 100`. Decreasing interval transformations reverse endpoints. Zero-crossing intervals establish no measurable difference for that experiment, not equivalence.

Single-click PNG and MP4 buttons reflect the complete current selection, exclude controls, preserve exact statistical geometry, and use the established white industrial treatment. PNG is 1600 × 1600, MP4 is H.264 1080 × 1080. The codec generates 70 reveal frames, then MP4 sample duration holds the final chart to 8 or the default 61 seconds. Reduced motion encodes one static sample. Repeat encodings are cached and can be repackaged at a different duration. Cancellation unlocks controls and disposal aborts pending work. No MediaRecorder, external encoder, playback-duration wait or server export is used.

## Bridge policy

Historical `run_context[run]` overrides the metric's current `scope`, `baseline`, `production_status`, `include_in_average` and bridge metadata for that selected run. A historical candidate remains visibly candidate-only even after a later integrated rewrite is published. Selecting an old run does not change the overview: it uses the preferred accepted run of each metric. Pending publication alone never makes an implementation a candidate. Both screen and share charts use this same canonical run context; bridge details expose the recorded kind and evidence without estimating overhead.

Explicit `metric.bridge` takes precedence: `{present, included_in_timing, separately_measured, kind, evidence}`. Presence, inclusion and separate quantification are independent facts. An explicit outside-timing bridge is labeled accordingly; an explicit absent boundary is **No FFI bridge**. Separate-measurement metadata points to evidence without inventing an amount.

For older metrics, each badge classification cites either the linked archive's `metadata.measurement` when it explicitly mentions a bridge/FFI, or that metric's canonical `scope` field. `bridgeFor()` returns the exact quoted scope and source identifier for each badge; the accessible badge details show both. Older resource lookups and explicit bridge scopes are classified as bridge-included; a scope without this evidence (including the legacy DataPack load scope) is **Bridge unclassified**. This is an evidence-backed timing-boundary classification, not an overhead attribution. It never treats a regression as proof that a bridge caused it.

## Validation and limitations

Actual results and artifact paths are in `../.scratch/benchmarks-site/report.json`, `data-validation.json` and `smoke-validation.json`. The data check independently recomputes raw medians, compares every published interval, verifies time transforms, reconstructs aggregate ratios and hashes evidence copies. It also tests explicit `final_run` precedence using actual recorded history and explicit outside-timing bridge metadata.

Browser smoke tests exercise responsive widths 320/390/768/1440/1920, configuration focus/Escape and 44px targets, search/categories/history order, all metric/run/pack tables and evidence links, all comparison/orientation/visibility selectors, overview scopes/methods, PNG, H.264 lengths/cache/cancellation/reduced motion and the unsupported-codec recovery path. Synthesized touch is Chromium emulation, not a physical phone. Artifacts include the actual square PNG and a 720px inspection image.

Physical iPhone/Android/tablet hardware, Safari, Firefox, assistive-technology sessions and upload/playback on X are not available in this environment. Browser/player settings still control looping. The dependency audit reports six low-severity development-tree findings in the pinned SvelteKit 2 toolchain; this static dashboard has no enhanced forms or cookie/session backend. They are recorded rather than silently applying a major-version migration. No benchmarks or Chromium validation claims are made by these UI tests.
