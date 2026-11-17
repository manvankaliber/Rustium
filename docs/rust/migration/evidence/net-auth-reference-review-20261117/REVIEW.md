# Independent review: NET-CONNECTION-REFERENCE-REVIEW

Disposition: **needs repair before qualification**. This is a review of the
bounded, source-only auth package at worker commit
`adcbf06ce7926b5f85273d6aa7cd7b7211fb6818`, not an integration or runtime
acceptance. No algorithm defect is established by this review. The required
observation windows do not yet reliably contain all Windows teardown, and the
isolated build graph remains a wiring proposal. Preserve all original cases,
expected values and lifecycle membership while repairing these issues.

All paths below are relative to `docs/rust/migration/drafts/net-connection-reference-independence`.

## Findings

### R1 — P1: teardown can escape the measured window

`proposal/net/http/connection_auth_reference_qualification_unittest.cc:98`
only calls `RunUntilIdle()` before reading the seven counters. The Windows
`DefaultProviderLaterRoundAndMalformed` destroys its handler at lines 254–255
without a ThreadPool flush. In `DefaultSspiCallbackMayDeleteHandler` lines
297–303, the flush precedes the main-sequence callback which deletes the
handler; that deletion can enqueue `SequenceBound` destruction *after* the
flush. The capture and cancellation cases similarly need an explicit complete
settlement contract, including provider/factory release where claimed. The
mock retains live handle accounting until its destructor
(`reference/observation/net/http/mock_sspi_library_win.cc:107`), while the
fixture retains the registry and library past `NoObservedRust`. Thus a zero
snapshot cannot establish that cleanup, library destruction and any work
posted by reply processing were observed; this is an evidence gap, not a
proven provider algorithm failure.

Repair by explicitly controlling completion/cancellation and ordering: keep
output and state alive, destroy handler, retire the test registry/factory and
last mock references where the window claims provider release, drive main
sequence and worker sequence through a deterministic barrier until the
`SequenceBound` destructor and mock handle checks have actually completed,
then read counters. For reentrant destruction, first run the reply that
performs the deletion and only then wait for its posted cleanup; account for
any resulting reply tasks before the final read. A blocking mock with explicit
entry/release signals should additionally prove cancellation while the OS call
is in flight. Retain separate success, cancellation, reentrant and captured
logging cases and their assertions. Never merely add a sleep or interpret an
unresolved cleanup as zero.

### R2 — P1: isolated GN/CXX implementation and linkage remain unproven

`proposal/BUILD.fragment.gn:1` and
`proposal/instrumentation/BUILD.instructions.gn:1` are instructions, not an
evaluated build graph. Each probe must replace the corresponding existing
crate root *in the isolated build*, not link a second instrumented copy next
to the ordinary library. `D:/Rustium/base/BUILD.gn:2120` and `:2129` already
make the Base64 and Strings crates CXX producers; the generated CXX binding and
the Rust crate must refer to the same instrumented source and module-relative
files. The five net crates are dependencies of `:net`. Test-executable direct
static-library edges, alternate toolchains, and other affected dependents must
be reconciled rather than assumed not to duplicate global C/Rust symbols or
instantiate independent counters. The export shims deliberately call symbols
inside their own `base`/`net` components
(`proposal/instrumentation/base/strings/rustium_reference_probe.h:7`,
`proposal/instrumentation/net/http/rustium_reference_probe.h:7`). A separate
probe config must put each shim into its owning component and use matching
Rust cfg/check-cfg, with no shim, cfg, export or altered source in the normal
production graph. The existing GN template accepts target `rustflags`
(`D:/Rustium/build/rust/rust_static_library.gni:36`), so this is implementable
in principle; no actual GN evaluation, symbol-table inspection, link or
production non-leakage evidence is present. Do not interpret this as an
algorithm defect or a passed build.

