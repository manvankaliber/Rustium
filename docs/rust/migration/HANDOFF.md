# Active Network and dedicated compilation - 2026-11-17T16:55:33.399787+00:00

## User policy and current goal

Network is the sole active subsystem; preserve held non-Network artifacts.
Director Sol/xhigh; coding workers and independent QA Terra/xhigh, max8 global
coding reservations. Do not recreate the existing migration goal. The latest
usage observation was 54% used and 46% weekly remaining. Sanitizer/fuzz/security
campaigns remain off. Ongoing scoped branch pushes to public GitHub are
authorized; automatic merges and releases remain outside that authorization.

ALL COMPILATIONS now run on linked laptop HQC-VAN_KALIBER with 4 to 8 Ninja jobs (default `-j4`) and one simultaneous link, including cold builds, QUIC, reference probes and fresh qualification. Desktop is editing
and targeted tests that do not compile. Cargo/Rust tests requiring a build must
compile remotely. The user rejected the local-j2 trial; do not run it or restart
the archived local build. See AGENTS, OPERATING and CI-RUNNER.md.

## Current family and feedback order

PR #1 is at `f27ce55cc1e84f8848583d679d7224e21d28e483` with the [frozen Digest reference](evidence/connection-jobs-digest-reference-integration-f27ce55c.json). The `a6028111` precursor added [frozen Basic reference parsing](evidence/connection-jobs-basic-reference-integration-a6028111.json). The `557c24af` precursor added [transport assignment archive retirement](evidence/connection-jobs-assignment-retirement-integration-557c24af.json). The `443551ed` precursor added [paired HostPortPair resolver and probe validation](evidence/connection-jobs-host-port-probe-integration-443551ed.json). It retains [Rust preconnect claim accounting](evidence/connection-jobs-preconnect-claims-integration-295159ef.json) and the reviewed and tested [authoritative Rust request queue](evidence/connection-jobs-request-queue-integration-f2e45f1e.json), following the earlier [assignment decisions](evidence/connection-jobs-assignment-integration-54f6f4df.json) and [stalled-group ranking](evidence/connection-jobs-stalled-group-integration-80374a37.json). The earlier qualification receipts below apply to `72ee5ddd7129a311a92b49f3705a031e68cbf4ca`. The [optimized components acceptance receipt](evidence/connection-jobs-optimized-components-laptop-72ee5ddd.json) records a passing study-filtering object and a passing warm `components_unittests` build. All 83 selected tests passed, including ten proxy preference cases, with zero skips and retries. The laptop restored its previous checkout and independently verified lock release.

The [fresh consumer build](evidence/connection-jobs-fresh-consumer-laptop-72ee5ddd.json) passed on the same head under the approved input digest. All 96 recorded roots resolved to 229 outputs. All 9,979 Ninja actions passed, and the before and after input objects and Ninja graph records matched. The laptop kept the clean pinned checkout and independently released its lock.

The [fresh Connection Jobs runtime](evidence/connection-jobs-fresh-ordinary-runtime-72ee5ddd.json) recorded 2,570 family passes, 34 skips, 23 Rust core passes and 52 verify-only workloads. The fresh component owns the idle Rust crate and exports all five policy projections imported by the test executable.

The [optimized QUIC supplement](evidence/connection-jobs-optimized-quic-runtime-72ee5ddd.json) passed 16 verifier workloads and 16 paired lifecycle cases on the same head. All six ordinary QUIC skips passed in the optimized output, including two among the lifecycle cases. The 28 ordinary dual-race skips have passing enabled counterparts.

The [bounded ABI result](evidence/connection-jobs-fresh-abi-strings-72ee5ddd.json) reconciled all 84 current C++ object dependency records in the reference target and its five executables. The eight Strings and three private auth-cache symbols were present at the expected export and import boundaries. The [accepted bounded review](evidence/connection-jobs-bounded-abi-acceptance-72ee5ddd.json) reconciles the legacy manifest's 119 aliases and 36 specific outputs against the current inventory and laptop artifacts. All 62 selected original and bridge cases passed with the sealed inputs. The Windows-wide overload and its headers match the archived source bytes. Its runtime result and a prechange export table remain unavailable.

The [Windows consumer runtime acceptance](evidence/network-consumer-runtime-acceptance-72ee5ddd.json) reconciles the full 36-program inventory and all 232 verify-only workload IDs on the exact head. The 30 remaining programs met their completion contracts from the fresh binaries. All 82 historical provisional consumer labels appear in the 96-root current build. The bounded consumer review and ordinary compile-repair artifacts are accepted. The two cookie-store captures had identical structured fixture and checksum records. Runtime qualification retains the limits of the declared source and output closure. Wider platform coverage, native providers, performance and cutover remain separate.

The transport scheduling follow-up is validating under `NET-CONNECTION-TRANSPORT-SCHEDULING`. Terra froze the original at `fc2d429f` and delivered the patch packet at `157c435f`. The [director candidate](evidence/connection-jobs-stalled-group-candidate-712ca01f.json) `712ca01f` applies the verified materialization on `72ee5ddd`, imports the stalled-ranking oracle, fixes differential fixture indexing and adds a real pool tie/saturation case. Twelve exact test names matched the source. The earlier idle policy and request-to-job assignment bodies are byte-identical to the base. The [independent source review and laptop dispatch](evidence/connection-jobs-stalled-group-review-712ca01f.json) clears the published candidate for warm ordinary Windows component feedback. The first three cases cover ranking and native tie order, followed by thirteen priority, scheduling, auth, callback and timer cases. Ranking, assignment, queue order and aggregate preconnect claims are on the PR. The named Windows family suite and configuration supplements have passed with the recorded skip reconciliation.

Published test-only successor `80374a37061121a5b3e5d6f82311f857a38cf29d` adds `ClientSocketPoolBaseTest.StalledGroupExcludesAuthBoundSlot`. The [independent source review](evidence/connection-jobs-stalled-group-auth-review-80374a37.json) confirmed competitive ranking with an occupied auth-bound slot and cleared the fixture for focused testing. The [initial laptop run](evidence/connection-jobs-stalled-group-laptop-712ca01f.json) passed all 16 cases, scoped GN includes and the DLL export/import check at `712ca01f`. The warm build took 190.5 seconds and completed 15 actions after regeneration. The [auth follow-up](evidence/connection-jobs-stalled-group-auth-laptop-80374a37.json) passed the added case and all three gate tests at `80374a37`. The two links ran serially, with a 4,192 ms gap. The earlier warning was a count of total actions. Exact post-regeneration graph hashes were unavailable; the [integration record](evidence/connection-jobs-stalled-group-integration-80374a37.json) binds the retained timing evidence.

`NET-CONNECTION-TRANSPORT-REQUEST-ASSIGNMENT` is validating. Terra delivered `b4435e70` on `rust/connection-transport-request-assignment-20261121` and released the isolated worktree. The [verified assignment candidate](evidence/connection-jobs-assignment-candidate-4304d7e0.json) applies ten scoped files at `4304d7e0`, retains the auth fixture on the parent, and replaces iterator batching with Rust front, donor and release choices from native queue counts. The [independent source review](evidence/connection-jobs-assignment-review-4304d7e0.json) cleared the exact candidate, published on its scoped branch. The [warm laptop assignment result](evidence/connection-jobs-assignment-laptop-4304d7e0.json) passed scoped GN and build, all 16 cases, and both wrapper exports/imports. All five links used a depth-one pool and their times did not overlap. Native queue order, reprioritization, preconnect accounting and lifecycle remain.

The published one-file test successor `54f6f4df827d30db2762557c7442056cc8a3a0a4` extends assignment coverage with two distinct preconnect jobs and explicit priority propagation. [Independent review passed](evidence/connection-jobs-assignment-job-order-review-54f6f4df.json). The laptop passed the new case at `54f6f4df` with each feature switch visible in child argv, and all five assignment gate cases. The warm build took 537 seconds and performed 142 actions. Two links ran serially. The reviewed assignment milestone is on PR #1.

