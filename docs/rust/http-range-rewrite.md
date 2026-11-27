# Rust HTTP Range header parsers

`HttpUtil::ParseRangeHeader` and `HttpUtil::ParseFetchSingleRange` now use Rust behind their unchanged public APIs. These are two complete header parsers, covering Range lists and Fetch's distinct single-range grammar. `ParseContentRangeHeaderFor206` remains the exact original C++ implementation: its Rust candidate did not pass the performance gate for every measured case.

The benchmark gallery preserves all 35 cases and every intermediate run. The 20 integrated list/Fetch cases passed the final configured-GN measurement gate, with each 95% paired-bootstrap interval showing lower elapsed time. The 15 Content-Range cases remain marked Candidate only and excluded from the aggregate. These are isolated public-header API measurements, not whole-browser performance claims. The empty-list workload exercises a thin C++ empty-input guard, rather than calling the Rust parser.

## Grammar, boundaries, and ownership

`http_range_parser.rs` forbids unsafe code and contains grammar, numeric interpretation, and validation. It reuses the allocation-free safe numeric core in `net/base/parse_number_parser.rs`. The public numeric C++ entrypoints remain C++ because their separately measured migration failed its performance gate.

C++ passes an explicit flat byte pointer and length to the Rust adapter. Zero-length input accepts null data without constructing a Rust slice from null. Shared result records have matching layout assertions; no standard-library private layout is shared. Fetch writes a fixed result only on success and supports uninitialized caller storage.

Lists append each valid prefix even if a later entry fails. A prepopulated caller vector makes a syntactically empty list succeed. Invalid input that never inspects caller storage retains the original null-output behavior. The Rust adapter transfers nonempty `Vec<ParsedRange>` allocations using documented pointer/length/capacity APIs. A C++ scope guard returns the unchanged record once to the Rust destructor. Allocation, append, ownership transfer, and destruction are included in benchmark timing.

List numbers preserve the original leading-plus allowance, signed last-position sentinel, ASCII whitespace, and quoted-comma tokenizer behavior. Fetch retains its case-sensitive grammar, optional delimiter whitespace, saturating overflow, and syntactically valid but semantically invalid ranges such as `bytes=-0` and `bytes=10-5`.

The retained Content-Range candidate demonstrates safe sequential input/output handling through a source trait. Each input borrow ends before a real raw scalar write, preserving scalar aliases and intermediate writes that can restore an overlapping input byte. This candidate is retained for future work; the production Content-Range body is C++.

## Validation on Windows x64, 2026-11-12

The exact Rust candidate source passed 24 native unit tests: 11 original cases plus 13 added differential, boundary, exhaustive-byte, truncation, partial-output, null-output, and alias tests. It also passed 1,000,000 differential libFuzzer executions from fresh seeds. The C++ harness/reference were coverage guided; Rust was not ASan-instrumented.

Instrumented authored Rust core and adapter coverage reached 249/249 executable lines, 331/331 LLVM regions, and 38/38 functions. Those totals include the retained Content-Range candidate. Branch coverage was not collected. The frozen shared numeric core separately passed 13 complete candidate tests with 103/103 executable lines, 126/126 regions, and 16/16 functions covered. Clippy passed with warnings denied.

Native tests compile the retained out-of-line adapter, including the Rust Content-Range candidate. The final public C++ Content-Range body was restored afterward; configured Chromium validation verifies the final production combination separately. Compiler flags, source and binary hashes, raw logs, coverage data, timing boundaries, source snapshots, and exact scope are recorded in [the validation manifest](benchmarks/http-range-validation-2026-11-12.json), [source archive](benchmarks/http-range-final-source-2026-11-12.zip), and [validation evidence](benchmarks/http-range-validation-2026-11-12.zip). The final 35-case measurement is [archived separately](benchmarks/http-range-final-2026-11-12.json).
