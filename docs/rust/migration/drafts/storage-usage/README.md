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
| CreateTest | `put` creates record; provider-backed execution pending |
| SetSizeTest | encode/decode usage round trip |
| SetLargeSizeTest | `i64::MAX` round trip |
| IncAndGetSizeTest | increment preserves usage and increments dirty |
| DecAndGetSizeTest | decrement at zero returns false and preserves record |
| IncDecAndGetSizeTest | inverse dirty transitions preserve usage |
| DecIncAndGetSizeTest | underflow rejection then increment |
| ManyIncsSameDecsAndGetSizeTest | repeated transition loop required in adapter tests |
| ManyIncsLessDecsAndGetSizeTest | repeated transitions leave dirty=1 |
| GetSizeWithoutCacheFileTest | missing-provider read maps to false/miss |
| IncrementDirtyWithoutCacheFileTest | missing-provider read maps to false/miss |
| DecrementDirtyWithoutCacheFileTest | source test calls IncrementDirty (preserved as source quirk) |

Codec and fake-provider test sources cover Pickle bool/padding behavior,
trailing reads, short reads/writes, failed open/write cleanup, capacity eviction
on a cached-key request, timer stop, and state transitions. Test sources have
not been run. Remaining ordinary coverage includes keyed fake handles, flush
condition matrix/failure, delete/reopen failures, timer restart/expiry and
destruction event sequences, all non-incognito original cases, and facade
first-use sequence binding. Signed usage overflow wrapping remains provisional
until a configured C++ oracle is run.

## Integration request

Director review remains required for shared Rust/C++ ABI, path/key ownership,
and native error translation. Existing C++ callers in quota backend,
sandbox quota observer, and sandbox backend remain production owners until
the bridge is reviewed, wired, and validated.
