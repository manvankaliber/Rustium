**NET-RECOVER disposition: failed historical run; repairs and validation remain pending.** Read-only intake completed. No files, queue, configured inputs, builds, tests, benchmarks or campaigns were changed or run.

**Evidence identity.** Let `P` denote this exact [run directory](D:/Rustium/.scratch/net-wave5/resumed-integration/native-runtime-delivery-v2/parent-http2-ordinary-20261114T003801945838Z). SHA-256 values independently read during intake:

```text
P/ordinary-conductor-result.json
f572700436c173c9aa2bb525c9d6817896fb9a1c09e57ebae52d62cb57fa5cb5
P/ordinary-execution/result.json
78fe9e67d9cd815e931ddcc5de6bdfcaa5d25369f580edea4b0c3b8b993145b6
P/ordinary-execution/net_unittests/result.json
6939897be176a94147970c6751f062415ae59da597ee57f2e09529d9b87eabff
P/ordinary-execution/net_unittests/summary.json
871a289907dc82072d150bf4dde5b638eafad6172eae57d1aad81071aee885be
P/ordinary-execution/net_unittests/gtest.json
621debb6bbe7d7f0b332d8ca17143cbe31f9a3e2e0c9b2294f152e64ea97abaa
P/ordinary-execution/net_unittests/output.log
da8c988a804530aa98d5130b5d799a29b482e972e35bb5be43676b90ddb151c0
P/snapshot/input-manifest.json
9f857b8a89a3cc260f941b4f8cc0808497da4918c3986e2cb08e7c84311a6796
```

**Execution and counts.**

- Conductor exit **1**, elapsed **1,311.456 seconds**, finished `2026-11-14T01:28:29.557774Z`.
- Only `net_unittests` executed: exit **1**; 33,659 listed tests, including 57 disabled.
- Summary contains 33,602 entries: **33,531 SUCCESS**, **9 CRASH**, **5 FAILURE**, **57 NOTRUN**. SUCCESS includes 111 native skips: **33,420 passed**.
- The 57 NOTRUN entries are distinct from the 57 disabled tests; examples include cookie tests left without results after child crashes.
- `gtest.json` actually contains **XML**, with attributes `tests=33602`, `failures=5`, `errors=66`. Its errors count must not be reported as 66 crashes.
- **16 selected HTTP2 programs never executed:** one Rust policy program (historical expectation 28 tests), seven native unit-test programs, and eight verify-only programs covering **69 planned workloads**.

Unexecuted native suites: `spdy_buffer_rust_unittests`, `http2_stream_family_rust_unittests`, `http2_read_queue_rust_unittests`, `http2_write_queue_rust_unittests`, `http2_priority_rust_unittests`, `header_coalescer_rust_unittests`, `alps_decoder_rust_unittests`. The eight workload programs comprise buffer, stream control, stream I/O, read queue, write queue, priority, coalescer and ALPS. These are conductor omissions, separate from native skipped tests.

**Failure classification and minimal requests.**

| Classification | Exact failures and evidence | Minimal repair / validation request |
|---|---|---|
| **Candidate defect strongly source-supported** | All nine crashes have `EXCEPTION_ACCESS_VIOLATION`, through `RtlTryAcquireSRWLockExclusive → ThreadChecker → CookieMonsterRustNative::Primitive → CookieMonster::GetKey`. | Repair static-context primitive dispatch before dereferencing `monster`; retain instance thread checks. Rebuild affected consumers and rerun all nine failures plus static GetKey coverage. |
| **Environment/harness failure demonstrated** | `FileOperationsInitWithNVSDirTest.InitSucceedsWhenUnneededFilesUndeletable`; `FileOperations/FileOperationsTempDeleteTest.TempSnapshotUndeletable/LegacySubdirectory`. Directory creation reports Windows **0xCE**, “filename or extension is too long”; assertions at source lines 182 and 283. | Give the next uniquely recorded run a short external TMP/TEMP root. Rerun both tests and then the broad suite; preserve this failure. |
| **Candidate-related observation mismatch suspected; behavioral cause unresolved** | `SpdySessionTest.TestYieldingSlowReads`, `SpdySessionTest.TestYieldingDuringReadData`, `SpdySessionTest.TestYieldingDuringAsyncReadData`: expected observer count **1**, actual **0**, at lines 3386, 3534, 3636. | Audit posted-task source location before altering read-loop policy. `FROM_HERE` now occurs inside the bridge lambda; the observer matches a function-name suffix `DoReadLoop`. Capture the location in the enclosing method and pass it into the lambda if confirmed. Retain exact yield-count assertions and verify actual task execution. |

The nine crash names are:

