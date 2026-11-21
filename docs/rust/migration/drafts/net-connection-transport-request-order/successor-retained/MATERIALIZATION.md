# Review-only source mapping

No patch is applied or supplied for the production tree. The prospective private
standalone test target would map these sources only in a director-owned review
checkout or laptop test sandbox:

| Packet source | Proposed Chromium path |
| --- | --- |
| `src/retained_queue.rs` | `net/socket/retained_queue.rs` |
| `src/ffi.rs` | `net/socket/retained_request_queue_ffi.rs` |
| `proposal/retained_queue_ffi.h` | `net/socket/retained_queue_ffi.h` |
| `proposal/native_owner_adapter.h` | `net/socket/native_owner_adapter.h` |
| `proposal/native_owner_adapter_unittest.cc` | `net/socket/native_owner_adapter_unittest.cc` |

`proposal/BUILD.gn.inc` is a director-owned Rust target sketch; component
linkage, headers and test linkage are deliberately not claimed resolved.
The queue test module is compiled standalone from the packet, not by the GN
fragment. Any real Group patch must separately map all queue call sites and
preserve native ownership/callback ordering. `packet-manifest.json` hashes the
exact packet bytes; `source-identity.json` records pinned Git object hashes.
