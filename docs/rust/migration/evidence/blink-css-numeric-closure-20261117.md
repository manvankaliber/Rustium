The numeric closure is locally available. The worker can implement Rust tokenization and token serialization around a narrow decimal provider without delegating tokenization back to C++. Decimal conversion and formatting would remain explicit C++ provider debt until replaced.

Read-only inspection completed; no writes, children, builds, tests, downloads, or campaigns were performed.

**Source identity and freezing strategy**

Use checkpoint `b1d5d004ec5ee1250dd6d9f8a46cc4c80065631a` in `C:/Users/jackv/.t3/worktrees/Rustium/t3-6013ec4a-29dd7b89022f530f` as the origin for the additional references. All listed implementation files exist at their original repository paths in that commit, even where the draft’s `reference-cpp` directory has not archived them.

I compared those checkpoint blobs with the primary sources after CRLF/LF normalization. They match. The inspected primary and configured copies are byte-identical, including `dtoa`, `StringBuilder`, numeric helpers, vendor sources, and `format.cc`. Observed checkout HEADs were:

- Primary: `285af1cc64796bba57178dc94a659f192343671d`
- Configured: `a1dfec08a2f3ce634fcfc77bd27a88b43cfd4136`

For freezing, extract the named paths from the checkpoint with Git rather than copying mutable working-tree files. Record the commit, original path, Git blob identity, and SHA-256 of the extracted bytes. Existing raw Windows file hashes differ from Git-extracted hashes where line endings differ; preserve that distinction.

Selected raw SHA-256 identities:

| Path relative to `third_party/blink/renderer/platform/wtf/` | SHA-256 |
|---|---|
| `dtoa.cc` | `8c1e576427c9398def6173dd6ce33f1d86851476e2a0001b6fb22bee9a8708ce` |
| `dtoa.h` | `6dacb0909a70ab454e3ef0056e2d022b9018597760423942f50b794cc2c5d4d4` |
| `math_extras.h` | `eeb5d43ce4793ef0043326a65318d4e1a66e17d4af218387619ebd71d598238f` |
| `text/string_builder.cc` | `1ec658bef96cfa5c6edc1da91d09fc4b58e8f9d00e5e10f6191ac8898d6edab1` |
| `text/string_builder.h` | `376fdd28ba42dbce01607a70e0a407df054fbc968e660a5abb084fbabf6810ac` |
| `text/integer_to_string_conversion.h` | `3250d00e828ad0390853665fd417fd5208e89caf23a5bf8a02ca28183fe3ffc6` |

**Concrete missing-file/helper mapping**

This is the behavior-bearing numeric closure, rather than every header required to compile Blink.

| Layer | Exact files and responsibility |
|---|---|
| Existing token references | `third_party/blink/renderer/core/css/parser/{css_tokenizer.cc,css_tokenizer_input_stream.cc,css_parser_token.cc,css_parser_token.h}`: numeric grammar, integer fast path, construction clamp, numeric kind/sign, serialization dispatch. Already archived. |
| WTF decimal parsing | `third_party/blink/renderer/platform/wtf/text/{string_to_number.cc,string_to_number.h,ascii_ctype.h}`: `StringToDouble` → `CharactersToDouble` → `ToDoubleType`; leading whitespace and complete-consumption policy. First two already archived; add `ascii_ctype.h`. |
| WTF conversion adapter | `third_party/blink/renderer/platform/wtf/{dtoa.cc,dtoa.h}`: actual converter flags, UTF-16 narrowing, processed length, shortest/precision formatting and trailing-zero handling. **Missing from archived closure.** |
| Numeric clamping | `third_party/blink/renderer/platform/wtf/math_extras.h`: `ClampTo<double>` and specialized `ClampTo<int64_t>`. **Missing.** |
| Builder formatting | `third_party/blink/renderer/platform/wtf/text/{string_builder.h,string_builder.cc,integer_to_string_conversion.h}`: integer decimal emission; double overload defaults to precision six. **Missing.** String storage work remains with the worker. |
| Base numeric helpers | `base/numerics/{safe_conversions.h,safe_conversions_impl.h,safe_conversions_arm_impl.h,integral_constant_like.h}`: saturated input-length conversion and signedness helpers. Freeze the architecture branch as well; this host does not validate ARM behavior. |
| Character representation | `third_party/blink/renderer/platform/wtf/text/wtf_uchar.h`: `LChar`/`UChar` representation. Coordinate with the worker’s existing string closure. |
| Unicode-range numeric serialization | `third_party/blink/renderer/platform/wtf/text/{format.h,format.cc}`: the reachable `U+{:X}-{:X}` path uses unsigned uppercase hexadecimal through `IntegerToStringConverter<uint64_t,16,true>`. Rust can implement this fixed format directly; generic floating-point `FormatTo` machinery is unnecessary for this call. |

