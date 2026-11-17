# Original test case and assertion map

The authoritative full assertions are archived in `reference/base/strings/string_split_unittest.cc`. This table names every gtest case and the assertion families it contains. Typed tests expand once for `StringPairs` (owned) and once for `StringViewPairs` (borrowed); both expansions have the same expected values and failure policy.

| Original case | Expansion | Assertion behavior represented by candidate |
|---|---:|---|
| `SplitStringIntoKeyValuePairsUsingSubstrTest.EmptyString` | 1 | empty input yields success and zero tuples |
| `...MissingKeyValueDelimiter` | 1 | malformed segment adds empty tuple, later valid tuple retained, overall false |
| `...MissingKeyValuePairDelimiter` | 1 | absent pair delimiter leaves one key and remainder value |
| `...UntrimmedWhitespace` | 1 | key/value interior whitespace retained |
| `...OnlySplitAtGivenSeparator` | 1 | exact substring delimiter only; other punctuation and whitespace preserved |
| `SplitStringIntoKeyValuePairsTest.EmptyString` | 2 | char pair delimiter, success and no tuples |
| `...MissingKeyValueDelimiter` | 2 | empty partial tuple plus later valid tuple; false |
| `...EmptyKeyWithKeyValueDelimiter` | 2 | empty key with nonempty value is successful |
| `...TrailingAndLeadingPairDelimiter` | 2 | outer empty pair segments discarded |
| `...EmptyPair` | 2 | adjacent pair delimiters discard empty segment |
| `...EmptyValue` | 2 | empty value tuple retained, false, later tuple retained |
| `...UntrimmedWhitespace` | 2 | key/value whitespace retained absent pair-edge trim |
| `...TrimmedWhitespace` | 2 | pair-edge ASCII whitespace trimmed |
| `...MultipleKeyValueDelimiters` | 2 | repeated key/value delimiter skipped at value start |
| `...OnlySplitAtGivenSeparator` | 2 | distinct char delimiters and punctuation preserved |
| `...DelimiterInValue` | 2 | split at first key delimiter; later delimiter remains in value |
| `SplitStringUsingSubstrTest.EmptyString` | 1 | empty input yields one empty token |
| `...EmptyDelimiter` | 1 | empty delimiter returns input as one token |
| `StringUtilTest.SplitString_Basics` | 1 | empty input, empty separator set, any-of separators, trimming, whitespace splitting, all-results empties |
| `StringUtilTest.SplitString_WhitespaceAndResultType` | 1 | empty input, whitespace-only trim, four keep/trim × all/nonempty combinations |
| `SplitStringUsingSubstrTest.StringWithNoDelimiter` | 1 | unchanged single token |
| `...LeadingDelimitersSkipped` | 1 | leading exact delimiters preserve empty all-results pieces |
| `...ConsecutiveDelimitersSkipped` | 1 | adjacent delimiters preserve each empty piece |
| `...TrailingDelimitersSkipped` | 1 | each trailing delimiter produces an empty piece |
| `SplitStringPieceUsingSubstrTest.StringWithNoDelimiter` | 1 | borrowed unchanged token |
| `...LeadingDelimitersSkipped` | 1 | borrowed empty pieces at leading delimiters |
| `...ConsecutiveDelimitersSkipped` | 1 | borrowed empty pieces between delimiters |
| `...TrailingDelimitersSkipped` | 1 | borrowed trailing empty pieces |
| `...KeepWhitespace` | 1 | borrowed pieces retain trailing ASCII whitespace |
| `...TrimWhitespace` | 1 | borrowed pieces trim ASCII whitespace |
| `...SplitWantAll` | 1 | all result mode preserves trailing empty pieces |
| `...SplitWantNonEmpty` | 1 | nonempty mode drops empties |
| `StringSplitTest.SplitStringOnce` | 1 | no match, empty separator list, char/set overload, first input separator, empty side |
| `StringSplitTest.RSplitStringOnce` | 1 | reverse analog; last input separator, empty side |
| `StringSplitTest.StringSplitKeepWhitespace` | 1 | whitespace retained with tab/newline delimiters |
| `StringSplitTest.SplitStringAlongWhitespace` | 1 | all 13 table inputs and result counts/tokens using ASCII whitespace |
| `StringSplitTest.NullSeparatorWantNonEmpty` | 1 | all 4 NUL-bearing inputs and nonempty expected slices |
| `StringSplitTest.NullSeparatorWantAll` | 1 | all 4 NUL-bearing inputs and empty boundary slices |
| `ConstexprSplitStringPieceTest.SplitStringPieces` | 1 | compile-time borrowed pieces, including empty piece |
| `...SplitStringPiecesSkipEmpty` | 1 | compile-time nonempty pieces |
| `...SplitStringUsingSubstr` | 1 | compile-time exact-substring borrowed pieces |

The Rust unit tests in `src/lib.rs` group these behavioral assertions into executable compatibility checks and add non-UTF-8 bytes, embedded NUL, Unicode trim, lone surrogate, exact borrowed-pointer, and delimiter edge cases. Original compile-time C++ assertions remain source-only: Rust cannot prove C++ `constexpr` behavior, so those cases remain in the configured C++ test obligation until the C++ surface is retired.
