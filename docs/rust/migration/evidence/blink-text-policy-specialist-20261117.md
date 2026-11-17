Candidate remains clean at `3f94c407d4f484185f26c3b840e3114a42227e4f`. **It is not a parity implementation.** The smallest coherent replacement is Blink’s complete local line-break scanner and its query/settings state, behind primitive ICU cursor/property operations. A boundary-list filter cannot implement this family.

This was read-only source research. No edits, queue writes, compilation, tests, builds, or measurements occurred.

**Prioritized defects**

1. **The candidate omits the algorithm that creates local opportunities.** [Candidate lines 41–67](C:/Users/jackv/.t3/worktrees/Rustium/t3-8e793e24-85c588c23fadd439/docs/rust/migration/drafts/blink-text-boundaries/src/lib.rs:41) enumerate ICU boundaries and filter them. Native instead scans every code unit, applies whitespace rules, evaluates its compatibility fast table, adds break-all opportunities, suppresses keep-all opportunities, and consults ICU only for unresolved cases. See [native scanner lines 289–405](C:/Users/jackv/.t3/worktrees/Rustium/t3-8e793e24-85c588c23fadd439/docs/rust/migration/drafts/blink-text-boundaries/frozen/text_break_iterator.cc:289). Filtering cannot create the extra break-all opportunities, ASCII compatibility breaks, or whitespace opportunities.

2. **Surrogate filtering is off by one and can panic.** A boundary `x` lies between `text[x-1]` and `text[x]`; [candidate line 71](C:/Users/jackv/.t3/worktrees/Rustium/t3-8e793e24-85c588c23fadd439/docs/rust/migration/drafts/blink-text-boundaries/src/lib.rs:71) examines `text[x]` and `text[x+1]`.
   - `[D83D, DE00, 'a']`, candidate boundary `1`: the check returns false and permits splitting the pair. Its own line-86 assertion therefore fails by inspection.
   - `['a', D800]`, boundary `1`: the lead-surrogate test succeeds, then indexes past the slice.
   - Simply fixing the predicate is insufficient: native break-all skips lead units, derives supplementary properties at trail units, preserves combining-mark state, and sometimes returns `i-1` ([native lines 330–355](C:/Users/jackv/.t3/worktrees/Rustium/t3-8e793e24-85c588c23fadd439/docs/rust/migration/drafts/blink-text-boundaries/frozen/text_break_iterator.cc:330)).

3. **KeepAll, Phrase, and strictness are missing.**
   - KeepAll needs the precise letter/number/mark and complex-context rule at native lines 158–165, applied at lines 357–362.
   - Phrase uses the normal scanner **with a different ICU locale keyword**, not ordinary line boundaries.
   - Strictness also affects local break-all hyphen rules; sending it only to ICU would remain incorrect.
   - [Native settings and locale construction](C:/Users/jackv/.t3/worktrees/Rustium/t3-8e793e24-85c588c23fadd439/docs/rust/migration/drafts/blink-text-boundaries/frozen/text_break_iterator.h:264) include consequential null/empty/default branches. Preserve those exact branches, including their early returns.

4. **Whitespace, start offset, and BreakCharacter are materially different.**
   - AfterRun scans through current spaces and returns at the first following nonspace, independently of ICU. Candidate only advances an already supplied boundary.
   - EverySpace creates opportunities and additionally recognizes **U+3000**, via [IsOtherSpaceSeparator](D:/Rustium/third_party/blink/renderer/platform/text/character.h:187). It is not a generic Unicode-whitespace predicate.
   - Native initializes previous/previous-previous context only from positions at or after `start_offset` (native lines 186–199). Candidate can inspect `text[x-1]` before that start.
   - Native BreakCharacter bypasses the whitespace and soft-hyphen scanner entirely and ignores the supplied scan limit ([native lines 438–462](C:/Users/jackv/.t3/worktrees/Rustium/t3-8e793e24-85c588c23fadd439/docs/rust/migration/drafts/blink-text-boundaries/frozen/text_break_iterator.cc:438)). Candidate applies generic filtering and caps results at `limit`.

5. **Provider normalization conceals contract failures.** Candidate line 49 sorts, deduplicates, and supplies a missing terminal boundary. Native consumes strict `following()` results and distinguishes `DONE`. Preserve provider output/errors; reject malformed responses rather than repairing them into plausible success. An explicit candidate error is useful, but it is not native failure parity: native line-iterator acquisition failure continues scanning.

6. **The fixture claims are unreliable even before execution.**
   - Basic calls `next(2,6)` expecting `5`; both the candidate’s documented inclusive query and native behavior permit `2`. Native’s enumeration helper increments between queries.
   - The reset fixture keeps raw boundaries `[1,3]` after shortening the provider slice to length `2`; candidate returns `Provider`, not its expected `Ok(2)`.
   - Lines 83–84 borrow temporary collected vectors into a policy retained across statements; those fixtures need named backing buffers.
   - The “Newline/Tab” fixture contains no tab case, and no executable soft-hyphen fixture is present.

