# Foundation strings candidate

This directory is a candidate artifact only. `src/lib.rs` implements safe Rust policy on byte and UTF-16 slices. Borrowed APIs return slices tied to their input; owned variants copy those slices. No Chromium production target calls this code yet.

## Frozen original closure

`reference/base/strings/` contains byte-for-byte copies of the original splitter, its public and Windows headers, internal template algorithm, original full unit-test file, the trimming helper, and the whitespace constants. `source-manifest.json` binds each copied file to its primary-checkout SHA-256 and records checkout attributes. The original files are text/LF under `.gitattributes`; copies retain exact read bytes. The queue's three original pinned hashes are checked in the manifest. No C++ caller implementation or test has been changed.

## Compatibility and limits

The port preserves the original separate empty-input rules for any-unit and substring splitting, delimiter matching, trim tables, result filtering, split-once direction, and key/value partial-result behavior. Bytes are never decoded. UTF-16 is processed as code units, so lone surrogates remain ordinary data. Key/value functions intentionally match the original byte-only surface; the original has no UTF-16 key/value or substring split-once API. Rust cannot encode C++ ABI/export or Windows `wchar_t` bindings here; the director owns those choices and platform/provider retirement.

`tests/original-case-map.md` enumerates every original gtest case, expands both typed fixtures, and maps its asserted behavior to candidate tests. The complete original assertions remain archived; compiler-backed candidate tests have not been run.

## Caller and lifetime audit

The C++ declarations mark `SplitStringPiece`, substring-piece, and split-once input views `LIFETIME_BOUND`; output views point directly into the input. Key/value view results have the same implicit contract. Owned split and key/value APIs copy bytes. Repository search found callers across `base`, `content`, `components`, and `chrome`; examples include `base/feature_list.cc`, `base/allocator/partition_alloc_support.cc`, `content/browser/service_worker/service_worker_loader_helpers.cc`, and `components/zucchini/main_utils.cc`. This candidate's returned slices are Rust-borrowed from the source at the type level. No callsite is migrated, so caller lifetime/reentrancy and platform consumers remain integration work.

## Local ordinary test request

From this directory, the exact candidate command is `cargo test --manifest-path Cargo.toml`. It is pending until the director releases the compiler gate. Production validation still requires the configured `//base:base_unittests` consumer/build and ordinary test request after the bridge/target is selected; no such target integration is included here.
