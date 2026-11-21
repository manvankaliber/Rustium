# Transport scheduling proposal: stalled-group decision slice

Status: **candidate only; incomplete assignment scope; uncompiled and unrun**.
Task checkout base: `4fadffffebb3a76fd8604245b29e9f4f5cd2cddc`.
Production patch base: `72ee5ddd7129a311a92b49f3705a031e68cbf4ca`.
Original reference: `bb1359ebe90ffbefb5f67a85bdf001f44415de3d`.
The oracle commit precedes all Rust policy work. `source-manifest.json` records
19 original blobs and the original/rebind manifests; `ORACLE.md` describes the
standalone transcription and its limits.

## Proposed boundary

`production.patch` changes the real `FindTopStalledGroup` entry point and adds a
scalar Rust policy, independent C++ model header and differential test source.
The loop keeps the map in native GroupId order, evaluates native unbound-request
and socket-slot eligibility, packs at most 16 eligible priorities into a scalar,
and calls Rust once per nonempty batch. Rust selects the first strictly higher
priority relative to the previous batch winner; equal priorities preserve the
first native map entry. The native code retains winner pointers locally. No
native pointer crosses FFI, and there is no heap allocation in this projection.
The priority nibble is valid for the pinned `RequestPriority` range 0..5; the
production source also has a compile-time upper bound guard. The existing idle
policy is not replaced. `director-linkage.patch` proposes the private exported
C++ wrapper, Rust static-library target, net component dependency and test source
registration. The director owns review and application of these shared surfaces.
These two patches are complementary and both are required for a build.

This is deliberately only the stalled-group ranking slice. The actual native
queue remains the authority for respect-limits FIFO/front insertion, same-priority
no-op and remove/reinsert reprioritization, the assigned prefix, unassigned job
FIFO, immediate released-job assignment, stealing the last assigned job, priority
propagation and preconnect claims. Auth-bound request/job exclusion, generation,
first pending error, backup timer, cancellation, destruction, callbacks, reentry,
socket and job providers also remain native. Moving those policies requires a
separately reviewed boundary that does not duplicate the decisions or repeatedly
cross FFI while traversing native iterators. The standalone oracle model is not
proof of production ownership or behavior, and it does not model auth, timer,
callback or provider execution.

## Materialization and verification

`materialization.json` binds every proposed after-image and both raw patch hashes
to the exact production base. `material/` contains those exact after-images,
including full Git-object-derived C++ and GN files for review. Do not copy them
onto a different revision or overwrite local edits. `materialize.py` reconstructs
both patches using only an isolated Git index, checks each patch against the
pinned tree, applies both to that index and runs `git diff --cached --check`.
It writes Git objects and packet files, never checks out production files. Run it
only in an appropriate isolated review environment; compare resulting hashes to
the retained manifest before accepting an exact identity. The original 19 Git
blobs and the three rebind region hashes were checked against both input
manifests. All nine named existing fixture cases were found in the pinned source.
The staged packet raw SHA-256 values and patch application checks passed; no
compiler, test, GN, Ninja, Cargo, benchmark, sanitizer or fuzz run was performed.
The source-only result is neither independently reviewed nor integrated.

## First laptop validation request

After director review, exact-source materialization, GN/ABI review and laptop
runner qualification, build `net_unittests` using the approved one-job validation
gate and then run the following as the first failing-sensitive filter (retain the
actual binary/build configuration, source hashes, full logs and exit status):

```
net_unittests --gtest_filter=TransportSchedulingPolicyTest.*:ClientSocketPoolBaseTest.TotalLimitRespectsPriority:ClientSocketPoolBaseTest.ReprioritizeResetFIFO:ClientSocketPoolBaseTest.CancelRequestReassignsJob:ClientSocketPoolBaseTest.HigherPriorityRequestStealsJob:ClientSocketPoolBaseTest.PreconnectJobGetsAssignedToRequest:ClientSocketPoolBaseTest.PendingJobCompletionOrder:ClientSocketPoolBaseTest.StallAndThenCancelAndTriggerAvailableSocket:ClientSocketPoolBaseTest.IgnoreLimitsCancelOtherJob:ClientSocketPoolBaseTest.CancelBackupSocketAfterCancelingAllRequests
```

The scripted differential cases cover empty and ineligible inputs, equal
priorities, changing winners and batches larger than 16. They compare policy
outputs to the frozen C++ model, not actual native Group objects. Existing pool
fixtures cover portions of connection scheduling but have not been executed
here and are not a substitute for a targeted native equal-priority GroupId-order
fixture, auth-bound exclusion, and callback/timer/reentry verification. Add or
identify those production-path assertions before acceptance. Compile and run the
standalone `oracle/oracle_main.cc` separately on the laptop too. Full native
consumer builds, cross-platform validation, independent ABI/ownership review and
all normal Network acceptance remain pending; sanitizer/fuzz campaigns remain
deferred under the standing override.