The complete local vendor source family is only **20 files** under `base/third_party/double_conversion/src/double-conversion/`:

```text
string-to-double.cc  string-to-double.h
strtod.cc           strtod.h
double-to-string.cc  double-to-string.h
fast-dtoa.cc        fast-dtoa.h
fixed-dtoa.cc       fixed-dtoa.h
bignum-dtoa.cc      bignum-dtoa.h
bignum.cc           bignum.h
cached-powers.cc    cached-powers.h
diy-fp.h
ieee.h
utils.h
double-conversion.h
```

The dependency mapping is:

```text
StringToDouble adapter
  → string-to-double
  → strtod
  → cached-powers + bignum + diy-fp + ieee + utils

Shortest / precision formatting adapter
  → double-to-string
  → fast-dtoa, with bignum-dtoa fallback
  → cached-powers + bignum + diy-fp + ieee + utils
```

Freeze the entire 20-file family to avoid severing fallback dependencies. `fixed-dtoa` is referenced by the shared formatter implementation, although CSS numeric serialization does not call fixed-width formatting.

Also pin `base/third_party/double_conversion/{BUILD.gn,README.chromium,crowbar.txtpb}`. The recorded upstream is **v3.4.0**, commit `9dd6227ee3e29807183e56877f0282aeb40b8b1e`, with “None” recorded for local modifications. This is corroborating provenance; the local checkpoint blobs should remain the acceptance reference. `README.chromium` names `src/LICENSE`, but that file is absent from the inspected primary tree; preserve the existing source notices and record the packaging gap.

No generated provider is needed for this decimal conversion/formatting path.

**Semantics that must survive**

The decisive source locations are [ConsumeNumber](/D:/Rustium/third_party/blink/renderer/core/css/parser/css_tokenizer.cc:303), [input conversion](/D:/Rustium/third_party/blink/renderer/core/css/parser/css_tokenizer_input_stream.cc:22), [token construction](/D:/Rustium/third_party/blink/renderer/core/css/parser/css_parser_token.cc:33), [serialization](/D:/Rustium/third_party/blink/renderer/core/css/parser/css_parser_token.cc:179), and [WTF converter configuration](/D:/Rustium/third_party/blink/renderer/platform/wtf/dtoa.cc:70).

