# MEDIA-RANGES candidate

Candidate source: `ranges.rs`; base/source identity: `5e3392ef8b62b8e8c50fc3af124753f256ec2791`.

## Frozen source closure

- `media/base/ranges.h` — `Ranges<T>` implementation, templates, `DCHECK_LT`; SHA-256: pending.
- `media/base/ranges.cc` — `TimeDelta` debug check specialization; SHA-256: pending.
- `media/base/ranges_unittest.cc` — all four original tests and exact assertion strings; SHA-256: pending.
- Helper closure: `base/check_op.h` comparison/debug assertion; `base/time/time.h` `TimeDelta` signed-microsecond representation and `ToInternalValue`/`FromInternalValue`; standard vector/pair/algorithm semantics.

## Source-case mapping and executable assertion vectors

`SimpleTests`: start empty; add `(0,1)->1`, `(2,3)->2`, `(1,2)->1` gives `[(0,3)]`; add contained `(1,4)->1`; add `(7,9)` then `(5,6)` gives `[(0,4),(5,6),(7,9)]`; add bridge `(6,7)->2` gives `[(0,4),(5,9)]`. The `(1,2)` bridge proves adjacency coalesces.

`ExtendRange`: `(0,1)+(0.5,1.5)->[(0,1.5)]`; `(0,1)+(-0.5,0.5)->[(-0.5,1)]`; for `[(0,1),(2,3),(4,5)]`, add `(0.5,1.5)` => `[(0,1.5),(2,3),(4,5)]`; add `(1.5,2.5)` => `[(0,1),(1.5,3),(4,5)]`.

`CoalesceRanges`: `[(0,1),(2,3),(4,5)] + (0.5,2.5) => [(0,3),(4,5)]`; plus `(0.5,4.5) => [(0,5)]`; `(0,1)+(1,2) => [(0,2)]`.

`IntersectionWith`: A=`[(0,1),(4,7),(10,12)]`; empty B => empty both directions; B=`[(-1,13)]` => A both directions; B=`[(1,4),(8,9)]` => empty (half-open touching endpoints); B=`[(0,3),(5,11)]` => `[(0,1),(5,7),(10,11)]` both directions; B=`[(0,12)]` => A both directions.

Additional executable Rust assertions cover empty-equal insertion, ordered operation results, containment's exclusive end, and `i64::MIN`/`MAX` endpoints. Reversed insertion is a debug assertion (release behavior remains caller-precondition/unspecified, matching C++ DCHECK); no NaN behavior is selected because the native floating template's comparisons do not define a total-order contract.

## Time and ownership notes

Native `TimeDelta` internal values are signed microseconds; the candidate transparent `i64` newtype round-trips all values including extrema without arithmetic. Native `TimeDelta` arithmetic has special overflow/saturation behavior; this candidate does not implement arithmetic or invent an overflow policy. Floating-point `Ranges<double>` remains unresolved for a generic Rust bridge; template instantiations observed include `int`, `double`, `base::TimeDelta`, and `const uint8_t*` through `SubsampleEntry`.

## Callers and remaining native ownership

Callers include demuxer/pipeline buffered-time APIs, media log serialization, `media/base/subsample_entry.{h,cc}` encrypted byte ranges, and the native unit test. Production exports, ABI/bridge types, GN/build integration, actual caller migration, and C++ retirement remain director-owned. Platform behavior beyond this source's portable templates is unvalidated.

## Validation request

Status: pending. Request native `media/base/ranges_unittest.cc` exact-vector parity plus Rust assertion execution after the active validation gate. No compiler, cargo, test, or benchmark commands were run in this preparation turn.

