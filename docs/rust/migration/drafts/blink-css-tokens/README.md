# Blink CSS tokens: source freeze and implementation boundary

This draft is a candidate workspace for `BLINK-CSS-TOKENS`; it is not production integration or a cutover. The canonical queue pins the three original files at revision `4f246b50e3f64f7c3880973ae986dadedfc43adf`. Their archived byte hashes match the queue. `reference-cpp/manifest.json` records hashes for the additional directly relevant C++ sources copied from `D:/Rustium`.

## Observed contracts to carry into the Rust family

- The input is a WTF `StringView` with 8-bit or UTF-16 backing. Token source ranges/offsets are code units in that backing, not UTF-8 byte indices. The stream's replacement-aware reads map NUL and lone UTF-16 surrogates to U+FFFD while non-replacing lookahead still distinguishes the source units.
- CSS tokenizer whitespace follows the implementation's HTML-space predicate (`TAB`, LF, FF, CR, space). Newline handling for escaped strings consumes CRLF as one whitespace sequence. Comments are either skipped or emitted by the two public single-token entry points.
- Escape consumption handles at most six ASCII hex digits, consumes one following HTML whitespace (or CRLF pair), and replaces zero, surrogate scalar values, and values above U+10FFFF. A backslash at EOF also yields U+FFFD.
- Numeric token boundaries are selected lexically before conversion: optional sign; decimal point only when followed by an ASCII digit; exponent only when its required digits exist. Integer tokens retain integer-vs-number metadata and sign. The original conversion uses `StringToDouble` and a <=14-digit 8-bit integer fast path; Rust float parsing must not be substituted without differential fixtures.
- URL parsing has a fast path and an escape-aware path; malformed quoted/parenthesized/non-printable URL input becomes a bad-URL token after consuming remnants. Bad strings stop before newline and reconsume that newline.
- Token equality, serialization, numeric formatting, unit IDs, keyword IDs, inline values, and block matching are part of `css_parser_token.cc/.h`; preserving only lexical token kinds is insufficient.

## Provider closure still required

The archive contains tokenizer, input stream, parser idioms, HTML-space helper, token representation/serialization source, test source, and direct WTF numeric conversion source. It does **not yet** constitute a reproducible complete helper build closure. Specifically inspect/archive the implementations behind WTF `StringToDouble` / `CharactersToDouble` and formatting/dtoa; WTF `StringView` / `String` width and visitor semantics; CSS markup escaping; unit formatting and generated CSS keyword/property tables. The direct source/provider hashes and current gaps are enumerated in `reference-cpp/manifest.json`; no Rust implementation may claim numeric, equality, or serialization equivalence until corresponding fixtures or exact provider behavior support it.

## Original test inventory

`reference-cpp/.../css_tokenizer_test.cc` contains 16 `CSSTokenizerTest` rows: `SingleCharacterTokens`, `MultipleCharacterTokens`, `DelimiterToken`, `WhitespaceTokens`, `Escapes`, `IdentToken`, `FunctionToken`, `AtKeywordToken`, `UrlToken`, `StringToken`, `HashToken`, `NumberToken`, `DimensionToken`, `PercentageToken`, `UnicodeRangeToken`, and `CommentToken`. This row list is an inventory only; assertion-level mapping to executable Rust tests has not been prepared and no coverage claim is made. Implementation work must map every assertion, not just these row names.

## Status

Reference files are frozen and hash-verified. The connected Rust tokenizer/token-value implementation, executable assertion-level coverage map, deterministic malformed/byte/UTF-16 tests, and integration proposal remain outstanding. In particular, the complete token serializer/equality and character-width/lifetime contracts cannot be represented as complete by this source-freeze artifact alone.
