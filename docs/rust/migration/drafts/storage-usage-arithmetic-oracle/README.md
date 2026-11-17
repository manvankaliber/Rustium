# Storage usage arithmetic oracle proposal

The original `AtomicUpdateUsageByDelta` passes `usage + delta` directly to `Write`. The Rust draft uses provisional wrapping for overflow. Its defined arithmetic domain already requires the same signed result; behavior outside that domain needs a recorded compiler profile and ordinary boundary observation before any stronger compatibility claim.

The configured Windows `obj/storage/browser/browser.ninja` records `/clang:-fno-strict-overflow` and `/O2` for the original source. `build/config/compiler/BUILD.gn` supplies that overflow flag. This utility isolates the scalar expression for the same installed clang-cl and target, with no file/cache/timer operations. Its DLL export is a test entry point, not a proposed shared bridge. It requires no C++ runtime or SDK libraries.

Build and execute only through the director validation gate after the active networking run ends. Record source and compiler/linker hashes, full command lines and exact native outputs for a fixed signed-boundary matrix. Compare mathematical modulo-2^64 expectations separately from the original signed expression. An observed Windows result supports only this compiler/profile and scalar expression; it does not define portable C++ overflow semantics, establish whole-function behavior, or validate other platforms. No sanitizer/fuzz/security campaign is requested.

The original source, configured Ninja record and compiler configuration must be copied and hashed into each actual run's evidence before compilation. Preserve failures and results; no execution is claimed by this proposal.
