# Subsystem delivery through GitHub Actions

The user selected the linked Windows laptop **HQC-VAN_KALIBER** as the compilation
machine on2026-11-17. All compilations run there; the desktop retains editing and
targeted tests that do not invoke a compiler. Cargo tests that compile must build
on the laptop. Network remains the sole active subsystem; coding workers and QA
use Terra/xhigh. The local two-job experiment was rejected. The former local build
was interrupted and its partial result archived; do not restart it here.

One Network integration PR collects the connected implementation, independent
original/reference closure, production bridges and assertion-complete tests.
Workers deliver reviewable task branches into that PR. A source checkpoint or
unrun draft is not subsystem acceptance. Independent review and CI failures feed
repairs to the same PR until required checks pass. No automatic merge or release.
Retain the distinctions between build/test acceptance, Rust ownership and final
C++ retirement. Keep the complete36-program232-workload ordinary batch and
separate16-lifecycle/optimized-QUIC obligations; campaigns remain off.

## Worker handoff and CI feedback

1. Each Terra worker commits the change and its tests on the assigned task
   branch. The handoff names the exact commit, affected targets and tests, and
   any remaining dependencies or review questions.
2. The director reviews the handoff, assembles the reviewed commits on the
   Network integration branch, and records the exact PR head. The user allows
   ongoing pushes to the public Rustium repository. The director assigns who
   pushes, and a Terra worker may push its own assigned branch. The director
   owns updates to the shared integration PR. For fresh build qualification, a
   maintainer dispatches the reviewed commit and its approved input record
   explicitly. Each new head or probe run gets its own dispatch.
3. GitHub Actions builds that revision on the Windows laptop. The run contains
   compiler and linker output and build artifacts. Link the run from the PR
   delivery record with the tested commit, outcome and actionable failures,
   then assign each failure to its owner. Push reviewed fixes to the same PR
   and use the new head's result for the next decision. Automatic PR checks
   await a trigger that approves a specific head and qualified inputs.
4. While the laptop builds, Terra workers continue independent implementation,
   review or test preparation in the active Network family.
   Keep at most eight coding workers and eight deliveries awaiting validation.
   At that limit, prioritize repairs and integration. Work on unrelated
   subsystems remains on hold until the active focus changes.

## Fast repair feedback

Connection Jobs is first. The maintenance operator or GitHub Actions uses the same checked-in driver. Use the existing prepared laptop checkout, ordinary component output, and cached Python environment. Keep one native job under `rb/.rustium-native.lock`. Let an active run finish at its recorded SHA before preparing the next head. Collect waiting fixes for the latest reviewed SHA into one dispatch, with the original failure first.

For a reviewed revision prepared under the laptop reservation, dispatch `network-consumer-build.yml` on its exact ref with `revision` and `feedback_targets`. For example, send `connection_jobs_family_rust_unittests` and the optional `gtest_filter` of `ConnectionJobsHttp2BaselineTest.SessionCloseCompletesPendingJobExactlyOnce`. A comma-separated target list runs each build and test in order and stops at the first failure. Leave `approved_inputs_sha256` empty for this diagnostic. The workflow can be used through GitHub once it is registered on the default branch. Until then the linked laptop can use the identical entrypoint directly:

```powershell
# Run on HQC-VAN_KALIBER in the prepared, reviewed checkout.
$revision = '<full reviewed PR head>'
$receipt = Join-Path "$env:USERPROFILE/rb/evidence" ("feedback-" + [guid]::NewGuid().ToString("N"))
$python = Join-Path "$env:USERPROFILE/chromium-tools/depot_tools" 'python3.bat'
& $python -I -B tools/ci/build_network_consumers.py --revision $revision --evidence $receipt --feedback-targets connection_jobs_family_rust_unittests --gtest-filter ConnectionJobsHttp2BaselineTest.SessionCloseCompletesPendingJobExactlyOnce
```

Prepare the exact reviewed commit and revision metadata using the instructions below. Once per builder configuration, provision the cached vpython environment, generated `testing/location_tags.json`, and ordinary output. The driver sets the vpython search path, short temporary directory, external Chromium log path, and `CR_SOURCE_ROOT` for every focused run. It checks the reviewed head, driver, family selection, clean checkout, and ordinary GN arguments, and regenerates the graph in the existing output. Automatic parallelism is four Ninja jobs with a one-link pool if both available RAM and commit headroom are at least 4 GiB, otherwise one job (minimum available RAM 2 GiB). The operator may lower the ceiling. QUIC-enabled or probe targets use their separate outputs.

