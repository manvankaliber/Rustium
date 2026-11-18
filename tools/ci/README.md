# Windows Network validation preparation

All compilation belongs on the linked laptop HQC-VAN_KALIBER. The desktop can
edit and run targeted checks that do not invoke a compiler. The GitHub runner
label is `rustium-build`; see
[the runner runbook](../../docs/rust/migration/CI-RUNNER.md) for installation and
the distinction between runner connectivity and Network acceptance.
The runbook also defines [who pushes, where the PR reports results, and what
Terra workers do while CI runs](../../docs/rust/migration/CI-RUNNER.md#worker-handoff-and-ci-feedback).

For the first result on a repair, use the checked-in driver with
`--feedback-targets` on the laptop's warm ordinary output. It builds and tests
each requested program in order. The [fast feedback instructions](../../docs/rust/migration/CI-RUNNER.md#fast-repair-feedback)
include an exact-case example and the handoff to full family qualification.

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
Source hashes bind actual candidate LF entrypoint blobs, with original raw hashes
retained. The1,270-input source/graph identity is historical discovery evidence.
The clean integration candidate includes its recorded closure and complete auth
union; neither the old graph nor past runtime results qualify that candidate.
Two cookie entrypoints changed since the original plan. Both old and current
identities are recorded, and the cookie-store clock contract still needs a
reviewed successor. No measurement or gallery import occurred.

The prepared `network-consumer-build.yml` builds the recorded roots on the
dedicated runner. First qualification uses manual dispatch with the reviewed
commit as its workflow ref and the SHA-256 of the approved laptop input record.
The runner builds the already prepared checkout. Checkout, dependency sync and
hooks are performed beforehand under the same laptop reservation. A probe build
uses a separate dispatch. Repository policy requires approval for all external
contributors. Automatic PR builds await a mechanism that approves the exact
head and its inputs.

`build_network_consumers.py` checks source/entrypoint/tool/config/dependency and
graph identity, runs fresh GN generation and header checks, resolves current GN
outputs (including nondefault toolchains), and builds all recorded roots in one
Ninja batch with `-k0`. It records failures and never reports ordinary or cutover
acceptance. All laptop builds use 4 to 8 Ninja jobs with four by default and one simultaneous link. Set `compiler_jobs` on dispatch or pass `--jobs N` to the driver.
Qualification uses a run-specific empty output tree and hashes the selected
external tool installations before and after execution. The [runner runbook](../../docs/rust/migration/CI-RUNNER.md)
defines the laptop host record and interruption recovery.
The workflow is published on the Network PR branch and awaits its first run.
Data-only planning checks, PowerShell parsing and actionlint cover preparation.
Full raw records persist under `C:/Users/micro/rb/evidence`; GitHub artifact
copies expire after3days. Preserve local records and archive acceptance evidence
before deleting any run. No automatic cleanup is configured.

Before ordinary Network acceptance:

1. Assemble the connected implementation, independent reference helpers and
   tests into the integration PR. Re-evaluate the included complete auth/probe
   union in fresh normal/probe graphs, preserving all existing inventory members.
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
the dedicated runner's PR workflow only for maintainer-approved source. The
director coordinates scoped branch pushes and PR feedback. See the runner
runbook for delivery and validation.
