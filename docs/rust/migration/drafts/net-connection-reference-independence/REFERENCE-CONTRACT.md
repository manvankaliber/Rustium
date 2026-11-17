# Contract for the injected reference test mechanism

The director's rule is that a non-null injected mechanism qualifies as an
external reference primitive only after its **separately named implementation
and complete policy/helper closure** have been reviewed and instrumented. The
arbitrary `HttpAuthMechanismFactory` callback remains unchanged: no factory
silently substitutes, rejects or constrains it. A passing test for the named
fake below says nothing about any other injected implementation.

`ConnectionAuthReferenceTestMechanism` is a test-only named fake; its source
and header hashes are in `proposal-manifest.json`. It is passed through the
actual `ConnectionAuthOriginalRegistry` / named Negotiate factory callback,
not registered as a shortcut handler. Source review for this candidate:

* `Init` returns true; `NeedsIdentity` depends on the single `sent_` bit;
  `AllowsExplicitCredentials` returns true; `SetDelegation` intentionally has
  no effect on this fake. It does not emulate a security provider or KDC.
* `ParseChallenge` uses the actual initialized native tokenizer fields via
  the proposed frozen `First`/`Later` adapter and strict frozen modp decoder.
  It does not invoke the live tokenizer constructor, live rounds functions,
  Base64 feature switch, Net error formatter, or NetLog value formatter.
* `GenerateAuthToken` records one outstanding output pointer and callback,
  posts one weak-bound task, and returns `ERR_IO_PENDING`. The input output
  storage must remain live through completion. `Complete` writes the fixed
  token, marks sent/completed, then moves and calls the callback with `OK`;
  it accesses no member after that callback, so reentrant destruction is
  supported. Weak invalidation on destruction suppresses a queued callback.
* Borrowed `ConnectionAuthReferenceTestState` outlives the handler and factory
  closure in each test. Task runner, `WeakPtrFactory`, raw-pointer checking,
  allocator and C++ callback implementation are named shared native primitives;
  they are not blanket assurances about arbitrary framework/provider code.

The source-only review above is pending independent review. Qualification
tests require exactly one constructed/destroyed instance, one posted task and
either one completion/callback or zero after cancellation. They also drive
first and later challenges, malformed and missing tokens through the actual
registry; the retained rounds tests cover output aliases. Seven independent
Rust-family entry counters observe setup-separated windows including posted
work and lazy logging, with a positive control per family. These are **prepared
runtime observations, not results**. Their process/thread and uninstrumented
family limits are in `AUTH-CLOSURE.md`; a separately injected implementation
needs its own binding, closure review, probes and results.