Director integration path: after the current build is finished and the gate is
held, recapture source/destination hashes, stage a separate frozen isolated
source/configuration, bind each of the seven existing crate labels to its one
instrumented copy with original relative module paths and CXX generation,
append cfg flags without clobbering existing flags, and add guarded component
shims. Add each reference source once, retain the existing reference component
for encoder/decoder and its single modp definition, and link the test and its
platform mocks once. Evaluate the actual affected graph, check generated CXX
calls and object/symbol ownership in component and executable, and prove a
normal configuration has no probe flags, sources or exported symbols. Fail
closed on missing counts, unresolved symbols, duplicated definitions or
separate counter instances. Recheck all shared header consumers and their ABI
before application. The patches require review and compilation; the friend
patch changes access, not object layout, while the encoder/decoder patch adds
component exports rather than recompiling modp into each consumer.

### R3 — P2: common qualification disappears when Kerberos is disabled

`proposal/net/http/connection_auth_reference_qualification_unittest.cc:50`
guards the entire fixture and all controls through the closing guard at the
end of the file. Consequently the Basic, Digest, portable/SSPI NTLM, Base64,
logging and all seven positive-control tests are absent in non-Kerberos
configurations, even though those common paths still exist. The instructions
also place qualification sources under `use_kerberos`
(`proposal/instrumentation/BUILD.instructions.gn:33`). A successful build
with an empty filter is not qualification. Move the common fixture and tests
outside that guard, keeping only Negotiate-specific members, mocks and tests
under the appropriate platform/Kerberos guards; keep positive controls in
each claimed configuration and verify discovered test names and nonzero
counts. This is test coverage/wiring, not evidence of a provider algorithm
mismatch.

## Source comparison and boundaries

* Candidate subtree: all 125 Git blobs matched the pinned worker commit; all
  22 + 24 + 25 frozen binding entries and all 40 indexed proposal SHA-256
  values matched their retained copies. The seven Rust copies, after removing
  only their cfg-gated entry hits and appended probe module/comment, matched
  frozen files ignoring whitespace. `input-bindings.json` records the checked
  manifests and identities. This does not revalidate current Root, destination,
  build output, or supplemental platform sources at a later authority.
* Basic changes only the encoding call to the exported frozen encoder
  (`proposal/net/http/connection_auth_original_basic.cc:96`). It preserves
  Unicode conversion and frozen modp buffer length/check semantics. Digest
  retains the native implementation and a factory-owned nonce generator
  (`proposal/net/http/connection_auth_original_digest_factory.cc:12`); the
  borrowed generator must outlive every produced handler, just as in the
  frozen factory. Test factory replacement/destruction only after handlers are
  destroyed. Native iterator, crypto, URL, random and provider helpers remain
  explicitly shared, not independently rewritten.
* NTLM copies the native mechanism and uses frozen rounds, including clearing
  the prior portable challenge before later parsing
  (`proposal/net/http/connection_auth_original_ntlm_mechanism.cc:104`). Its
  separate copied time/random/hostname setter state must be configured by any
  fixture that expects deterministic output; native setter state does not
  control it. SSPI carries the native copied security context/credential
  ownership and `SequenceBound`/weak-reply flow
  (`proposal/net/http/connection_auth_original_sspi.cc:307`, `:445`, `:457`;
  `proposal/net/http/connection_auth_original_sspi.h:40` and `:54`). Completion
  clears the output pointer before invoking the callback; output storage must
  survive through completion. Weak invalidation suppresses replies, not the
  in-flight OS call. Prove handle cleanup with R1's ordered drain.
* POSIX GSSAPI keeps native provider and scoped context lifetimes; the named
  copy releases GSS output through its own scoped buffer and retains the
  import/init capture ordering (`proposal/net/http/connection_auth_original_gssapi.cc:38`,
  `:413`, `:469`). The friend declarations enable private state access in the
  named class; they do not by themselves establish runtime compatibility.
  Negotiate preserves native factory policy, borrowed preferences, resolver
  ownership, state machine and injected callback
  (`proposal/net/http/connection_auth_original_negotiate_factory.cc:21`, `:36`;
  `proposal/net/http/connection_auth_original_negotiate_handler.cc:260`).
  HostResolver's cancellation contract and reentrant callbacks remain runtime
  obligations. Only the named fake is reviewed; arbitrary injected mechanisms
  require their own source, helper closure and instrumentation.
