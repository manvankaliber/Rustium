# Chrome Tracing Tools

These tools symbolize Perfetto traces in proto format. Use
`tools/tracing/symbolize_trace --help` for the available options.

## Collecting desktop traces

On Linux and macOS, collect profiles with [Perfetto UI](https://ui.perfetto.dev):

* Enable `memory-infra` (high overhead) in Chrome categories for heap profiles.
  Samples are collected every ten seconds, so record for longer than ten seconds.
* Enable `cpu_profiler` (high overhead) in Chrome categories for CPU profiles.
* Download the resulting proto trace for symbolization.

Windows traces can also be collected through Perfetto UI. This symbolization
script currently supports Linux and macOS hosts.

## Symbolizing local builds

Build Chrome with `symbol_level=1` or `symbol_level=2`. Level 2 includes more
function names and takes longer to build. Build `dump_syms` in the same output
directory:

```sh
ninja -C out/Release dump_syms
tools/tracing/symbolize_trace trace.pb --local_build_dir=out/Release
```

Use `--dump_syms=PATH` if the tool cannot locate `dump_syms` automatically. Add
`--view` to open the symbolized trace in Perfetto UI.

## Symbolizing official builds

For Linux and macOS traces, the tool can fetch official symbols:

```sh
tools/tracing/symbolize_trace trace.pb
```

## Reusing symbols

Store generated Breakpad symbols with `--breakpad_output_dir`, then reuse that
directory with `--local_breakpad_dir` for other traces from the same build:

```sh
tools/tracing/symbolize_trace trace1.pb --breakpad_output_dir=/tmp/chrome-symbols
tools/tracing/symbolize_trace trace2.pb --local_breakpad_dir=/tmp/chrome-symbols
```
