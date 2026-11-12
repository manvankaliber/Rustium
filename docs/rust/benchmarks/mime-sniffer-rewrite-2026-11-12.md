# Rust MIME sniffer

The signature-matching, HTML/XML, Office/CRX, binary-classification and MIME-type
policy algorithms from `net/base/mime_sniffer.cc` now live in
[safe Rust](../../../net/base/mime_sniffer_parser.rs). Its public C++ header and
API remain unchanged. C++ classifies the existing GURL schemes for eligibility,
conditionally extracts the first header value, and owns the output `std::string`.
Rust applies MIME-type and `nosniff` rules to that metadata. Rust does not parse
private C++ object layouts or call back into C++ per byte or signature.

The core forbids unsafe code and performs no heap allocation. Normal calls borrow
input bytes. Content/output aliasing borrows a stack scratch slice, kept out of
the normal path: 64 bytes of capacity for short input, or 1024 bytes otherwise.
This avoids moving a 1024-byte array while preserving the same clipped input
length and sniffing thresholds. It models the legacy parser's early output
assignments, which can change subsequent content reads. Hint/output
identity, joint aliases, subviews, and views longer than the sniffing limit are
tested against the frozen C++ implementation.

The private C ABI accepts pointer/length views and returns a small `repr(C)` POD.
The narrow adapter documents its unsafe input-borrow contract; C++ and Rust assert
the POD's size/alignment/offsets and boolean width. Returned MIME pointers refer
only to immutable static Rust strings, and input borrows end before C++ changes
the output. Empty views may be null. C++ guards for empty binary/local inputs
avoid crossing the bridge when there are no bytes to inspect.

The exact original signatures, order, masks, wildcard dots, completion thresholds,
first-header `nosniff` behavior and URL restrictions are preserved. This includes
the original Canon CR2 literal's greedy C++ `\x00C` escape and its actual byte
value, rather than silently changing recognized content. A compile-time table
audit preserves the original constructor checks. No new formats are recognized.
The local and final network signature checks use two compile-time first-byte
indexes (2 KiB of read-only data) to skip impossible signatures. Ascending
candidate bits preserve the original priority, including wildcard and masked
entries; the remaining comparisons use the same matcher.

The [validation record](mime-sniffer-validation-borrowed-scratch-2026-11-12.json) includes source
hashes and archived logs. The [source snapshot](mime-sniffer-sources-borrowed-scratch-2026-11-12.zip)
preserves the final candidate and frozen reference independently of later edits.
The [earlier validation](mime-sniffer-validation-2026-11-12.json) and its
[source snapshot](mime-sniffer-sources-2026-11-12.zip) remain available. The
intermediate [shared-index validation](mime-sniffer-validation-shared-index-2026-11-12.json)
and [snapshot](mime-sniffer-sources-shared-index-2026-11-12.zip) are also preserved. The
current validation also archives the exact initial corpus for its fuzz run.
On Windows x64, 288 focused tests passed: 275 existing tests and 13 additional
differential/ABI families. The exact final source passed one million differential
fuzz inputs, including output aliases. Native reference/harness coverage guided
libFuzzer; Rust was checked differentially. Rust ASan is not claimed.

LLVM reports 302/330 authored lines (91.515%), including unused runtime forms of
three private constructors and the index builder invoked only during constant
evaluation. Excluding those explicitly documented 28 lines, 42 regions and four
functions gives 302/302 runtime lines, 359/359 regions and 37/37 functions. These
are source totals aggregated across generic instantiations; individual
monomorphized paths are not claimed fully covered. The raw report remains
available; no artificial runtime constant-builder calls were added to inflate coverage.
Branch coverage was not collected. Clippy with warnings denied and C++/Rust
formatting checks passed.

Benchmarks include the actual C++ interface, Rust boundary and output assignment.
The final harness measures 30 cases with 20 paired repetitions each: network
sniffing, Office/XML, local media, binary scans, policy and five alias workloads.
Alias cases include identical output reset/capacity setup in both timings.
Readers run in randomized order on one logical CPU with warm buffers; disk,
network, rendering and browser startup are excluded. Full output values and
statuses are compared before timing. Earlier prototypes and their regressions
remain in the gallery. The [final configured GN measurement](mime-sniffer-final-check-2026-11-12.json)
includes every case and its raw paired samples, exact medians and uncertainty.
All 30 cases passed acceptance: 28 were faster with 95% paired-bootstrap
intervals excluding zero; the binary-first-byte and empty-input cases met the
predeclared practical-equivalence bound of ±1 ns per call, with their 90%
intervals contained inside that bound. No case had a significant slowdown.

The geometric mean of the 30 unitless C++/Rust median-time ratios was 2.16554,
or **116.55% speedup**, with equal weight per case. This is an isolated API
benchmark aggregate point estimate, including the actual adaptation, bridge
and output costs. It is not a whole-browser speedup or an average of raw
nanosecond values. Earlier regressions remain available in their original runs.

No whole-browser performance, startup, memory or binary-size improvement is
claimed. Only Windows x64 execution was validated in this work.
