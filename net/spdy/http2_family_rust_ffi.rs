// Copyright 2026 The Chromium Authors
// BSD-style license: see LICENSE.
use http2_policy_core as policy;
use std::ffi::c_void;

#[path = "spdy_buffer_rust_ffi.rs"]
mod buffer_ffi;

struct NativeServices {
    context: *mut c_void,
    call: unsafe extern "C" fn(*mut c_void, u32, i64) -> i64,
}

impl policy::Services for NativeServices {
    fn call(&mut self, operation: u32, argument: i64) -> i64 {
        // SAFETY: the synchronous callback/context contract is established by
        // the sole unsafe ABI entry. No native initialized reference is formed.
        unsafe { (self.call)(self.context, operation, argument) }
    }
}

/// # Safety
/// Valid private family/method IDs, a live aligned native stack context and a
/// nonnull non-unwinding primitive callback are required through this call.
/// The stack context remains alive even when a delegate deletes its owner.
/// Native operation IDs/arguments match the bound method table. The callback
/// must preserve the original declared observation/write/lifetime contract.
/// Rust stores neither context nor callback after synchronous return and never
/// constructs native slices/references or writes foreign output storage.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn http2_family_rust_dispatch(
    family: u32,
    method: u32,
    context: *mut c_void,
    callback: unsafe extern "C" fn(*mut c_void, u32, i64) -> i64,
) -> i64 {
    policy::dispatch(family, method, &mut NativeServices { context, call: callback })
}

#[path = "spdy_session_rust_ffi.rs"]
mod session_ffi;
#[path = "spdy_session_pool_rust_ffi.rs"]
mod session_pool_ffi;
