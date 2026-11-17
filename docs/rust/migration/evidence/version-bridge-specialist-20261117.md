The smallest coherent milestone is **Rust policy ownership behind a typed CXX bridge, with the existing C++ `Version` object and storage retained**. It requires no caller API changes and no changes to `base/version.h`. It is not a full class or subsystem cutover.

I read the complete production C++, accepted Rust source, frozen helper implementations, original tests, bridge patterns and key consumer boundaries. No files or queue state were changed; no compilers, tests, builds, benchmarks or subagents were started. No native pass is claimed.

**Proposed implementation**

Keep these C++ responsibilities exactly where they are:

- The sole `std::vector<uint32_t> components_` member, native vector constructor and inline `components()` const reference.
- Default/copy/move constructors, destructor and inline assignments, including their current declarations and exception specifications.
- `IsValid()` as the empty-vector check.
- Existing exported member/free-function symbols, relational wrappers and stream insertion.

These contracts are visible in [version.h](/D:/Rustium/base/version.h:20) and [version.cc](/D:/Rustium/base/version.cc:89). Do not introduce an opaque Rust handle, cache, second component owner, allocator conversion or Rust object inside `Version`.

Add director-owned `base/version_rust.rs` and `base/version_rust_policy.rs`, with this bridge:

```rust
#[cxx::bridge(namespace = "base::internal::rust_version")]
mod ffi {
    extern "Rust" {
        fn parse(input: &[u8]) -> Vec<u32>;
        fn compare(lhs: &[u32], rhs: &[u32]) -> i32;
        fn is_valid_wildcard(input: &[u8]) -> bool;
        fn compare_wildcard(lhs: &[u32], input: &[u8]) -> i32;
        fn serialize(components: &[u32]) -> String;
    }
}
```

Operation contracts:

| Operation | Required behavior |
|---|---|
| `parse` | Return nonempty components on success; empty on invalid input. Discard all partial results before returning invalid. Preserve arbitrary bytes, embedded NUL rejection, `u32` bounds and first-component leading-zero rules. |
| `compare` | Borrow both native vectors directly. Return exactly `-1`, `0`, `1`; pad absent trailing components with zeros. Define this helper for empty slices too, preserving existing release behavior. No allocation or component copying. |
| `is_valid_wildcard` | Strip only a terminal `.*`; validate the remaining bytes using the same Rust parser. |
| `compare_wildcard` | Borrow the native LHS; implement existing wildcard policy, including the release compatibility behavior described below. Never construct an owned Rust `Version` from an LHS copy. |
| `serialize` | Empty slice becomes `"invalid"`; otherwise decimal components joined by dots, preserving the number of trailing zero components. |

Convert `std::string_view` to a byte slice, never `rust::Str`. For component slices use `rust::Slice<const uint32_t>(components_.data(), components_.size())`. CXX handles null empty slices; [span_rust.h](/D:/Rustium/base/containers/span_rust.h:25) documents the zero-length pointer convention.

The string constructor should receive `rust::Vec<uint32_t>`, copy its elements into native storage only when nonempty, then destroy the Rust vector through CXX. `GetString()` should receive `rust::String` and construct its native return string using explicit data and length.

**Required Rust refactor and invalid-call compatibility**

Do not bridge the accepted owned `Version` API directly. Its `Option<Vec<u32>>` representation and `Option<Ordering>` results differ from the native contract. The accepted implementation is at [lib.rs](/D:/Rustium/docs/rust/migration/drafts/foundation-version/rust/src/lib.rs:5).

Extract shared, stateless policy functions into the production Rust policy module. Both owned candidate adapters and bridge adapters should use the same algorithms. Preserve the accepted artifact unchanged as evidence; review and validate the production derivative as a successor.

The parser needs an internal outcome equivalent to:

```rust
struct ParseOutcome {
    valid: bool,
    prefix: Vec<u32>, // completed components before the first failing component
}
```

This is private Rust state, not an ABI type. Ordinary construction maps an invalid outcome to an empty vector. Wildcard release compatibility may use its retained prefix.

The distinction matters:

- Native `CompareTo` DCHECKs both operands, then compares their vectors even when DCHECKs are disabled. Empty versus `{0}` compares equal; empty versus `{1}` compares less. Returning `None`, a sentinel or unconditional equality would change release behavior. See [version.cc](/D:/Rustium/base/version.cc:169).
- Wildcard comparison without terminal `.*` constructs a normal `Version`; invalid parsing therefore produces an empty RHS.
- With terminal `.*`, native code retains completed prefix components even when parsing fails. For example, source inspection implies `"1.3"` versus `"1.2.bad.*"` returns `1` in release, whereas `"1.2.3"` versus that same invalid wildcard returns `0`. This follows from the partial parser and wildcard prefix loop at [version.cc](/D:/Rustium/base/version.cc:28) and [version.cc](/D:/Rustium/base/version.cc:125).

Retain the public C++ entry DCHECKs and their order: LHS validity, then wildcard validity. Let Rust provide the release comparison result. Do not substitute `unwrap()`, Rust assertions, `CHECK`, exceptions or an invalid-result fallback. These compatibility results remain **invalid-contract debt**, not newly supported public usage.

