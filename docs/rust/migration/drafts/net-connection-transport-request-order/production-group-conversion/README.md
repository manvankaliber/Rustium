# Transport Group retained-order production candidate

Status: **source candidate, uncompiled and unreviewed**. The production patch
base is exactly `54f6f4df827d30db2762557c7442056cc8a3a0a4`. The earlier source
freeze and all `60adca89` and `45f14af` packets remain unchanged. The director
reported that focused assignment/preconnect cases passed on the prior base;
that is not evidence for this candidate.

`production.patch` replaces only the transport pool's RequestQueue alias and
Group consumers with a retained Rust order and a native token-to-owner map.
Other `PriorityQueue` users are untouched. Rust generates nonzero scalar tokens,
keeps six linked priority buckets and returns actual front, release and donor
tokens. The map stores native `unique_ptr<Request>` owners only; it is not a
second ordered structure. Native `size`/`empty` are used between atomic handoff
steps. Handle/job identity lookup scans that map without a queue-order decision;
Group sanity checks take one full token snapshot and job teardown takes one
bounded assigned-prefix snapshot. Both use initialized native buffers. Existing
assignment-rust wrappers and tests are retained as historical differential
artifacts; no live Group call uses their rank/count policy.

Changed reprioritization detaches the retained Rust node and extracts its native
map node without a callback between them. The native request remains absent
from unbound lookup during the original ReleaseJob, reassignment, timer-stop
and sanity-check sequence; the original post-removal ignore-limits DCHECK stays
in place. The same native node is reinserted after allocation-free Rust relink
and before the new assignment action. No Rust borrow or lock survives any native
provider call. The existing director-supplied contract holds: Group and relevant
objects survive and same-Group request/job ownership is not mutated while
`AssignJob` / `ChangePriority` propagates. Normal resolver work may start
synchronously; the change adds no reentry guard. Existing auth-binding and
native job-transfer functions remain byte-identical, with their pop and removal
helpers now using tokens. New insertions can allocate as before and allocation
or invariant errors are fail-stop, never new recoverable socket errors.

`director-linkage.patch` holds net/BUILD.gn and the private opaque queue ABI
header. The two patches are complementary. All `from_raw_parts_mut` snapshot
calls require initialized, aligned, nonaliased u64 storage for the *entire*
capacity; the native adapter uses `std::vector<Token>(count, 0)`. The Rust ABI
retains a documented raw opaque queue pointer and caller-owned output buffers;
no native request address crosses it. Independent unsafe/ABI and ownership
review remains required. `materialization.json` binds before/after images and
both raw patch SHA-256 hashes to the pinned base. `materialize.py` checks both
patches in a separate Git index without a production checkout. The isolated
application check, source/identity inspection and whitespace check passed.

The production pool fixture `RetainedQueueSameAndChangedPriority` exercises
same-priority FIFO and release/reinsert behavior. The exact live Rust queue also
has unit-test source for front/donor/release selection, retained nodes and
bounded prefix snapshots. `trace/` and `original_trace.cc` are paired standalone
native/Rust job-priority traces; `native_owner_adapter_unittest.cc` is a
separate ABI and snapshot fixture source, not wired into net_unittests, because
component/test private symbol linkage still needs director review. No compiler,
GN, Ninja, native test, Cargo, benchmark, push or shared checkout change was
performed. C++ formatting was manual; compiler, formatter and all platform
results are pending. The core uses `HashMap` and snapshot allocation for some
scans, so memory use and hot-path performance are unmeasured.

## First laptop gate

After director review and materializing both patches into the approved laptop
source, use the laptop's already configured GN output directory and its actual
`gn`/`ninja` executables. Substitute that directory for `LAPTOP_OUT` below;
retain toolchain/source/configuration hashes, logs and exit codes. Generate GN
once for the changed target graph, then perform the warm one-job build:

```
gn gen LAPTOP_OUT
ninja -C LAPTOP_OUT -j1 net_unittests
```

Run the resulting `net_unittests` binary with:

```
--gtest_filter=ClientSocketPoolBaseTest.RetainedQueueSameAndChangedPriority:ClientSocketPoolBaseTest.AssignmentStealsLastAndReturnsReleasedJob:ClientSocketPoolBaseTest.HigherPriorityRequestStealsJob:ClientSocketPoolBaseTest.CancelRequestReassignsJob:ClientSocketPoolBaseTest.PreconnectJobGetsAssignedToRequest:ClientSocketPoolBaseTest.PreconnectClaimsOldestUnassignedJob:ClientSocketPoolBaseTest.PendingJobCompletionOrder:ClientSocketPoolBaseTest.IgnoreLimitsCancelOtherJob:ClientSocketPoolBaseTest.CancelBackupSocketAfterCancelingAllRequests:ClientSocketPoolBaseTest.ProxyAuthStaysBound:ClientSocketPoolBaseTest.ProxyAuthOnceDeleteHandle:ClientSocketPoolBaseTest.ProxyAuthOnceFlushWithError:ClientSocketPoolBaseTest.StalledGroupExcludesAuthBoundSlot
```

Separately compile the exact `trace/retained_queue.rs` as a Rust test target, run
its tests and `trace/no_alloc_trace.rs`, then compile the independent C++
`original_trace.cc` and `trace/trace.rs`, retain stdout and compare with
`compare_trace.py`. Build the separate adapter fixture only after the director
settles test linkage. An ABI/layout review, normal complete Network acceptance,
all supported platforms and performance remain pending. Sanitizer/fuzz campaigns
remain deferred under the standing override.
