// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//! Narrow synchronous ABI. No pointers are dereferenced in Rust; the callback
//! is the only unsafe boundary. All policy executes in the unsafe-free core.
mod http_response_info_rust_core;
use std::ffi::c_void;
#[repr(C)]
pub struct Reply {
    value: i64,
    ok: bool,
}
#[repr(C)]
pub struct Bridge {
    context: *mut c_void,
    call: unsafe extern "C" fn(*mut c_void, u32, u32, i64) -> Reply,
}
impl http_response_info_rust_core::Services for Bridge {
    fn call(&mut self, op: u32, field: u32, value: i64) -> Option<i64> {
        // SAFETY: the C++ caller provides a live, correctly aligned Services
        // object and its matching callback for this synchronous call. Rust
        // never borrows or retains any C++ input/output storage.
        let result = unsafe { (self.call)(self.context, op, field, value) };
        result.ok.then_some(result.value)
    }
}
/// # Safety
/// The bridge callback/context pair is valid for the entire synchronous call;
/// callbacks do not throw, retain references, or recursively mutate the bridge.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn rustium_response_info(
    mut bridge: Bridge,
    operation: u32,
    first: bool,
    second: bool,
) -> bool {
    http_response_info_rust_core::execute(&mut bridge, operation, first, second)
}