**Bounded implementation brief for the existing Luna worker**

Own only this draft’s Rust implementation, fixtures, reference additions, and scope documentation. Do not edit native callers, ABI, GN, shared exports, or configured-tree inputs.

Replace `boundaries()` plus generic filtering with a direct port of:

- `Context` initialization, fetching, advancement, and three-state `ShouldBreakFast`.
- Both whitespace branches, with their original ordering.
- `LineBreakPropertyValue`, the complete ICU-78 break-all table, `ShouldBreakAfterBreakAll`, and `ShouldKeepAfterKeepAll`.
- Lazy ICU `following()` consultation and the original soft-hyphen retry loop.
- BreakCharacter’s separate path.
- Forward query, limited query, `IsBreakable`, and reverse query. Reverse traversal must retain the original code-point stepping and `min` behavior.
- Start/settings state, resolved locale keyword state, line/character handle invalidation, and the copy-with-new-text settings behavior.

Preserve Latin-1 versus UTF-16 input semantics explicitly. For the smallest initial milestone, Rust can own the line-policy family while `CharacterBreakIterator` and public grapheme helpers remain native providers. Do not claim those helpers or thread-pool internals have been rewritten.

The scanner’s required order is:

1. Initialize context relative to `start_offset`; initialize break-all property state.
2. Apply the selected space rule.
3. Evaluate fast break; immediately return `CanBreak`.
4. Apply break-all additions/property updates or keep-all suppression.
5. Skip ICU for `NoBreak`.
6. For `Unknown`, lazily acquire the line cursor; query `following(i-1-start)` when necessary; map offsets back; retry disabled soft-hyphen results.
7. Accept the matching ICU boundary under the native preceding-space condition; otherwise advance.
8. Return the scan limit on exhaustion.

**Minimal primitive provider contract**

Use opaque, thread-confined sessions rather than `Vec<usize>`:

- Acquire/reset a **line** session over the entire suffix `text[start..]`, with the exact resolved locale identifier.
- `following(relative_i32) -> Boundary(relative_i32) | Done | Error`.
- Separate character/grapheme `following` session.
- Primitive ICU general-category and line-break-property lookups. Rust must retain Blink’s `+` tailoring, mark handling, table decisions, and supplementary decoding.
- Immutable, identity-bound generated fast-pair data; the U+2019 feature setting must be explicit.
- Release/reset operations with an enforceable backing-text lifetime and same-thread contract.

`following(q)` must return an offset strictly greater than `q`, within the suffix, or `Done`. Validate signed values before conversion and checked addition; enforce ICU’s `int32_t` length/index limits. Do not require monotonicity across unrelated queries—callers also search backward.

The [pinned ICU declaration](D:/RustiumBuild/src/third_party/icu/source/common/unicode/brkiter.h:186) states that `setText(UText*)` shallow-clones the UText while retaining underlying text. Closing the temporary UText does **not** release that backing-text obligation. Choose either:

- an owned, stable provider buffer retained by the session; or
- a lifetime-bound borrowed session whose pool return/reset contract prevents subsequent access after the borrow ends.

The current trait’s short-lived `&[u16]` argument does not express a retained native session lifetime. `Rc<RefCell<P>>` prevents Rust transfer across threads, but does not establish native pool ownership or cleanup correctness.

Retain native pools initially:

- Line pool: thread-local, keyed by requested locale, capacity four with oldest cached entry evicted; invalid nonempty locale falls back to current text-break locale ([pool lines 45–113](C:/Users/jackv/.t3/worktrees/Rustium/t3-8e793e24-85c588c23fadd439/docs/rust/migration/drafts/blink-text-boundaries/frozen/text_break_iterator_icu.cc:45)).
- Character pool: thread-local, unkeyed, LIFO. Its `Vector<…,4>` is inline capacity, **not an eviction limit** ([character pool](D:/Rustium/third_party/blink/renderer/platform/text/character_break_iterator.cc:41)).

Rust owns lazy acquisition/release decisions; native retains physical pool implementation. A later pool migration is a separate explicit ownership change.

**Source-derived acceptance fixtures and traces**

Preserve both native assertion paths: exhaustive `IsBreakable(i)` and forward enumeration with increment after each returned boundary ([test helpers](C:/Users/jackv/.t3/worktrees/Rustium/t3-8e793e24-85c588c23fadd439/docs/rust/migration/drafts/blink-text-boundaries/frozen/text_break_iterator_test.cc:23)). A few sampled `next()` calls do not replace them.

The existing test source supplies these exact expectations:

