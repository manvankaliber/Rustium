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

Decoder entrypoints expose `u32`, `i32`, `u64`, `i64`, and signed `i33`. `LEBHelper` exposes write operations for u32/i32/u64/i64; only `read_u32v` among its read operations; and exactly three native size helpers: `sizeof_u32v(size_t)`, `sizeof_i32v(int32_t)`, and `sizeof_u64v(uint64_t)`. There is no native `sizeof_i64v`. General variable-width decode lives in `Decoder`. i33 is used for WasmGC heap/block type encodings. Unsigned 33-bit is not an exposed entrypoint in this revision.

Observed caller families include function/module decoders (indices, locals, branch depths, constants, offsets, alignments, heap/block types and prefixed opcodes), baseline liftoff, disassembly, module instantiation, serialization, streaming, module building and wasm external references. Callers depend on consumed byte count, validation mode, names in diagnostics and unchanged input pointers for non-consuming `read_*` methods; `consume_*` methods advance the owned decoder cursor.

## Status and evidence

Archived host evidence at [candidate-tests-20261117T083826507172Z/v8-wasm-leb-result.json](D:/Rustium/docs/rust/migration/evidence/candidate-tests-20261117T083826507172Z/v8-wasm-leb-result.json) records 8/8 passing tests for the older `d8c4bbfa7ebbe0922ff98ad854f8bf4e41a4884a` candidate and its then-frozen input hashes. That result does not cover the added base-helper source closure or the revised tests in this checkout. The newly added/revised Rust assertions are **pending execution** by explicit instruction. Native configured targets, ABI/platform behavior, performance and production ownership remain unverified. This is not production integration or cutover.

The frozen base closure for the two decoder SWAR helpers and their direct V8 header dependencies is listed in `reference/source-manifest.json`. The generated `include/v8-gn.h`, compiler/system headers, and target-specific behavior remain configuration-specific dependencies.
