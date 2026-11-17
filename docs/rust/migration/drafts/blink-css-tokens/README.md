# Blink CSS tokenizer and token candidate

This is a task-local Rust candidate, not production integration or cutover. The original CSS tokenizer/token/test files are frozen at the queue's reference revision. Numeric/provider sources are extracted as exact Git blob bytes from `b1d5d004ec5ee1250dd6d9f8a46cc4c80065631a`; generated CSS keyword/unit outputs are read-only copies from configured HEAD `a1dfec08a2f3ce634fcfc77bd27a88b43cfd4136`. See the three manifests in `reference-cpp/` for source path, origin, blob identity, and raw SHA-256. Raw blob bytes are preserved; no CRLF normalization is applied to the new closure.

## Rust behavior in this candidate

`rust/src/lib.rs` contains safe input cursor/span handling, lexical number boundaries and the 8-bit <=14-digit integer fast path, provider parse consumption/status, float-max construction clamp, token kind/sign/value metadata, names and escapes, strings, URL/bad-URL, comments, delimiter/block and match tokens, Unicode ranges, token IDs and unit IDs, numeric/token serialization dispatch, semantic equality and inserted-comment policy. `rust/src/css_tables.rs` is generated from the pinned configured keyword table and pinned unit inputs; counts and hashes are in `reference-cpp/generated-table-manifest.json`.

The approved internal numeric provider trait has exactly three operations: selected ASCII decimal to binary64 bits plus consumed length/status; ECMAScript-shortest ASCII formatting; and WTF six-significant-digit ASCII formatting. There is no std parser/formatter fallback and no default implementation. Integer saturation/emission, numeric grammar, sign, kind, clamp, `.0`, percentage suffix, escaped dimension suffix/unit IDs and the documented `4e3e2` serialization behavior remain Rust policy. Provider parity remains director-gated debt.

The precise seam and raw-versus-normalized/provider fixture boundary are detailed in [provider-contract.md](provider-contract.md).

Input spans count source units (bytes in `Input::Bytes`, UTF-16 code units in `Input::Utf16`). 16-bit unchanged values can borrow from the source; byte input is widened into task-owned UTF-16 storage for token values. Escapes and replacement transformations own decoded values. This representation does not claim Blink ABI/layout compatibility.

## Coverage and current limits

`coverage/all-original-assertions.csv` indexes 281 assertion/case macro sites across all 16 original `CSSTokenizerTest` rows and associates each row with an executable Rust test name. The Rust suite has representative executable checks for each row and added byte/UTF-16, malformed URL/string, numeric boundary, IDs, spans and serialization cases. **The mapping is partial:** those representative checks do not reproduce every indexed original expected-value assertion. Ordinary Rust tests and all native/configured checks remain unrun.

Generated inputs and outputs are locally present and pinned; there is no missing keyword/unit provider input. The three decimal algorithms remain external provider debt by design. No provider adapter, shared ABI, GN/export change, production caller change, build, compiler, test run, benchmark, sanitizer, or fuzz campaign is included in this candidate.