`NET-CONNECTION-TRANSPORT-REQUEST-ORDER` was implemented in the reused exclusive T3 checkout on `rust/connection-transport-request-order-20261121`, at packet base `b4435e70` and production base `54f6f4df`. The [first request-order packet intake](evidence/connection-jobs-request-order-intake-dd87e538.json) verifies all nine proposed files and preserves the independent original oracle. The policy-only patch leaves the queue in C++ and adds two FFI crossings. The [standalone queue prototype](evidence/connection-jobs-request-queue-prototype-60adca89.json) at `60adca89` implements Rust order and a paired trace. It requires a second allocation across the existing native reassignment step. The [retained-node successor](evidence/connection-jobs-request-queue-retained-45f14af5.json) at `45f14af5` keeps the boxed Rust node and generated token while detached, then relinks without allocating. Its opaque ABI and native owner map are proposals. The worker implemented the production Group patch against `54f6f4df`. The [independent core and ABI review](evidence/connection-jobs-request-queue-abi-review-45f14af5.json) confirmed the retained order and count rules and required initialized token storage for the snapshot slice, or an output writer that accepts uninitialized storage. The worker applied that fix in the [production Group candidate](evidence/connection-jobs-request-queue-candidate-50a5a8c1.json). All nine materialized files at `50a5a8c1` match packet `cfc7132c` on `54f6f4df`. The worker released the isolated slot. The [review and laptop feedback dispatch](evidence/connection-jobs-request-queue-feedback-4685fd40.json) records exact successor `4685fd40`, with the warnings-as-errors repair, enabled live Rust unit tests, restored thread diagnostics and a linked Rust/C++ layout probe. The [first laptop attempt](evidence/connection-jobs-request-queue-gn-discovery-4685fd40.json) passed both GN checks and stopped before compilation on an unsupported GN wildcard. The retry uses the exact generated Rust test label, `net_unittests`, `-j4` and one simultaneous link on the same warm output and candidate. The [reviewed and repaired dispatch fixture](evidence/connection-jobs-request-queue-dispatch-review-7f88361a.json) is on its own published branch at `7f88361a`. It adds synchronous dispatcher-start coverage during a priority change, drains posted completion and clears mock waiters on early assertions. The GN retry resolved both exact targets and reached the compiler, which stopped at a dead return after `NOTREACHED()` in `Group::SetPriority`. The [one-line compile repair](evidence/connection-jobs-request-queue-compile-repair-f2e45f1e.json) at `f2e45f1e` passed the failed-object replay in 34.8 seconds and the two scoped targets in 495 seconds, then all five Rust and 19 C++ cases. The independent check corrected the export parser mismatch, verified both DLL boundaries and the linked Rust archive, and confirmed checkout and lock restoration. The exact tested commit is now in PR #1. The [retained queue pool regression](evidence/connection-jobs-retained-pool-runtime-f2e45f1e.json) ran from those exact binaries without building. All 218 passed in 7.409 seconds with zero skips, retaining every prior case and adding two reprioritization cases.

The [local preconnect claim successor](evidence/connection-jobs-preconnect-claims-candidate-9a136b6f.json) `9a136b6f` moves one-time aggregate preconnect accounting to the same Rust queue and removes the native counter. Independent review found no counter or ABI defect and required targeted regression coverage. The [published test successor](evidence/connection-jobs-preconnect-claims-feedback-60105b29.json) at `60105b29` adds three actual pool cases for cancellation, failure/flush, and proxy auth, plus the Rust invalid-release and waiter/return case. All seven Rust cases passed. The Windows native link failed because the inline C++ test accessor referenced a private Rust symbol across the DLL boundary. The [linkage repair](evidence/connection-jobs-preconnect-component-repair-5932850e.json) at `5932850e` moved the accessor into `net.dll`, passed the Windows component boundary and all seven Rust tests, and passed the cancellation case. The failure/flush fixture expected false where existing active sockets contribute OK to the maximum result. The [test-only repair](evidence/connection-jobs-preconnect-result-repair-295159ef.json) at `295159ef` keeps the count and callback sequence checks. The [first replay preflight](evidence/connection-jobs-preconnect-replay-scope-295159ef.json) stopped on dry-run regeneration without compiling. The [actual warm replay](evidence/connection-jobs-preconnect-claims-integration-295159ef.json) built in 513 seconds, passed nine individual checks and all 221 pool cases, and verified the C++/Rust DLL boundary. PR #1 now includes the exact tested commit `295159ef`. The [named Connection Jobs attempt and retry](evidence/connection-jobs-named-family-retry-295159ef.json) records the earlier scope stop after 19 completed actions, with no compiler error or tests. The [successful ordinary family retry](evidence/connection-jobs-named-family-runtime-295159ef.json) built 68 actions and passed 2,570 tests with 34 configuration skips, zero retries or failures. The [optimized QUIC supplement](evidence/connection-jobs-quic-supplement-295159ef.json) passed all six QUIC-only cases and confirmed matching passing enabled counterparts for all 28 dual-race ordinary skips. No skip remains unreconciled. The [source-bound standalone comparison](evidence/connection-jobs-retained-queue-trace-295159ef.json) at packet `49412f30` matched all 18 C++ and Rust trace records and passed the allocation probe for Rust detach/relink. The [current CI source recheck](evidence/network-ci-planning-recheck-295159ef.json) passed 18 data-only tests and all 18 entrypoint pins. An independent review is reconciling earlier provider/reference and probe evidence against `295159ef`. The [lifetime audit](evidence/connection-jobs-request-queue-lifetime-audit-54f6f4df.json) traces the original ownership and callback order, including synchronous resolver dispatch start with asynchronous completion. The original Group transaction assumes native ownership survives priority changes. Arbitrary custom-provider mutation or deletion remains outside the reviewed contract. The first runtime-only preflight verified the existing `54f6f4df` executable and DLL hashes and stopped before discovery because the prior receipt lacked a baseline for some dependencies. It is preserved at `C:/Users/micro/rb/network-54f6f4-transport-runtime-preflight-20261121a`. The [scoped runtime retry](evidence/connection-jobs-transport-pool-runtime-54f6f4df.json) verified both exact candidate binaries and stable current runtime fingerprints. All 216 selected tests passed, with zero skips and no rebuild. The next queue validation should include `PrioritizedDispatcherTest.ChangePriority` for immediate dispatch and `HostResolverManagerTest.ChangePriorityAsync`, `HostResolverManagerTest.ChangePrioritySync` and `HostResolverServiceEndpointRequestTest.ChangePriority` for underlying resolver sequencing. Those separate cases do not replace actual pool reprioritization coverage.

At parent `dc811953201f580f575653242de04468e4d61fd8`, the [focused proxy and Feed laptop result](evidence/connection-jobs-proxy-feedback-laptop-dc811953.json) records both GN checks, three Feed object builds and all ten `PrefProxyConfigTrackerImplTest.*` cases passing. Broad acceptance passed the previous Feed failure and stopped after 500 seconds on a stale task-type constant in an annotation-index test. The [repair and replay record](evidence/connection-jobs-annotation-index-feedback-c4058976.json) binds the raw failure and the one-line candidate `c4058976`.

The warm replay built the annotation-index object at `c4058976` in 80.328 seconds. The umbrella build stopped after 1,013.484 seconds in the filter suggestion generator test. The same commerce cleanup had deleted the test task type and 13 local annotation fixtures while leaving their uses. Candidate `1c843c0c` restores the fixtures and their original task-type check using a generic fixture type. Both repaired objects passed a single focused build. See the [multistep filter repair record](evidence/connection-jobs-filter-suggestion-feedback-1c843c0c.json).

The umbrella at `1c843c0c` stopped after 659 action lines because Merchant Trust tests still called the removed protobuf validation API. Candidate `e41c0f7` removes the retired validator test from the source set and aligns the service cases with the remaining empty-response callback and interaction metrics. Its GN check and service object passed. The umbrella then stopped after 808.250 seconds on six Personal Context fixture calls to a removed helper that preprocessed to Win32 `CreateServiceW`. Candidate `16de62f` restores the helper using the current eligibility service constructor and a distinct name. Its object replay passed in 84.383 seconds. See the [Personal Context feedback record](evidence/connection-jobs-personal-context-feedback-16de62f.json).

The umbrella at `16de62f` stopped after 904.753 seconds in `session_proto_db_unittest.obj`, whose fixture depended on a removed commerce proto. The [SessionProtoDB feedback record](evidence/connection-jobs-session-proto-db-feedback-d08074af.json) binds candidate `d08074af`, which keeps the existing cases and both distinct message types using the component's test schema. The scoped replay passed in 106.531 seconds. The umbrella ran 1,541 action lines with Ninja `-k 6` and reported one failing edge, the Variations study platform assertion.

