# Integration proposal (director-owned surfaces)

Candidate crate: `rust/src`, safe Rust only. `Version` owns `Option<Vec<u32>>`; parsing consumes arbitrary bytes and never decodes UTF-8. Keep the Rust implementation as the policy/data owner. Do not route parsing, comparison, wildcard validation, or serialization back to C++.

## Proposed boundary

Add a Rust target for this crate and a director-owned bridge adapter that exposes construction from `(ptr,len)`, validity, component access/copy, serialization bytes, compare, wildcard validation and wildcard comparison. The adapter must validate null/length contracts, preserve embedded NUL and arbitrary bytes, translate invalid precondition results explicitly, and document allocator ownership. No raw `std::string_view` ABI crosses the boundary. Existing C++ `base::Version` remains the production owner until all callers are migrated and the director has reviewed ABI/layout and configured tests.

## Caller families found

`base/win/windows_version.{h,cc}` stores/compares versions and inspects components; `base/file_version_info_win.cc` constructs from native file-version components; `base/version_info/version_info.{h,cc}` exposes process version; Windows utility code compares constants. These call sites include platform-specific native behavior and remain C++ owned. Migrate call sites in coherent groups and retain native providers.

## Validation request

Director gate: add/compile the crate through approved GN/Rust integration, run `cargo test --manifest-path docs/rust/migration/drafts/foundation-version/rust/Cargo.toml` (host toolchain, offline/no dependency resolution required), plus existing `//base:base_unittests` filtered to `VersionTest.*` after configured source sync. Verify actual evaluated target and Rust toolchain before scheduling. No build or test was run by the candidate worker.
