# Oracle boundaries

`prefix_original.h` independently transcribes the original prefix traversal;
its source authority is the raw baseline Group and PriorityQueue sources frozen
at `../net-connection-transport-scheduling/frozen/`, not Rust output. The earlier
`oracle/oracle.h` models queue insertion, reprioritization, AddJob, and request
removal. It is useful for scoped sequences but omits RemoveUnboundJob, auth-bound
pairs, `never_assigned_job_count_`, provider callbacks, backup timers, and
teardown. It must not be treated as a complete pool oracle. In particular,
removing a job and removing a request have different transient state and must
not be equated. The native fixtures remain necessary. No compiled oracle or
native fixture has been executed here.
