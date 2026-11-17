# Windows Network validation preparation

All compilation belongs on the linked laptop HQC-VAN_KALIBER. The desktop can
edit and run targeted checks that do not invoke a compiler. The GitHub runner
label is `rustium-build`; see
[the runner runbook](../../docs/rust/migration/CI-RUNNER.md) for installation and
the distinction between runner connectivity and Network acceptance.

`Install-WindowsRunner.ps1` prepares the pinned official runner package and
registers it using a short-lived token file. `-PrepareOnly` performs no
registration. The initial execution mode is per-user interactive; a Windows
service is a separate elevated installation. Keep registration tokens local to
the laptop and exclude runner credentials from evidence archives.

`network-validation-inventory.json` is a portable inventory, **not an executable
CI profile**. It preserves the ordered 36 ordinary programs, all 232 paired
workload cases and 96 necessary logical GN roots from the retained current
source/graph evidence. It records the four historical Rust test counts, exact
verify-only arguments and completion contracts. Workload completion text alone
does not establish correctness or source/binary provenance.

The inventory contains repository-relative entrypoint paths and GN labels.
Source hashes bind those entrypoints to the recorded 1,270-input authority;
they are not claims that a clean integration PR already contains that closure.
Two cookie entrypoints changed since the original plan. Both old and current
identities are recorded, and the cookie-store clock contract still needs a
reviewed successor. No measurement or gallery import occurred.

Before adding an executable Network workflow:

1. Assemble the connected implementation, independent reference helpers and
   tests into a clean integration branch. Apply and review the pending complete
   auth/probe union, preserving all existing inventory members.
2. Qualify the laptop's executable/toolchain/dependency/configuration identity.
   Resolve GN outputs on that machine, including the three nondefault allocator
   roots; desktop Rust output hashes and DLL/PDB names are not portable.
3. Reconcile all affected consumers and port the ordinary conductor's source,
   build/ABI freshness, runtime-copy and before/after identity checks. Retain raw
   results, discovery/execution/skip/failure counts and every failed run.
4. Include the additional lifecycle, optimized-QUIC, probe and platform
   obligations listed in the inventory. The 96 roots are necessary recorded
   roots, not proof of complete consumer closure.

The existing manual `windows-runner-inventory.yml` only records hardware and tool
paths. It neither compiles nor accepts Network. The repository is public; enable
the dedicated runner's PR workflow only for maintainer-approved source. Workflow
publication, automatic pushes and merges are not authorized by preparation.