The run prints compilation and test outcomes separately as each completes. Read the first failed phase in `progress.json` and its raw log. The terminal `result.json` records the commit, target, elapsed times, and focused outcome. Return those to the repair owner immediately. After a successful reproduction, omit the filter for the whole family suite. Use fresh approved inputs and the complete acceptance inventory for family signoff. Track time from dispatch to first result and from first result to signoff separately.

## Initial builder connection

Repository: https://github.com/manvankaliber/Rustium . Initial inspection found no
registered self-hosted runners. The laptop is now linked, reachable and has full
access under environment39b1955f-e0f8-4d4a-ad20-0a3449c1b0eb. It is Windows x64.
T3 can now run Codex Terra/xhigh there. The existing maintenance project is
`C:/Users/micro/.t3/projects/laptop-buildvoorbereiding`; its host-maintenance task
has completed and released ownership. GitHub CLI 2.104.0 is installed and logged
in as `manvankaliber`, with repository admin access. Runner **21**, named
`HQC-VAN_KALIBER-rustium`, is registered and independently observed online/idle,
with labels `self-hosted`, `Windows`, `X64`, `rustium-build`. The pinned package
and installer hashes matched, and the temporary token file was removed. Earlier
missing-provider/CLI and zero-runner observations are historical; see
`evidence/linked-builder-runner-online-20261117.json`.

Initial execution mode is a per-user interactive runner under
`C:/Users/micro/actions-runner-rustium`, using `HQC-VAN_KALIBER\micro`. This avoids
service-install elevation, but the runner process must remain running to receive
jobs. Its listener version check passed. The three previously blocked programs
(existing C++ smoke executable, CIPD and vpython help) subsequently exited0 with
no matching new Code Integrity events. The stored Smart App Control value was0;
although the user explicitly permitted turning it off, no SAC/Defender setting
was changed by this task. Defender antivirus reported enabled and real-time
protection false before and after; retain that observation without attributing
it to runner setup. Scoped ATL/MFC installer execution exited0; both components
were reported present and the instance complete. No new compilation or full
toolchain qualification occurred. The earlier pending-reboot observation is
historical; the scoped installer reported no reboot requirement. No reboot ran.

The single persistent checkout at `C:/Users/micro/rb/src` completed its resumed
dependency sync on `4485cc56f5f0e151d8433886976cdf679355e872` with hooks disabled.
Its `.gclient` is in `rb`, and depot_tools is under
`C:/Users/micro/chromium-tools/depot_tools`. The repository now sets
`core.autocrlf=false` and `core.longpaths=true`. Existing CRLF worktree files
must be reconciled against candidate input bytes during qualification. See
`evidence/linked-builder-dependency-sync-20261117.json`. Preserve the single
checkout, and keep its logs and cache on the laptop.

`tools/ci/Install-WindowsRunner.ps1` pins the official Windows x64 runner2.338.2
and its release SHA-256. It checks existing installation identity and never
overwrites an existing runner registration. PrepareOnly downloads/verifies without
registering. Registration takes a short-lived token file; obtain it through the
authenticated GitHub admin session on the laptop after runner execution is
verified. Keep generation, temporary token storage and consumption on that
machine; remove the exact temporary file after setup. Never commit the token or
put it in chat. RunAsService needs an elevated builder
session. A standard dedicated CI identity and build directory should hold the
checkout/cache; personal development files and credentials stay outside it.

The initial workflow, `.github/workflows/windows-runner-inventory.yml`, is manual
and targets `[self-hosted, Windows, X64, rustium-build]`. It reports machine/tool
inventory and explicitly runs no compilation or subsystem test. It is preparation
for choosing the exact repository-compatible MSVC/SDK, depot_tools, GN/Ninja and
Rust provider configuration; it is not a Network acceptance check.

## Fresh build qualification

Provision one runner-owned Chromium checkout and persistent output/cache tree.
Each job binds the exact PR revision, dependencies, build args and toolchain.
Prepare only the controlled CI checkout, preserving its incremental build cache.
Do not copy an uncommitted editor snapshot or let machine-local scratch paths be
required PR inputs. Stage all necessary Network sources and independent helper
closure into the actual integration branch before requesting compilation.

The Network job must compile every affected consumer, run the full ordinary
subsystem suites, and retain source/tool/config hashes, raw logs, discovered and
executed counts, failures/skips and result files as checks on the exact PR head.
Compiler parallelism is a recorded builder configuration; the abandoned local
experiment is not an instruction to change the running job. Serialize substantial
validation through the builder concurrency group and explicit director handoff;
do not run competing local native jobs while CI owns that validation window.
Set `CR_SOURCE_ROOT` to the verified source checkout for ordinary tests.
Windows derives the default test-data root from two parents of the executable
directory, and the CI output directory has an additional level.

