# Native transaction and ownership handoff

This is a proposed mapping for `54f6f4df827d30db2762557c7442056cc8a3a0a4`,
not implemented production behavior. `RequestQueue` is the sole authority for
order in the proposed design. Native code separately owns each `Request` and
`ConnectJob`; a native ownership table maps a generated, nonzero scalar token
to the request, and an identity lookup maps handles/jobs to those tokens. Such
lookup maps are not ordered scheduling state. A token is never a request
address, is never recycled during the group's life, and remains the same across
reprioritization. Overflow, native object-table insertion, allocation failure,
Rust queue allocation, and token generation need explicit production rules.
No Rust borrow, returned slice, or Rust lock may survive a native call that can
reenter. A future ABI needs an audited opaque queue handle, a bounded scalar
snapshot buffer, no escaping native pointer, and no panic/unwind across C++.
None is supplied in this prototype.

## Insertion and assignment

1. After `RequestSocketInternal` returns pending, native code owns the new
   `Request` and reserves a fresh token. Establish the token-to-owner mapping
   and Rust queue insertion as one reviewed handoff; on failure neither may
   leave an externally visible orphan. The Rust queue receives native assigned
   and unassigned-job counts and returns the newly committed rank and an action.
2. If Rust chooses the unassigned front, call `AssignJob(front)` and then
   `pop_front`, in that order. If it chooses a donor token, native resolves it,
   calls donor `ReleaseJob`, then destination `AssignJob`. Otherwise do nothing.
   `AssignJob` can call `ChangePriority`; preserve this point and priority.
3. Run the existing sanity and later socket-layer/post-task behavior. Never
   expose the temporary hole as a completed, consistent transaction. The core
   itself performs no callback and retains no native object.

## Removal of a job still in native ownership

Find the assigned request for the job using a native job-to-token lookup or a
single ordered token snapshot plus native resolution; do not export a job or
request address to Rust. The native `jobs_` entry is still present. Call
`ReleaseJob`, then ask the Rust queue for the hole action with
`jobs_.size() - unassigned_jobs_.size() - 1`. Apply front/donor as above, then
remove the native job, clamp `never_assigned_job_count_`, stop the backup timer
if the last job disappeared, and perform the original sanity check. Native
code must not assume a queued token survives a provider call without the
existing lifetime and reentry guarantees being proved.

## Remove a request and bind auth

For a normal cancellation or pop, detach the token from Rust order and remove
native ownership in the same externally indivisible handoff. If the request has
a job, native code releases it, pushes the job to the unassigned deque back,
then queries `select_released` using the post-push counts. Resolve the target
and call `AssignJob(back)` before `pop_back`. Stop the backup timer only when
the queue becomes empty, at the existing point, then return the owned request.

For auth, native first checks existing bound pairs. Otherwise it obtains the
front token and checks the native request's proxy-auth callback. If absent,
return without changing state. If present, perform the full pop/removal and its
job reassignment above; only then remove the specified `ConnectJob` using the
job-removal sequence, log the binding and store the native owned request/job,
generation and first-error state. The reassignment may change which request is
associated with that job between the two removals; caching an earlier donor or
job association would be wrong.

## Reprioritization

Locate the token by native handle. If the queue reports the same priority,
return without detaching, assigning a job, stopping a timer, or changing FIFO
position. For a changed priority, detach the same token, perform the complete
native request-removal/release/reassignment sequence, update the native
request's priority, and insert that token again through the insertion sequence.
The original has a debug assertion about ignore-limits requests after removal;
the standalone core rejects such invalid changes before removal and is not yet
behavior-equivalent for this invalid-input/debug path. A production bridge must
make that difference an explicit reviewed decision rather than silently ship it.

## Integration blockers

* `take_for_priority` and the subsequent `insert` are separated by native job
  operations. If the second queue reservation fails, the old order and possibly
  job assignment cannot simply be restored. Pre-reserving alone is insufficient
  unless callbacks/reentry cannot consume the reservation or a transactional
  resource guarantee is designed. The core does not solve this multi-step
  failure/rollback problem.
* Native provider dispatch and group deletion/reentry must be audited through
  `AssignJob` -> `ConnectJob::ChangePriority` -> `ConnectJobRustBridge::Run`,
  auth callbacks, backup timer, teardown and posted tasks. No invariant here
  proves that a request, token, group, or borrowed native object remains live
  across these calls. The proposed mapping and an ownership-safe lifetime guard
  must be reviewed against the existing native behavior before wiring it.
* The prototype checks duplicate live tokens, but the native non-reuse rule is
  not enforced across detach/reinsert. Token exhaustion, generation ownership,
  unexpected reentry and object-table atomicity remain unspecified. There is no
  audited ABI, GN linkage, cross-platform build or compatibility proof.
* Current token lookup and duplicate checking are linear in queue length. The
  production queue's costs, one-crossing batch snapshot needs, pointer stability,
  allocation behavior and memory footprint require a concrete benchmark plan
  after the standing hold is lifted. This candidate has no performance evidence.

Until those blockers are resolved, no production `Group` should use the new
queue, and the existing PriorityQueue must remain the sole production order.
