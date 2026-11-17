# NET-HTTP2-BASELINE intake proposal

## Reused archive

Archive-v4 is a hash-bound reuse of the existing `connection-baseline-parent-fixes-v4` package; no baseline algorithm was implemented here. Its source `application.json` SHA-256 is `5405cc7c5994ff26aa0b2cc9ade42acfb128277faea2c17544fe74e87e3f32fc`. The archive preserves all 46 exact after-images and the independently frozen original references. The current repair authority is the 1222-entry frozen snapshot at `D:/Rustium/.scratch/net-wave5/migration-recovery-20261117T041914330876Z/source-snapshot`, manifest SHA-256 `0b2375820f5ce42c0371308cc98e2a808a9cc2928510d9feb7156967bfd41883`. The earlier 1222-entry authority `768bf8f97f23d517269fca450285f9dd73e3fb323a31705da2f3360e8d3c3fd0` remains historical provenance only.

The package declares 39 source entries and seven parent-only proposals; those seven are the existing shared-file before-images. The application's per-binding `before_sha256` fields also include prior staged source replacements (19 non-null guards across all 46 bindings), so these are preserved distinctly in `intake-manifest.json`; they must not be relabeled as seven parent proposals. All primary-root before guards were checked before archiving, including absent paths. Both review fixes were already in v4: explicit `Create(nullptr)` and closing/draining the original HTTP/2 pool at three existing between-iteration sites. The v3 functional review findings are recorded in the source package. Package originals remain untouched.

## String bridge dependency audit

Search of all 46 staged after-images and the seven staged parent proposals found one direct migrated-policy-sensitive call: `net/socket/ssl_connect_job_rust_original_unittest.cc:1275`, `base::SplitString(GetStringValueFromParams(events[0], "selected_trust_anchor_ids"), ", ", TRIM_WHITESPACE, SPLIT_WANT_NONEMPTY)`. If the foundation Strings bridge replaces `base::SplitString` policy, this independent original C++ test oracle inherits that migrated behavior. This test must bind an independent frozen C++ splitter before bridge-dependent acceptance; retain the test and all fixed workload membership. The director has prepared that binding in FOUNDATION-STRINGS-BRIDGE connection-application-v5.json; this v4 archive remains unchanged. Do not change the shared bridge or algorithm in this task. The staged sources otherwise use local C++ string/view `substr` operations; these are native standard-library slicing, not calls into a migrated `base::SplitString` bridge. `internal::*T` conversion template matches are unrelated number conversions.

## Requested validation plan (not executed)

1. Run the 23 ordinary connection-family tests from the exact frozen intake with the retained original-provider fixture. Preserve exact test IDs, command, target/filter and per-test outcome in the director's configured run record. Include the two reviewed regressions and all 16 fixed lifecycle IDs; do not infer pass from source review.
2. Verify the 16 existing lifecycle cases against the unchanged canonical driver and frozen workload identity; retain clock exclusions, fixed inputs and all cases. No workload or benchmark source was edited here.
3. Request a separate optimized-QUIC validation gate: successful HTTP/2-over-QUIC tunnel path plus optimized build/configuration identity, provider ownership and consumer coverage. Existing pending/error QUIC scripts do not establish a successful session/tunnel. Run only in a director-owned configured validation slot.

Compiler, tests, configured builds and benchmarks were not run. No acceptance or performance result is claimed.

## Remaining work and ownership

Director must bind the archive to the primary queue, decide/apply the independent Strings split-helper binding, and schedule ordinary/optimized-QUIC configured validation. Remaining debt includes ABI/GN/export ownership, provider/session ownership across native and Rust boundaries, complete original HTTP/2 helper closure, QUIC success coverage, platform/provider seams and performance acceptance. This is an archive/intake proposal only, not a new implementation or cutover.
