# Original test case and assertion map

The complete authoritative test file is `reference/base/strings/string_split_unittest.cc`. Each byte splitting fixture is run through both borrowed and owned wrappers by `any`/`substr`; each key/value fixture is run as both `ViewPair` and owned `Pair` by `kv`, which asserts exact contents and status. Typed C++ tests instantiate both `StringPairs` and `StringViewPairs`; the Rust helper deliberately checks both outputs for every row.

| Original C++ case(s) | Rust executable mapping |
|---|---|
| `SplitStringIntoKeyValuePairsUsingSubstrTest.EmptyString`, `MissingKeyValueDelimiter`, `MissingKeyValuePairDelimiter`, `UntrimmedWhitespace`, `OnlySplitAtGivenSeparator` | `original_key_value_cases_both_typed_variants`; all five are covered, each with both borrowed and owned outputs, exact tuples and bool status. |
| `SplitStringIntoKeyValuePairsTest.EmptyString`, `MissingKeyValueDelimiter`, `EmptyKeyWithKeyValueDelimiter`, `TrailingAndLeadingPairDelimiter`, `EmptyPair`, `EmptyValue`, `UntrimmedWhitespace`, `TrimmedWhitespace`, `MultipleKeyValueDelimiters`, `OnlySplitAtGivenSeparator`, `DelimiterInValue` | `original_key_value_cases_both_typed_variants`; each original char-delimiter fixture checks full tuple bytes and success status for both representations. |
| `SplitStringUsingSubstrTest.EmptyString`, `EmptyDelimiter`, `StringWithNoDelimiter`, `LeadingDelimitersSkipped`, `ConsecutiveDelimitersSkipped`, `TrailingDelimitersSkipped` | `original_substr_all_owned_and_borrowed`; exact full vectors, including every empty token, checked for owned and borrowed forms. |
| `SplitStringPieceUsingSubstrTest.StringWithNoDelimiter`, `LeadingDelimitersSkipped`, `ConsecutiveDelimitersSkipped`, `TrailingDelimitersSkipped`, `KeepWhitespace`, `TrimWhitespace`, `SplitWantAll`, `SplitWantNonEmpty` | `original_substr_all_owned_and_borrowed`; matching fixture vectors checked by both wrappers. |
| `StringUtilTest.SplitString_Basics`, `SplitString_WhitespaceAndResultType` | `original_split_any_all_owned_and_borrowed`; empty inputs and separator set, all result flag combinations, trimming and exact values. |
| `StringSplitTest.StringSplitKeepWhitespace` | `original_split_any_all_owned_and_borrowed`; all three inputs and exact results. |
| `StringSplitTest.SplitStringAlongWhitespace` | `original_split_any_all_owned_and_borrowed`; all 13 original table rows run against the ASCII whitespace separator set, with full exact token lists. |
| `StringSplitTest.NullSeparatorWantNonEmpty`, `NullSeparatorWantAll` | `original_split_any_all_owned_and_borrowed`; all four NUL table inputs are checked under both result modes. |
| `StringSplitTest.SplitStringOnce`, `RSplitStringOnce` | `original_split_once_permutations`; both directions, character/list forms, forward and reverse separator ordering, absent separators and empty sides; exact pair slices asserted. |
| `ConstexprSplitStringPieceTest.SplitStringPieces`, `SplitStringPiecesSkipEmpty`, `SplitStringUsingSubstr` | Native C++ obligation only. Exact constexpr expected pieces remain archived; Rust runtime tests cannot establish C++ constexpr compilation. |

Additional executable cases are in `utf16_bytes_and_owned_variants` (non-UTF-8 bytes, UTF-16 including lone surrogates, exact Chromium Unicode whitespace, owned wrappers), and `malformed_default_empty_differs_from_borrowed_empty` (input-relative offsets and explicit origin metadata). Compiler-backed execution is pending the director's gate.