| Original group | Required expectations |
|---|---|
| Basic / Newline / Tab | `{2,5,6}` / `{2,5,8,9}` / `{2,5,6}` |
| LatinPunctuation | Normal/KeepAll `{5,8}`; BreakAll `{2,5,6,8}`; Character `1..8` |
| Chinese | Normal/All/Character `1..5`; KeepAll `{5}` |
| ChineseMixed | Normal `{1,4,5,7,9,10}`; All `{1,4,5,6,7,9,10}`; Character `1..10`; KeepAll `{1,4,9,10}` |
| ChineseSpaces | Normal/All/KeepAll `{3,6,9,10}`; Character `1..10` |
| Isolated emoji family / modifier | `{11}` / `{3}`, across all four tested modes |
| Emoji family in text | Normal/KeepAll `{4,16,19}`; All `{1,2,4,16,17,18,19}`; Character `{1,2,3,4,15,16,17,18,19}` |
| Emoji modifier in text | Normal/KeepAll `{4,8,11}`; All `{1,2,4,8,9,10,11}`; Character `{1,2,3,4,7,8,9,10,11}` |
| Phrase, Japanese source strings | `{3,5,10}` and `{4,8,12,19}` |
| Ethiopic / loose hyphen | `{2,4,5}` / `{1,2,3,4}` |
| SoftHyphen enabled / disabled | `{3,6,9,12,14}` / `{9,14}` |
| Both high-Latin hyphen cases | `{6,11}` |

These are **native source assertions**, not results observed against the current ICU runtime.

Also retain unchanged:

- Empty and null `IsBreakable(0)`, and end queries, across all five modes.
- Strictness’s six output/locale transitions: `(1,"ja")`, `(2,"ja@lb=strict")`, twice `(2,"@lb=strict")`, twice `(1,"")`.
- U+2019 enabled/disabled equality across five modes × three locales × four strictnesses.
- Both pool identity/reuse assertions.
- Every grapheme-length and cluster-list assertion at test lines 273–353.
- WordBreakSwedish: English `following(0)=1`; pinned ICU ≥77 branch Swedish `=1`. Keep this as native-provider coverage, outside the line-policy rewrite.

Add deterministic **policy traces**, without inventing ICU boundaries:

- `"a b  c"` with a provider that must not be called: native whitespace/ASCII logic yields `{2,5,6}`.
- `"  a"`: AfterRun first opportunity `2`; EverySpace first opportunity `1`.
- `"a\u3000b"` under EverySpace: first opportunity `2`, independent of provider.
- `"a b"`, start `2`, query `2`: pre-start space must not create a boundary at `2`.
- Character mode over `"ab"`, query `(0,limit=1)`: requires `following(0)`, never accepts a provider’s zero boundary.
- Character mode over CRLF, limit `1`: native returns `2`; do not clamp.
- BreakCharacter results remain unaffected by space/soft-hyphen settings.
- Hyphen-digit traces distinguish `"-1"` from `"A-1"`; resetting start to the hyphen removes the preceding `A` context.
- Unknown-path transcript: `following(i-1-start)`, mapped soft-hyphen result rejected, next `following(previous_result-start)`, then `Done`.
- Malformed provider offsets, error/retry, repeated start reset, phrase transitions, strictness transitions, nested sessions, release, and thread isolation.
- Valid pair, isolated lead/trail, reversed pair, and starts/limits inside pairs. For malformed Unicode, assert local indexing/property-call traces; obtain final ICU boundaries from the pinned provider later.

**Freeze and remaining ownership**

The four existing frozen hashes match the README. The inspected native text/helper files showed no diff against base `b4681bf483649cdca320952d59d2c31719b338b6`. Before implementation, extend the independent frozen closure with:

- `character_break_iterator.{cc,h}`, `layout_locale.{cc,h}`, `layout_locale_test.cc`.
- `text_break_iterator_internal_icu.{cc,h}`, `icu_error.h`.
- Relevant `character.h`, `unicode.h`, `ascii_ctype.h`, character-name and ICU UTF-16/property definitions.
- Generated `break_iterator_data_inline_header.h`, its `LineBreakData` generator, and generation/build inputs.
- U+2019 runtime-feature definition/default and test scope.
- Default-language resolution and thread-local pool support.
- Direct caller references: `core/layout/inline/line_breaker.{cc,h}`, `platform/fonts/shaping/shaping_line_breaker.{cc,h}`, and their relevant tests. Actual consumers exist beyond the candidate’s sparse checkout: style changes set every policy field; layout resets/restores start offsets; shaping uses forward and reverse queries.

The configured generated table currently spans `0x21..0xFF`; observed SHA-256 is `a32fc28f5ffcca51e0aad5d586df4361b321906f5428b0fd0e75e391724e540b`. ICU source remains the requested `b09986242cfb165b9111ade98be6b042ef973b86`; its header identifies Unicode 17.0, while explicitly requiring runtime data inspection to establish the loaded Unicode version.

Director ownership remains: ABI/GN integration, native opaque handles and pool retention, locale/default-language bridge, provider error compatibility, backing-text and destruction order, actual loaded ICU/data binding, and platform validation. Windows uses Chrome UI language for this default, explicitly distinguished from OS UI language ([locale source](D:/Rustium/third_party/blink/renderer/platform/text/text_break_iterator_internal_icu.cc:31)). Word/sentence segmentation, ICU rules/dictionaries, public grapheme helpers, and caller layout/shaping behavior remain native debt. Completion of this brief would establish a local-policy candidate behind a documented provider—not subsystem cutover.