- **Grammar determines integer kind.** A fraction or valid exponent marks a number as non-integer even when its value is integral. Invalid exponent suffixes remain outside the consumed number.
- **Integer fast path:** unsigned digit accumulation applies only to 8-bit input with at most 14 digits excluding the sign. Other cases use decimal conversion. Applying minus afterward preserves negative zero.
- **Parsing:** WTF requires complete consumption of the selected slice. Its underlying converter allows leading spaces and trailing junk, but the outer wrapper rejects partial results. Hex/octal flags are disabled; infinity/NaN symbol pointers are null.
- **Overflow and underflow:** vendor parsing can produce infinity or signed zero. The token constructor then clamps to `±f32::MAX` represented as a double. It does **not** round all finite token values through `float`.
- **Rounding:** decimal-to-binary fallback resolves exact halfway cases toward an even binary significand. Shortest formatting’s bignum fallback resolves decimal halfway choices toward even; counted precision formatting rounds an exact half upward in magnitude. These are distinct rules.
- **Signed zero:** the numeric value can retain `-0`, and the lexical sign is separately retained. ECMAScript formatting uses `UNIQUE_ZERO`, suppressing its minus sign.
- **Plain integer tokens:** serialize after saturation to `int64_t`, using ordinary decimal digits. Thus a huge integer token serializes differently from a huge non-integer token.
- **Plain non-integer tokens:** shortest formatting, followed by `.0` when the result contains neither `.` nor lowercase `e`. For example, `2.0` remains `2.0`.
- **Dimensions:** shortest formatting plus escaped unit, without that `.0` rule. Preserve the existing documented serialization defect involving exponent-like units, such as `4e3e2`.
- **Percentages:** `AppendNumber(double)` uses **six significant digits**, not shortest formatting. WTF strips trailing fractional zeros only when there is no exponent; exponential output retains precision padding.
- **Exponent formatting:** shortest mode uses fixed notation for decimal exponents `[-6,21)`, lowercase `e`, an explicit positive exponent sign, and no default exponent zero-padding.
- **Percentage cutoff subtlety:** actual `ToPrecision` parameters allow six leading padding zeros, giving the lower transition at exponent `-6`; the nearby `%g` explanatory comment describes a different cutoff. Follow the implementation.

Consequently, neither Rust `f64::parse` nor `to_string` can be adopted as an equivalent solely from their names or general round-trip guarantees.

**Viable implementation and validation approach**

The worker can own the complete tokenizer control flow, token construction/clamping, integer emission, serialization dispatch, `.0` rule, percentage suffix, and unit handling in Rust. Propose three internal provider operations:

```text
parse selected ASCII decimal slice → binary64 bits + consumed length/status
format binary64 → ECMAScript shortest ASCII
format binary64 → WTF six-significant-digit ASCII
```

These are semantic seams, **not proposed shared exports or ABI definitions**. The director defines any bridge. A temporary adapter should call the pinned numeric provider only, return copied output with explicit length/capacity handling, and expose no borrowed WTF buffers or token objects. Keep clamping in the Rust token constructor so raw provider overflow remains observable and testable.

That permits Rust ownership of tokenizer behavior while explicitly retaining decimal-provider debt. It does not complete numeric migration or subsystem cutover. Eventual replacement includes the vendor parser, formatting algorithms, cached tables, bignum fallbacks, and platform-sensitive arithmetic assumptions.

For later authorized validation:

1. Freeze independent original references before implementation changes. Add existing `css_parser_token_test.cc`, WTF `dtoa_test.cc`, `text/string_to_number_test.cc`, and `text/string_builder_test.cc` alongside the already archived tokenizer tests.
2. Build a small source-bound oracle around the pinned conversion adapters/vendor sources; use production Blink tests separately for token integration.
3. Compare raw binary64 bits and consumed lengths for parsing, exact bytes for formatting, then token kind, lexical sign, clamped bits, consumed offset, and serialized output.
4. Cover 14/15-digit and 8/16-bit paths; `2^53` neighbors; `int64_t` boundaries; float-limit and binary64-overflow boundaries; subnormal/zero halfway cases; signed zero; long significands beyond the vendor’s 772-digit retention threshold; long exponents; notation transitions near `1e-6`, `1e-7`, `1e20`, and `1e21`; six-digit rounding ties/carries; and percentage exponent padding.
5. Keep provider-level expectations separate from token-level expectations: token clamping hides binary64 overflow, and plain integer serialization applies an additional saturation.

All proposed validation remains **unrun**. The concrete closure and source pin are available now; no full Chromium clone or vendor download is required.
