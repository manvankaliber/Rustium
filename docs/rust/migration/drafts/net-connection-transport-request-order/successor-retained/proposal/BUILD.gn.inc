# Director-owned proposal only; not applied to net/BUILD.gn.
# Materialize src/ffi.rs as socket/retained_request_queue_ffi.rs and
# src/retained_queue.rs as socket/retained_queue.rs before linking.
rust_static_library("retained_request_order_rust") {
  crate_root = "socket/retained_request_queue_ffi.rs"
  sources = [
    "socket/retained_queue.rs",
    "socket/retained_request_queue_ffi.rs",
  ]
  allow_unsafe = true  # Documented private opaque/output-pointer ABI only.
  visibility = [ ":*" ]
}
# Director must review single-owner component linkage and ABI export before
# adding dependencies and proposal/native_owner_adapter_unittest.cc to tests.
