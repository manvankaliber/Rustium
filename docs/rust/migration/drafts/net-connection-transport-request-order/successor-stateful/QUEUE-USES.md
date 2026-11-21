# Native queue use inventory

At the pinned candidate, `unbound_requests_` is read by `Group` construction,
destruction and `SanityCheck`; backup timer firing; removal of all jobs;
`GetConnectJobForHandle`; `GetNextUnboundRequest` and `PopNextUnboundRequest`;
`FindAndRemoveUnboundRequest`; `SetPriority`; handle/job lookup for testing;
`FindUnboundRequestWithJob`; assignment/release; and the inline group methods
for pending request count, emptiness, available slots and top pending priority.
The real insertion and erasure sites are `InsertUnboundRequest` and
`RemoveUnboundRequest`. Auth binding combines front lookup, pop and job removal.
The stalled-group selector consumes native `TopPendingPriority`; it must receive
the priority of the Rust-selected first token, not recompute a separate order.

Any future production conversion must account for each consumer, all native
Request/ConnectJob ownership and the transaction order in `TRANSACTIONS.md`.
A scalar-token index may accelerate lookup but cannot become a second ordered
queue. The new prototype has no adapter and does not touch these consumers or
generic PriorityQueue code.
