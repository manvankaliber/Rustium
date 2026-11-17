The strongest opportunities are **earlier compilation of changed bridge tests, controlled use of `-j2`, and avoiding unnecessary invalidation**. Repeated GN evaluation costs tens of seconds; thousands of serial C/C++ compilations account for most of the long run.

This inspection was read-only. I ran no builds, tests, GN generation, benchmarks or process stops. The follower attestation was active during inspection and was left intact.

**What the retained evidence establishes**

- The initial run planned 3,819 actions after adjustment. It failed at action 3,561 after **6,501.96 seconds**, on two unsafe-pointer-arithmetic diagnostics in `string_split_rust_bridge_unittest.cc`. Ninja recorded 3,560 successful edges.
- Deduplicating multi-output entries in `.ninja_log` by start/end/command hash gives **6,492.35 seconds of successful edge durations**. C/C++ object-producing edges account for **6,343.74 seconds**, about 97.6% of those durations. The longest completed edge was `win_clang_x64_with_system_allocator/.../win_util.obj`, at **35.02 seconds**.
- The successor changed only the bridge-test source within the recorded 1,260-source manifest. It passed with **259 actions in 777.24 seconds**: 255 CXX actions, one library action and three links. Those remaining compilations were primarily unfinished work after the first failure; their presence does **not** establish that the test edit invalidated 255 objects.
- The earlier 40-consumer invocation took approximately **140.8 seconds** but explicitly logged **“ninja: no work to do.”** It measures invocation/graph overhead on an already-built state, not compile throughput.

Sources: [initial result](D:/Rustium/.scratch/shared-strings-connection/20261117T090327315709Z/build-result.json), [initial diagnostics](D:/Rustium/.scratch/shared-strings-connection/20261117T090327315709Z/consumer-build.log), [successor result](D:/Rustium/.scratch/shared-strings-connection/20261117T105702922642Z/build-result.json), [40-consumer record](D:/Rustium/.scratch/net-wave5/migration-recovery-20261117T041914330876Z/consumer-delivery-build-record.json), [40-consumer output](D:/Rustium/.scratch/net-wave5/migration-recovery-20261117T041914330876Z/consumer-build-output.log), [Ninja history](D:/RustiumBuild/src/out/Default/.ninja_log).

**Prioritized recommendations**

1. **Compile changed bridge/test objects before the broad consumer build.**

   The new bridge test was reached approximately 108 minutes into the failed invocation. Its corrected successor object compiled in **1.875 seconds**, after approximately eight seconds of Ninja startup. GN header checks had passed, but do not catch this compiler diagnostic.

   Smallest implementation: after the existing GN generation/checks, add an ordinary gated Ninja phase for:

   ```text
   ninja.exe -C out/Default -j1 obj/base/base_unittests/string_split_rust_bridge_unittest.obj
   ```

   Include other changed implementation/generated-bridge objects when warranted, then execute the complete required consumer build unchanged. Bind all phases to the same source/configuration/toolchain identity and archive failures.

   **Benefit:** much earlier compiler feedback; potentially avoids spending most of a broad build before discovering a local error. It does not eliminate the necessary compilation work. Cold prerequisites may make the object preflight longer than this successor’s measured time.

   Source: [object dependency](D:/RustiumBuild/src/out/Default/obj/base/base_unittests.ninja), [build driver](D:/Rustium/.rustium-runtime/build_shared_strings_connection.py).

2. **Trial `-j2` on a future necessary build, conditional on memory headroom.**

   Both examined builds use `-j1`. Their edge timings show almost continuous serial execution, with many independent CXX edges and no dominant long link. This is the clearest actual compile-throughput opportunity.

   The host is a Ryzen 7 2700X, eight cores/16 logical processors, with approximately **15.95 GiB usable physical RAM**. At inspection, available physical memory was only **3.14 GiB**. There is no retained peak compiler/linker memory trace proving that two simultaneous heavy edges fit safely.

   Smallest implementation: make the director’s next appropriate build request explicitly select `-j2`, retaining the existing **link pool depth of one**. First record available RAM/commit and competing processes; during the run capture compiler working sets/private bytes and paging. Use `-j1` for subsequent phases/runs if pressure appears. Do not change the active attestation or launch a competing build.

   **Benefit:** ideal scheduling alone could approach a twofold reduction for independent compile work; actual benefit is unmeasured and may be substantially smaller. Paging could erase it. Two remains the ceiling, including overlapping compiler/linker work. Rust and linker internal threading also mean two Ninja jobs do not imply only two CPU threads.

   Sources: [serial invocation](D:/Rustium/.rustium-runtime/build_shared_strings_connection.py), [link pool](D:/RustiumBuild/src/out/Default/build.ninja:24), [compiler/link rules](D:/RustiumBuild/src/out/Default/toolchain.ninja).