Candidate `72ee5ddd` accounts for reserved Fuchsia enum value 7 alongside reserved values 8 and 9. A source comparison confirmed that the fixture lists all eight declared enum values exactly once. The [study filtering feedback record](evidence/connection-jobs-study-filtering-feedback-72ee5ddd.json) binds the repair launch that produced the passing umbrella result.

At parent `8a5e79b466161a70edd05f98edb74dc6015dc97a`, the [optimized QUIC laptop result](evidence/connection-jobs-quic-success-laptop-8a5e79b4.json) records separate passes for the original and production successful tunnel cases and four existing RFC v1 socket and datagram cases. Both job cases use the same native QUIC session and socket provider.

The larger optimized components build at `8a5e79b4` stopped while compiling `feed_api_notice_card_unittest.obj`. A platform cleanup dropped the include that declares `feed::Experiments` and left its renamed translator and shared preferences implementations out of the Feed build. The [repair and laptop receipt index](evidence/connection-jobs-proxy-feedback-dc811953.json) records zero proxy preference cases run in that attempt and independent lock release. Commit `a7212a5d` repairs the Feed dependency and successor `dc811953` adds the standalone `//components/proxy_config:proxy_config_unittests` feedback target.

At parent `d106f5634f65abf5004aa990baf657e79465e90c`, the [warm reference result](evidence/connection-jobs-reference-laptop-d106f563.json) records both exact resolver and Happy Eyeballs comparisons passing in order. The full family suite passed 2,570 cases and skipped 34 of 2,604 with zero retries or disabled cases. The laptop restored its earlier checkout and independently verified lock release. The optimized QUIC candidate `11a7c1cc` passed GN include checking and failed compilation on 13 Chromium style checks in the shared fixture. The [published repair](evidence/connection-jobs-quic-fixture-style-repair-011bcc5f.json) `011bcc5f` moved the constructor, destructor and 11 unchanged virtual bodies out of line. Its focused replay rebuilt the shared fixture and passed GN checking. [Successor `3c7c4c05`](evidence/connection-jobs-quic-tunnel-object-repair-3c7c4c05.json) fixed the downstream QUIC test object's missing counter storage and matcher declaration.

At `3c7c4c05`, the optimized tunnel candidate built successfully, passed GN checking and passed four existing RFC v1 socket and datagram cases. The exact original tunnel case failed because the fixture omitted the expected priority update before CONNECT, so the production case was unrun. The [first packet repair](evidence/connection-jobs-quic-priority-packet-repair-a1a6a528.json) produced padding because the existing packet maker defaults to nonincremental priority. The [incremental-priority successor](evidence/connection-jobs-quic-incremental-priority-8a5e79b4.json) `8a5e79b4` uses one dedicated packet maker with incremental priority and strict, separate control and CONNECT packets.

At parent `76992d0e`, the [GRIT laptop replay](evidence/connection-jobs-grit-laptop-76992d0e.json) passed 22 actions and the focused Python case. Corrected verification decompressed the pack and confirmed one copy of the exact minified input. The depfile names the same input.

At parent `4b1e24848d1080edce5e9bad8e27372760674736`, the [warm idle validation](evidence/connection-jobs-transport-idle-laptop-4b1e2484.json) rebuilt the original failing object and `net_unittests`. The direct case, eight ordered cases, two remaining differential cases, and all 308 pool tests passed. The family suite recorded 2,568 passes and 34 skips with zero retries. The five component projection symbols exported from `net.dll` and imported by the unit executable verified. Rust now owns the reviewed idle-socket decisions on the PR. Native socket ownership and request scheduling remain under Connection Jobs. Fresh consumer, platform and performance evidence remain open.

The optimized QUIC retry on PR `4b1e2484` failed at GRIT action 7,724 of 26,758 before proxy preference tests could run. The [isolated replay and candidate record](evidence/connection-jobs-grit-path-repair-18f8c741.json) captures the ordered bundle/minify producers and successful reads of the 6,495-byte resource through normalized paths. The shorter-path repair `18f8c741` passed all 27 data-only HTML inlining tests. Its dry run planned 22 actions and stopped before execution on the initial one-action limit. The [reviewed successor](evidence/connection-jobs-grit-mode-review-76992d0e.json) `76992d0e` restores the two original executable modes with the same script contents and passed the laptop replay above.

The exact `a4bcf5eb` reference test delta was published at `d106f563` on `rust/connection-jobs-reference-grit-validation-20261120` and subsequently passed the warm checks above.

The [successful QUIC job candidate](evidence/connection-jobs-quic-success-candidate-6757e9b5.json) extends `a4bcf5eb` with two RFC v1 CONNECT tests that use fresh native packet fixtures and inject a real session handle. Both jobs open their own tunnel stream. Source review found a GN include ownership gap at `6757e9b5`. Published `4bb3d1b5cb47122cd93ab631c67199e47e56bab0` on `rust/connection-jobs-quic-success-reference-20261120` [isolates the new test and its workload header](evidence/connection-jobs-quic-gn-review-repair-4bb3d1b5.json) under the QUIC flag. Later revisions passed GN include checking and the six focused optimized cases described above. The wider native QUIC provider boundary remains open.

The independent fixture review identified starting points for three remaining Connection Jobs comparisons: resolver result and alias consumption, the Happy Eyeballs v2 two-connector race, and successful QUIC proxy session acquisition. The [fixture intake](evidence/connection-jobs-reference-fixture-intake-df7a5e64.json) pins 37 source and build inputs at `df7a5e64` and records the provider boundaries. The matched comparisons and helper-closure audit remain open under `NET-CONNECTION-REFERENCE-INDEPENDENCE`.

The [resolver reference candidate](evidence/connection-jobs-resolver-reference-447d37a9.json) is retained locally at `447d37a92273b5b0f761c13c3beb4195d54e5071`. It adds an exact differential test for asynchronous endpoint and alias consumption, resolution failure, and pending request cancellation. Its one-file diff passed static checks. The mock resolver retains shared DNS-name validation, and the native result, independent source review, Happy Eyeballs v2 race and successful QUIC comparison remain open. Keep the candidate separate from the queued `dd17ccc60e` idle-policy validation.

On 2026-11-20 Terra recovery on laptop thread `mcp:launch:92c0a64defb0350eece2301b2d3e2f8c5b808bcf8e306bc7de45828a489fd65e` recovered the successful 84-action font-copy run and an incomplete components log ending at action 5,688 of 47,440. The exact `df7a5e64` warm font retry reported zero remaining actions. PR #1 advanced to `df7a5e64f112b86c8b2292630649aecedcc313bb`, whose only change restores the original test-font notice. The previously reported PR checks are on `bb1359ebe9`. The new `components_unittests` dry run passed and resolved 41,752 remaining Ninja actions. The build used four Ninja jobs and one simultaneous link.

The QUIC components retry terminated at action 15,074 with exit 1 and released the native lock. `ISimpleDOMDocument` and `iaccessible2` failed MIDL comparisons on the recorded source path in generated comments. The proxy preference suite did not run. The [failure and repair record](evidence/connection-jobs-quic-midl-failure-df7a5e64.json) pins the full build logs and first reported failure. Both [isolated baseline edges](evidence/connection-jobs-midl-edge-repro-df7a5e64.json) reproduced the comment-only difference and released the native lock. The [reviewed one-file repair](evidence/connection-jobs-midl-repair-review-2c61fa63.json) `2c61fa63` passed 44 data checks and is published on `rust/network-nested-output-midl-20261120`. Terra owns the exact repaired-edge replay under a new laptop reservation. Reconcile its receipt and explicit release before changing the laptop checkout.

The published integration candidate `rust/connection-transport-idle-fonts-validation-20261120` at `dd17ccc60e2a1498298ca3fbc62578ff9fc57aa6` merges `df7a5e64` and `67907d09`. A diff against either parent verifies the other parent's exact reviewed inputs without content changes. Run the ordered idle-pool cases on the ordinary output after the focused MIDL repair releases the native lock. All 18 data-only CI planning tests passed from the exact `bb1359ebe9` driver, inventory and focus blobs. The 36-program, 232-workload inventory retains 96 roots and 18 entrypoint pins; all 1,326 archived input objects verified. Eight deliveries remain in validation. The [transport scheduling baseline](evidence/connection-jobs-transport-scheduling-baseline-bb1359.json) pins 19 original inputs and nine existing case names for future request-to-job assignment work in the same family.

