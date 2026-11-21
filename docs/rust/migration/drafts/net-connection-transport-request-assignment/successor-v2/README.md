# Successor: count-based request assignment decisions

Status: **source proposal; hold for director review**. The previous packet at
`1315bb0872dda47a9919ed845173869c36066ce0` is preserved unchanged. These new
patches target production candidate `712ca01f84b44e86ca53fb197ac54ac95b47b425`;
they supersede the earlier assignment patches, not the ranking or idle changes.
The original source and dependency freeze is at `fc2d429f3493ade4272d3b1e2ade96a81ba03d41`.
`source-identity.json` pins the candidate source objects and the additional
PriorityQueue fixture. No source was extracted from mutable worktree C++ files.

## Decisions and transient state

The Rust hole action takes the native request rank, request count, assigned-job
count, and unassigned-job count. It returns front, none, or the exact donor
rank; an invalid-state sentinel is checked by C++. For insertion, the queue
reports the position before insertion from the sizes of higher-priority buckets
plus the current bucket size for FIFO/back insertion, or no current bucket
contribution for front insertion. The assigned count is `jobs - unassigned`.
For assigned-job removal, the existing job search records its request rank; the
removed job still resides in `jobs_` after `ReleaseJob`, so the supplied count is
`jobs - unassigned - 1`. A donor is at rank `assigned` exactly when the hole
precedes that rank. Native traversal starts at the known hole and moves only to
that donor, followed by the original native `TransferJobBetweenRequests`.

For add or release, the native code has already pushed the job at the back of
the unassigned deque. Rust receives request count, unassigned count and
`jobs - unassigned`, and returns none or the rank of the first request without
a job. Native code maps that rank from `FirstMax`, calls `AssignJob` on the back
job and then pops the back. The front job is still used on the separate hole
path. Existing queue insertion order, priority propagation, auth-bound state,
preconnect accounting, job/request ownership, callbacks, timers and destruction
remain native. No pointer or iterator crosses FFI and no shadow scheduling state
or new heap allocation is introduced. The old boolean identity export and
16-pointer batching are absent from this successor.

Compared with baseline, insertion reads up to the fixed number of priority
bucket sizes, even when the action will take an unassigned job; this adds work
but does not walk request entries. The removal search visits the same assigned
prefix as baseline and counts its rank. A donor walk visits the entries from the
hole through the selected donor; the baseline additionally observed the next
entry to find the end. No-donor and no-pending release paths can avoid native
iteration. A release with a target walks the same prefix to its first missing
request. One Rust call occurs per hole or release decision. These are structural
observations, not a performance measurement; costs of the new FFI and bucket
reads remain unmeasured.

`oracle/original_actions.h` independently transcribes the original loop and
compares exact actions and donor/target ranks for valid insertion holes,
assigned-job removal holes including the still-owned removed job, available
unassigned jobs, and post-push release states. The earlier queue model is
preserved but remains incomplete for auth, timers and providers. New native
fixture source checks insertion ranks against actual queue positions and checks
steal/return behavior in the real pool. Pending mock callbacks in the latter
outlive the handles. None of these tests has been run.

## Files, ownership and validation

`production.patch` contains pool changes, Rust, reference and test sources.
`director-linkage.patch` contains the private ABI/export and GN proposal plus
`PriorityQueue::InsertionRank`; director review of this shared header and all
consumers is required. Both patches are required and apply only to the pinned
candidate. Exact after-image and patch SHA-256 identities are in
`materialization.json`; `materialize.py` reconstructs and applies both in an
isolated index and checks Git whitespace. The 19-file freeze and additional
fixture remain available for independent review. The source and material hashes,
patch applicability and scope were checked; Rust formatting was applied with the
available rustfmt. No C++ formatter was available, so C++ formatting was applied
manually and still requires normal Chromium formatting review.

The proposal is not compiled, tested, independently reviewed or accepted. Do not
run native validation on the preceding `1315bb` design. Following director
approval and normal laptop scheduling, the first failing-sensitive target is
`net_unittests` with:

```
--gtest_filter=TransportAssignmentPolicyTest.*:PriorityQueueInsertionRankTest.*:ClientSocketPoolBaseTest.AssignmentStealsLastAndReturnsReleasedJob:ClientSocketPoolBaseTest.CancelRequestReassignsJob:ClientSocketPoolBaseTest.HigherPriorityRequestStealsJob:ClientSocketPoolBaseTest.PreconnectJobGetsAssignedToRequest:ClientSocketPoolBaseTest.ReprioritizeResetFIFO:ClientSocketPoolBaseTest.PendingJobCompletionOrder:ClientSocketPoolBaseTest.IgnoreLimitsCancelOtherJob:ClientSocketPoolBaseTest.CancelBackupSocketAfterCancelingAllRequests:ClientSocketPoolBaseTest.StallAndThenCancelAndTriggerAvailableSocket
```

Actual compiler/link behavior, feature/platform consumers, auth-bound exclusion,
reentry and callback ordering, all relevant teardown cases, independent ABI and
ownership review, and the ordinary Network suite remain pending. Native code
still owns queue insertion/front and reprioritization policy, preconnect and auth
state, timer/provider and cancellation operations, and the unassigned deque's
mutation; the whole family is not cut over. Sanitizer/fuzz campaigns remain
deferred under the standing directive.