```text
CookieMonsterTest.GetKey
CookieMonsterLegacyScopeTest.DeleteAllAliasCookies
SQLitePersistentCookieStoreTest.LoadingPartitionedCookies
SQLitePersistentCookieStoreTest.SourcePortIsPersistent
SQLitePersistentCookieStoreTest.TestInvalidVersionRecovery
SQLitePersistentCookieStoreTest.TestLoadOldSessionCookies
SQLitePersistentCookieStoreTest_OriginBoundCookies.DeleteCookie
SQLitePersistentCookieStorev24UpgradeTest.UpgradeToSchemaVersion24/0
SQLitePersistentCookieStorev24UpgradeTest.UpgradeToSchemaVersion24/4
```

Frozen source explains the cookie failure: `CookieMonster::GetKey` creates a null-owner context; dispatch operation 2045 requests `Key`/2013 and `EmitKey`/2068, whose native handlers lie after unconditional `*monster` and thread validation. Source SHA-256:

```text
snapshot/configured/net/cookies/cookie_monster.cc
2494e487866a950bcb171b793a40b79287319962e1850f084530fa59eacfc1bd
snapshot/configured/net/cookies/cookie_family_monster_native.cc
61115ff4cbddb96d6a5069f44deb9ebc12a5958ffcd544157f5546a4dfa1ea6b
snapshot/configured/net/cookies/cookie_family_monster_dispatch.rs
9ea05eb6b6c5c13b0a32e84e039f81f455d7c38d6ccedd088925531f0a52e593
snapshot/configured/net/spdy/spdy_session.cc
dd66dcd55d7040442fd0f816a683bde21d4ee84b222cf2a842b9e88782ff68fb
```

No comparison run establishes any failure as **preexisting**.

**Snapshot identity.** Before execution, the recorded check matched the frozen manifest and external Python. After execution, it failed with `Copied data membership changed: …/snapshot/configured/out/Default`, causing the conductor to break before HTTP2 programs.

Current membership comparison finds **15,151 expected files versus 15,152 actual**, with only added `debug.log`, no removed files or added directories. Its contents include launcher “no test result” messages. SHA-256: `8a6cf91f004a24da23d553bed8e069fa0344077918340798bb8f98f1cca6e0a1`. This explains the membership rejection; it does **not** supply a successful post-run content-identity check. Route logging outside the next sealed snapshot; preserve the existing snapshot unchanged.

**Existing staged work to reuse.**

- [Connection functional review v3](D:/Rustium/.scratch/net-wave5/resumed-integration/connection-baseline-functional-review-v3/REVIEW.md) identified missing `Create(nullptr)` and three missing original-H2-pool close/drain sites. [Parent fixes v4](D:/Rustium/.scratch/net-wave5/resumed-integration/connection-baseline-parent-fixes-v4/verification.json) already prepares both corrections in the combined **46-binding intake**: 39 sources plus seven parent proposals. Native execution and application were false in its record. Reuse this package after checking every before hash; do not redispatch implementation. Its `application.json` hash is `5405cc7c5994ff26aa0b2cc9ade42acfb128277faea2c17544fe74e87e3f32fc`.
- Connection validation request: rebuild affected consumers; execute all **23 prepared ordinary tests**, relevant original/production suites, all16 verify-only lifecycles and the separately defined optimized-QUIC phase. Callback lifetime, ABI and optional provider coverage remain open.
- [Corrected timing review v2](D:/Rustium/.scratch/net-wave5/resumed-integration/immutable-timing-corrected-review-v2/REVIEW.md) records **39 passing synthetic tests**, resolving R1–R4 within that scope.
- [HTTP2 timing qualification](D:/Rustium/.scratch/net-wave5/resumed-integration/http2-timing-input-qualification-v1/HANDOFF.md) nevertheless finds the installed helper rejects actual edge-local/inherited dependency bindings, first at `toolchain.ninja:246`; 903 textual assignments were observed. Parent capture was unexecuted. Request actual retained commands/graph/deps and reviewed rule/tool/response/input policies for all eight roots. Resolve the helper limitation through reviewed scoped-graph qualification or a narrow helper correction; retain raw evidence.

**Frozen1221 and tooling1222 remain distinct.**

```text
Frozen1221 source authority:
1753cc6fb55b82a918e866edc89293b14c8b5785731a0708eaa1dd267698bf8a

Tooling1222 authority, also observed in current last-synced.json:
768bf8f97f23d517269fca450285f9dd73e3fb323a31705da2f3360e8d3c3fd0
```

The recorded delta changes the runner from `41dd9874…` to `61a3a1fb…` and adds helper `a6187ed3…`. Rebinding proposals are provenance work, not validation of the older failed runtime snapshot.

Acceptance remains blocked on repairs, fresh source/configuration bindings, affected-consumer rebuilds, complete ordinary execution and successful before/after identity checks. No current pass, performance acceptance, publication approval or final cutover is established. Sanitizer/fuzz/security campaigns remain OFF.
