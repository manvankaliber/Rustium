# Request-order decision slice

Status: **uncompiled candidate; native queue remains the owner of stored order**.
The production base is `54f6f4df827d30db2762557c7442056cc8a3a0a4`. The earlier
assignment and stalled-ranking packets are preserved, including source and test
feedback already in that base. The independent original-order oracle was frozen
first in commit `bd8b1077791e6e3001bd4290ccde86fa1be0946e`; the 19 original
blobs and the original/candidate reprioritization and preconnect-claim regions
were checked. `source-identity.json` pins eight exact candidate source objects.

## What moves and what remains

The real `Group::InsertUnboundRequest` supplies native priority bucket counts
and the actual `RespectLimits` enum value to Rust. Rust chooses front versus
back and the zero-based insertion rank in one scalar result. Native code applies
the selected queue mutation and passes that same rank to the already present
assignment decision. The Rust priority-change decision takes both actual
priorities and chooses either retain-position or remove/reinsert; the original
native removal, job reassignment, priority mutation and insertion calls remain
in their existing order. The zero/nonzero enum representation is guarded with
C++ static assertions. The rank result has a checked unrepresentable sentinel;
no pointer, callback, job, Request or iterator crosses the bridge.

This is a **policy slice**, not full Rust ownership of the request queue.
PriorityQueue's buckets, FIFO links, traversal, erase and Request ownership are
native; its generic ordering implementation remains behavior-bearing for other
callers and for execution of the selected insertion. The native queue is the
only persistent order: no Rust shadow queue or state persists across callbacks.
Auth-bound state, counted preconnect claims, job lifecycle, priority
propagation, cancellation, backup timers, teardown and reentry remain native.
A full queue ownership port still needs a reviewed lifetime/iterator and
reentrancy design before those behaviors can be retired. The existing debug
check that ignore-limits requests use maximum priority remains at its original
point; same-priority no-ops still precede the ignore-limits reprioritization
check.

The additional native queue helper reads the sizes of higher and equal priority
buckets without touching request values. It is proposed in the separate
`director-linkage.patch` with GN and private ABI changes; other PriorityQueue
callers are not converted. The count projection loops over the fixed priority
levels, as the previous insertion-rank helper did. The new insertion and
reprioritization calls each add an FFI crossing; performance has not been
measured. `production.patch` and `director-linkage.patch` must be reviewed and
applied together at the exact base. `materialization.json` retains all before
and after SHA-256 values, blob IDs and raw patch hashes. `materialize.py` uses
an isolated index and verifies applicability and Git whitespace without
checking out production files.

## Reference and validation

`oracle/order_original.h` and its standalone main independently model original
priority order, FIFO and front insertion, same-priority no-op, erase/reinsert,
and preconnect count/claim/cap transitions. They do not model native job
assignment or callback and timer effects. Differential test source compares
insert/reprioritize sequences and insertion plans against that model; a native
PriorityQueue fixture checks bucket observations, and a real pool fixture checks
same-priority retention and changed-priority reinsertion with callbacks that
outlive their handles. These are source artifacts, not executed evidence.

The exact 19 original blobs, source identities and patch application to a clean
isolated index were checked. No compiler, native test, GN, Cargo, Ninja, laptop
job, benchmark or push was run. Rust was formatted with the available rustfmt;
C++ formatting was applied manually and needs normal formatter review.
Following director review, laptop availability, and exact combined-source
materialization, the first failing-sensitive target is `net_unittests` with:

```
--gtest_filter=TransportRequestOrderPolicyTest.*:*PriorityQueueTest.InsertionBucketCountsMatchNativeRanks*:ClientSocketPoolBaseTest.ReprioritizeSameAndChangedPreservesOrder:ClientSocketPoolBaseTest.ReprioritizeResetFIFO:ClientSocketPoolBaseTest.ReprioritizeRequestStealsJob:ClientSocketPoolBaseTest.IgnoreLimits:ClientSocketPoolBaseTest.IgnoreLimitsCancelOtherJob:ClientSocketPoolBaseTest.PreconnectJobGetsAssignedToRequest:ClientSocketPoolBaseTest.PreconnectClaimsOldestUnassignedJob:ClientSocketPoolBaseTest.PreconnectJobsTakenByNormalRequests:ClientSocketPoolBaseTest.CancelBackupSocketAfterCancelingAllRequests
```

Also compile and run the standalone oracle on the laptop with assertions
active. Real auth-bound, callback/reentry, platform-specific behavior and the
full Network suite remain pending. The user-reported passing 4304d7e0 results
are historical evidence for that exact candidate, not a pass for this change;
54f6f4df test feedback was pending at dispatch. Sanitizer/fuzz and performance
campaigns remain held by the standing instructions.
