# Rust gzip-header rewrite

`net::GZipHeader` now parses its entire incremental header state machine in
Rust behind the existing C++ API. This includes detection, fragmented fixed
headers, extra fields, names, comments, opaque header CRC bytes, and resets.
Payload decompression and `GzipSourceStream` remain with the existing codec.

The parser core forbids unsafe code, allocates no heap memory and retains no
input references. The C++ object keeps an inline eight-byte state with its
previous size and alignment. Each read crosses the bridge once; single-byte
reads use a scalar adapter to avoid constructing a Rust slice. Large name and
comment searches use Chromium's already-vendored `memchr` dependency. The only
handwritten unsafe declaration is the documented CXX trivial-type mapping,
with matching size, alignment and field-offset assertions on both sides.

Compatibility includes legacy behavior that is intentionally permissive:
reserved flags and FTEXT are ignored, FHCRC is skipped without validation,
and incomplete/invalid reads leave the caller's output offset untouched.
Tests compare internal state as well as public results after every chunk.
The follow-up audit gives Rust an initialized local completion offset, then
commits it only on completion. This supports uninitialized caller storage and
input/output overlap without borrowing uninitialized storage as a Rust reference.

## Validation on Windows x64, 2026-11-12

- Full configured Chromium build: `chrome`, `ui_base_unittests`,
  `gzip_header_unittests`, and `gzip_header_perftests` passed. `gn check` passed.
- 123 gzip tests passed: 19 new differential/edge-case tests plus 104 existing
  gzip-source-stream cases. All 60 DataPack/ResourceBundle tests also passed.
  Test launcher retries were disabled.
- The final parser passed 1,000,000 differential libFuzzer inputs against the
  frozen C++ reader, including state/output comparisons and fragmented reads.
  The native harness/reference were coverage guided; this was not Rust ASan.
- Instrumented Rust coverage: 105/105 executable lines, 129/129 regions and
  7/7 functions across the authored parser and adapter. Branch coverage was
  not collected. Vendored/generated code is outside those coverage totals.
- Clippy with warnings denied, Rust/C++/GN formatting and Python syntax passed.
- The rebuilt browser decoded local plain, gzip, and fragmented optional-header
  HTTP responses correctly. This was a functional smoke test, not startup timing.

The [initial validation record](benchmarks/gzip-header-validation-2026-11-12.json)
contains commands, source hashes, coverage summaries and archived log hashes.
The [combined follow-up validation](benchmarks/parallel-rewrites-integration-2026-11-12.json)
records the current build, 123-test run, local HTTP checks and final benchmark.
Only Windows x64 execution was tested. No browser-wide speed, startup, memory,
or final binary-size improvement is claimed.

## Performance and reproduction

The authoritative [final GN benchmark](benchmarks/gzip-header-final-2026-11-12.json)
contains all 300 paired samples: 15 cases, 20 repetitions each. Both readers
use out-of-line C++ API calls in the same executable, with identical native
flags and equal-size state, pinned to logical CPU 4. Warm input buffers exclude
network/disk I/O and payload decompression. Pair order is randomized; reported
intervals use paired bootstrap resampling.

All 15 final cases took less time with Rust; their 95% time-change intervals
are below zero. Median elapsed-time reductions range from 6.1% to 70.7%,
equivalent to 6.5% to 241.4% speedup. The geometric mean
of the 15 C++/Rust ratios is 1.7867x (+78.7% isolated-operation speedup).

The gallery retains the initial, refined, scalar-adapter and final versions,
including early regressions. Configured GN screening and final runs are labeled
separately from native prototypes; absolute timings should not be compared
across different build paths. The first long-field and one-byte regressions were
resolved before the final result.

From the configured checkout (`D:/RustiumBuild/src` on this machine):

```powershell
third_party/ninja/ninja.exe -C out/Default chrome ui_base_unittests gzip_header_unittests gzip_header_perftests
buildtools/win/gn.exe check out/Default //net:*
out/Default/gzip_header_unittests.exe --test-launcher-retry-limit=0
out/Default/ui_base_unittests.exe --gtest_filter=*DataPack*:*ResourceBundle* --test-launcher-retry-limit=0
```

From the editing checkout, run and import another measurement:

```powershell
python tools/run_rust_rewrite_benchmarks.py --module gzip-header --executable D:/RustiumBuild/src/out/Default/gzip_header_perftests.exe --build-root D:/RustiumBuild/src --output-dir .scratch/gzip-header --run final --cpu 4
python tools/update_gzip_header_charts.py .scratch/gzip-header/final-benchmark.jsonl --run final --label "Final GN build" --source-hashes .scratch/gzip-header/final-source-hashes.json --date 2026-11-12
```

The standalone [benchmark gallery](../../rust-performance.html) recalculates
its overview from the latest unique cases, preferring final runs and excluding
prototypes. Its default is the geometric mean of C++/Rust median-time ratios,
with equal case weights. It offers per-module scopes and arithmetic mean,
and does not invent an aggregate confidence interval.

All charts export square PNGs and H.264 MP4s with safe margins. Videos reveal
the measured rows once and hold the final chart. MP4 export generates frames
directly through browser codecs offline, with cancellation and a small encoding
cache. Only the reveal needs encoding; MP4 sample timing holds the final frame,
so a 61-second video generates in roughly the same time as an 8-second video.
The default 61-second duration avoids X's documented short-video loop rule;
an 8-second option is also available. Playback ultimately belongs to the host
platform. Reduced-motion preferences produce a static video.

## Audit of the preceding rewrite

The DataPack Rust core, bridge and C++ API implementation still match the
previously validated source hashes. Review found no justified performance or
quality improvement: checked offsets, zero-copy views, cached layout and
legacy lookup behavior were retained. They were left unchanged, and their
existing measurements remain in the gallery. Mapping, ownership, iteration
and writing remain C++; the completed new module here is `GZipHeader`.
