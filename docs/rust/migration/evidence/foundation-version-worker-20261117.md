{
  "task_id": "FOUNDATION-VERSION",
  "candidate_commit": "6d81943827c60e65b7a60472f0ed9a3bd654aff1",
  "source_identity": {
    "candidate_base": "d74d9794fda7efc1d6d33cdc2339aa44703540bb",
    "reference_workspace": "D:/Rustium",
    "observed_reference_head": "e2f0a5e07ebf97115ab84e90356181c1ded93b49",
    "version_files_match_pinned_hashes": true,
    "frozen_reference_manifest": "docs/rust/migration/drafts/foundation-version/reference-cpp/manifest.json"
  },
  "tests": {
    "status": "pending",
    "commands": [
      "cargo test --manifest-path docs/rust/migration/drafts/foundation-version/rust/Cargo.toml",
      "Existing configured //base:base_unittests filtered to VersionTest.* after director-approved integration"
    ],
    "toolchain": "Host Rust toolchain; version/configuration must be recorded by director before gate execution",
    "outcomes": "Not run per instruction. rustfmt syntax/format pass completed; stable rustfmt emitted warnings for repository nightly-only config options. git diff --check passed."
  },
  "review": {
    "status": "pending"
  },
  "platform_limits": [
    "No compiler, linker, native tests, configured build, sanitizer, fuzz, benchmark, or publication was run.",
    "No cross-platform behavior or ABI validation; the proposal requires director-owned bridge and GN integration."
  ],
  "remaining_cpp_bridge_ownership": "All production C++ Version code and callers remain unchanged. C++ callers include Windows version utilities, file version info, and base/version_info. Director owns bridge, GN, exports, ABI, production integration, and configured validation.",
  "next_production_integration_step": "Director reviews the frozen reference manifest and candidate semantics, then prepares the Rust target and byte-oriented bridge proposal for a guarded configured validation gate. Keep production C++ ownership until caller migration and ABI review.",
  "workspace": {
    "path": "C:/Users/jackv/.t3/worktrees/Rustium/t3-e12ddf5f-e3152e5128c99c06",
    "branch": "rust/foundation-version-20261117",
    "head": "6d81943827c60e65b7a60472f0ed9a3bd654aff1",
    "clean": true,
    "scope": "Only docs/rust/migration/drafts/foundation-version/** changed"
  }
}