* The rounds adapter preserves scheme checks, encoded assignment before
  rejection/decode and decoded output on failure
  (`proposal/net/http/connection_auth_original_rounds.cc:10` and `:25`;
  `reference/current/net/http/auth_challenge_reference.cc:24`). Prepared
  differential tests cover padding and five alias modes. The decoder writes a
  private buffer before assignment. Lazy log capture and sensitive-mode gating
  are retained in the copied wrappers; the prepared differential capture tests
  compare event type, phase and complete parameters in three modes, but have
  not run. These checks do not qualify unrelated Rust entry points.
* Probe hit coverage inspected: auth 6 exports, cache 1, request headers 12,
  net error 11 with the platform-gated entry, NetLog 4, Strings 4 Rust CXX
  entry functions, and Base64 encoded-length plus both mutually exclusive
  encode definitions. Generated CXX wrappers must call those actual bodies.
  Enabling `kBase64SimdBase64Encode` matters because the frozen live
  `base64.cc:85` otherwise selects modp. The aggregate positive controls are
  required before zero deltas mean anything. The atomics are process-wide,
  monotonic and bounded to the instrumented functions; they cannot exclude
  direct core calls, another process, opaque provider internals or uninstrumented
  families. Preserve quiet windows and the R1 drain.

## Required subsequent validation (not run here)

1. Director: finish the active build unchanged, acquire the validation gate,
   recapture the current 1,261-input source authority and actual platform
   config/toolchain and hashes, compare each bound supplemental input and
   destination, then reconcile the two patches and isolated source/graph.
   Retain the newer Root lifecycle header; its older bound copy is historical
   evidence and must not replace the compiler fixture repairs. Bind all new
   inputs and graph outputs before invoking a compiler.
2. Evaluate the isolated graph and affected header/ABI consumers, then build
   the reviewed target set under the gate, for example
   `D:/RustiumBuild/src/third_party/ninja/ninja.exe -C <isolated-out> -j1 <reviewed-targets>`.
   Include `connection_jobs_family_rust_unittests`, all changed component
   consumers and mock dependencies. Record generated bindings, command lines,
   symbol ownership and production no-leak evidence. This command is a future
   director action, not one run by this reviewer.
3. In a quiet process run the executable with
   `--test-launcher-jobs=1 --gtest_filter=AuthReferenceQualificationTest.EachInstrumentedFamilyHasPositiveControl`,
   require seven nonzero family counts, then run
   `--test-launcher-jobs=1 --gtest_filter=AuthReferenceQualificationTest.*:OriginalProviderRouteTest.*:ConnectionAuthOriginalRoundsTest.*`.
   Verify test discovery/counts first, archive raw output and exact identities;
   also run the unmodified complete ordinary family suite and the 16 original
   lifecycle verifications with their scripts, counts, QUIC configuration and
   expected assertions intact. Use the preexisting broader Network acceptance
   ledger for remaining obligations.
4. Independently execute Windows SSPI and POSIX GSSAPI configurations, plus any
   separately supported non-Kerberos configuration claimed. Exercise first,
   later, missing and malformed challenges, aliases, provider teardown,
   pending cancellation, reentrant destruction, capture-disabled and all
   three capture modes. Compare full event order and parameters to production,
   drain all threads, preserve the exact pre/post source and binary identity,
   and do not promote a host result to other platforms. Add instrumentation
   only for new reachable families before widening a claim.

HTTP/1, HTTP/2, range, MIME and other helper edges remain open non-auth debt;
other-platform runtime, production Rust ownership and cutover are not claimed.
Sanitizer and fuzz campaigns remain deferred by user instruction. All runtime
and compiler validation is pending; no expectations have been weakened.
