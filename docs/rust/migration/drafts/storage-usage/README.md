# STORAGE-USAGE candidate

This directory holds a Rust behavior candidate and byte-frozen references. It
does not replace the production C++ class. `src/lib.rs` specifies the 24-byte
FSU5 Pickle codec, signed usage and unsigned dirty count, checked arithmetic,
validity transitions, private in-memory isolation, and a file-provider seam.
The production adapter still owns sequence checking, native file handles,
two-handle eviction, five-second close timer, and Chromium error translation.

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

Codec tests cover exact header bytes, signed values, truncation, bad length,
bad magic, and invalid boolean. Still required before integration: provider
fake tests for every table row; malformed existing-file behavior; write,
flush, open, delete, reopen, and close-timer failures; dirty `u32::MAX` and
usage overflow; independent incognito instances; and sequence-affinity tests.
No compiler or test command has been run per validation hold.

## Integration request

Director review is required for a shared Rust/C++ bridge contract covering
path/key ownership, `Result`/native error mapping, task sequence affinity,
timer/handle lifecycle, and atomic delta semantics. Keep the Rust codec/state
policy as the behavior owner; native services may implement only the named
provider/sequence/timer adapters. Existing C++ callers in quota backend,
sandbox quota observer, and sandbox backend remain production owners until
the bridge is reviewed, wired, and validated.
