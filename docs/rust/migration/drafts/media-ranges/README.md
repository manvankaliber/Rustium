# MEDIA-RANGES candidate

Candidate identity: base `5e3392ef8b62b8e8c50fc3af124753f256ec2791`; current implementation is a safe Rust range-set proposal. This is not production integration or cutover.

## Frozen original closure

`reference/source/` archives the complete independent implementation (`media/base/ranges.h`, `.cc`), all original tests (`ranges_unittest.cc`), and behavior-bearing `base/time/time.h`, `base/check_op.h`, `base/check.h`. `reference/source-manifest.json` binds each copy to its source Git blob and raw SHA-256, and records archive hashes/read-only status. `reference/assertion-map.csv` has one row for every one of the 75 original `ASSERT_EQ` / `ASSERT_RANGES` assertions, with exact native source line/text and Rust test case. Originals include every double `.5` case, all endpoints/counts/content, and intersection calls in both directions.

## Behavior and bounded policy

`ranges.rs` uses `PartialOrd`/`PartialEq` and mirrors the C++ comparison operator directions/order; finite `f64` cases follow native comparisons. NaN remains unordered; no total-order policy or NaN compatibility claim is made. Equal insertion exits before the debug precondition. Reversed endpoints trigger debug assertion corresponding to `DCHECK_LT`; release behavior is not elevated beyond the caller precondition. The `TimeDeltaMicros` wrapper stores exact signed internal microseconds and mirrors the specialized debug check's comparison and signed-value diagnostics. No arithmetic/overflow behavior is invented; native sentinels and `i64` extrema are preserved without arithmetic.

The four Rust tests transcribe all native assertion sequences, including clear/reset, exact range content and counts, and both intersection directions. Extra assertions cover empty insertion, half-open contains, and `i64` extrema. Validation status: **pending**; do not run compiler/Cargo/tests until the active validation gate clears. Rust formatting, diff checks, and hashes are permitted.

## Remaining ownership

Director owns production exports, bridge/shared types, ABI, GN/build integration, caller migration, platform integration and C++ retirement. Observed native instantiations include `int`, `double`, `base::TimeDelta`, and `const uint8_t*` (`SubsampleEntry`); pointer ordering/lifetime bridge semantics and other platform/provider concerns remain native debt. No production behavior, migration acceptance, or subsystem cutover is claimed.
