// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//! Invocation-local scalar ABI. No field reference or callback is retained.
use crate::connect_job_rust_core as core;
use std::ffi::c_void;
type Service = unsafe extern "C" fn(*mut c_void, u32, i64, *mut i64) -> bool;
struct Native {
    context: *mut c_void,
    service: Service,
}
impl core::Services for Native {
    fn call(&mut self, action: core::Action, argument: i64) -> core::Reply {
        let mut value = 0_i64;
        // SAFETY: the caller owns this synchronous context and service. The
        // output points to initialized local scalar storage and is not retained.
        let alive = unsafe { (self.service)(self.context, action as u32, argument, &mut value) };
        core::Reply { value, alive }
    }
}
/// # Safety
/// Context and service are valid for this synchronous invocation. Native
/// services initialize their scalar output even if the receiver is deleted.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn connect_job_rust_run(
    operation: u32,
    argument: i64,
    context: *mut c_void,
    service: Service,
) -> i64 {
    let Some(operation) = core::Operation::from_wire(operation) else {
        return core::invalid_operation();
    };
    core::run(operation, argument, &mut Native { context, service })
}
/// # Safety
/// Same invocation-local context/service contract as connect_job_rust_run.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn connect_job_rust_timeout(context: *mut c_void, service: Service) {
    core::timeout(&mut Native { context, service });
}
#[unsafe(no_mangle)]
pub extern "C" fn connect_job_rust_done_error(has_error: u32, error: i32) -> i32 {
    core::done_error(has_error != 0, error)
}
#[unsafe(no_mangle)]
pub extern "C" fn connect_job_rust_top_level(has_log: u32) -> bool {
    core::top_level(has_log != 0)
}