Both repaired MIDL edges passed on `2c61fa63`, with eight declared outputs and the extra iaccessible2 TLB byte-identical to their baselines. The laptop released the native lock, and PR #1 advanced to the tested revision. The [laptop acceptance record](evidence/connection-jobs-midl-laptop-2c61fa63.json) binds the receipts and tracks the remaining conventional-layout, dynamic-GUID, and optimized components checks. The published combined head `86550d262c8f1c49dfe2c2ea3814271b282fbabb` on `rust/connection-transport-idle-midl-validation-20261120` preserves the exact reviewed idle and MIDL files. Terra now owns the serial warm idle-pool build and direct-destruction case first. Resume optimized components validation after that assignment releases the laptop.

The [resolver review repair](evidence/connection-jobs-resolver-review-repair-dc8a79e5.json) `dc8a79e5` checks callback aliases and the raw timeout diagnostic, fixes the mock socket rule lifetime, and adds cancellation after the resolver queues work. The shared resolver provider boundary and focused native check remain open.

The initial `86550d262c` idle-pool build compiled the Rust crate and linked `net.dll`, then failed on raw-array indexing in the C++ fixture before any idle cases or ABI checks. The [one-line std::array repair](evidence/connection-jobs-transport-idle-fixture-repair-4b1e2484.json) was applied at `4b1e2484`, whose warm checks are recorded above.

The [Happy Eyeballs v2 candidate](evidence/connection-jobs-happy-eyeballs-candidate-9e1aca35.json) `9e1aca35` pairs the original and production TCP job race at the original fallback time. Both address families win separate deterministic runs. The test compares published peers and service endpoints, losing socket teardown, and the completion count. The eight exact fixture/oracle source pins and shared providers are recorded. Source review and native validation remain open.

At `bb1359ebe90ffbefb5f67a85bdf001f44415de3d`, both PR planning and focused warm-cache checks passed. HQC-VAN_KALIBER ran the exact HTTP/2 regression twice with zero Ninja actions and one selected Rust case. Both Python environments reused their pinned caches. The sign-in supervisor and listener remained unique through a repeated installation. The laptop receipt is `C:/Users/micro/.t3/projects/laptop-buildvoorbereiding/bb1359-test-receipt.md`.

The full ordinary Connection Jobs native suite recorded 2,568 passes and 34 skips out of 2,602 discovered, with zero disabled, unrun or retried cases. All 52 ordinary verify-only workloads completed in three programs. The separate Rust core suite passed all 23 cases. The dedicated optimized-QUIC output completed all 16 verifier workloads and all 16 exact differential cases with zero skips and retries. The warm graph and scoped target builds reported zero actions with four Ninja jobs and one concurrent link. Receipt locations and scope are recorded in [the warm validation record](evidence/connection-jobs-warm-validation-bb1359.json).

The broader optimized-QUIC run at `bb1359ebe9` passed 44/44 proxy value tests and 2,458/2,458 net consumer tests across the 18 requested suites. There were no skips or retries. `components_unittests` stopped at Ninja dry run with missing `third_party/test_fonts/LICENSE`. No components cases ran. Prior evidence and lock release are recorded in `C:/Users/micro/rb/network-bb1359-quic-consumers-20261119a/receipt.md`. The test-font notice was deleted by license consolidation and remains an input to the font-data copy required by this Connection Jobs consumer. The scoped public task branch `rust/network-quic-test-fonts-20261119` at `df7a5e64` restores the exact original blob. The laptop owns the serial font-copy retry and optimized proxy preference checks. See [the dependency record](evidence/connection-jobs-quic-components-fonts.json). Fresh build qualification remains open.

The exact-head Windows reference probe passed on `bb1359ebe9`. The 14 selected auth/proxy cases, 29 probe header cases and isolated direct ABI counter control passed with zero skips or retries. Scoped Ninja reported no work. All six ownership artifact hashes and the recorded flag-off hashes matched the parent qualification. The operator released the native lock. See `C:/Users/micro/rb/network-reference-bb1359-20261119a/receipt.md`. The independent auth-reference review artifact was accepted at `baf5ec4a` with this follow-up. POSIX, non-Kerberos execution and other reference obligations remain open. See [the review intake record](evidence/connection-jobs-auth-review-intake-bb1359.json).

The parent `0eaf99456e03232b7ed8ad5837c739542f0ffbb1` receipt established the probe headers executable's imports and `net.dll` ownership. The isolated direct call advanced the component counter. It also recorded 28/28 ordinary headers passes and no probe markers in the inspected ordinary graph, bindings and exports. See `C:/Users/micro/rb/network-reference-0eaf99-20261118a/receipt.md`. The earlier `e29dd630` probe stopped at GN generation on a nonempty CXX binding-list reassignment and is archived at `C:/Users/micro/rb/network-reference-e29dd6-20261118a/terminal.json`.

The independent Terra source review of `bb1359ebe9` identified `TransportClientSocketPool` as the production owner of ordinary socket reuse, request scheduling and socket lifecycle. `NET-CONNECTION-TRANSPORT-IDLE` delivered the C++ oracle at `f758ed40b` and the Rust idle-socket policy and paired tests at `fe5d79e2`. The [candidate review](evidence/connection-jobs-transport-idle-review-fe5d79e2.json) verified all 21 original pins and identified missing real-pool tests, redundant ABI crossings, and director-owned GN linkage. The worker's successor `e94513ca` adds six tests using the production pool and routes production and differential tests through the component-owned policy projection. The [wired head](evidence/connection-jobs-transport-idle-repair-e94513ca.json) `62cd081a` passed source inspection of the GN linkage and production fixtures. The director successor `67907d09` replaces selection marker phases with one stateful Rust transition per usable socket. The frozen oracle and executable scripted adaptation have separate hashes in [the repair record](evidence/connection-jobs-transport-idle-selection-repair.json). Independent review cleared `67907d09` for scoped publication and laptop validation. The public task branch `rust/connection-transport-idle-wired-review-20261119` points to that SHA. Run the [ordered laptop checks](evidence/connection-jobs-transport-idle-review-67907d09.json) after the QUIC components repair releases the laptop. No build, runtime or integration evidence exists on the idle candidate yet. The completed Foundation Version checkout was released after verifying it had no ignored or untracked files. Its branch and delivery `5e34605c` remain retained.

On resume, run `python tools/rustium_coord.py check` and `status`. Reconcile the active optimized QUIC proxy consumer assignment and native lock before scheduling another laptop build. The independent non-auth source audit retains the resolver, Happy Eyeballs v2 and successful QUIC session gaps in [its evidence record](evidence/connection-jobs-non-auth-reference-review-bb1359.json). The manual Network workflow still awaits registration on the default branch. Keep fresh qualification tied to the approved input digest and a separate output.

On parent `0c618a57` all three focused Windows checks passed. Normal: the exact regression passed at 42.4 seconds; a separate invocation reported 2,568 native passes, 34 skips, and 23 Rust passes at 51.8 seconds. The three ordinary verifier programs emitted completion markers for 15, 16 and 21 planned workloads in 94.46 seconds. The QUIC-enabled upper verifier printed its 16-lifecycle marker, and the 16 named JobsFamilyTest comparisons (/0 through /15) all passed with zero skips and retries. One Ninja job was used for each QUIC build. The verifier exited successfully before an EOF at the original report gate; the family follow-up completed under its own reservation. The broader proxy/components and ownership qualification remains pending. Quoted times for normal runs exclude preparation/queuing.

Laptop receipts under `C:/Users/micro/rb/`: `network-focus-0c618a-case-20261118a/result.json`, `network-focus-0c618a-family-20261118a/result.json`, `network-focus-0c618a-connection-jobs-20261118a/result.json`, `network-focus-0c618a-quic-jobs-20261118a/upper-receipt.json`, and `network-focus-0c618a-quic-family-20261118a/result.json`.

