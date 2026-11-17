# MEDIA-RANGES candidate

Candidate identity: base `5e3392ef8b62b8e8c50fc3af124753f256ec2791`; current implementation is a safe Rust range-set proposal. This is not production integration or cutover.

## Frozen original closure

`reference/source/` archives the independent implementation (`media/base/ranges.h`, `.cc`), all original tests (`ranges_unittest.cc`), and behavior-bearing `base/time/time.h`, `base/check_op.h`, `base/check.h`. The manifest binds Git blobs and raw bytes. The assertion map has 73 assertion sites in the four original tests plus two separately labeled helper/macro records. Every mapped range assertion traverses `len`, `start` and `end`, matching the native helper's public traversal. Finite fractional cases, both intersection directions, counts and endpoints are included. This is a bounded behavior reference, not a standalone native platform build closure.

## Behavior and bounded policy

`ranges.rs` uses `PartialOrd`/`PartialEq` and follows the native comparison direction and operand order. Bit-level signed-zero tie tests cover `std::min`/`std::max` selection. NaN remains unordered; no total-order or native NaN insertion claim is made. Equal insertion exits before the precondition. Rust debug assertion configuration and panic/fatal behavior are not Chromium DCHECK equivalence. `TimeDeltaMicros::debug_check_lt` is an unconnected diagnostic adapter proposal, explicitly not wired into generic `add`; the eventual native facade must retain the original configured check and signed-value diagnostic. Native sentinels/extrema are preserved as signed microseconds without arithmetic.

The four original test sequences and existing edge test passed in the prior exact candidate. New accessor/clone/empty/back/TimeDelta/signed-zero and Rust-debug precondition tests are pending a fresh director-gated run. Returning `Option` from `back` and references from accessors is a safe Rust candidate API, not proof of the native return and invalid-index contracts; bridge choices and native preconditions remain separate obligations. A manual `Default` preserves empty construction without requiring endpoint types to implement `Default`.

Raw checkout hashes and Git blob identities are distinct. Metadata files are now stored under `-text`, preserving their exact bytes across future checkouts. Earlier evidence remains tied to its recorded raw checkout identity.

## Remaining ownership

Director owns production exports, bridge/shared types, ABI, GN/build integration, caller migration, platform integration and C++ retirement. Observed native instantiations include `int`, `double`, `base::TimeDelta`, and `const uint8_t*` (`SubsampleEntry`); pointer ordering/lifetime bridge semantics and other platform/provider concerns remain native debt. No production behavior, migration acceptance, or subsystem cutover is claimed.
