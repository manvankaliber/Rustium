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
## Follow-on source closure and implementation

Read-only caller search in the bounded checkout found direct `LazyLineBreakIterator` API behavior asserted in `text_break_iterator_test.cc`; in this checked-out sparse workspace no additional consumers were present. This is not an exhaustive whole-tree caller census.

Additional exact helper identities in the canonical checkout at this inspection:

| Helper | SHA-256 |
|---|---|
| `character_break_iterator.cc` | `7fd64af1972334b362e63df42ab3a579383d50b7a93191605b62070f13311fae` |
| `character_break_iterator.h` | `84d0adbc149b7f1ccb5fc6d38394d0e4e17923841ac4c39de4883c2fe677fd5d` |
| `character.h` | `f11445bf82f002cf027992bac6acf8e6972536f5bf8f533f27495dd369d40ef7` |
| `layout_locale.cc` | `96b911c51bc271b574eefc56937e218d6e31c315f9940f171faa1347b0b8ceb4` |
| `layout_locale.h` | `f516605e61062501a2c94f0012fb74a0faf585d5a7a424341c6056c1585eda04` |
| `character_property_data_generator.cc` | `24de6bddd1f5cca528a491344e2e6654c5a16e1ed80a1244278c6a4d1f2cbe27` |

The UTF-16 local-policy candidate is `src/lib.rs`. It owns the borrowed-slice lifetime, UTF-16 range validation, start-offset adjustment, provider cache invalidation, soft-hyphen filtering, space-run policy and surrogate-safe `break-all` boundary filtering. ICU character/line decisions remain behind `BoundaryProvider`; provider failures remain errors. Six executable Rust fixtures cover source expectations for Basic, Newline/Tab, malformed UTF-16, surrogate boundaries, cache/reset and provider failure. Fixtures have not been run.

Provider identity is not yet fully pinned: the exact Blink ICU adapter source is frozen above, but selected ICU revision/build config/data package hash are absent from this bounded checkout and must be read-only pinned from the configured canonical build by the director. The generated Blink fast-line-break table and all downstream test assertions remain outside this local milestone. This candidate deliberately doesn't implement word/sentence, ICU line algorithms, full break-all/keep-all/strictness/phrase policy, grapheme algorithm, locale fallback resolution, thread-local pool reuse/cancellation, or U+2019 generated fast path. No caller/API/GN/ABI edit is proposed as installed integration.

Validation status: source/hash inspection only. No rustfmt, compiler, Cargo, tests, native build, benchmark, sanitizer or fuzz was run.

## Configured ICU source and observed data identity (read-only)

Inspected `D:/RustiumBuild/src/third_party/icu` and `D:/RustiumBuild/src/out/Default/args.gn`; no files there were changed. ICU submodule/source HEAD is `b09986242cfb165b9111ade98be6b042ef973b86`; `README.chromium` identifies ICU 78.2 / upstream tag 78-2. SHA-256 identities: README `625f3eac2a023bdbb1df8e3bfb496db0587d2bb593bdf41d6d845330c34da1a7`; `BUILD.gn` `dde9f2fdf32341dd02b4c3fe834c89cf48ae50a531f59a25586aa524468ec163`; `version.gni` `c148e76cae94f629d6636e580fd533493b22e5da44197e0bd940becf4e6dc897`; `config.gni` `b89a3361107157566815234168b7a7398f2023e3f95464e6e22d035bc6fdf109`; data file `common/icudtl.dat` SHA-256 `eccdb4c815b98d90dba72c85f156ca3ced5a2625923655f50d8702217951d15c`.

Observed configured args: `is_debug=false`, `is_component_build=true`, `symbol_level=0`, `blink_symbol_level=0`, `v8_symbol_level=0`. ICU config defaults `icu_use_data_file=true`, `icu_use_stub_data=true`, `icu_copy_icudata_to_root_build_dir=true`. The `.dat` hash is a read-only observation, not a runtime-loaded-file proof; exact output/runtime binding and target platform remain pending. The input is source-tree data, not copied into this draft.

Frozen native behavior bounds the implemented subset: start-of-space scanning skips collapsible U+0020/U+0009/U+000A until first nonspace (then returns its index if preceded by space); soft-hyphen suppression skips provider breaks whose preceding code unit is U+00AD; `break-character` delegates grapheme boundaries; and `break-all` avoids splitting a surrogate pair. The Rust slice maps these source rules but remains a candidate: its fixtures assert selected outputs with a deterministic provider stub and do not prove ICU parity or full original-test coverage. In particular, exact space-mode, invalid-range, malformed-surrogate, and provider-boundary trace matrices should be expanded before acceptance.
