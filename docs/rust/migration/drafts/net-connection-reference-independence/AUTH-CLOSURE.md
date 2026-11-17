# Auth provider source and helper map

This map is source evidence for the proposed auth-provider scope, not an
executed proof. Sources are bound by `input-bindings.json`,
`provider-input-bindings.json`, `observation-input-bindings.json`, the retained
v4 application and `proposal-manifest.json`. Supplemental paths are identified
as such. The authority advanced while the work was underway; the observation
inputs were rechecked against `18b7f5db159a365eddb87d4a0be1a2b1ec7029a244cb6fb658bc252db2d6c152`.
After another authority advance, all 25 observation inputs were also checked
against `e508c7541a4a65a99382fab725e47d686007168ba6dc366347a76813bf1f85c4`.
The director must recapture the final identity before applying any proposal.

| Entry and selected overload | Proposed next edge and state | Explicit helper/provider boundary |
|---|---|---|
| Original `CreateAuthHandlerFromString(std::string_view, …)` | `OriginalPolicyTag` constructs native tokenizer storage; `InitTokenizer` uses frozen `auth_challenge_reference::frozen::StringViewTokenizer`, frozen LWS trim and `base::ToLowerASCII`. Registry `CreateAuthHandler` dispatches the actual factory. | Scheme map, `HttpAuthPreferences` and `URLSecurityManager` remain native; audit policy/filter and URL/provider inputs. No native tokenizer constructor or native `base64_param()` is used on this route. |
| Original Basic `Init` / `ParseRealm` | Native tokenizer `param_pairs()` constructs `HttpUtil::NameValuePairsIterator(params, ',')`; iterator uses `ValuesIterator::GetNext`, quoted-token iteration, `TrimLWS` and strict/non-strict unquote paths in `http_util.cc`. | Native Latin-1 `ConvertToUtf8AndNormalize`, ICU and string/allocator behavior. Native iterator methods are shared C++ and must be rebound if those methods migrate; the whole `http_util.cc` is not assumed independent. |
| Original Basic `GenerateAuthTokenImpl` | UTF-16 username/password conversion and `StrCat`, then proposed `auth_challenge_reference::FrozenEncode(std::string_view)` and `"Basic "` prefix. | Frozen modp encoder uses checked length and exact returned-length assertion; Unicode conversion and allocation remain native. The live `base::Base64Encode(std::string_view)` instead forwards to the span overload and can invoke Rust SIMD when the feature is enabled. |
| Digest factory | Named factory owns the same dynamic nonce-generator lifetime and invokes `ConnectionAuthOriginalHandlerPolicy::InitFromChallenge` on native `HttpAuthHandlerDigest`. Its virtual `Init`, later challenge and token method remain native Digest code. | `HttpUtil::NameValuePairsIterator`, native random/nonces, `GetRequestMethodAndPath`, URL formatting, OpenSSL digest and UTF conversions; all configuration/provider branches need separate review. The handler must not outlive its factory's nonce generator. |
| NTLM factory / handler | Named handler with independently named platform mechanism; `Init` preserves channel binding, properties, score; original base-handler wrapper owns callback. First/later parser uses frozen rounds. | Portable: copied `NtlmClient`, time/random/hostname hooks and token encoding; named `ScopedProcSetter` is distinct from native setter and requires fixture-aware tests. Windows: named SSPI mechanism below. X509 channel binding remains native. |
| Portable NTLM `ParseChallenge` / `GenerateAuthToken` | Before first token: frozen `First`; later: clear old decoded challenge, frozen `Later`. Generate keeps original missing-credentials, first-token state, hostname/random/time and native NTLM crypto flow; encoded token uses `FrozenEncode`. | `ntlm::NtlmClient`, `GetHostName`, `base::RandBytes`, time, native NTLM v2 feature preferences, and callbacks are shared primitives only after their helper closure is audited. |
| Windows NTLM and Negotiate | Named SSPI copies `SspiContext`, task runner, security context state, error mapping and completion flow. `ParseChallenge` reads its own `has_security_context_` and uses frozen rounds; its token encoder uses `FrozenEncode`. | `SSPILibrary` and `SSPILibraryDefault`, security handles, `DetermineMaxTokenLength`, `SplitDomainAndUser`, OS SSPI, and DLOG/UTF helpers remain native. In-flight OS calls are not cancellable; weak reply and SequenceBound destruction must be verified after full drain. |
| POSIX Negotiate | Named handler calls named `ConnectionAuthOriginalGSSAPI`; parser uses frozen rounds, token method copies original import/init status mappings and uses `FrozenEncode`. | `GSSAPILibrary`/`GSSAPISharedLibrary`, OS dynamic loading and GSS functions, `ScopedSecurityContext` and reference token helpers; external library lifetime outlives borrowed handles. Library load/bind and error diagnostics remain separately observable native seams. |
| Negotiate resolver / completion | Named handler retains `DoResolveCanonicalName`, completion and callback ordering; resolver failure diagnostic uses `net_error_rust_reference::ErrorToString`. | HostResolver request cancellation and callback/reentrant destruction, target network, preferences/delegation, and platform credentials require actual platform evidence. |
| First/later rounds | `First` and `Later` use initialized native tokenizer field views, frozen padding trim, strict `FrozenDecode`, and native `HttpAuth::SchemeToString`. Later preserves encoded-before-decoded assignment and no decoded mutation on failure. | `auth_challenge_reference` component's frozen modp implementation and its data header. Borrowed `params` may alias encoded or decoded output; the five alias modes in the retained rounds source must execute. |
| Auth cache/controller | `ConnectionAuthOriginalCache`, `ConnectionAuthOriginalPolicy` and `ConnectionAuthOriginalHandlerPolicy` from v4 operate on native cache/controller/handler storage and use frozen partition predicates and handler completion. | Native map/list, NAK key and URL/clock operations, refcounting, deferred callbacks and task ordering. The dedicated auth-cache probe detects its Rust `http_auth_cache_rust_run` entry only. |
| Request/response headers | Original header policy and helpers handle request mutations; native `HttpResponseHeaders` parses storage and supplies `EnumerateHeader` to auth dispatch. | Header policy probe covers all 12 `rustium_request_headers_*` exports. Native response parser, chunked drain, HTTP/1 and HTTP/2 helper dependencies are non-auth closure and are not declared independent here. |
| Registry and handler event creation | Original factory uses frozen `NetLogStringValue` for scheme/challenge; original handler wrapper uses frozen authorization and `ConnectionOriginalLog::EndEventWithNetErrorCode`. Capture callbacks are lazy and retain their original sensitive-mode gates. | Native NetLog dispatch, observer delivery and `base::Value`; GSS copied context diagnostics route string/binary values through `net_log_values_rust_reference`, while Windows SSPI dictionaries and UTF conversions are retained native C++. Observe default, sensitive and socket-byte modes and compare payload and event order. |
| Custom injection | Named `ConnectionAuthReferenceTestMechanism` has its own source-bound state, frozen round calls and weak-posted completion. `HttpAuthMechanismFactory` callback is still invoked, unchanged. | Only separately source-reviewed and instrumented injected implementations qualify as external reference primitives. This one test fake is not proof about arbitrary callbacks or OS mechanism implementations. |

## Entry observations and limits

The probe source instruments six auth-adjacent families plus Base64: auth
challenge (six exported Rust functions), auth cache (one), request headers
(12), Net error (11, including the Windows-only entry when configured), NetLog
values (four), Strings (four CXX bridge functions) and Base64 (encoded length
and the platform-selected encode function). Each has its own monotonic atomic
counter and positive control; the test snapshots after setup and reads again
after completion, cancellation, destruction, ThreadPool and task drainage and
lazy logging. No counter is reset during active work. `kBase64SimdBase64Encode`
is enabled for these qualification tests so conditional Rust entry cannot be
hidden by the default feature state. All tests and the probe GN/ABI wiring are
uncompiled and unrun.

Counters prove only calls through their named instrumented functions in the
same process and exact build. They do not capture a direct Rust-core call,
an uninstrumented family, another process, or opaque OS/provider internals.
Parallel tests and background work contaminate a process-wide window. Native
source still calls other migrated policies, including HTTP range/MIME, HTTP2,
connection and logging subfamilies; resolve each reachable edge and add a
separate instrumented entry and positive control before any claim beyond this
bounded auth scope. A passing auth count cannot establish independence of any
other family or of an untested configuration.
