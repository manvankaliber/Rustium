# Decisions

## 2026-11-17: reference provider injection and build preflight

Connection reference qualification must retain production injection semantics. A non-null injected authentication mechanism needs a separately named, source-bound and independently reviewed reference implementation, with observations of its actual policy entries. Arbitrary callbacks cannot be declared independent. Default NTLM, SSPI, GSSAPI, Negotiate and Digest reference routes still require their complete helper, callback, cancellation and logging closure; this decision grants no acceptance to the current proposals.

The local build expert delivered the throughput report under `drafts/net-native-build-throughput`. Changed-source object compilation now precedes the complete required consumer build, and dirty-edge explanations are retained. Failed preflight and full builds remain archived. This changes feedback order, not acceptance membership or compiler checks. Current concurrency stays one; any future two-job trial requires measured memory headroom and telemetry within the existing maximum and link pool.

## 2026-11-17: Network focus and Terra workers (current)

The user changed future coding workers to Terra and requested one active subsystem, finishing Network before unrelated work. Coding-worker defaults are now `gpt-6.1-terra`/xhigh in project config, role config and the queue. The global ceiling remains eight coding workers plus the existing auxiliary allowance; it is model-neutral. Historical assignments retain their actual model/effort. Coordinator claims enforce the active Network focus, with the already-applied Strings bridge explicitly allowed because it is a dependency of the current Network cohort.

Blink Text is held clean at `0d89e8f35fdc4ef5370f89e96213867c66008b55`; its worker has ended and was cancelled against further runs. The Version bridge source-freeze is held clean at `295b0e3938da20e86ddc91470362794e03c2fc6b`; no production derivative was authored. Retain both branches and their workspaces; no unrelated new dispatch. Prior integrated draft artifacts remain evidence, not production cutovers.

An independent Sol/high build expert is investigating actual retained native build logs, evaluated dependencies and local CPU/RAM/disk constraints. No hired service, API billing, remote execution, changed compiler checks or parallelism above the two-job ceiling is authorized. Recommendations need source-bound review and gated validation before changing configured inputs. The existing goal and usage cutoff remain unchanged.

## 2026-11-17: repository-wide continuation

The user expanded active scheduling to all Chromium-derived subsystems. Existing networking work is retained, not restarted: its source-bound acceptance ledgers and fixed benchmark batches remain authoritative. The repository queue coordinates assignments across those ledgers and new families. Sanitizer/fuzz campaigns and security-focused side campaigns remain off. No automatic pushes/publication/messages.

## Roles and global budget

Preserve Sol/xhigh director, Terra/xhigh managers, independent reviewers and coding workers, and Sol/high temporary specialists. The original Luna/high worker default is superseded by the current user directive. Root configuration requests ten children; the observed native runtime allows ten children plus the parent. Eight reserved or active coding workers across every participating session is the global ceiling, with two auxiliary slots used on demand. All dispatch goes through the director, so nested delegation support is unnecessary. Runtime settings can override project defaults; inspect actual model identity rather than assuming config changes existing sessions.

## State and recovery

One JSON queue, atomic same-directory replacement, short OS file locks and a persistent director receipt are sufficient. Workers return results rather than concurrent edits to queue copies. A shared Git common directory routes all worktrees to the same primary state/runtime location. Interrupted reservations survive; stale timestamps do not reclaim them. A persistent build marker prevents another job after an interrupted launcher while a child may still be running. This is cooperative local coordination, not enforcement against arbitrary manual tools.

## Storage and builds

Reuse the configured checkout and caches. Sparse Git worktrees share objects and are bounded/reusable; no worker gclient checkouts, historical full-checkout accumulation or dependency duplication. Existing `tools/git/worktree.py` remains available but its full gclient workflow is inappropriate for this pool. Native helper creates sparse worktrees; T3-owned workers must use T3's isolation facility and inspect its storage behavior. No remote execution. One substantial job initially, two compiler/linker jobs maximum on the 16 GB host; lower to one under pressure. Pending validation is bounded at eight and repairs take precedence.

Disk inspection found approximately 42.7 GiB logical bytes in D:/Rustium and 65.4 GiB in D:/RustiumBuild. The two CI preview directories are small configuration exports, not full Chromium checkouts. Editing .scratch accounts for approximately 18.9 GiB, .git 17.8 GiB; both contain valuable history/artifacts and are not cleanup targets without provenance/liveness review. Logical file sizes are not exact reclaimable disk allocation (compression, sparse files and shared storage can differ). About 28 GiB was free before setup. No existing directory was deleted. Worker allocation refuses below 8 GiB free; this is a workspace reserve, not a guarantee a browser build will fit.

## Candidate milestones and production integration

Initial independent tasks prepare real candidate families, fixtures, reference closure and integration proposals in exclusive draft directories. This avoids mutating the large existing dirty production tree concurrently. Candidate commits carry no production/cutover credit. The director creates explicit integration/bridge dependency tasks as soon as a candidate's contract is ready; do not accumulate endless candidates or count draft delivery as a completed rewrite. Networking recovery/repair remains first in delivery priority. Stable production integration includes callers and combined validation.

## V8 is a nested source repository

The editing checkout's v8 directory is empty; the configured checkout has V8 at a1175dcfe49146bd7165715a9919eb4e94be0e77 with dirty BUILD.gn and gni/v8.gni. Use read-only pinned helper copies for the initial LEB candidate. Future production integration must explicitly own the nested repository and preserve its local build edits. Do not report V8 missing or silently copy/sync the entire dependency tree.
