// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//! Narrow ABI: only scalar snapshots and a stack-owned opaque context cross it.
//! No references to C++ objects or initialized output slices are formed.
mod disk_simple_index_rust;
use std::ffi::c_void;
type Service = unsafe extern "C" fn(*mut c_void, u32, u64, u64) -> u64;
struct Native {
    context: *mut c_void,
    service: Service,
}
impl disk_simple_index_rust::Services for Native {
    fn call(&mut self, op: u32, a: u64, b: u64) -> u64 {
        // SAFETY: private C++ entrypoint supplies a valid stack context and
        // nonthrowing dispatcher for this call. It may delete its owner, but
        // never this stack context. No native pointer is dereferenced in Rust.
        unsafe { (self.service)(self.context, op, a, b) }
    }
}
#[unsafe(no_mangle)]
/// # Safety
/// `context` and `service` must be the live native stack dispatcher supplied by
/// DiskSimpleIndexRustServices for the duration of this call. Neither is
/// retained.
pub unsafe extern "C" fn rustium_simple_index_run(
    context: *mut c_void,
    service: Service,
    op: u32,
    a: u64,
    b: u64,
) -> u64 {
    disk_simple_index_rust::run(&mut Native { context, service }, op, a, b)
}
#[unsafe(no_mangle)]
pub extern "C" fn rustium_simple_index_metadata(op: u32, a: u64, b: u64, c: u64) -> u64 {
    disk_simple_index_rust::metadata(op, a, b, c)
}
