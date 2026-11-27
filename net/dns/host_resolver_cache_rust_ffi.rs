// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
use dns_resolution_policy::cache as core;
use core::{Op, Services, Values};
use std::ffi::c_void;

#[repr(C)]
pub struct Request {
    values: Values,
    data: *const u8,
    length: usize,
}
type Callback = unsafe extern "C" fn(*mut c_void, u32, Request) -> Values;
struct Native {
    context: *mut c_void,
    callback: Callback,
}
impl Services for Native {
    fn call(&mut self, op: Op, values: Values, bytes: &'static [u8]) -> Values {
        let request = Request { values, data: bytes.as_ptr(), length: bytes.len() };
        // SAFETY: a synchronous non-unwinding provider and live invocation-local
        // context are supplied by the caller. Only initialized PODs and static
        // read-only key/prefix bytes cross the call; no native reference exists.
        unsafe { (self.callback)(self.context, op as u32, request) }
    }
}

/// # Safety
/// The callback/context form a live, synchronous, non-unwinding native service
/// invocation. Its method-specific cache/entry/input views remain valid under
/// the original API contract. Reentry creates a distinct context. The provider
/// must enforce const methods' read-only cache access. Method is a valid private
/// driver ID. All returned scalar POD fields are initialized.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn host_resolver_cache_rust_run(
    context: *mut c_void,
    callback: Callback,
    method: u32,
    args: Values,
) -> Values {
    core::run(&mut Native { context, callback }, method, args)
}

#[unsafe(no_mangle)]
pub extern "C" fn host_resolver_cache_rust_capacity_valid(capacity: usize) -> bool {
    capacity > 0
}
#[unsafe(no_mangle)]
pub extern "C" fn host_resolver_cache_rust_at_capacity(size: usize, capacity: usize) -> bool {
    size >= capacity
}

#[unsafe(no_mangle)]
pub extern "C" fn host_resolver_cache_rust_stale_result(generation: bool, expired: bool) -> bool {
    core::stale_result(generation, expired)
}

/// # Safety
/// The comparison context/callback are live and non-unwinding for this call.
/// They provide the native NAK comparison primitive without mutating keys. No
/// input borrow crosses that callback; Rust owns composite lazy ordering.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn host_resolver_cache_rust_key_less(
    domain_compare: i32,
    context: *const c_void,
    compare: unsafe extern "C" fn(*const c_void) -> i32,
    left_network: i64,
    right_network: i64,
) -> bool {
    core::key_less(
        domain_compare,
        || {
            // SAFETY: the provider comparison follows the caller's contract above.
            unsafe { compare(context) }
        },
        left_network,
        right_network,
    )
}
