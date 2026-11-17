# Blink text boundaries: frozen inputs and candidate scope

Status: source-bound local-policy milestone; no compiler/provider/native validation. This is not a cutover.

## Frozen native reference closure

Copied byte-for-byte from `D:/Rustium`, observed against task base `b4681bf483649cdca320952d59d2c31719b338b6`, before any candidate edits. SHA-256:

| Input | SHA-256 |
|---|---|
| `text_break_iterator.cc` | `1bf73e8a30d14c71dbd041f23a949ca01188812411c9bad845973854fbf5af0b` |
| `text_break_iterator.h` | `ec36a5e58a3780ced7314a3a5ef07df2b8d2889ecebb8bc54e0b112e86024593` |
| `text_break_iterator_icu.cc` | `9bc656d5b45c59f3be4006a849d56c8b6926ecfbf5d13a73766500bf039b9ba9` |
| `text_break_iterator_test.cc` | `a40641a52be68727b80c7d9785fc1397659b96c075027663cdd119e1ed6f1b3e` |

Copies are in `frozen/`. These are references, not Rust helpers. `text_break_iterator_icu.cc` remains a native provider; generated fast line-break data and Unicode property helpers are not independently frozen here.

## Local behavior that must move into Rust

The non-provider core scans UTF-16 code units and combines local policy with provider boundaries. It preserves start-offset-relative input, lazily creates/caches provider iterators, invalidates them on relevant locale/start changes, handles break type (`normal`, `break-all`, `break-character`, `keep-all`, `phrase`), whitespace modes, strictness, soft-hyphen filtering, and fast-path compatibility. `break-all` must not return an offset inside a surrogate pair. The current `GetIterator()` adjusts provider offsets by `start_offset`; `Context` carries previous and previous-previous code units, and fast handling includes hyphen context, generated ASCII/Latin data, soft hyphen and optional U+2019 behavior.

This family is too broad to claim complete implementation without a pinned provider and generated-data contract. No ICU-forwarding Rust façade is presented as a rewrite. This milestone freezes the actual dependency closure and gives the director exact integration boundaries.

## Proposed director-owned provider boundary

Rust policy should consume a narrow `BoundaryProvider` whose request includes boundary kind, locale identifier, and borrowed UTF-16 slice; response is a sequence/cursor of UTF-16 offsets plus explicit provider error. Rust owns local policy, range checking, cache lifecycle, offset adjustment, filtering and iterator lifetime/thread ownership. Provider identity must include ICU version, locale fallback rules, Unicode data version, and generated fast-table identity. Do not expose ICU C++ objects across the Rust boundary. Director owns ABI/GN/export wiring and chooses the concrete provider bridge. A provider error must not silently become a fabricated successful boundary list.

## Original assertion mapping and required Rust fixtures

`frozen/text_break_iterator_test.cc` is the assertion authority. Rust fixtures must enumerate every assertion before acceptance; currently this mapping is deliberately marked incomplete. The first required groups are:

- Pool reuse and scoped return: `PooledBreakIterator`, `PooledCharacterBreakIterator` (thread ownership, reset, cancellation/drop, no cross-thread reuse).
- Empty/null, strictness/locale invalidation: `EmptyString`, `NullString`, `Strictness` (include fallback and locale keyword changes).
- Line modes: `Basic`, `Newline`, `Tab`, `LatinPunctuation`, `Chinese`, `ChineseMixed`, `ChineseSpaces`, `Phrase`, break-all Ethiopic/hyphens, soft hyphen and high-Latin hyphens.
- Grapheme helpers: `LengthOfGraphemeCluster`, `GraphemesClusterListTest` (combining marks, Hangul, flags, ZWJ, invalid/unpaired UTF-16).
- Fast path parity: `RightSingleQuotationMark` across locales, strictnesses and break types.
- Word provider version behavior: `WordBreakSwedish`, explicitly keyed to ICU >=77 versus older behavior.

Additional explicit fixtures required by assignment: malformed UTF-16 (isolated lead/trail and reversed pair), UTF-16 start/limit offsets, locale fallback, per-thread cache isolation, reset after locale/type/start changes, cancellation/drop while borrowed text is live, and provider error traces. They are not yet executable here.

## Remaining ownership/debt

Native ICU line/word/sentence/character implementations and all provider data remain native. Callers, `CharacterBreakIterator`, `character.h`, `layout_locale.*`, generated line-break tables, Unicode property helpers and `TextBreakIterator` ICU tests require closure mapping and pinned identities before further porting. Native ABI, platform contracts and configured compilation are director-owned. No build, Cargo, test, benchmark, sanitizer or fuzz campaign was run per assignment.