3. **Preserve incremental state and collect dirty-edge explanations before future broad rebuilds.**

   The long run’s retained-history classification was:

   - 2,909 successful edges had outputs not previously recorded in the retained log.
   - 499 had the same prior command hash.
   - 152 had a changed prior command hash.

   “Not previously recorded” is not proof of never having been built. Nevertheless, this supports a substantial newly reached build closure, including the system-allocator toolchain, rather than one pathological source file. Same-command rebuilds also show that command changes alone cannot explain all repeated work.

   The intake changed several shared Network headers, `base/strings/string_split.cc`, and GN dependencies. Those can legitimately expand rebuilding. The current graph makes `system/module.pcm` an explicit dependency of test compilations; module changes can therefore fan out widely. However, the logs lack pre-build dirty explanations and complete prior dependency identities, so **the precise trigger for each rebuilt object is unresolved**.

   Smallest implementation: under the next idle validation gate, before compilation, archive a Ninja dry-run with explanations:

   ```text
   ninja.exe -C out/Default -n -d explain -j1 base_unittests net_unittests
   ```

   Preserve its input identity, relevant graph/dependency files and subsequent real-build log. During scoped synchronization, write a destination file only when its bytes actually differ; retain existing before/after guards. Do not suppress legitimate dependencies, touch outputs to fake freshness, or clean the cache.

   **Benefit:** prevents avoidable timestamp churn and supplies evidence for a targeted dependency repair. Current avoidable savings are unknown.

   Sources: [application overlays](D:/Rustium/.scratch/shared-strings-connection/20261117T105702922642Z/application-result.json), [module definition](D:/Rustium/build/modules/BUILD.gn:103), [explicit PCM edge](D:/RustiumBuild/src/out/Default/obj/build/modules/system.ninja).

4. **Reduce repeated orchestration only through a reviewed receipt change.**

   GN generation plus the two GN checks cost **59.67 seconds** initially and **72.81 seconds** on the successor. Each generation evaluated **36,705 targets from 4,965 files**. This is meaningful for incremental/no-work invocations but small relative to the 108-minute build.

   Repeated generation is **not proven to cause mass recompilation**. The successor repeated generation yet reused the completed work. The modulemap generator already uses content-preserving atomic output, and its Ninja rule has `restat = 1`.

   The follower currently deliberately repeats the exact GN/check/Ninja sequence and binds evaluated configuration/runtime identities. Do not remove those commands from its existing acceptance contract.

   Smallest future improvement: move the qualified receipt collection into the substantive successful build invocation, with the same reviewed command, source, graph, output and postcheck guarantees. This could avoid a separate follower invocation. Any conditional GN-generation reuse must first track the full GN dependency closure, args, environment and tool identities—not merely unchanged `base/BUILD.gn` and `net/BUILD.gn`.

   **Benefit:** saves repeated evaluation/startup/capture overhead; no evidence supports an hours-scale saving.

   Sources: [attestation implementation](D:/Rustium/docs/rust/migration/drafts/base-ordinary-delivery/attest_build.py), [exact-command binding](D:/Rustium/docs/rust/migration/drafts/base-ordinary-delivery/ordinary_base_v2.py:146), [content-preserving module output](D:/Rustium/build/modules/unified/generate_system_modulemap.py:675).

**Disk, antivirus and toolchain limits**

D: maps to the **Samsung SSD 960 EVO NVMe**, with approximately **21.5 GiB free**; C: had approximately 18.6 GiB free. An HDD exists, but the configured output is not on it. No storage latency, queue-depth or paging history was retained, so a disk bottleneck is unproven.

`MsMpEng.exe` was present, which does not establish antivirus interference. Do not change exclusions or protections on this evidence. First collect ordinary OS disk/process telemetry alongside a necessary build.

The two build receipts pin identical GN, Ninja, clang-cl, lld-link and rustc hashes. Generated commands also consume MSVC **14.44.35207**, Windows SDK **10.0.26100.0 UCRT**, and **10.0.28000.0** headers/libraries. Their complete provider bytes and historical resource behavior are not established by the five executable hashes. Preserve those provider paths and record their identities in future throughput evidence.

The existing release/component/zero-symbol configuration already avoids a large debug-symbol workload. Keep optimization, hardening, diagnostics, plugins, tests and frozen identities unchanged.

The next useful sequence is: **finish the active attestation unchanged; add changed-object preflight and dirty-edge capture to the next reviewed driver; then trial bounded `-j2` when memory permits.** This report establishes neither benchmark acceptance nor native/runtime/platform cutover.