This repository is public. Initially accept only maintainer-approved code on the
dedicated runner; do not automatically execute public fork PR code on a persistent
host. Repository fork-PR approval was changed from first-time contributors to
**all external contributors** as the dedicated public runner's trust boundary.
The first consumer-build workflow uses manual dispatch only. Select the workflow
ref at the exact reviewed commit and pass that commit plus the SHA-256 of its
approved input record. Use a separate dispatch for the reference probe after
the ordinary build. PR labels cannot grant approval across revisions. This
workflow is published on the Network PR branch and awaits its first execution.
Do not approve public-fork execution on this persistent personal host. The
inventory workflow cannot stand in for a compile/test check.

Keep checkout, dependency sync and hooks in the preparation window. Any operator
or script that mutates the checkout or runs native work must hold the same
exclusive byte-range lock on the first byte of `rb/.rustium-native.lock`. Finish
preparation before recording inputs. If provisioning used a hooks-disabled
sync, run the `lastchange`, `minibranch`, `gpu_lists_version`, `lastchange_skia`,
and `lastchange_dawn` actions from the pinned `DEPS` under that lock. For
qualification against the exact Git heads, pass `--filter=` to each action
except `minibranch`, which searches for upstream branch metadata separately.
Verify the generated revision identifiers against the source and dependency
heads, and verify `build/util/LASTCHANGE.committime` against the checked-out
commit and retain the generated file hashes. Regenerate source-derived
metadata whenever the candidate revision changes. Run fresh GN generation on
the laptop to qualify provisioning before recording inputs.

On the laptop, run the reviewed
`tools/ci/build_network_consumers.py` with `--record-inputs --revision <commit>`
and `--evidence <new-directory-outside-src>` using the qualified depot_tools
Python and environment. It locks the native window, checks each Git worktree,
and records actual file contents by hash, including ignored package inputs.
For relative file symlinks, it records the stored target and hashes the file
named by that target. It rejects links that leave the source tree or name
anything outside the recorded files. Only Git metadata and the two named CI
output trees are excluded. Review the
record and approve its digest for dispatch. Preparation changes invalidate
that approval. The Actions job verifies the pinned record against the checkout
before building and verifies the inputs again before releasing the lock.

Before recording inputs, create `rb/build-host.json` with four keys,
`visual_studio`, `windows_sdk`, `depot_tools`, and `git`. Each value names the
actual absolute installation directory on the laptop. Point `git` to the Git
for Windows installation containing `cmd/git.exe`. Use the same depot_tools
Python for recording and dispatch. The input record hashes those installation
trees and the running Python's base installation, records the chosen tool paths
and relevant environment, and records the Windows version. The Windows OS and
its system components remain platform dependencies. This record does not claim
to seal the entire machine. Qualify the actual laptop selections before launch.
Use `host_config_values` in `evidence/linked-builder-host-toolchain-20261117.json`
as the measured starting configuration. The driver sets both `vs2026_install`
and `GYP_MSVS_OVERRIDE_PATH` because this Build Tools installation is outside
Chromium's default Visual Studio 2026 search path. The independent laptop
lock check established contention and release for two participating processes.

Each qualification creates a new `out/NetworkCI/<run-id>` or
`out/NetworkProbeCI/<run-id>` directory. Existing output and logs stay in place.
A repeated run ID fails rather than inheriting objects or Ninja state. Cache
reuse for later PR builds needs its own provenance and invalidation policy.

If a run is interrupted, retain any partial records in `rb/evidence` and the
available GitHub Actions logs. Check the run and attempt IDs before resuming.
Treat a missing final result or evidence upload as incomplete. Confirm that
the native process has stopped before taking the laptop reservation again.
No paid hosted runners, API billing or purchased execution service is configured.

Runner registration/connectivity and the observed executable/component checks
passed. The laptop has completed GN generation, an incremental build of 96
selected roots, and ordinary unit execution on `d88308a2`. Fresh-output
qualification, the remaining workload gates, and Actions rollout are separate.

The repository now contains `tools/ci/network-validation-inventory.json`, with
the exact ordered 36-program / 232-workload membership and 96 logical GN roots.
Its data conversion checks passed without invoking GN or a compiler. It is an
inventory only. In fresh qualification mode, `network-consumer-build.yml` and
`tools/ci/build_network_consumers.py` perform fresh GN checks, resolve all 96 roots
on the laptop and build them together with bounded Ninja parallelism. They retain
raw build logs and before/after source/tool/config/dependency/graph identities.
The focused mode builds and executes the selected family programs. Full ordinary
coverage, affected consumers, ABI/runtime and other platforms have separate gates. The auth union is present in the clean
integration candidate and remains unqualified by a fresh graph/build.
See `tools/ci/README.md` and the inventory's explicit prerequisites. Do not run
the historical desktop conductor against a new laptop checkout unchanged.
