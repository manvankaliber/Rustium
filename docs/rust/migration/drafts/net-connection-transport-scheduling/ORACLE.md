# Independent source and oracle freeze

The 19 files under `frozen/` are raw `git show` blobs from the baseline commit;
`source-manifest.json` records their blob IDs, SHA-256 hashes and the independently
verified rebind regions. They include the actual request queue, priority queue,
assignment helpers, job providers, capacity decisions and timer implementation.

`oracle/oracle.h` transcribes the baseline's queue order, assignment, release,
reprioritization and stalled-group selection into standalone C++ containers.
It is a model with an executable `oracle_main.cc`, not a claim that Chromium's
original code is compiled verbatim or that the model covers every native path.
The frozen originals remain authoritative. GroupId ordering is represented by
already ordered input; eligibility is supplied by native state. The original
`Group::BindRequestToConnectJob`, `RemoveUnboundRequest`, `SetPriority`,
`TryToAssignUnassignedJob`, `TryToAssignJobToRequest`, `GetFirstRequestWithoutJob`,
`PriorityQueue::Insert`, `InsertAtFront` and map iteration must be reviewed with
any extension. Job and request ownership, provider state, auth binding, callbacks,
reentry, errors and timers remain native and need the real production fixtures.
No oracle execution or compilation has occurred on this desktop.
