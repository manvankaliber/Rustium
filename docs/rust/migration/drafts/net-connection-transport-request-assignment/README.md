# Transport request assignment: assigned-prefix milestone

Status: **proposal only; incomplete assignment migration, uncompiled, unrun and
unreviewed**. The desktop branch starts at `157c435f186f098c102e255c6af7f86c57f349bc`;
the production patches target exactly `712ca01f84b44e86ca53fb197ac54ac95b47b425`.
The source authority remains `bb1359ebe90ffbefb5f67a85bdf001f44415de3d`.
`source-identity.json` records original and candidate helper regions and hashes.
The 19 original blobs frozen by `fc2d429f3493ade4272d3b1e2ade96a81ba03d41`
were verified; the original/candidate assignment, AddJob, RemoveUnboundJob,
SanityCheck and priority queue regions are byte-identical.

## Decision and boundary

The real `Group::GetFirstRequestWithoutJob` and
`Group::TryToAssignJobToRequest` use Rust's scalar prefix decision in batches of
at most 16. For release/new preconnect, the first missing request is selected;
for insertion or removing an assigned job, the last assigned request after the
gap is selected for transfer. Rust also selects use of the available deque
front before scanning. Native code only projects job-presence bits in existing
queue order and translates the returned boundary into its own iterators. The
native `unassigned_jobs_` still owns FIFO order; release still pushes the job to
its back and immediately assigns that same back item when a request is waiting.
No native pointer, iterator, request, job, callback, timer or ownership crosses
FFI. There is no persistent shadow state and no new heap allocation. The ABI
returns the first zero index in `0..count`, or -1 on invalid count; native code
checks the result before indexing. Every native `AssignJob`, `ReleaseJob`,
`ChangePriority`, timer stop and sanity check stays in its original sequence.

This is an incremental ownership slice. Native code still decides FIFO versus
front insertion for respect-limits, reprioritization including its no-op case,
request/job removal, the push/pop order of the unassigned deque, binding to auth
and its first error/generation, preconnect accounting, provider calls, callbacks,
reentry, cancellation, backup timers and teardown. Those behavior-bearing
policies are **not Rust-owned** and complete assignment cutover is not claimed.
The projection may observe up to 15 additional job pointers after the first
unassigned request and constructs a fixed iterator batch; this changes hot-path
work and needs performance review and later accepted measurement. It performs one
Rust call for each nonempty batch plus the front-or-scan decision. The earlier
stalled ranking and idle code remain intact at this candidate base.

The independent reference is `oracle/prefix_original.h`, based on the original
C++ loops, with its limits described in `oracle/README.md`. The earlier queue
model remains partial: it does not model removal of a job, auth, timer or
preconnect accounting. Its state transitions cannot replace production fixture
assertions. `transport_assignment_unittest.cc` proposes exhaustive bit-pattern
comparison against the original prefix and multi-batch boundary sequences. The
production fixture `AssignmentStealsLastAndReturnsReleasedJob` adds assertions
for two assigned jobs, a higher-priority insertion and cancellation return.
Neither source has been compiled or executed.

## Materialization and validation

`production.patch` and the separate director-owned `director-linkage.patch`
require each other. `materialization.json` records each before/after SHA-256 and
patch hash. `material/` contains raw Git-object-derived after-images; do not
copy them over another revision or existing edits. `materialize.py` builds and
applies both patches in order to an isolated index at the pinned candidate and
runs the Git whitespace check; it writes objects and packet files only. The
application check and raw hash verification are source checks, not builds or
behavioral validation. No changes were applied to the configured tree and no
compiler, test binary, GN, Cargo, Ninja, benchmark or external push was run.

After director ABI/GN review and an approved quiet laptop validation slot, build
`net_unittests` against the exact combined candidate, then first run and retain
full logs and exit status for:

```
net_unittests --gtest_filter=TransportAssignmentPolicyTest.*:ClientSocketPoolBaseTest.AssignmentStealsLastAndReturnsReleasedJob:ClientSocketPoolBaseTest.CancelRequestReassignsJob:ClientSocketPoolBaseTest.HigherPriorityRequestStealsJob:ClientSocketPoolBaseTest.PreconnectJobGetsAssignedToRequest:ClientSocketPoolBaseTest.ReprioritizeResetFIFO:ClientSocketPoolBaseTest.PendingJobCompletionOrder:ClientSocketPoolBaseTest.IgnoreLimitsCancelOtherJob:ClientSocketPoolBaseTest.CancelBackupSocketAfterCancelingAllRequests:ClientSocketPoolBaseTest.StallAndThenCancelAndTriggerAvailableSocket
```

First verify actual target, binary, toolchain, component/export configuration,
source hashes and runner state; preserve raw evidence. Test traces must include
empty and no-eligible prefixes, first/last assignment, the 16-element boundary,
repeated same-priority requests, front insertion, reprioritization, preconnect
claims, assigned and unassigned job removal, cancellation/backup, auth-bound
exclusion and callback/reentry ordering. Existing or additional real-pool
fixtures must demonstrate those cases before acceptance; the scalar differential
alone cannot. Full consumer/platform validation and independent ABI/lifetime
review are pending. Sanitizer and fuzz campaigns remain deferred by the user.
