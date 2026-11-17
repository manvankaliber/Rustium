# STORAGE-USAGE candidate

This directory holds a Rust behavior candidate and byte-frozen references. It
does not replace the production C++ class. `src/lib.rs` specifies the 24-byte
FSU5 Pickle codec, dirty/validity/update policy, private records, and a
primitive provider seam. Rust owns the logical two-handle cache and close-timer
restart/expiry/stop decisions; the native facade owns sequence checking and
must call `close_for_destruction` before dropping the cache.

## Original test mapping

| Original test | Candidate coverage / remaining adapter check |
| --- | --- |
| CreateTest | creates a valid zero-usage record |
| SetSizeTest | `240122` round trip |
| SetLargeSizeTest | `i64::MAX` round trip |
| IncAndGetSizeTest | usage `98214`; dirty becomes `1` |
| DecAndGetSizeTest | usage `71839`; decrement at zero is false and unchanged |
| IncDecAndGetSizeTest | usage `198491`; increment/decrement succeeds |
| DecIncAndGetSizeTest | usage `854238`; zero decrement false, next dirty is `1` |
| ManyIncsSameDecsAndGetSizeTest | usage `82412`; 20 increments/decrements preserve usage |
| ManyIncsLessDecsAndGetSizeTest | usage `19319`; dirty remains `1` after 20/19 |
| GetSizeWithoutCacheFileTest | missing usage returns false |
| IncrementDirtyWithoutCacheFileTest | missing usage increment returns false |
| DecrementDirtyWithoutCacheFileTest | source assertion calls `IncrementDirty`; candidate preserves that exact case |

Fake-provider tests use per-handle key mappings and inject open/read/write/
short-I/O/flush/delete errors. They cover both persistent and private original
assertions, unchanged outputs on errors/corruption, dirty wrap, read/write
eviction at capacity, the saved-handle/dirty flush conditions, timer restart,
expiry, deletion, and destruction ordering. The facade sequence token is an
internal candidate seam; native `SEQUENCE_CHECKER` remains authoritative.
Signed usage overflow wraps provisionally pending the configured C++ oracle.
Sources remain unexecuted because the validation gate is occupied.

`reference/SHA256SUMS` now also pins the directly relevant Pickle alignment
reader (`base/containers/span_reader.h`), alignment helper (`base/bits.h`),
and `base::File`/`file_util` common, Windows, and POSIX implementations used
for exact read/write/flush/delete semantics. These are focused helper inputs,
not a recursive Chromium snapshot. All entries are raw `-text` reference
copies; their inspected source revision is `e2f0a5e07ebf97115ab84e90356181c1ded93b49`.

## Integration request

Director review remains required for shared Rust/C++ ABI, path/key ownership,
and native error translation. Existing C++ callers in quota backend,
sandbox quota observer, and sandbox backend remain production owners until
the bridge is reviewed, wired, and validated.
