# Blink CSS tokens

Candidate workspace for `BLINK-CSS-TOKENS`; this is not production integration or a cutover. The queue-pinned tokenizer, token, and test sources at `4f246b50e3f64f7c3880973ae986dadedfc43adf` match their recorded SHA-256 values. The additional direct behavior sources are archived under `reference-cpp/` and hashed in `reference-cpp/manifest.json`.

## Observed contracts

- Inputs use WTF 8-bit or UTF-16 storage. Token spans/offsets count input storage units: bytes for 8-bit input and UTF-16 code units for 16-bit input. NUL and lone surrogates are replacement-aware; valid surrogate pairs remain paired.
- Whitespace uses HTML space (TAB, LF, FF, CR, space). Escaped string newlines consume CRLF as one sequence. `TokenizeSingle` skips comments; `TokenizeSingleWithComments` returns them.
- Escapes accept at most six ASCII hex digits and one following HTML whitespace/CRLF; zero, surrogate scalar values and values above U+10FFFF become U+FFFD. Backslash at EOF also becomes U+FFFD.
- Numeric lexical boundaries are independent of conversion: optional sign, decimal point only before a digit, exponent only with required digits. Blink converts with WTF `StringToDouble`, uses a <=14-digit 8-bit integer fast path, and clamps token values to float max. A Rust float parser is not assumed equivalent.
- URL parsing distinguishes URL/bad-URL, consumes bad remnants, and preserves the comment/string newline rules. Token value serialization, equality, keyword/unit mapping and inserted-comment policy are part of this family.

## Source and provider closure

Archived sources cover tokenizer/input/idioms, token representation and serialization, original tests, HTML-space behavior, WTF StringView/String/character visitor, numeric conversion, dtoa/format, CSS markup and primitive-value sources. The closure is still incomplete: generated CSS keyword/unit tables and transitive numeric conversion dependencies must be located and pinned. Numeric behavior remains a provider boundary until the director's requested numeric analysis provides a reviewed contract. No copied or pinned provider is modified.

## Candidate status

`rust/src/lib.rs` is an early safe Rust lexer candidate with explicit 8-bit/UTF-16 input, source spans, token records, lexical paths for whitespace, names/escapes, strings, numeric boundaries, comments, URL/bad-URL, delimiters, blocks and match operators. Numeric conversion is injected through `NumberProvider`.

This is not yet a complete connected token family. Token serialization/equality, Unicode-range handling, IDs/unit mapping and complete parity for replacement-aware names, string fast paths, blocks and malformed cases remain unimplemented or unverified. `coverage/original-tests.csv` inventories all 16 original test rows; individual assertions are not yet mapped into executable Rust tests. Rust tests, native tests and configured validation remain pending. No production integration or cutover is claimed.

Caller inspection found the tokenizer is consumed by `CSSParserTokenStream`, token-stream tests, parser tests and token serialization tests. A later director proposal must define value lifetime and source-span ownership across these consumers; this draft does not add a production bridge or ABI.
