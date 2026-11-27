// Copyright 2026 The Chromium Authors
// BSD-style license; see LICENSE.
//! Synchronous native ownership bridge for the connected HTTP/2 pool policy.
use http2_policy_core::session_pool;
use std::ffi::c_void;

type Operation = unsafe extern "C" fn(*mut c_void, u32, u64, u64, u64) -> i64;

struct NativeServices {
    context: *mut c_void,
    operation: Operation,
}

impl session_pool::Services for NativeServices {
    fn op(&mut self, action: u32, a: u64, b: u64, c: u64) -> i64 {
        // SAFETY: a source-bound native stack closure implements every action
        // for this method. No pointer is dereferenced or borrowed by Rust.
        unsafe { (self.operation)(self.context, action, a, b, c) }
    }
}

/// # Safety
/// The method ID and native operation table come from the same generated
/// source-bound specification. Context and all local ownership live through
/// this synchronous call. Native callbacks must not unwind across the ABI;
/// actions that invoke user callbacks preserve the original lifetime contract
/// and subsequent action availability. No context/owner references are retained.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn spdy_session_pool_rust_dispatch(
    method: u32,
    context: *mut c_void,
    operation: Operation,
) -> i64 {
    session_pool::dispatch(method, &mut NativeServices { context, operation })
}