Connection Jobs is the only family open for new implementation and dispatch. Retain the older multi-family receipt at `C:/Users/micro/rb/network-d228-verifiers-20261118a/terminal.json`. The gclient dependency check launched vpython with a new temporary directory and omitted the previous cache root, creating 200 bytecode files. All 216 normal workload IDs remain unexecuted on that head and 16 need the separate QUIC configuration. Before the next full qualification, set the proven persistent `VPYTHON_VIRTUALENV_ROOT` for the exact gclient spec in the child command environment and confirm reuse in preparation, before spending a validation window on an inventory. The focused runner needs no gclient revinfo. Start with the failed case, run the family suite next, then reconcile the original/reference, optimized QUIC, ABI, consumers, platform and C++ retirement obligations before switching the family. Other existing deliveries keep their place and evidence. See [CI-RUNNER.md](CI-RUNNER.md#fast-repair-feedback).

## Linked builder

T3 environment39b1955f-e0f8-4d4a-ad20-0a3449c1b0eb, Windowsx64, reachable with
full access. Local environment08fa9e90-7ae3-495b-ab28-c65db54bb027 is DESKTOP-VAN-
KALIBER. Codex Terra/xhigh now runs remotely; executed model was verified.
Maintenance projectd2a095b6-7f03-4125-93da-c919d7b874e1 is at
C:/Users/micro/.t3/projects/laptop-buildvoorbereiding. Its Sol host-maintenance
task completed/released ownership; final callback failed runtime-mode escalation,
so director independently read the terminal final. Windows SDK10.0.28000.2270 and
debug tools installed. The later checks reported ATL/MFC installed and the
previously blocked executables working. GitHub CLI2.104.0 authenticates as
manvankaliber. Runner 21 is registered and was observed online and idle.

NET-CI-BUILDER-READINESS owns the dedicated per-user runner and checkout. Runner
installation and hooks-disabled dependency sync passed. Terra recorded the
local tool installations and verified the laptop byte-range lock. See
`evidence/linked-builder-runner-online-20261117.json` and
`evidence/linked-builder-host-toolchain-20261117.json`. Candidate preparation,
workflow qualification and actual compilation remain pending.

Bootstrap commitffa0d0dac57f80d62d2cfb44cbd214acd4100ca4 contains the pinned
official Windows runner2.338.2 installer, manual-only inventory workflow and CI
runbook. Both installer and workflow inline PowerShell parse. No YAML linter,
GitHub execution, publication, PR or native acceptance. Future Network workflow
must retain exact PR source/tool/config identity, all required consumer builds,
ordinary tests and logs; source code cannot depend on machine-local scratch.
tools/ci/network-validation-inventory.json now retains the exact ordered36-program
and232-workload inventory plus96logical GN roots (including3nondefault allocator
labels). Conversion/membership/source-hash checks passed without compilation.
The two cookie entrypoint changes are explicit; controller/clock rebinding remains
pending. This data inventory is not the portable conductor or build workflow;
fresh graph, complete closure and runtime qualification are still required.

## Local build reconciled, no active native job

Current source authority e6ce572ada636ed753132f44ffca42e3c2784d55102fb665fb0cb329d9018e2e:
1270inputs,94application bindings, applied guarded29-file parent package09db0827.
Fresh GN generation and Base/Net header checks passed.77preflight actions passed.
Full build interrupted at495/875, originalexec31296 absent and no launcher/child
or compiler remained. No finished exit/result, no full build or native pass.
Recovery-time1270Root/configured hashes matched; not a completed build postcheck.
Stale gate recovered, no restart. Archive: evidence/network-build-interruption-
20261117/raw-evidence.zip and reconciliation.json. Source/graph prior archive:
evidence/network-current-build-20261117T150542774256Z. Preserve caches and snapshots.

## Prepared Network work and outstanding qualification

Complete46-after-image auth/provider/shared-patch/probe union2402b776fdd2b57d940f52004668ebae189bc6f4
is UNAPPLIED. It verifies40parent proposals and71historical bindings, maps only
recorded director input changes, retains current lifecycle fixtures and applies
shared patches only to isolated copies. Parent auth sourcec36d1aaf and concrete
probe wiring-v2 ee173912 source review984d2f06 is retained in Rootf04597b5.
Review supports teardown/in-flight cancellation repairs and normal proxy graph
single-owner, but actual links/exports, observed-process counter uniqueness,
seven positive controls and runtime/provider/platform evidence remain pending.
Direct test deps on3instrumented Rust libraries require actual symbol inspection.

Harness provenance gatead3f20e387be5bd13d20adf7c3b4ab4aa7b66700 requires trusted
retained copied source/binary/build/ABI/execution/raw-output bindings.9synthetic
tests pass normal/optimized; no actual native capture or timing qualification.
Strict comparator parent09db has7synthetic tests; raw schema success alone is
not source/binary provenance. Store fixture timestamps must remain equal.

Full36-program232-workload ordinary batch is UNRUN at current source.16connection
lifecycles and separate optimized-QUIC phase remain required. Old exact cookie
clock contract must be rebound explicitly in a successor controller/plan after
reviewed output/locale changes; preserve prior controller/contract and evidence.
ABI/compilerdeps captures remain required. Historical native Strings62/62 at
source2c59 and HTTP2 17programs/33886actualpasses/111skips atsource0b237582 remain
historical scopes, not current four-family acceptance or subsystem cutover.

## Coordination and next action

Root branch rust/local-resume-setup, D:/Rustium editing; configured tree at
D:/RustiumBuild/src/out/Default is preserved. Director receipt remains in ignored
.rustium-runtime/director.json; preserve lease. No active coding workers or native
job on the desktop. The remote manager is preparing the public PR on the laptop.
Pending validation is at bound8: prioritize integration/repair, no extra coding
delivery dispatch. Preserve unrelated dirty/deleted paths.

The laptop runner is registered, online and idle at the latest GitHub check.
The resumed dependency sync completed with exit code 0 at 18:31 UTC on
`4485cc56f5f0e151d8433886976cdf679355e872`, with hooks disabled. The original
interrupted sync had no terminal exit code. The dedicated checkout has local
`core.autocrlf=false` and `core.longpaths=true`, with 42,391 existing LF-index
files materialized as CRLF. Reconcile the actual reviewed candidate bytes when
preparing the build. See `evidence/linked-builder-dependency-sync-20261117.json`.

The public branch `rust/network-ci-integration-20261117` advances draft PR
[#1](https://github.com/manvankaliber/Rustium/pull/1) to
`d228644aeff60b744bc669c013a9c2433c947329`. Verify the displayed head before
interpreting the checks.
Commit `d63109297040b585091cd1a309bd19f6aa91fca1` added the public-push operating
documents to the reviewed candidate `07f55089eac0210c4685a551c98f0edfe8a406a6`.
V3 review of the earlier candidate
found that reusable Ninja outputs and external host tools were outside its
approved input record. This successor uses a new output directory for every
qualification attempt and records the selected host installations before and
after the run. The measured VS installation is outside Chromium's default
search root. The driver sets both Windows toolchain discovery overrides. It
retains manual dispatch for a reviewed commit and approved input digest. Nine
data-only tests and workflow lint passed. The independent V4 review
`4c8c55fa00f6ed761bd0f777d35dac568756f125` cleared the source candidate for laptop
preflight. The PR head has a pending `Network / laptop input preflight` status.
The laptop's Git fetch stalled, including a bounded HTTP/1.1 retry. Its API
requests worked, and the checkout stayed on the pinned baseline. The preparation
receipt is `C:/Users/micro/actions-runner-rustium/readiness-prep-result-20261117.json`
with SHA-256 `6e74ad4268c292ed895cd6435bd507b0c1201397f3746db9a0be1aef1e68640f`.
The public transport branch `rust/network-ci-transport-20261117` at
`1f10f19cad7190c1d0deea0f5299ff719c64e8cd` contains only the exact PR bundle.
The desktop downloaded it through the GitHub Contents API and verified all
6,398,282 bytes against SHA-256
`6b9c426ddd7fa68eaa23ab42aa90b87def45e5d4c2d1fc796e5e699f547aea92`.
The laptop verified and fetched the bundle locally. It confirmed the candidate
DEPS blob matches the baseline, and verified the three-document delta from the
reviewed revision. The first byte check found one affected file, Cargo.lock,
materialized as CRLF on the laptop with an LF baseline blob. The reserved Terra
operator backed up the original bytes, verified all 3,062 differences were
CRLF line endings, and checked out `d631092` with a clean Git status. The
checked-out driver and Cargo.lock matched their Git blobs. Its first input
record attempt stopped before a build with WinError 2 because the restricted
PATH did not include Windows PowerShell. Commit `44c42684` resolves
PowerShell by its system path and adds a regression test. All ten data-only
tests passed and the desktop process query also ran under the restricted PATH.
The two-file successor bundle requires `d631092` and is retained at transport
commit `19e836d0e0d3e7a2bfab1f0a630c6f7e8cd1a729`, SHA-256
`086ae9332039836dc7c8327737562b471213ddba417b701b68216a89193beaf8`.
The laptop ran the revised process query with its normal settings and
established that no compiler was running. The input scan stopped at the
`chromium-bidi` package's `acorn` symlink. The read-only scan found 45 relative
file symlinks naming ordinary checkout files. The current driver binds their
stored target spelling and hashes the named inputs. Twelve planning tests
passed. The transport commit is `95e829acd62b83e1ad5f40f354a27b7bf70c5bc5`.
Laptop input recording passed for `b9167f8`, recording all 45 file links and
the exact dependency revisions. The approved input SHA-256 is
`28903b0dde5fc663bcc1ff29fcb9c424b90efc073e4d336c1fd594830d3ed0af`.
Evidence is under `C:/Users/micro/rb/network-successor-efda28f8793944f7987d9ef148c00446/record-inputs`.
The director assigned the same Terra operator the first laptop-only
selected-consumer build via the reviewed driver, run ID
`pr1-b9167f8-20261117a`, with one compiler job. The laptop byte-range lock
guards the build; no other native work has permission to start. Terminal build
and ordinary validation were pending at launch. The Actions workflow awaits
first dispatch.
The laptop verified an idle runner, the launch environment, the driver bytes,
and absence of the output directory, then started the driver at that revision.
Its evidence path is
`C:/Users/micro/rb/network-build-pr1-b9167f8-20261117a-ab923365aa8e46b4baa0c63eac0b4d1e/build-evidence`.
The approved input guard passed. GN failed before compilation because pinned
V8 imports `build/config/ios/config.gni`, removed by `55cef5a6c6`. The older
configured V8 checkout has local edits removing three unconditional imports,
including its Android config import. Candidate `4149369e172248f7c0cd21a288274f27305b3eb8`
supplies all three imports with assertions for the disabled platforms. The V8
gitlink and DEPS stay pinned. GN advanced past the V8 failure and stopped
at the missing `build/util/LASTCHANGE.committime`. The raw log SHA-256 is
`5417df9619042eafa131c71033d8ff5fe1a0e8487ff763c95f9026761005b6fa`.
The laptop had been synced without hooks, including the metadata generators.
The next revision adds the import-only ChromeOS config used by pinned DevTools
and documents metadata preparation in CI-RUNNER.md. The Terra operator is
ran all five metadata actions. Chromium's default `^Change-Id:` filter found
no commit in the shallow Rustium checkout and generated a zero revision and
timestamp. The retained output is under
`C:/Users/micro/rb/network-focus-3dc46e7-cc197bda12bc4c98bddd05c8fcc0761f`.
The operator retried preparation at the same `3dc46e7` revision using
`lastchange.py --filter=` for main, GPU, Skia and Dawn metadata. This selects
the pinned heads and leaves minibranch's separate footer search intact.
The timestamp matched `1794953747`, the pinned RC matched SHA-1
`dea7da0d9bfcb8dad6cf53ca608dddac3459faaa`, and fresh GN succeeded. The input
record and qualified copy both matched SHA-256
`1b74db86e78114ba8e4bb91214c6c240dc7179d12654bec53266acd4189d6bae`.
The focused and input-recording receipts are under
`C:/Users/micro/rb/network-prep-3dc46e7-29a110b2f2c34af38cdda679a443b984`.
The director assigned the approved `3dc46e7` laptop-only build with one compiler
job and fresh run ID `pr1-3dc46e7-20261117a`. The input guard, GN generation
and `gn check //base:base_unittests` passed. `gn check //net:*` failed because
the connection family tests included two mock headers owned by `net_unittests`.
The run compiled no targets. Its retained evidence is
`C:/Users/micro/rb/network-build-pr1-3dc46e7-20261117a-62f276f254a74c7e810aa8f3dd3b6a90/driver-evidence`.
Commit `ad617037` moves the auth preference, SSPI and GSSAPI mocks together
into one test-only GN target used by both suites. Laptop GN generation and
the full Net header check passed. GN confirmed both direct dependencies and
the intended Windows mock sources. The `ad617037` input record matched its
qualified copy at SHA-256
`c41d54fe2acd0aeb91db7043d01fad067c19f8ed0b1dfdaa684e7726810958c7`.
Evidence is under `C:/Users/micro/rb/network-prep-ad617-e1d3bbd8b7684bf1b90a598a60ecc78f`.
The director assigned the exact laptop build `pr1-ad617-20261118a` with one
compiler job. Compilation, ordinary tests and Actions qualification are pending.
The run reached 2,186 of 9,977 Ninja actions. The user then authorized a
four-job trial on the laptop. The operator is checking whether it can interrupt
Ninja cleanly and resume the same graph and outputs under the build lock. The
trial will record system and child-process memory and reduce parallelism if the
laptop runs short. Preserve the original qualification and the trial receipts.
The first attempt confirmed that Ninja shares the launcher's Windows process
group. It left the original build running at one job after 2,220 actions.
A follow-up is checking actual console membership with `AttachConsole` and
`GetConsoleProcessList`. Interrupt only if the console is exclusive to the
build job, then verify a clean exit before resuming. The four-job trial has
not started yet.
Console inspection confirmed that only the build launcher, driver, Ninja,
its compiler child and the probe were attached. The `-j1` build continued
to 2,839 of 9,977 actions with about 4.9 GiB available physical memory.
The operator stopped a redundant read-only input rescan. The continuation
will use the approved pre-build record and verify the same graph, outputs,
and console before starting the user-authorized `-j4` trial. Receipts are
in `C:/Users/micro/rb/network-build-pr1-ad617-20261118a-32c8c86870be4d93a78b58391d691064`.
Ninja stopped cleanly at 2,979 of 9,977 and resumed in the same output tree
at four jobs. Four simultaneous Clang processes were observed with about
5.0 GiB available RAM and 6.8 GiB of commit headroom. The user authorized a
step to eight jobs. The operator is applying it within the same diagnostic
build. Eight jobs were observed. The first eight-job controller and Ninja
exited without a final log. Their cause is undetermined. The recovered run
used the same output and ended with seven undefined-symbol link failures.
Its samples reached 223 MiB available RAM and 335 MiB commit headroom.
No allocation failure was reported. The original qualification remains
interrupted and the diagnostic continuation failed. See
`trial-terminal-summary.json` and `trial-j8-recovery-attempt-j8-ninja.log`
in the laptop evidence above.

The reviewed and pushed `a873ab7` adds the missing Digest and DNS constructor
and virtual exports and links the DNS unit test to the existing stateless Rust
FFI library. Independent Terra review cleared an actual laptop relink attempt.
The laptop relinked all seven in 76 actions with four jobs and one concurrent
link. `gn check //net:*`, all five C++ export/import checks and the Rust FFI
link check passed. Only the four intended source files and three revision
metadata files changed; post-build source, host, tool and dependency identity
matched. The minimum available RAM was 2.75 GiB with 3.01 GiB commit headroom.
See `C:/Users/micro/rb/network-a873-diagnostic-20261118b/terminal.json`.
Both ordinary suites exited 1 on this revision. Connection jobs discovered
2,602 cases and recorded 2,514 passes, 34 skips, 10 failures, 6 crashes and
38 not run. DNS discovered 33 cases and recorded 28 passes, 1 crash and 4 not
run. Pre- and post-test source, graph, host, toolchain and runtime identities
matched. The wider selected-consumer build stayed pending. Retain the terminal
receipt and launcher evidence under
`C:/Users/micro/rb/network-a873-suites-refresh-20261118c/`.

`c46c54594829585fef0c3b596ba251614561e999` makes the DNS test deregister its
fixture context before deleting the manager in the completion callback. The
laptop imported `c46c5459` and refreshed its metadata. Preflight stopped before
compilation because the existing `ok_cert.pem` uses 113 CRLF line endings
and the unchanged Git blob uses LF. The normalized bytes matched exactly.
See `C:/Users/micro/rb/network-c46c-tests-20261118a/terminal.json` and
`fixture-comparison.json`. The lock was released and no tests ran.

The pushed successor `f7f039c78b81a8c0702810ef9f1e0dbe2289ebd2` corrects the
expected CONNECT header order and allows scripted reset writes to follow a
pending read. It stops the original test fixture from pumping queued socket
callbacks after its case-local mock data have expired and uses normal session
shutdown between auth iterations. Assertions stay in place. Both complete
suites are assigned on this successor with four Ninja jobs, one concurrent
link, and `CR_SOURCE_ROOT` set to the pinned checkout. The run preserves and
records the unchanged certificate bytes, verifies their LF-normalized Git
identity, and applies that comparison only to the known fixture. Full
ordinary and selected-consumer qualification remain open.

The first `f7f039c78b` attempt stopped before checkout. The host inventory
found five newly generated Python bytecode cache files, each reported twice
because two tool roots overlap. The checkout remained at `c46c5459`. Neither
suite compiled or ran. The safety and terminal records are in
`C:/Users/micro/rb/network-f7-validation-95d95eaf/`.

The laptop retry preserved the five hash-verified bytecode files externally
and then matched the complete host inventory. GN regeneration, the net header
check, and both test targets built successfully at `f7f039c78b`. DNS passed all
33 cases. Connection jobs discovered 2,602 and reported 2,581 passes, one
failure, five crashes, and 15 not run. There were no native skips. Final
source, binary, graph, dependency, and host comparisons passed under the
lock. Preserve `C:/Users/micro/rb/network-f7-validation-20261118b/terminal.json`
and `connection_jobs_family_rust_unittests-nonpassing-cases.json`.

The scripts made a synchronous read wait for CONNECT, but the H2 session
starts reading before CONNECT completes. The mock returned `ERR_UNEXPECTED`
and aborted the pending paths. Pushed successor `d88308a2c6bcdf0b3b3c953af2bfacf2b2297848`
puts the never-completing read ahead of CONNECT and keeps the reset order and
all assertions. The laptop is assigned both full suites on that revision
with four Ninja jobs and one concurrent link. The two successful target
builds and DNS result above apply to `f7f039c78b`.

At `d88308a2c6bcdf0b3b3c953af2bfacf2b2297848`, both targets rebuilt and the
Net header check passed. DNS passed 33/33 and Connection jobs passed
2,602/2,602, each unfiltered with one launcher job and zero retries. The
laptop verified source and graph, dependency and host identities, and the
certificate under the native lock. The first controller attempt rejected an
implicit `ASYNC` argument and is retained separately. The passing terminal
receipt is `C:/Users/micro/rb/network-d883-validation-20261118b/terminal.json`,
SHA-256 `a3e79a67e3e9c0d3be801f4e57f9723b45ee2b78beed2a61cd27ba638ded13d7`.
Both focused results are posted to PR #1 at the tested revision.

All 96 selected consumer root labels are assigned for an incremental laptop
build at the same revision and with the retained output cache, four Ninja
jobs, and one concurrent link. The operator must reconcile their mapping
with the 229 historical Ninja targets and retain before/after identities.
The laptop resolved all 96 labels to the same 229 Ninja targets, passed both
GN checks, and finished 85 actions with exit 0. Source, graph, host and
pinned dependencies matched before and after. Available RAM reached 2.65 GiB
and commit headroom reached 3.28 GiB. The terminal status reports incomplete
runtime evidence because 24 programs list `testing/location_tags.json`,
which was absent. The incremental build result was posted to PR #1. Retain
`C:/Users/micro/rb/network-d883-consumers-20261118a/terminal.json`, SHA-256
`3a0e0626666f8d7435eacf8de99742c453f02ebf831ef3d0d366ef9d0acc74e5`.

The pinned DEPS `Generate location tags for tests` hook creates that ignored
file for ResultDB. Test metadata generation was not part of the hooks-disabled
sync. The laptop is assigned that one generator under lock, followed by all
14 native and four Rust unit programs on the same binaries. The generated
file is the only authorized source inventory addition. Preserve the reviewed
skip accounting, PRE enumeration, external logging and short temp controls.

The first 18-program run executed all units. The four Rust suites passed all
141 cases. The 14 native suites recorded 36,611 passes, 145 skips, three
failures, 57 disabled cases and no PRE cases. All three failures were
`PythonUtils` process launches. Its Windows helper specifies `vpython3.exe`
for `CreateProcess`, and the test PATH omitted the installed `.cipd_bin`.
Final source, graph, dependency, host and 3,018 runtime file checks matched.
Preserve `C:/Users/micro/rb/network-d883-ordinary-20261118a/final-report.json`
and `python-failure-diagnosis.json`. The native runtime evidence remained
unqualified because its generated location file was removed after the run.

The recovered pinned generator exited 0 and produced the Chromium repository
entry with no directory tags. The exact main Git tree contains zero tracked
`DIR_METADATA` files. An empty tag set is consistent with that source. Bind
this output to the pinned generator and verify it by repeating the generation.
The exact generated SHA-256 is
`37a5335950cc9df8ff45d439b3779d0bb94f510de0687af248a3f77425306636`.
The laptop retry will retain that generated fixture in the runtime inventory
and add the verified `.cipd_bin` to test subprocess PATH only. It will smoke
`PythonUtils.*` and run all 36 programs on the same `d88308a2` binaries.

All 36 programs ran on `d88308a2`. The 18 unit suites passed with 36,614
native passes, 141 Rust passes, 145 skips, 57 disabled and no PRE cases.
The three Python launch failures became passes. The 11 passing workload
programs cover 109 planned IDs. Seven workload programs covering the other
123 IDs failed. Retain the complete run under
`C:/Users/micro/rb/network-d883-ordinary-recovery-20261118a/`.

Five verifier crashes reached uninitialized test timeouts, one async DNS
verifier reached an uninitialized command line, and the upper-job verifier
required QUIC proxy support. Both cookie-store attempts printed the same fixed
fixture and 15 checksums, then failed during shutdown. The run also grew
`debug.log` and an uncached vpython smoke invoked `pip --compile`, placing
4,419 bytecode files in external temporary storage. Preserve those receipts.
Source, host, dependency, graph and recorded runtime inventories matched, and
the generated location fixture remained present at its qualified hash.

Pushed `d228644a` initializes command line and test timeouts for the failing
standalone programs, routes their diagnostics to stderr, and rebinds the
seven source hashes in the inventory. The workload IDs and build roots are
unchanged. The laptop is assigned the rebuild and 17 non-QUIC verifiers
covering 216 planned IDs. Keep the cookie-store fixed-time and double-capture
checks. Reuse the now-pinned Python environment when executing affected units.
The previously passed unit results remain tied to `d88308a2` unless executable
and runtime byte identities can be carried forward.

Use a separate output for the QUIC-enabled regular phase. Preserve the normal
output and its flags, set only `enable_quic_proxy_support = true` on the
optimized component profile, and verify the effective QUIC and bracketed-URI
flags. The connected upper verifier requires all 16 lifecycle cases and must
keep its capability check. Additional optimized-QUIC coverage is specified in
`.scratch/net-wave5/resumed-integration/optimized-quic-feature-checkpoint-v1/regular-validation-plan.json`.
Full ordinary, probe, fresh build and Actions qualification, and cutover remain
open.
The verified local bundle `.rustium-runtime/network-ci-07f55089eac0.bundle`
has SHA-256 `e68ac5d234ffcb48668e7a1116f8a8dc5dea612e8def232fba4b9426d75eb915`
and requires the pinned baseline. It remains a local backup of the reviewed
pre-policy revision.

Continue preparing ordinary Network validation and consumer closure without
compiling on the editing machine. Use exact before guards for future auth
intake. Retain existing source, output caches and historical evidence.

The exact-head auth/reference reconciliation confirmed the current wiring on `295159ef`. The [HostPortPair candidate](evidence/connection-jobs-host-port-probe-candidate-0f5f6e3c.json) `0f5f6e3c` adds paired resolver cases and opt-in counters that isolate request creation at the shared MockHostResolver boundary. Independent Terra review found no source blockers. The [scoped successor](evidence/connection-jobs-host-port-probe-dispatch-443551ed.json) `443551ed` adds a GN check-cfg entry for existing Rust tests and is on the laptop queue for the ordinary cases and DCHECK-enabled component probe. PR #1 remains at `295159ef`.

The [ordinary HostPortPair result](evidence/connection-jobs-host-port-ordinary-feedback-443551ed.json) on `443551ed` records both resolver cases passing in one focused component build (178.782 seconds) and no flag-off probe artifacts. The separate probe stopped when two copied cache paths exceeded the Windows limit. The retry is probe-only, with ordinary output left alone. The earlier ordinary GN-generated environment change remains recorded.

The [HostPortPair integration receipt](evidence/connection-jobs-host-port-probe-integration-443551ed.json) records 2 ordinary and 3 opt-in DCHECK passes at `443551ed`. Both CXX files match across configurations. The ordinary PATH change removed only the depot_tools CIPD bin entry. The validation is scoped to scripted resolver behavior and Windows component ownership.

The [predecessor and current native helper bindings](evidence/connection-jobs-native-helper-bindings-443551ed.json) identify the shared auth parameter iterators and Digest handler and the directly reached native TLS helpers. Source comparison preserves the distinction between frozen auth entry policy and shared native behavior. Existing exact-head probe artifacts are in use for the Basic and Digest runtime-only checks.

The [auth runtime receipt](evidence/connection-jobs-auth-runtime-443551ed.json) records three positive-control, Basic and Digest passes at `443551ed` using the existing DCHECK probe executable and DLL. No build was needed. The observations cover the seven original instrumented entry points.

The [transport assignment retirement candidate](evidence/connection-jobs-assignment-retirement-candidate-f59000ad.json) f59000ad removes the superseded stand-alone Rust archive. The existing frozen C++ oracle cases now target the rank routines used by the production retained queue. Independent review is in flight. Compilation and candidate native tests are pending. PR #1 remains at 443551ed.

The [retirement review repair and dispatch](evidence/connection-jobs-assignment-retirement-dispatch-557c24af.json) records the three crate-visible declarations in `557c24af` and focused native feedback on that exact candidate. The desktop made no compiler invocation. The public integration branch remains at `443551ed`.

The [assignment retirement integration receipt](evidence/connection-jobs-assignment-retirement-integration-557c24af.json) binds both GN checks, 15 focused passes and 7 standalone Rust passes at `557c24af`. The obsolete archive is absent from the net.dll edge. The already-built net_unittests is in use for the affected pool runtime-only aggregate.

The [pool retry record](evidence/connection-jobs-assignment-pool-recovery-557c24af.json) preserves the mismatched current warm DLL and zero test executions. The laptop will recover the pair if available or rebuild the scoped warm target, then run the pool tests before restoring the checkout.

The [pool aggregate receipt](evidence/connection-jobs-assignment-pool-pass-557c24af.json) records 221 passes at `557c24af`. The earlier mismatch was a deliberate restoration of the previous DLL after the original build captured candidate fingerprints. The retry rebuilt and tested before restoration. The named ordinary family validation at the integrated head is underway.

The [named ordinary family receipt](evidence/connection-jobs-named-family-runtime-557c24af.json) records 2,571 passes, 34 configuration skips, and zero failures at `557c24af`. The pool aggregate passed all 221 cases on the same revision. Four Windows transport deliverables are recorded as integrated using the verified PR remote-tracking ref. The coordinator passed 17 Python tests, including stale-ref and editing-branch rejection. This leaves four entries in validation. Connection Jobs remains active for reference and provider closure. Prior optimized QUIC evidence stays bound to `295159ef`.

The six named QUIC configuration skips are queued at `557c24af` in the existing optimized warm component output. The dispatch is node:delegated-task:command%3Amcp%3A61f7f2a7-2597-444e-b050-1d69cdc80412%3Aremote-delegate%3A8a37bf9c-f15c-43d2-99dd-ac3177507ef6. The ordinary suite and transport delivery results are complete for their recorded scope.

The [optimized QUIC supplement](evidence/connection-jobs-quic-supplement-557c24af.json) records six exact-head passes at `557c24af`. All 28 other ordinary skips have passing enabled dual-race counterparts. The DLL was fingerprinted before and after execution. The module observer missed the 141 ms run.

The [original Basic parameter candidate](evidence/connection-jobs-basic-reference-candidate-8f5306fe.json) 8f5306fe disconnects the Basic reference parser from the shared HTTP parameter iterator using its pinned predecessor semantics. The source review is underway. Basic's native normalization and Digest's native handler remain explicit boundaries.

The [Basic reference review repair](evidence/connection-jobs-basic-reference-dispatch-a6028111.json) adds missing grammar and direct later-challenge cases at `a6028111`. The reviewed reference implementation is unchanged. The exact warm candidate is in Windows validation. PR #1 remains at `557c24af`.

The [Basic reference integration receipt](evidence/connection-jobs-basic-reference-integration-a6028111.json) binds all three focused passes at `a6028111`. Shared normalization and the production-backed Digest handler remain explicit reference dependencies.

The [Digest reference candidate](evidence/connection-jobs-digest-reference-candidate-f27ce55c.json) `f27ce55c` owns frozen Digest parsing, wire assembly and later-challenge policy within the reference target. Nonce, conversion, URL and digest primitive providers remain shared. Independent source review is in flight. PR #1 remains at `a6028111`.

The [Digest validation dispatch](evidence/connection-jobs-digest-reference-dispatch-f27ce55c.json) records the passing source review at `f27ce55c` and the pending ordinary grammar and DCHECK fixed-nonce wire tests. The source is published on the scoped branch. PR #1 remains at `a6028111`.

The [Digest reference integration receipt](evidence/connection-jobs-digest-reference-integration-f27ce55c.json) binds four ordinary and two DCHECK probe passes at `f27ce55c`. Scope includes fixed-nonce wire equality and the seven observed entry points. The distinct existing non-Kerberos six-case gate is now in progress on this revision.

The [non-Kerberos selection reconciliation](evidence/connection-jobs-nonkerberos-selection-reconciliation-f27ce55c.json) records five passing common cases at `f27ce55c`. The prior request selected a guarded Negotiate capture test. The pinned common set instead includes the unguarded NTLM wire and later-round case, now queued from the dedicated output.

The [completed no-Kerberos gate](evidence/connection-jobs-nonkerberos-pass-f27ce55c.json) records six scoped passes at `f27ce55c`, including the unguarded NTLM wire case from the already-built artifacts. A runtime-only proxy cross-DLL test and symbol inspection are queued in the same output for the wiring review.

The [Windows wiring acceptance](evidence/connection-jobs-reference-wiring-windows-acceptance-f27ce55c.json) records the single proxy DLL owner, selected PE symbols and the cross-DLL lifecycle pass at `f27ce55c`. The retained review delivery is integrated. The broader reference task remains active for independent header/HTTP2/SSL helper inventory and current lifecycle verification.

The [exact-head remaining reference dispatch](evidence/connection-jobs-reference-lifecycle-dispatch-f27ce55c.json) covers the header, HTTP/2 and SSL original helper inventory and the optimized Windows 16 verify-only workloads and 16 paired lifecycle tests at `f27ce55c`. No performance work or family focus change.

The [header, HTTP/2 and SSL helper audit](evidence/connection-jobs-header-http2-ssl-source-audit-f27ce55c.json) binds the precise reviewed paths at `f27ce55c`. The shared response parser's reached helpers are C++; original SSL tests use the frozen split. The optimized HTTP/2 script exercises attach and send with the response pending. Separate native response fixtures remain the source for response coverage. The exact optimized lifecycle run is pending.

The [Windows reference acceptance](evidence/connection-jobs-reference-windows-acceptance-f27ce55c.json) records 16 verify-only workloads and 16 paired lifecycle passes at `f27ce55c` with no skips. The scoped reference delivery is integrated. The Connection Jobs focus continues until remaining provider, platform, performance and retirement obligations are resolved.

The [read-only CI recheck](evidence/network-ci-readonly-recheck-f27ce55c.json) records 18 passing planning tests and two PowerShell parse checks at `f27ce55c`. The runner was online and idle. The Network consumer workflow exists in the PR source and is absent from GitHub's registered workflow catalogue. No build, hosted configuration change or dispatch was made. The V4 review workspace and untracked inputs were preserved. There are 21 integrated, two validating, two blocked and two previously assigned/implementing backlog records. No task is ready for dispatch under the current family and dependency rules.
