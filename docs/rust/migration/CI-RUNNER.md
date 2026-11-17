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

## Initial builder connection

Repository: https://github.com/manvankaliber/Rustium . Initial inspection found no
registered self-hosted runners. The laptop is now linked, reachable and has full
access under environment39b1955f-e0f8-4d4a-ad20-0a3449c1b0eb. It is Windows x64.
T3 can now run Codex Terra/xhigh there. The existing maintenance project is
`C:/Users/micro/.t3/projects/laptop-buildvoorbereiding`; its host-maintenance task
has completed and released ownership. GitHub CLI 2.104.0 is installed and logged
in as `manvankaliber`, with repository admin access. The read-only follow-up still
found zero runners. Earlier missing-provider and missing-GitHub-CLI observations
are historical; see `evidence/linked-builder-readiness-20261117.json`.

Initial execution mode is a per-user interactive runner under
`C:/Users/micro/actions-runner-rustium`, using `HQC-VAN_KALIBER\micro`. This avoids
service-install elevation, but the runner process must remain running to receive
jobs. Verify the runner executable launches before requesting a registration
token. Windows application-control blocks reported for depot_tools and locally
built executables remain separate build-readiness blockers. Preserve the host
owner's exact evidence and existing policy; GitHub login does not resolve them.

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

## PR validation to enable after inventory

Provision one runner-owned Chromium checkout and persistent output/cache tree.
Each job binds the exact PR revision, dependencies, build args and toolchain;
update only the controlled CI checkout, preserving its incremental build cache.
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

This repository is public. Initially accept only maintainer-approved code on the
dedicated runner; do not automatically execute public fork PR code on a persistent
host. Add the trusted PR trigger and required-check policy after the machine and
the complete portable Network command/profile are qualified. The current inventory
workflow has no PR trigger and cannot stand in for the future compile/test check.
No paid hosted runners, API billing or purchased execution service is configured.

Machine registration, inventory execution, portable Network profile, trusted PR
trigger, actual build/test run and required-check configuration remain pending.

The repository now contains `tools/ci/network-validation-inventory.json`, with
the exact ordered 36-program / 232-workload membership and 96 logical GN roots.
Its data conversion checks passed without invoking GN or a compiler. It is an
inventory only: the portable conductor, full affected-consumer closure, pending
auth/probe graph and source/build/ABI/runtime qualification remain to be completed.
See `tools/ci/README.md` and the inventory's explicit prerequisites. Do not run
the historical desktop conductor against a new laptop checkout unchanged.
