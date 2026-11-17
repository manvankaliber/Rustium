# Connection reference independence: blocked candidate

This is the historical `6862a026` checkpoint. The subsequent provider and
factory proposals, and their remaining blockers, are described in
`SUCCESSOR.md`; statements below about the adapter being unwired apply to
that checkpoint only.

This is **not a complete independent connection reference** and is not a build,
test, benchmark, acceptance, or cutover result. It contains a candidate for one
confirmed remaining policy edge, source-bound evidence and the exact existing
16-case verification obligations. The v4/v5 reference package remains the
starting point. Do not replace its `Create(nullptr)`, pool close/drain fixes,
or the director's two current test-support field/access repairs.

## Confirmed blocker and smallest director action

`ConnectionAuthOriginalRegistry::Create` registers native Digest, NTLM and,
under `USE_KERBEROS`, Negotiate factories. For NTLM, native
`HttpAuthHandlerNTLM::ParseChallenge` enters `HttpAuthNtlmMechanism` on the
portable path or `HttpAuthSSPI` on Windows. Negotiate invokes the injected
`HttpAuthMechanism` (GSSAPI on POSIX, SSPI on Windows). The provider methods in
the copied current `http_auth_ntlm_mechanism.cc`, `http_auth_sspi_win.cc`, and
`http_auth_gssapi_posix.cc` call `ParseFirstRoundChallenge` and
`ParseLaterRoundChallenge`. The current definitions call `rustium_auth_first`,
`rustium_auth_later_decision`, and `rustium_auth_decode`. Thus a non-Basic
challenge can enter migrated Rust policy from the original registry. The
existing Basic-only `http_auth` lifecycle does not exercise this path. Native
provider linkage alone cannot establish independence.

The proposed `connection_auth_original_rounds.{h,cc}` adapts the existing
frozen `auth_challenge_reference.cc` logic to the already initialized native
tokenizer fields, using the independently named frozen modp decoder. Its
prepared gtest source covers first/later success and failure, output state,
padding, and five storage-alias modes against the frozen oracle. **It is not
wired into providers and has not been compiled or run.** The native provider
methods currently call process-wide functions; changing those symbols globally
would also change production. The minimal director-owned dependency is a
reviewed **reference-only dispatch seam or independently named provider
variants** at all three provider call sites, with original registry factories
actually constructing the reference variants. Preserve provider state,
platform library selection, ownership, callbacks and cancellation. Wire the
adapter and frozen modp/reference source target only to the original path;
do not replace the process-wide production functions. GN and any shared
interfaces are director-owned proposals. No route has been selected or proven
safe here. Mock SSPI/GSSAPI and portable NTLM first/later tests must verify the
actual original-registry-to-provider route as well as independently observe
the Rust auth entrypoints; cover each compiled platform path separately.

Additionally, the native Digest/NTLM/Negotiate factory code calls native
`HttpAuthHandler::InitFromChallenge`. Native handler completion calls
`NetLogWithSource::EndEventWithNetErrorCode`; unlike the frozen handler's
`ConnectionOriginalLog` route, this is a further logging/error-policy boundary
to trace for those providers. Do not assume that adapting rounds alone closes
the family. The precise reachability of every logging capture, provider callback,
and externally selected mechanism remains unverified. A single auth Rust probe
does not exclude Strings, Net error, logging, MIME, or other migrated policies.

## Source map and boundaries

* Original auth: `connection_auth_original_factory.cc` freezes tokenizer
  initialization and registry selection; `connection_auth_original_basic.cc`
  calls the native `NameValuePairsIterator` and native charset conversion.
  Frozen cache/controller/handler bodies and the original request-header
  helpers are retained in the v4 application, not regenerated here. Native
  `HttpUtil::NameValuePairsIterator` and its tokenizer/trim and conversion
  dependencies require their own transitive, configuration-specific audit.
* HTTP/1 original parser: `http_stream_parser_rust_original.cc` calls native
  `HttpUtil::LocateStartOfStatusLine`, `LocateEndOfHeaders`,
  `HeadersContainMultipleCopiesOfField` and `HttpResponseHeaders::TryToCreate`,
  `IsChunkEncoded`, `IsKeepAlive`, and `GetContentLength`. The current bodies
  are copied and hash-bound for further audit; `GetContentLength` uses native
  `StringToInt64`. Their presence in a C++ file alone is not proof of the
  entire transitive closure, including captured logging or chunked drain.
* HTTP/2: the existing original context/pool/session/stream and header/log
  helpers remain v4 authority. The native frame decoder/encoder, socket
  providers, task runner, header storage, and shared HTTP response primitives
  remain explicit native providers, not proven policy-independent merely by
  a top-level reference name. Follow callbacks and both capture modes.
* SSL: current `ssl_connect_job_rust_original_unittest.cc` uses the existing
  local split, local trim and local whitespace implementation. Original SSL
  job uses `net_error_rust_reference::IsCertificateError`; TLS, certificate
  verification, socket and platform providers remain native external seams.
* Strings: the current `base/strings/string_split.cc` routes four once/reverse
  overloads and four key/value APIs to Rust. Direct original-file searches are
  insufficient: any native transitive helper can call these. Known network
  callers include MIME and data-URL helpers; reachability from each reference
  and capture mode is not yet established. `SplitStringPiece` inline calls
  are distinct from these out-of-line entrypoints. The local SSL test split
  does not demonstrate full Strings independence.
* Other live migrated leaves must be resolved by actual call edge, including
  `net_errors.cc`, NetLog value/redaction paths and MIME adapters. Existing
  `net_error_rust_reference` is explicitly used at several original call
  sites, but does not cover calls buried in shared providers. Cryptographic
  libraries, OS SSPI/GSSAPI, clocks, allocators, URL, resolver and socket
  providers have platform, lifetime and ABI obligations; their use is shared
  only as named primitives after their policy call closure is reviewed.

The source map above is intentionally **not exhaustive**. Finish the
configuration-specific transitive call graph and instrument each migrated
family independently before any whole-reference claim. No stub, redirect to
production policy, reduced registration set, or assumed no-op logging is an
acceptable closure.

## Binding and checks

The latest ROOT source-manifest bytes hash to
`774df90a3f27bf0433babf0cd31606703ed2edb962481b51f1dc58cb613d929e`;
all 1,261 entries matched ROOT before copying. All 46 v4 source bindings
matched their manifest. Current ROOT differs from v4 at SSL original test
(the later local split) and `net/BUILD.gn` (director dependency overlay).
The 22 copied current inputs were checked against the current manifest where
listed and individually hashed before and after copy; the five provider files
absent from the 1,261-file authority are explicitly marked supplemental, not
silently promoted into it. See `input-bindings.json` and raw manifests under
`reference/`. The historical `2c59…212dc7a2` snapshot is provenance, not the
current authority. The director's support files are copied as evidence only;
their current hashes match the new authority, and they are not proposed writes.

The adapter/test are source proposals only. No compiler, Cargo, native test,
benchmark, sanitizer or fuzz campaign was run. Independent review, configured
build and source identity recapture, all 23 prepared ordinary tests, relevant
original/production suites, all 16 lifecycles, optimized-QUIC configuration,
platform/provider coverage and callback/ABI/lifetime checks remain pending.
Sanitizer/fuzz campaigns remain deferred by standing override; never mark
them passed or claim final cutover while required checks are deferred.
