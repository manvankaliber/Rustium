# String runtime bridge proposal, in progress

The accepted Rust policy is copied byte-for-byte beside a director-owned CXX typed bridge proposal. The authored Rust uses safe slices and returns owned offset/length records. It retains explicit default-empty metadata and diagnostic segment provenance; returned references never outlive an input or a temporary code-unit buffer. No production or configured file has changed.

The native/GN beforeimages bind current primary bytes, including the preexisting dirty base/BUILD.gn that matches the configured checkout. The independent original algorithm/helper/test archive remains in accepted-policy. Hashes and Git blob identities are recorded separately.

The C++ adapter and GN proposal remain to be written and independently reviewed. Their required contracts are:

- Preserve all public signatures and native vector/string ownership. Runtime split policy executes in Rust. Preserve the existing C++ constant-evaluation path until those callers migrate; that is explicit remaining C++ policy ownership.
- Translate null native empty inputs to a stable nonnull empty CXX slice, and reconstruct default-empty versus input-borrowed empty views separately. Return the original input view directly for a whole-input result, preserving a null empty input without null pointer arithmetic. Validate offsets with subtraction before taking native substrings.
- For char16_t and Windows wchar_t, materialize a typed uint16_t input/delimiter vector with exact code-unit casts, then return indices into the original native input. This avoids reinterpreting unrelated C++ object types. Lone surrogates and embedded NULs remain code units. The extra allocations are unmeasured provider costs, not a demonstrated performance blocker.
- Native diagnostics must retain the original per-pair messages/order using Rust's diagnostic decisions and segment offsets. Native adapters do not rescan or delegate policy to the old C++ core.
- Generated CXX FFI requires the existing audited-provider unsafe allowance; authored policy/glue is safe. GN/export/component/ABI, allocator/exception/OOM, supported-platform, constexpr and callback-free lifetime contracts need ordinary independent review and configured tests.

Before production intake, audit existing frozen networking originals for calls to base split APIs. Any such baseline must bind its frozen C++ splitting helper before the production API starts routing to Rust. Reuse existing independent references, preserve benchmark membership and raw evidence, and schedule measurement only under the established batch gate. No original baseline may silently inherit migrated Rust policy.

Validation is pending: typed-bridge compiler/tests, native null/default-empty/offset/code-unit/lifetime cases, all original C++ string split fixtures including constant evaluation and Windows wide surfaces, GN/dependency consumers, and whole production-path checks. No performance, production ownership or cutover acceptance is claimed.
