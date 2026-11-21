// Copyright 2026 The Chromium Authors
// BSD-style license; see LICENSE.
use core::ffi::c_void;
use dns_resolution_policy::{Services, Words};
#[path = "host_resolver_cache_rust_ffi.rs"]
mod cache_abi;
type Callback = unsafe extern "C" fn(*mut c_void, u32, Words) -> Words;
struct Native {
    context: *mut c_void,
    callback: Callback,
}
impl Services for Native {
    fn call(&mut self, operation: u32, input: Words) -> Words {
        // The synchronous native invocation retains its frame until this call
        // returns. No native pointer is interpreted or retained in safe Rust.
        unsafe { (self.callback)(self.context, operation, input) }
    }
    fn bytes(&mut self, operation: u32) -> &[u8] {
        let view = self.call(operation, Words::default());
        if view.b == 0 {
            return &[];
        }
        // Only manifest-declared Bytes operations may enter this path. Native
        // initialized string storage remains readable until the next service
        // call; the mutable borrow prevents calling the service concurrently.
        // These algorithms invoke no callbacks while holding this view.
        unsafe { core::slice::from_raw_parts(view.a as *const u8, view.b as usize) }
    }
}
/// # Safety
/// Callback/context identify a live synchronous frame implementing the selected
/// method's primitive contract. Bytes operations loan initialized immutable
/// storage until the next callback. No callback unwinds through this ABI.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn rust_dns_resolution_run(
    domain: u32,
    method: u32,
    context: *mut c_void,
    callback: Callback,
) -> Words {
    let mut native = Native { context, callback };
    match domain {
        1 => dns_resolution_policy::manager::run(method, &mut native),
        2 => dns_resolution_policy::job::run(method, &mut native),
        _ => Words::default(),
    }
}
