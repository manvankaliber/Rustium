# Stateful Rust request-order kernel

Status: **standalone, uncompiled implementation slice; production integration
blocked**. No production patch, ABI, GN change, or Group integration is offered.
The preceding freeze `bd8b1077791e6e3001bd4290ccde86fa1be0946e` and held packet
`dd87e538a5fb00581b22c4d648767898c29417b1` are preserved byte-for-byte. The
production source reference remains `54f6f4df827d30db2762557c7442056cc8a3a0a4`.
The 19 original blobs and eight previously frozen candidate blobs are reused;
`source-identity.json` additionally pins the candidate ConnectJob bridge source.

`src/request_queue.rs` implements an authoritative six-bucket `VecDeque` order
with generated nonzero `u64` identities. It supports FIFO insertion, maximum
priority ignore-limits front insertion, scalar rank and front access, a
caller-provided token snapshot, token removal, unchanged-priority no-op,
changed-priority detach, and concrete front/donor/release target actions using
native job counts. It stores no Request or ConnectJob and has no persistent
second queue. In this standalone artifact it is the sole order; it is not
running alongside the native Group and must not be integrated as a shadow.
Each operation validates its preconditions and reserves before inserting; a
configurable capacity is a harness boundary, not a proposed Chromium limit.
Live duplicate tokens are rejected, but lifetime-wide non-reuse is a native
integration obligation. Selection assumes the native assigned-prefix invariant;
it does not track job state or verify that external counts reflect actual jobs.

`oracle/original_trace.cc` is an independent executable transcription of the
original native loops with a small native-job model. `src/trace.rs` applies the
stateful Rust kernel to the same preconnect, FIFO, donor-steal, changed-priority
release/reinsert, cancellation, ignore-limits front and assigned-job-removal
sequence. `compare_trace.py` compares their retained stdout in order, including
exact selected tokens. `src/request_queue_tests.rs` prepares additional
state/no-op/failure/bounds cases. All of these are source artifacts; none was
compiled or run. The oracle is not a complete provider, timer or auth simulator.

`TRANSACTIONS.md` gives the exact proposed native callback and ownership order,
including the concrete blocker: changed-priority detachment is separated from
reinsertion by native job operations, and safe failure/reentry handling across
that interval has not been established. `QUEUE-USES.md` inventories the native
queue consumers. A future bridge must also handle lifetime-wide token generation,
native ownership-table atomicity, Group destruction, exported ABI, panic/unwind,
all platform builds, and O(n) token lookup costs. Until those are reviewed and
resolved, native PriorityQueue remains the sole production order. This packet
contains no performance, native behavior, or subsystem-cutover claim.

When the director authorizes an isolated laptop validation, compile the two
standalone trace sources using the approved actual C++ and Rust toolchains,
retain each stdout and exit code, and run `python compare_trace.py <original>
<candidate>`. Compile the queue module as a Rust test target and run its tests
only through the approved laptop gate. Retain source/compiler/config hashes and
raw logs; success would validate only this isolated model. A later integrated
candidate needs the full targeted pool fixtures, auth-bound and callback/reentry
cases, normal Network suite, cross-platform consumer builds and ABI review.
Sanitizer/fuzz and benchmarking remain held by the standing user instruction.
