# Retained-node transaction and native owner mapping

The director-supplied existing contract is authoritative: the Group and
relevant native objects survive, and same-Group request/job ownership is not
mutated while `AssignJob` / `ChangePriority` executes during a queue transaction.
Injected providers are subject to this same Group contract. It is not a ban on
provider work: the independent audit reports that transport/TCP priority can
synchronously reach `HostResolverManager::Job` and
`PrioritizedDispatcher::ChangePriority`, which can call `Job::Start`; standard
DNS/system/mDNS completion is asynchronous. SOCKS/SSL forward nested priority,
and HTTP proxy forwards nested priority with current stream-priority no-ops.
Isolated ConnectJob deletion/recursion injections do not establish Group-safe
reprioritization seams. The frozen candidate chain includes `AssignJob`,
`ConnectJob::ChangePriority`, `ConnectJobRustBridge::Run`,
`Action::ChangePriorityInternal` and virtual transport/SSL/SOCKS/HTTP proxy
entry points. Preserve their behavior; do not add reentry guards.

## New insertion

Generate the nonzero token inside the Rust queue while allocating its boxed
node and ordering it in the appropriate priority list. Establish the native
owning-map entry for the returned token before calling any native operation that
can observe the queue. There is no callback between these steps. Native C++ OOM
and Rust allocation failure terminate; the adapter must CHECK a Rust allocation
status rather than turn it into a recoverable socket error. Apply Rust's action:
for front call `AssignJob(unassigned.front())` then pop front; for donor resolve
the token in the native owner map, call donor `ReleaseJob`, then destination
`AssignJob`. Both retain the native priority propagation order. No Rust borrow,
iterator or lock survives the ABI call.

## Changed priority and same-priority no-op

Find the request token by handle in native ownership lookup. Rust detach returns
unchanged without mutation when priorities match. Otherwise it unlinks the
existing boxed node, retains that exact Rust allocation and token, records the
requested priority, and omits it from all scheduled views. Native extracts the
corresponding `std::map` ownership node, with no callback between Rust unlink and
native extraction. The extracted node keeps the native Request alive while
unbound handle lookups cannot find it. If the request has a job, native releases
it, pushes the job back, selects the first unassigned token, invokes `AssignJob`
then pops the back; perform the original backup-timer stop if the ordered queue
is empty. Only after this removal/reassignment does the original
ignore-limits DCHECK occur; then native changes Request priority.

Rust finish relinks the retained box at its pending priority without allocation
and returns the rank and assignment action. Native reinserts the *same* C++ map
node into the same map with equal allocator, without allocation, before applying
that action. No callback occurs between Rust relink and native owner insertion.
Within the supplied same-Group contract, this addresses the prior allocation
rollback and speculative same-Group reentry concern. The C++ sketch is not a
production Group conversion, and the nonallocation property still needs native
allocator observation and standard-library/toolchain verification.

## Job removal, cancellation and auth

When removing an assigned native job, the job remains in `jobs_` across its
`ReleaseJob`. Resolve its owning request token, release the job, and ask Rust
for a hole action with `jobs.size() - unassigned.size() - 1`; apply the native
front/donor transfer before erasing the job, clamping preconnect counts and
stopping the backup timer when appropriate. No native pointer crosses the ABI.

For request cancellation/pop, remove the token from Rust order and take its
native owner without a callback between the two changes. Release its job and
assign from the back as above; retain the original timer stop point. Auth
binding first checks existing bound pairs, examines the first unbound Request's
native auth callback, and only when it is present performs the entire pop and
possible reassignment. Then remove the specified native ConnectJob, log, and
store the native bound pair, generation and first pending error. Do not cache
a job-to-request association across the pop, which may reassign jobs.

## Remaining implementation and validation

The retained core and `proposal/native_owner_adapter.h` are **not wired into
Group**. Every existing queue consumer, including handle/job lookup, assigned
prefix sanity checks, Group inline counts, stalled-group top priority, backup
timer, cancellation, auth binding and teardown must be converted coherently.
The adapter sketch still needs real Group integration, exact handling of
teardown and outstanding extracted nodes, private ABI ownership/link review,
and caller-visible debug/release equivalence, including post-removal DCHECK.
No independent review has found a remaining provider-lifetime blocker under the
provided contract; these are unfinished implementation and validation work,
not a claim that arbitrary reentry must be forbidden.

`RequestQueue::create` can return allocation failure from `try_reserve`; the
adapter terminates on all non-success statuses. `Box::new` follows the Rust
allocator's terminating handler. Token exhaustion is also fail-stop and needs
normal platform review. No allocation occurs in the Rust detach or finish paths
by code structure; the standalone global-allocator probe and C++ map-node
behavior have not been compiled or run. `HashMap` rehash can move the owning
box pointer but not the boxed node; no node address is exposed. Snapshot
enumeration allocates a native token vector and token lookups use maps; cost and
platform compatibility remain unmeasured. Production must use the configured
abort/no-unwind Rust ABI and independently review every unsafe bridge precondition.
