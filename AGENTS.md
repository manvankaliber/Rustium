# Rustium local Rust migration

## Startup: go / continue / resume

These instructions mean autonomous continuation of the repository-wide migration, not a request for a plan. Read [OPERATING.md](docs/rust/migration/OPERATING.md), [HANDOFF.md](docs/rust/migration/HANDOFF.md), [queue.json](docs/rust/migration/queue.json), and [ownership.json](docs/rust/migration/ownership.json). Reconcile actual Git branches, dirty files, worktrees, live sessions/processes, pending validation and review results before dispatch. Recover interrupted work without discarding it or assuming old sessions are dead. Select ready work, reserve global worker slots, deliver, review, validate, repair and integrate; checkpoint after meaningful events. Continue while useful authorized work remains. Do not stop after one worker round. On stop, cease dispatch and checkpoint active work safely.

## Scope, roles and ownership

Replace Chromium-derived C++ incrementally across **all subsystems**, preserving a working browser. Networking is already in progress: read `docs/rust/net-migration-execution.json` (especially `resumed_delivery_intake`) and `docs/rust/net-migration-plan.md`; preserve all existing drafts, references and evidence. The new queue coordinates work; existing networking ledgers remain the acceptance authority. New repository-wide scheduling supersedes historical network-only team counts and push authorization.

Director: Sol xhigh (`gpt-6.1-sol`). Managers and independent QA: Terra xhigh (`gpt-6.1-terra`). Coding workers: Luna high (`gpt-6-luna`), **eight active globally**, including reserved/interrupted assignments. Temporary specialists: Sol high. Models must not silently fall back. Managers request workers; only the director dispatches. Native Codex and separately launched sessions both consume the shared pool. Use the repository queue/lease helper; per-session config is not a global budget. Actual tool/runtime limits always apply.

Each editing worker needs an exclusive task branch/worktree. Prefer sparse checkouts, retain branches after delivery, and reuse at most eight worker workspaces plus necessary integration/review space. Check D: free space and existing copies before allocating; never duplicate the configured Chromium checkout or run worker `gclient sync`. Shared GN/toolchains, Rust exports/types, bridges, IPC/Mojo, task/thread contracts, `net/http/http_util.cc`, benchmark gallery and configured-tree writes belong to the director/integration owner. Workers submit proposals for these surfaces.

## Engineering and delivery

Inspect callers and preserve errors, ownership, lifetimes, threading, cancellation, ABI and platform behavior. Freeze independent original C++ algorithms **and helper dependencies** before changes. Implement coherent connected families, with tests prepared alongside implementation; integrate buildable milestones frequently, not a whole-browser uncompiled handoff or per-function benchmark loop. Unsafe code must be narrow, justified and independently reviewed.

Distinguish candidate, validated/integrated behavior, Rust ownership behind a documented bridge, and completed subsystem cutover with relevant C++ retired. Forwarders, disabled code, placeholders and C++ core delegation are not completed rewrites. Preserve remaining native/platform/provider debt. A host build does not prove other platforms. Never weaken acceptance or promote stale evidence.

**Sanitizer/fuzz campaigns and security-focused side campaigns remain OFF** under the standing user override. Security, sandboxing and process boundaries still have explicit owners and require ordinary correctness/ABI review. Record deferred validation, never mark it passed or claim final cutover while required checks remain deferred.

## Local resources and evidence

Use only this computer and the existing ChatGPT subscription. No API billing, purchased services, external task store or remote execution. No automatic pushes, release publication or messages to others. Task-local commits and reviewed local integration are authorized; preserve unrelated dirty work.

Use `python tools/rustium_coord.py check` and `status`. The director lease, global claims and one validation gate are documented in OPERATING.md. Initially **one substantial job; at most two compiler/linker jobs** (reduce to one under memory pressure). Keep timing free of competing builds/tests. No worker full-browser builds. Bound pending validation to eight deliveries, then prioritize repair/integration.

`D:/Rustium` is the inspected editing checkout; `D:/RustiumBuild/src/out/Default` is the existing configured build. Recheck paths on restart. Do not alter a running build's inputs. Synchronize only intended, hash-checked files after checking destination changes; never broad sync/reset. Freeze copied, hash-bound inputs for long runs, no hard links to mutable outputs. Evidence applies only to its exact identity.

Read and obey [benchmark-policy.md](docs/rust/migration/benchmark-policy.md) before measurement/import/export/site work. Archive **every** run/regression, retain raw provenance and uncertainty, hold publication until the complete batch passes its gate or residual regressions have demonstrated blockers and concrete follow-ups. Use archive-only imports while held. Keep the canonical offline gallery and dashboard in sync only after accepted canonical imports. Do not publish pending work or redefine batch membership to hide regressions.
