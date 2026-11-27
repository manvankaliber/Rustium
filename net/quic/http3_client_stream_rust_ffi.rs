// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#![deny(unsafe_op_in_unsafe_fn)]
mod http3_client_stream_rust_core;
use http3_client_stream_rust_core::{run, Services};
use std::ffi::c_void;
struct NativeServices {
    context: *mut c_void,
    service: unsafe extern "C" fn(*mut c_void, u32, i64) -> i64,
}
impl Services for NativeServices {
    fn call(&mut self, operation: u32, argument: i64) -> i64 {
        // SAFETY: the private C++ dispatcher supplies a synchronous stack
        // closure with this ABI, alive throughout run(), even after deletion
        // of a captured object. No Rust borrow refers to native object or
        // uninitialized output storage; only native primitives touch buffers.
        unsafe { (self.service)(self.context, operation, argument) }
    }
}
/// # Safety
/// context identifies the live C++ stack service closure paired with method.
/// The callback must not unwind or retain context. IDs are a private paired
/// ABI, never untrusted network input.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn http3_client_stream_run(
    method: u32,
    context: *mut c_void,
    service: unsafe extern "C" fn(*mut c_void, u32, i64) -> i64,
) -> i64 {
    run(method, &mut NativeServices { context, service })
}

#[unsafe(no_mangle)]
pub extern "C" fn http3_client_stream_callback_gate(current: bool, expected: bool) -> bool {
    http3_client_stream_rust_core::callback_gate(current, expected)
}
#[unsafe(no_mangle)]
pub extern "C" fn http3_client_stream_is_first(id: u32, first: u32) -> bool {
    http3_client_stream_rust_core::is_first_stream(id, first)
}
