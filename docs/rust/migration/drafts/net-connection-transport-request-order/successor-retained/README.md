# Retained-node request queue successor

Status: **uncompiled source and adapter proposal; not a production Group patch**.
The pinned production base is `54f6f4df827d30db2762557c7442056cc8a3a0a4`.
The existing standalone packet `60adca89bdaf13042eee0709cd5cbad955088829` and
all earlier artifacts remain unchanged. `source-identity.json` pins the newly
reached provider and OOM sources; they are frozen from Git objects here.

`src/retained_queue.rs` owns a single authoritative order in six doubly linked
priority buckets. Native Request identity is a generated, nonzero scalar token;
no Request address is stored. New nodes are boxed and owned by a Rust HashMap.
A changed-priority detach unlinks but retains the exact box and token, so all
ordered views omit it. Finish relinks that box at the pending priority with no
allocation in the Rust detach or finish code. Same-priority requests remain
scheduled and unchanged. The queue also selects native unassigned-front, donor,
and release targets and supports a single-call bulk token snapshot. There is no
persistent duplicate order. Tests include detached invisibility, same node
across a map rehash, exact token retention, error states and FIFO/front rules.

`src/ffi.rs` and `proposal/retained_queue_ffi.h` propose an opaque Rust-owned
queue pointer, scalar tokens, fixed reply and caller-owned output buffers. No
native Request pointer crosses it; safety preconditions, layout checks and
fail-stop status handling require independent ABI review. `proposal/` also
contains a native owner-map adapter sketch using C++ map node extraction and
same-map reinsertion, a small ABI/ownership fixture, and a GN fragment. The map
is an ownership index, not another ordered queue. These are standalone review
materials; no Group call sites or shared files are changed.

The director-provided Group lifetime and no-same-Group-mutation-during-
`AssignJob` contract resolves the speculative provider reentry concern raised
in the previous packet. `TRANSACTIONS.md` records that contract, the precise
native job transfer/priority and auth/timer sequence, the audited provider
routing, OOM policy and what remains. Native and Rust new-node allocation are
fail-stop rather than recoverable socket errors; the C++ sketch CHECKs all Rust
failure statuses. Rust detach/finish retain their allocation. There is no
assertion that normal resolver work cannot begin synchronously.

`oracle/original_trace.cc` is an independent original-loop job model;
`src/trace.rs` drives the Rust queue and a native-job harness through matching
preconnect, donor, changed-priority, cancellation, front and job-removal cases.
Both emit priority-propagation events with callback-visible ordered tokens,
including the absence of the detached request. The trace is a simulation, not a
real resolver or pool execution. `src/no_alloc_trace.rs` is a separate
single-threaded test-only allocation counter; it contains an isolated unsafe
allocator shim that is not linked into the queue. `compare_trace.py` compares
retained outputs line by line. None has been executed.

## Remaining work and smallest validation

The native owner adapter is a sketch, not a completed conversion. All real
Group queue consumers, private ABI/linkage, auth binding, teardown, callback
lifetime and exact post-removal DCHECK still need implementation and review.
No unproved provider reentry restriction is being used as a blocker; this is
unfinished integration work. The previous rollback concern is resolved by the
retained Rust and C++ nodes under the supplied contract, subject to toolchain
validation. Snapshot allocation, O(n) token lookup, Rust HashMap costs and
cross-platform ABI behavior remain unmeasured.

No compiler, tests, benchmark, GN, Cargo, Ninja, push or shared checkout change
was performed. After review and laptop scheduling, the smallest first gate is:
compile `src/retained_queue.rs` as a Rust test target and run its tests; compile
and run `src/no_alloc_trace.rs`; compile the independent C++ oracle and
`src/trace.rs`, retain both raw outputs and compare with `compare_trace.py`.
Then build a private test target linking `src/ffi.rs` with
`proposal/native_owner_adapter_unittest.cc` and run
`RetainedRequestOwnersTest.*`, including reply layout checks. These would test
only the standalone components. A future integrated patch needs real pool,
preconnect, auth, callback/teardown and platform consumer coverage. The 54f6
focused Windows result is user-supplied evidence for that prior base only.
