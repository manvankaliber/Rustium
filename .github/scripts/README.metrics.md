# Codebase metrics

Run with Python 3.10+ and Git (no pip dependencies):

```sh
python3 .github/scripts/codebase_metrics_test.py
python3 .github/scripts/codebase_metrics.py /tmp/dashboard --cache /tmp/metrics.sqlite3
```

Without an output argument the dashboard is written at the repository root.
Without `--cache`, SQLite data lives at `git rev-parse --git-path codebase-metrics.sqlite3`.
Use `--revision <commit>` to measure a different committed snapshot. Local edits
and untracked files never affect the counts. The dashboard template is read from
the working tree so template edits can be tested before committing.

## Reading and caching

A single `git ls-tree -rz` lists regular committed files. Symlinks and submodules
are omitted. Path exclusions run before any content reads. Selected cache misses
are streamed through one persistent `git cat-file --batch` process, with a writer
thread to avoid pipe deadlock. Only requested blobs are read, never all Git objects
or history. Memory for blob data is bounded to one file at a time.

SQLite entries use the blob hash and language. Duplicate contents share analysis;
renames reuse results, while their path category and example paths are rebuilt.
A hash of the complete scanner invalidates the cache when rules change. Deletions
are naturally absent from the next tree. Interrupted scans keep completed batches
of valid cache entries. GitHub Actions restores the latest compatible cache and
saves a new immutable cache after a successful scan. Cache misses are safe and
require a cold scan; cached runs still enumerate and aggregate the current tree.

## What is counted

The headline and language shares cover **maintained production source**. All
source counters are also emitted under `categories.maintained`, `categories.tests`
and `categories.vendored`. Test and vendored LOC are shown separately on the page.
These labels indicate path scope, not authorship: inherited Chromium code outside
vendor directories remains part of maintained production source.

- Vendored: any `third_party` or `vendor` directory.
- Tests: `test`, `tests`, or `testing` directories; common `test_`, `_test`,
  `_unittest`, `_browsertest`, `.test`, `.spec` filenames; `Test`/`Tests` stems.
  Vendor classification takes priority for tests shipped inside dependencies.
- Generated: `gen`, `generated`, `node_modules`, `__pycache__` directories;
  `.min.js`, `.min.css`, `.bundle.js`, `.bundle.css`, `.pb.cc`, `.pb.h`, `.pb.c`,
  `.pb.go`, `_pb2.py`, `.generated.cc`, `.generated.h`, `.generated.ts` suffixes;
  generated-file comment notices in the first 4 KiB. This is heuristic, not an
  exhaustive generated-file detector. Ordinary string literals do not qualify.
- Fixtures: `testdata`, `test_data`, `fixtures`, `corpus`, `corpora` directories,
  or `data` directly under `test`, `tests`, or `testing`.
- Docs, configuration, lockfiles, binaries, dashboard outputs and unsupported
  extensions are excluded from source totals. Known configuration paths are
  counted without reading their contents; root folders come from tree metadata.

`build/` and `tools/` are retained unless another rule applies. The exact language,
configuration, binary and filename lists live in `codebase_metrics.py`. A tracked
file is retained even if it matches a local `.gitignore`: committed snapshots
must give reproducible results across machines. `.git` contents are never source
inputs. The targeted Git object reads use repository storage internally.

`excluded_files` gives counts by exclusion reason. `scan` reports cache hits,
unique blobs read and elapsed time. Comment stripping, function detection and
path-based categories are heuristics. LOC describes the selected snapshot, not
recent lines added, ownership, or code quality.

## Daily history

Every successful scan also updates `metrics-history.json` in the output directory.
The workflow commits it with the latest snapshot and runs on pushes plus daily at
00:15 UTC. Repeated runs replace the same UTC date with the latest measured
snapshot. Stale retries cannot replace a newer snapshot for that day. A day with
no successful run stays missing; no values are invented or carried forward.

The history has `schema_version`, `timezone` and a date-sorted `records` array.
Each record retains the measured commit, timestamp, counting-rule version,
production/test/vendored counters, source bytes, language LOC, configuration and
root counts, and exclusion counts. Its `delta` mirrors those numeric fields and
compares to `previous_date`; `days_since_previous` makes gaps explicit. Deltas
are net differences in snapshot totals, not Git additions/deletions.

The first record and the first record after a counting-rule change have null
deltas. Bump `COUNTING_RULES_VERSION` in `metrics_history.py` whenever counting
semantics change; keep it unchanged for UI, caching or history-format changes.
`HISTORY_SCHEMA_VERSION` versions the file structure independently. Out-of-order
daily updates recalculate subsequent deltas. Invalid existing history fails the
run instead of discarding history, and writes replace the file atomically.
