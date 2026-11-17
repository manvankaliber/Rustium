# Network compiler review repairs

This combines the reviewed19-file compiler packet and five-header auth export
packet, preserving the independent review at c3c2e63a. R1 changes the existing
proxy reference label from source_set to one component with its own export
annotation. All four frozen definition bodies remain byte-identical. Component
consumers import that owner; static configurations use the same component label
as a static library. No production proxy implementation supplies the reference.
One additive ordinary case exercises proxy values through SPDY key construction,
copy, comparison and destruction across the DLL boundary. Actual graph, link
response, exports/imports, dependency closure and runtime proof remain pending.

R2 replaces the prospective comparator with always-on strict parsing and complete
source-bound workload contracts. Empty, truncated, duplicate, reordered, malformed
or non-finite records fail, including under optimized Python. Baseline and contract
SHA-256 pins are mandatory; trusted provenance must establish those pins. Both
records must cover all expected verification and timing fields/cases. Durations
may differ but must be finite and nonnegative; checksum, fixture time, sample
order, lifecycle count and seed must match. A store run using a different fixture
time is intentionally not comparable. Capture stderr records before stdout for
this input format. No actual retained verify output has yet been compared.

Both cookie harness output streams explicitly use the classic locale. Existing
cases, seeds, loop counts, checksums, ordering and timing boundaries are preserved.
Seven strict-validator tests pass in both normal and optimized Python; GN format
parses the proposal. None of these checks is a native/runtime/ABI/performance or
subsystem acceptance. Apply only after all source/destination guards and a free
director gate. Preserve the complete36-program232-workload ordinary batch.
