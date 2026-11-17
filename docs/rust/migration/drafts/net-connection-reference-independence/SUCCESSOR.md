# Reference-provider successor: candidate, not closed

**Authority advanced during this work.** The latest pointer names a 1,261-entry
manifest SHA-256 `83075f105ebbc29f0fb1b01b031667a6448a7bcc4ec381f774b2d950679ad2f5`,
replacing the captured `774df90a…613d929e` authority. Its two manifest changes
are `net/http/connection_auth_baseline_unittest.cc` from
`f40cf2d0b69ce0b8f9fa5c7e019a6814e5bebab0c5eb240b7a16adb2fc43fdde` to
`e97ddce6f2fc2e9f5ee84e14018f2e6b9d08b5cd395274a64acd2a0303a0d0e6`,
and `net/socket/connection_jobs_upper_rust_job_workloads.h` from
`b0cdef02345c4b0ba8a94295b924ba0d0831e5a5f9fd7b9309a2cda92952012d` to
`1c5f88c4f1598520aba5d7bde7be11c326ad13986df0b04e97fa1ce7528daee2`.
At inspection the latter contains `#include "base/test/task_environment->h"`
instead of the real header path. The director must correct and rebind that
compiler repair, then recapture authority; this worker did not change it.
The lifecycle source copy and mapping remain bound to the earlier authority,
not a claim about the newer executable inputs. The other 45 copied inputs
still matched their recorded ROOT hashes at the recheck.

This successor responds to the authorization for reference-only provider and
factory variants. It advances the previously blocked auth path but **is not a
complete, reviewable-for-integration closure**: no compiler or tests ran,
Negotiate later-round mock coverage and per-family Rust entry instrumentation
are not prepared, and the full non-auth transitive closure is not proven.
Do not install or claim independence on this evidence.

## Proposed route

* `connection_auth_original_factory.cc` retains Basic and registers named
  Digest, NTLM and, under `USE_KERBEROS`, Negotiate factories. It preserves
  preferences and default Windows SSPI / external GSS library selection.
* Digest uses the existing native Digest behavior and nonce-generator lifetime
  with `ConnectionAuthOriginalHandlerPolicy::InitFromChallenge`; token
  generation and subsequent handling use the existing original base-handler
  policy. This has not established every native Digest transitive helper.
* NTLM's reference-only derived handler overrides initialization and
  rechallenge, reusing the native instance's state and token generation. On
  portable platforms the named parser preserves `first_token_sent_`, the
  pre-parse clear of `challenge_token_`, and first/later result semantics. On
  Windows it reads the native SSPI context state and writes its existing
  decoded token. No production policy switch, global function replacement or
  production object layout change is proposed. Native credential/token API,
  certificate channel binding, allocator, callback and weak-pointer behavior
  still require independent platform review.
* Negotiate selects named SSPI/GSSAPI overrides and a copied, independently
  named handler. The latter uses original base-handler initialization and
  frozen `ErrorToString` at the CNAME failure diagnostic. Its original channel
  binding capture gate and callback/state machine are retained. GSS token
  generation has a named copy of the original status/flags/OID/context and
  buffer/name lifetime helpers; GSS string/binary log values use the existing
  frozen NetLog reference. Library load, SSPI logging, DNS, allocator and
  callback paths still require complete transitive tracing and capture tests.
* The rounds adapter consumes the already frozen tokenizer fields and exports
  the existing strict frozen decoder through `reference-decoder-export.patch`.
  This avoids calling an unexported modp symbol across a Windows component
  boundary and avoids compiling the same global C symbols twice. The existing
  decoder body remains unchanged. `friend-declarations.patch` proposes
  access to native state and the Digest constructor, without layout changes;
  both patches apply to the copied frozen inputs. The build fragment is a
  proposal only and needs GN owner reconciliation.

The route exists only in `proposal/`; no live source or GN file was changed.
The replacement original registry is a complete proposed source file, not an
instruction to overwrite a newer ROOT file without identity checks. The
platform guards and symbol visibility have not been compiled.

## New source-bound interface blocker

`HttpAuthMechanismFactory` in `net/http/http_auth_mechanism.h` is an arbitrary
caller-supplied repeating callback returning `std::unique_ptr<HttpAuthMechanism>`.
The existing `ConnectionAuthOriginalRegistry::Create` accepts it and the
Negotiate factory calls it. `HttpAuthMechanism::Init`, `ParseChallenge`, and
`GenerateAuthToken` are virtual; an injected implementation can itself call
any migrated Rust policy, retain callbacks or invoke its own providers. No
wrapper can preserve all of those behaviors and simultaneously prove that an
arbitrary supplied implementation is reference-independent. The proposed
factory preserves the injection contract, so this remains an explicit hole.

The minimal director interface decision is to define the qualification
boundary: either require a separately reviewed, instrumented **reference**
mechanism factory whenever that hook is non-null, or restrict the independence
claim to the named default providers and record injected providers as external
dependencies. Do not silently reject the callback, route it to a dummy, or
claim its behavior is independent. The default provider path still requires the
remaining checks below even after that choice.

## Prepared and missing verification

`connection_auth_original_provider_unittest.cc` proposes actual registry routes
for Digest, portable NTLM, Windows SSPI NTLM and Negotiate first challenge,
including NTLM malformed and later state, and an async Windows token callback.
The existing rounds source proposes first/later, malformed, padding and five
alias modes against the independent frozen oracle. They are **uncompiled and
unrun**; the registry tests use mock OS providers where needed and do not prove
the full closure.

Still required: actual-registry Negotiate **later** and malformed token tests
after a mock security context is established on Windows and POSIX; provider
cancellation, reentrant destruction, both logging capture modes and original
versus production event payload/order checks; source-mapped checks for all
existing lifecycle obligations in `LIFECYCLES.md`; and independent entry
observations for auth, Net error, NetLog values, Strings and every other
reachable migrated family. An auth counter by itself does not observe any
other Rust entry, and an absent counter does not prove absence of calls. The
implementation of those probes, including asynchronous thread coverage and
the generated CXX Strings bridge, is not included. Tests must distinguish
setup/production control calls from the original path and account for lazy
logging callbacks without suppressing them.

The map in `STATUS.md` remains incomplete for HTTP1 response parsing, HTTP2,
SSL, native provider helpers, range/MIME, and all eight out-of-line Strings
APIs. Resolve actual overloads and native transitive callees before a full
closure claim. Sanitizers and fuzzing remain deferred; no runtime execution,
benchmark, platform qualification or production cutover is asserted.

## Source binding

The original checkpoint binds the current 1,261-source manifest and v4/v3
evidence in `input-bindings.json`. `provider-input-bindings.json` independently
hashes the additional provider, handler, logging and decoder bodies used by
this successor; paths outside the 1,261-entry manifest are explicitly marked
supplemental. The copied bytes are frozen before adaptation and retained with
their original whitespace. `proposal-manifest.json` hashes proposed files.
No later ROOT authority is implied for supplemental files: the director must
recheck all source and destination hashes before any application.
