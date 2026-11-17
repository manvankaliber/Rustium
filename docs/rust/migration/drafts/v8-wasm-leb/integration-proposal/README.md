# V8 WebAssembly LEB integration proposal

This directory proposes semantics only. The director owns production exports, Rust/C++ ABI, GN, configured validation, and all production integration.

## Ownership still outside this candidate

- Native `Decoder` class state, arena ownership, error aggregation and absolute buffer offsets. `leb::DecodeError.offset` is relative to the supplied slice; the adapter must add the enclosing decoder base and preserve V8's error placement/message behavior.
- Native validation tags. The Rust API exposes `Full` and `AssumeValid`; the latter retains memory safety and still returns an error for malformed input, while V8's no-validation path has caller preconditions and DCHECK assumptions. Adapter policy must be decided by the owner.
- Trace/tracer callbacks and V8 formatting/flag behavior.
- `LEBHelper` raw-pointer APIs are deliberately not reproduced as Rust ABI. A safe slice/`Vec` API needs an owner-approved bridge and lifetime contract.
- Target-specific SWAR loads, unaligned access strategy, preserve-most/must-tail attributes, code-size and performance behavior. This is a semantic candidate, not a performance replacement claim.
- Generated/provider code and the C++ users of `Decoder`/`LEBHelper`; no consumer or GN changes are included.

## Actual widths and callers inspected

Decoder entrypoints expose `u32`, `i32`, `u64`, `i64`, and signed `i33`. The LEB helper separately exposes `write/read/size` for u32/i32/u64/i64 (only `read_u32v` is in the helper; general decode lives in `Decoder`). i33 is used for WasmGC heap/block type encodings. Unsigned 33-bit is not an exposed entrypoint in this revision.

Observed caller families include function/module decoders (indices, locals, branch depths, constants, offsets, alignments, heap/block types and prefixed opcodes), baseline liftoff, disassembly, module instantiation, serialization, streaming, module building and wasm external references. Callers depend on consumed byte count, validation mode, names in diagnostics and unchanged input pointers for non-consuming `read_*` methods; `consume_*` methods advance the owned decoder cursor.

## Status

Candidate source, frozen original headers/tests, and assertion-level Rust cases are prepared. Rust tests and native configured targets were not run by instruction. This does not establish compilation, differential parity, performance, production ownership, or cutover.