The frozen numeric helper confirms that whitespace makes conversion invalid, unsigned negative input fails, and Version separately rejects leading `+`. See [string_number_conversions_internal.h](/D:/Rustium/docs/rust/migration/drafts/foundation-version/reference-cpp/base/strings/string_number_conversions_internal.h:161). No definite valid-input policy mismatch was found by inspection.

**Minimal C++ algorithm retirement**

In [version.cc](/D:/Rustium/base/version.cc:28), remove the native parser and component comparator. Replace the policy bodies of string construction, wildcard validation/comparison, `CompareTo` and `GetString` with bridge calls and native storage/result adaptation. Remove the now-unused number-conversion, split and string-util includes.

Keep all current externally visible definitions. Relational operators should continue to call `CompareTo`; stream insertion should continue to call `GetString`. This retires actual C++ policy while leaving native representation and value mechanics explicit.

For GN, follow the existing typed patterns in [BUILD.gn](/D:/Rustium/base/BUILD.gn:2120) and [string_split_rust.rs](/D:/Rustium/base/strings/string_split_rust.rs:9):

- Add `rust_static_library("version_rust")`, its policy source and `cxx_bindings`.
- Add a private dependency from `:base`; permit test access only if needed.
- Depend on `//third_party/rust/cxx/v1:lib`.
- Scope `allow_unsafe = true` to generated CXX machinery; authored policy/glue needs no unsafe code.
- Keep generated bridge headers private to implementation files.

Do this only after the active Strings build is terminal and the director’s gate is available.

**Ownership, allocation and lifetime contract**

All input borrows end synchronously before the bridge returns. No slice, native pointer or reference is retained. No callbacks, tasks, TLS, shared mutable state or thread-affinity requirement are needed. Concurrent const operations remain safe under the existing requirement that callers do not mutate the same native vector concurrently.

Rust owns and frees temporary `Vec`/`String` allocations; C++ owns and frees its vector/string allocations. Never adopt Rust capacity into `std::vector`, even if an evaluated allocator happens to share an underlying provider.

Malformed input is a normal invalid result. OOM is not malformed input and must not become an empty vector. Preserve the configured fatal allocation policy and verify the evaluated allocator/panic configuration. The repository has both C++ allocator and `System` paths plus an allocation error handler at [allocator/lib.rs](/D:/Rustium/build/rust/allocator/lib.rs:37); source presence does not establish which configuration a new target uses.

The minimal bridge adds a temporary Rust allocation and native copy for parsing and serialization. Accepted serialization also allocates per-component strings and a temporary vector. These are performance debts. Direct comparisons must remain allocation-free; wildcard comparison may allocate only its parsed RHS. No performance acceptance follows from this design.

**Caller and consumer findings**

- [windows_version.cc](/D:/Rustium/base/win/windows_version.cc:258): `Kernel32Version()` and `Kernel32VersionNumber()` index native components. `Kernel32BaseVersion()` caches a native `Version`, returns it by value, allows blocking around native resource reads, falls back from `kernel32.dll` to `kernelbase.dll`, then CHECKs success. Preserve this provider and lifecycle. `base::win::Version` is a separate OS enum.
- [file_version_info_win.cc](/D:/Rustium/base/file_version_info_win.cc:216): `GetFileVersion()` constructs four numeric components from `VS_FIXEDFILEINFO`. This path should retain its native vector construction; Rust comparison/serialization subsequently consumes borrowed slices. Resource ownership, `raw_ref`, module/file lifetimes, language/codepage fallback and Win32 errors remain native.
- [version_info.cc](/D:/Rustium/base/version_info/version_info.cc:20): `GetVersion()` returns a process-lifetime const reference through `NoDestructor`; major-version access directly indexes component zero. Preserve both lifetime and generated version provider. Its GN generation inputs are listed in [version_info/BUILD.gn](/D:/Rustium/base/version_info/BUILD.gn:44).
- [version_mojom_traits.h](/D:/Rustium/mojo/public/cpp/base/version_mojom_traits.h:19) exports the native vector by const reference. [Deserialization](/D:/Rustium/mojo/public/cpp/base/version_mojom_traits.cc:12) accepts an empty vector and constructs an invalid `Version`. Preserve the existing `array<uint32>` schema and successful invalid-version round trip.
- Variations validates wildcard filter strings in [processed_study.cc](/D:/Rustium/components/variations/processed_study.cc:39), then compares them in [study_filtering.cc](/D:/Rustium/components/variations/study_filtering.cc:252). Preserve that validation/filter relationship.
- The [on-device model installer](/D:/Rustium/chrome/browser/component_updater/optimization_guide_on_device_model_installer.cc:109) validates the installed version but passes dynamic `target_version` to wildcard comparison without a local validity check. Upstream validity remains a consumer-closure question.
- [Extensions](/D:/Rustium/extensions/common/extension.cc:694) relies on native assignment, component count and normalization; [component installation](/D:/Rustium/components/component_updater/component_installer.cc:369) uses optional native values and comparisons.

