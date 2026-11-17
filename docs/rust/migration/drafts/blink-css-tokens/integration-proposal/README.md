# Director-owned integration proposal

This is a proposal only; no GN, shared exports, ABI, configured source, or production caller was changed.

- Keep the candidate's input distinction explicit: `LChar` bytes and `UChar` UTF-16 code units with spans in their corresponding storage units. During any bridge, preserve the original `StringView` lifetime contract and never return a borrowed C++ token/value buffer past its owner.
- Rust should own tokenizer cursor/grammar, decoded token values, block state, numeric grammar/fast path/clamp/kind/sign, keyword/unit table lookup, equality and token serialization dispatch. Do not forward tokenizer or token policy to C++.
- The only candidate provider seam left by the numeric review is the three internal decimal operations documented in `provider-contract.md`. They are not shared exports. A director may temporarily implement them over pinned WTF/double-conversion sources; output must be copied with explicit lengths and exact bits/status.
- Existing consumers include `CSSParserTokenStream`, CSS parser tests, token stream tests, and token serialization tests. The director must choose an ownership bridge and migrate consumers only after reviewing representation, allocation, lifetimes, block semantics, and ABI/layout. C++ token, stream, parser, and GC/interning/provider debt remain visible until each caller family moves.

Exact follow-up validation request: run the Rust suite after assertion-level mappings are complete; a pinned decimal oracle comparing parse bits/consumed/status and exact shortest/WTF6 bytes; then the configured `CSSTokenizerTest.*`, `CSSParserTokenTest.*`, `StringToNumberTest.*`, `DtoaTest.*`, and `StringBuilderTest.AppendNumberDoubleUChar` cases in the owning unit-test binary. The queue's proposed target is `//third_party/blink/renderer/core:unit_tests`, but the director must resolve the actual filtered executable from the configured graph. No request here represents validation already performed.
