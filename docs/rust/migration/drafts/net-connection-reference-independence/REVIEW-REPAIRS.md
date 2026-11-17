# Director source repairs for review 1fd755aa

The parent repair retains all 13 authored qualification test names and their
order. It does not apply any proposal to the configured tree.

R1: Windows mocks now post a release proof on the destruction sequence after
the complete derived/base destructor, then reply to the test sequence. The
final counter read follows handler retirement, registry/factory and last local
mock release, every release proof, a worker barrier and main-sequence drainage.
The mock's existing live credential/context checks therefore finish before the
proof. Reentrant deletion is settled after its reply, rather than before it.
Cancellation explicitly blocks InitializeSecurityContext, proves entry without
return, destroys the handler while the call is in flight and releases the call.
A scope cleanup runs before local output/state destruction on fatal assertions;
the blocked state is refcounted by the mock to avoid a dangling wait-state seam.
Captured reference and production paths release owned registries and providers
before collecting counters and final event payloads. No sleep establishes zero.

R3: the common fixture and six Basic/Digest/NTLM/logging/Base64/control tests
are outside the Kerberos guard. Only Negotiate registration and tests remain
Kerberos-specific. Probe GN instructions now include the common test in every
claimed probe configuration; injected Negotiate sources remain conditional.

Both repairs are source-only until their recorded compiler/runtime checks are
complete and a new independent review accepts the exact successor. R2 remains:
the probe source/target selection, CXX generation, component exports and normal
configuration isolation need concrete GN wiring and actual graph/link evidence.
No Windows, POSIX, non-Kerberos runtime or full reference independence is claimed.