The textual include inventory found **453 C++/header/Objective-C++ files** containing `base/version.h`. That is an inventory signal, not completed semantic or evaluated-GN closure. Director acceptance still needs transitive/forward-declared consumers, generated code, platform conditionals and nested repositories accounted for.

**Native assertion mapping**

Keep and run the original production-path suite unchanged after integration:

| Native family and source | Accepted Rust fixture | Native obligation |
|---|---|---|
| [DefaultConstructor](/D:/Rustium/base/version_unittest.cc:16) | `original_default_constructor` | Invalid native object and empty components |
| [ValueSemantics](/D:/Rustium/base/version_unittest.cc:21) | `original_value_and_move_semantics`, assignment supplement | Actual C++ copy constructor/assignment and source lifetime |
| [MoveSemantics](/D:/Rustium/base/version_unittest.cc:35) | `original_value_and_move_semantics` | Original uses `std::move` on a **const vector**, so add real nonconst vector and `Version` move coverage |
| [GetVersionFromString](/D:/Rustium/base/version_unittest.cc:43) | `original_get_version_from_string_every_row` | All 26 rows, validity, component count and first value |
| [Compare](/D:/Rustium/base/version_unittest.cc:88) | `original_compare_rows_and_all_relational_operators` | All 13 rows, both wildcard-free directions and actual C++ operators |
| [CompareToWildcardString](/D:/Rustium/base/version_unittest.cc:149) | `comparetowildcardstring` | All 13 rows through Rust |
| [IsValidWildcardString](/D:/Rustium/base/version_unittest.cc:168) | `isvalidwildcardstring` | All 11 rows |
| [LeadingZeros](/D:/Rustium/base/version_unittest.cc:184) | `leadingzeros` | Native formatting, equality and ordering assertions |
| [GetString](/D:/Rustium/base/version_unittest.cc:208) | `getstring` | All six rows and native string return |

Supplement with bridge byte/overflow cases, empty vector construction, const-reference identity/stability, stream insertion, copy/move assignment and native lifetime tests. Do not require a particular moved-from vector value beyond the applicable native guarantees.

Add DCHECK death coverage and **separate DCHECK-disabled** compatibility coverage for invalid comparisons and partial wildcard parsing. The current release configuration reportedly enables DCHECKs; `is_debug=false` alone does not cover that release path. Rust `None` assertions do not discharge these obligations.

Request independent ABI/ownership review, export-symbol and native layout comparisons, combined configured validation, Mojo `VersionStructTraitsTest.*`, relevant Windows file-version/OS tests, and affected variations/extensions/updater consumers. Platform/provider and performance gates remain outstanding; sanitizer/fuzz campaigns remain deferred.

**Source bindings and independent original baseline**

Observed checkout HEAD was `2c1771be99dfb9088f7dd25b0402c436daef2f26`. The candidate subtree had no diff against `5e34605ceb2bc0db7b077e5cd21c6a040ce14aa6`; scoped production Version files were clean.

Observed SHA-256 bindings:

```text
base/version.cc
4cecfafb183409ebad6d029ae4f1e30a4e8b06aa4e27d57a0808ebcfd3ffa170
base/version.h
fe050e9b11620e79c89c435b44cf99a8c0873c1632a24153774c01b611feab06
base/version_unittest.cc
7e93daa5ce2674620aacb98991ea6aff53eb596bdf66672913fe1fe7cf53b4ab
accepted rust/src/lib.rs
d41b63dd43ba1e8b1535684d2c6a7d28615810863ecf812d3084001b41ad7e51
base/BUILD.gn
089c9f83c08395b455937bfc62fe750a7d23db9ae862494c22a2829351c5c4b9
```

Use the preserved [14-source manifest](/D:/Rustium/docs/rust/migration/drafts/foundation-version/reference-cpp/manifest.json:1) plus [18-helper supplement](/D:/Rustium/docs/rust/migration/drafts/foundation-version/reference-cpp/helper-closure.json:1). They are a behavior reference archive, not a complete independent native build closure.

Before implementation, the director needs an immutable baseline package that:

1. Resolves original includes recursively against frozen headers, especially `string_split.h` → `string_split_internal.h`, numeric conversions, starts/ends, trimming, arithmetic and span helpers.
2. Prevents original helper symbols or inline definitions from resolving to live migrated `base` implementations. Use a separate frozen baseline executable or a reviewed private namespace/symbol closure; merely copying `version.cc` is insufficient.
3. Binds missing generated/provider inputs, check/logging behavior, platform headers, compiler/STL/toolchain, allocator, GN args, defines and link inputs.
4. Records editing and configured destination before-hashes, new Rust derivative hashes, CXX generator/runtime identity and intended consumer bindings before any guarded sync.

The original baseline **must not include future migrated Strings inline policy or link its migrated implementations accidentally**. The 12 recorded host tests remain evidence for the accepted candidate only. The proposed bridge requires new source-bound review and native validation before Rust policy ownership can be recorded.
