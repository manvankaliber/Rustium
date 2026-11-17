# Subsystem delivery through GitHub Actions

The user selected an existing separate Windows PC as the compilation machine on
2026-11-17. Network remains the sole active subsystem; coding workers and QA use
Terra/xhigh. The proposed local two-job experiment was rejected and is cancelled.
The already-running local consumer build may finish unchanged; preserve its
result. Substantial future validation moves to the dedicated builder after it is
connected and qualified.

One Network integration PR collects the connected implementation, independent
original/reference closure, production bridges and assertion-complete tests.
Workers deliver reviewable task branches into that PR. A source checkpoint or
unrun draft is not subsystem acceptance. Independent review and CI failures feed
repairs to the same PR until required checks pass. No automatic merge or release.
Retain the distinctions between build/test acceptance, Rust ownership and final
C++ retirement. Keep the complete36-program232-workload ordinary batch and
separate16-lifecycle/optimized-QUIC obligations; campaigns remain off.

## Initial builder connection

Repository: https://github.com/manvankaliber/Rustium . Inspection found no
registered self-hosted runners. The current T3 session exposes no remote-machine
link tool; the selected PC's SSH/PowerShell access or manual setup route is still
needed. Registration has not been attempted and no external settings changed.

`tools/ci/Install-WindowsRunner.ps1` pins the official Windows x64 runner2.338.2
and its release SHA-256. It checks existing installation identity and never
overwrites an existing runner registration. PrepareOnly downloads/verifies without
registering. Registration takes a short-lived token file; obtain it through the
existing authenticated GitHub admin session after host access is established.
Never commit the token or put it in chat. RunAsService needs an elevated builder
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
