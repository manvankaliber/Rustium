# Director verification request: auth-provider proposal

Candidate sources, GN fragments and shared-header/component patches are
source-only. They have not been compiled, linked, run or independently
reviewed. The retained v4/v5 references, current director repairs and all
existing assertions, 23 ordinary test obligations, 16 lifecycle IDs/order and
counts remain required. No Native, Rust ownership or cutover acceptance follows
from this proposal.

Preparation checks: both shared patches apply to their frozen input copies;
the proposal manifest and copied inputs were checked against indexed bytes;
and each of the seven instrumented Rust copies reduces to its frozen source
after removing only the probe module and entry hooks (after rustfmt
normalization). `rustfmt --check` passed with stable-rustfmt warnings that the
repository's nightly-only `wrap_comments` and `ignore` settings were skipped.
These are source checks, not compilation or execution. The latest authority at
that check was `e508c7541a4a65a99382fab725e47d686007168ba6dc366347a76813bf1f85c4`;
the old lifecycle header copy differs from the director's newer, repaired
ROOT header, as recorded in `proposal-manifest.json`.

1. Stop at the validation gate and recapture the latest ROOT source manifest,
   all proposal/source/destination hashes, configuration, toolchain and live
   build state. Compare every bound input, including supplemental provider and
   generated CXX sources. Reconcile the evolving fixture-only repairs without
   overwriting them. Review the named NTLM/SSPI/GSS/Negotiate bodies against
   their frozen originals and the explicit `AUTH-CLOSURE.md` map, especially
   buffer ownership, platform types, exception-free callbacks, privacy gates,
   C++ ABI and the separate named NTLM test-hook state.
2. In an isolated director-owned qualification build only, reconcile
   `proposal/BUILD.fragment.gn`, `proposal/instrumentation/BUILD.instructions.gn`,
   the friend patch and frozen encoder/decoder export patch with the current
   graph. Wire all seven instrumented Rust source copies into their own crates,
   their per-component exported accessors into `base` and `net`, and the three
   new ordinary test sources into the existing connection-family test target.
   Apply probe `--cfg`/`--check-cfg` only to this build, with the matching C++
   define. Confirm the generated CXX bridge, component symbols, Rust static-lib
   retention, target source membership and all relevant consumer/header
   dependencies. No probe symbols or flags may leak to a production build.
3. With the gate and the existing compiler limit, build the director's reviewed
   complete affected-consumer manifest including the existing
   `connection_jobs_family_rust_unittests` target (for example, from the
   configured checkout: `ninja -C out/Default -j1 <reviewed-targets>`).
   A single target does not establish the changed-header consumer closure.
4. Run the positive-control test first as a single isolated process:
   `connection_jobs_family_rust_unittests --test-launcher-jobs=1
   --gtest_filter=AuthReferenceQualificationTest.EachInstrumentedFamilyHasPositiveControl`.
   Require all seven nonzero counts. Then, with no competing tests or build,
   execute `AuthReferenceQualificationTest.*`, `OriginalProviderRouteTest.*`
   and `ConnectionAuthOriginalRoundsTest.*` using the same one-job launcher,
   followed by the complete existing ordinary suite. Preserve individual test
   lists, expected counts and raw output. If a new family is reachable, add
   and validate a separate probe before making an independence claim.
5. Run both POSIX GSSAPI and Windows SSPI configurations: first and later
   valid, malformed and missing challenges; default-provider mock context
   creation/teardown; Windows pending/cancel and reentrant destruction;
   injected named-reference completion and cancellation; default, sensitive
   and full socket-byte capture with event order and full parameters compared
   to native production. Record library/provider and output identities,
   callbacks and both pre/post source hashes. A configured host result does not
   establish the other platform. Capture-disabled, capture-enabled and async
   paths must all be observed; counters cover only the named families.
6. Independently review captured cases and remaining non-auth edges. Run all
   16 lifecycle verification cases with their original scripts, expected
   values, count and QUIC configuration; run the other ordinary Network
   acceptance obligations and separately planned performance gate. Sanitizer
   and fuzz campaigns remain deferred by standing instruction. Do not claim
   final cutover or publish while required validation remains deferred.
