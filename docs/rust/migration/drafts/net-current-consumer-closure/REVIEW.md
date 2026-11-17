**Review outcome: the 36-program build boundary is identifiable, but the current consumer/ABI closure is not yet qualified.** The retained graph supports building the selected programs and their generated dependencies; it does not establish the smallest complete set of changed-header consumers. The 82 historical proposals remain provisional discovery inputs, not an approved current manifest. No source-only native, ABI, or cutover pass is warranted.

The exact selected program list is `cases[].target` in the [current plan](D:/Rustium/.scratch/net-wave5/resumed-integration/native-runtime-delivery-v2/network-focus-20261117-plan.json): 36 programs comprising 14 gtest programs, four Rust test programs, and 18 verify-only workload programs with 232 planned workloads. The retained Ninja graph has an owner for each selected program. The four Rust test aliases resolve through their generated test runner targets, so the director should use the plan’s target names rather than inventing executable names.

**Findings that change the old proposal boundary:**

- The current [56-file application](D:/Rustium/.scratch/shared-strings-connection/20261117T112907106914Z/application.json) adds the Strings bridge and connection reference closure, including changes to six `net/http` headers and four local split-helper headers. The old proposals predate these bindings; the earlier HTTP/2 40-consumer build is also bound to a different source identity.
- The retained graph links the private `string_split_rust` rlib and generated CXX library into `base.dll`; `net.dll` and selected native executables link `base.dll.lib`. This is a **shared base-component dependency**, not an isolated Network test dependency. Capture generated bridge, DLL, import-library and executable inputs when qualifying the cohort. The bridge reconstructs views from checked offsets and makes synchronous calls, but its lifetime, aliasing, diagnostics and generated ABI still require the current tests and review evidence. See [string_split.cc](D:/Rustium/base/strings/string_split.cc:15) and [string_split_rust.rs](D:/Rustium/base/strings/string_split_rust.rs:9).
- The [SSL original test](D:/Rustium/net/socket/ssl_connect_job_rust_original_unittest.cc:1276) calls the local C++ split implementation, with local trim and whitespace dependencies, rather than the new Rust bridge. That establishes the inspected split path’s source independence; it does **not** establish independence of the full SSL/HTTP2/auth reference closure. The DNS original manager’s `SplitStringPiece` call currently resolves to the inline C++ implementation in `string_split.h`.
- The retained graph compiles the connection reference sources in `//net:connection_jobs_rust_reference` and links their objects directly into `connection_jobs_family_rust_unittests`, `connection_jobs_upper_rust_perftests`, and `connection_jobs_upper_rust_jobs_perftests`. The v4 workload uses `ConnectionAuthOriginalRegistry::Create(nullptr)` and closes the corresponding original pool at the three fixture lifecycle sites in [connection_jobs_upper_rust_job_workloads.h](D:/Rustium/net/socket/connection_jobs_upper_rust_job_workloads.h:69). These are source observations, not execution or full behavioral acceptance.
- The six applied HTTP headers expose original-policy friends and, in the tokenizer, an additional private constructor/tag. Friends alone do not change object layout, but this inspection did not establish a complete before/after ABI and inline-consumer comparison. Do not infer that every affected consumer is safely covered by the selected executables.

The current [base attestation](D:/Rustium/.scratch/base-attested-build/914bd46aa7384d3e8678f6d709c7be2d/receipt.json) records the base/net build and GN checks at `2c59b861…212dc7a2`. Since the review started, the [copied Strings ordinary result](D:/Rustium/.scratch/base-ordinary-delivery/n1117113822/result.json) was recorded as passed: 62 cases, with its listed post-run identity checks true. That result is limited to its stated Strings scope; the all-four runtime remains unrun.

**Next free-gate capture and build.** The following is a concrete *qualification capture*, not a claim that the resulting 36 targets alone are the complete consumer closure. Run from `D:/RustiumBuild/src` under the director’s validation gate, after reconciling active processes and verifying both checkout hashes, configuration and toolchain against the current identity. Save stdout, stderr, exit codes, command lines and hashes in a new evidence directory outside the configured tree.

```powershell
$gn = 'D:/RustiumBuild/src/buildtools/win/gn.exe'
$ninja = 'D:/RustiumBuild/src/third_party/ninja/ninja.exe'
$planPath = 'D:/Rustium/.scratch/net-wave5/resumed-integration/native-runtime-delivery-v2/network-focus-20261117-plan.json'
$appPath = 'D:/Rustium/.scratch/shared-strings-connection/20261117T112907106914Z/application.json'
$plan = Get-Content $planPath -Raw | ConvertFrom-Json
$app = Get-Content $appPath -Raw | ConvertFrom-Json
$labels = @($plan.cases | ForEach-Object { $_.label })
$targets = @($plan.cases | ForEach-Object { $_.target })
$headers = @($app.bindings | Where-Object { $_.path -like '*.h' } |
  ForEach-Object { '//' + $_.path })
```

1. Capture `gn refs` for **every applied header**, and the new bridge source and target; capture `gn desc` dependencies and outputs for every selected label and the three shared owners:

```powershell
foreach ($header in $headers) { & $gn refs out/Default $header --all }
& $gn refs out/Default //base/strings/string_split_rust.rs --all
& $gn refs out/Default //base:string_split_rust --all
foreach ($label in ($labels + @(
  '//base:base', '//base:string_split_rust',
  '//net:net', '//net:connection_jobs_rust_reference'
))) {
  & $gn desc out/Default $label deps --tree
  & $gn desc out/Default $label outputs
}
```

2. Capture the evaluated Ninja inputs and dirty edges, including the actual component outputs and every selected alias:

```powershell
foreach ($target in ($targets + @(
  'base.dll', 'net.dll', 'phony/net/connection_jobs_rust_reference'
))) {
  & $ninja -C out/Default -t query $target
  & $ninja -C out/Default -t commands $target
}
& $ninja -C out/Default -n -d explain -j1 @targets
```

Preserve the retained graph files, generated CXX headers/sources, response files and relevant dependency records alongside this capture. Reconcile the output with the four old proposal files, the 56 applied bindings, actual header inclusion and link edges. Resolve any missing or differently named aliases from `gn desc outputs` and Ninja query before building. **The exact additional consumer set cannot responsibly be declared smallest until that reconciliation is done.** In particular, `gn refs` is discovery evidence and does not by itself prove actual transitive C++ includes or ABI compatibility.

3. Once the director has reviewed that capture and added any demonstrated affected consumers, run the bounded selected-program build under the same identity, with the reviewed extra targets included in the same manifest:

```powershell
& $ninja -C out/Default -j1 @targets
```

This command is the selected-program baseline; it is **not** a substitute for the unresolved additional-consumer list. Require successful build and fresh input/output identity checks before freezing copied runtimes for all-four ordinary execution. Preserve per-program results, expected test/workload counts, runtime dependencies, and before/after source, configuration and snapshot hashes. Independently finish the original-reference/helper dependency review and the changed public-header/FFI ownership, lifetime, cancellation and reentry review before treating the paired results as acceptance evidence.

Broader services, content and browser consumers, supported platforms, native/provider ownership, performance acceptance and publication remain open. Sanitizer and fuzz campaigns remain deferred under the standing instruction. No Chrome rebuild or whole-browser closure is proposed by this review.
