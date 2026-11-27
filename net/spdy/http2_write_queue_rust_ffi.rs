// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
mod http2_write_queue_rust_core;
use http2_write_queue_rust_core::{self as core, Storage};
use std::ffi::c_void;

type Service = unsafe extern "C" fn(*mut c_void, u32, usize, usize, usize) -> usize;
struct Native {
    context: *mut c_void,
    service: Service,
}
impl Native {
    fn call(&self, operation: u32, p: usize, i: usize, arg: usize) -> usize {
        // SAFETY: the caller keeps its context and native queue alive
        // throughout this synchronous dispatch. Services take no Rust
        // pointers/references, invoke no user callbacks, and retain neither the
        // context nor a borrow.
        unsafe { (self.service)(self.context, operation, p, i, arg) }
    }
}
impl Storage for Native {
    fn size(&self, p: usize) -> usize {
        self.call(0, p, 0, 0)
    }
    fn frame(&self, p: usize, i: usize) -> u8 {
        self.call(1, p, i, 0) as u8
    }
    fn stream(&self, p: usize, i: usize) -> usize {
        self.call(2, p, i, 0)
    }
    fn stream_id(&self, p: usize, i: usize) -> u32 {
        self.call(3, p, i, 0) as u32
    }
    fn append(&mut self, p: usize) {
        self.call(4, p, 0, 0);
    }
    fn erase(&mut self, p: usize, i: usize) {
        self.call(5, p, i, 0);
    }
    fn move_to(&mut self, p: usize, i: usize, to: usize) {
        self.call(6, p, i, to);
    }
    fn count(&self) -> i32 {
        self.call(7, 0, 0, 0) as i32
    }
    fn set_count(&mut self, count: i32) {
        self.call(8, 0, 0, count as usize);
    }
    fn set_removing(&mut self, removing: bool) {
        self.call(9, 0, 0, usize::from(removing));
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn http2_write_queue_rust_capped(frame: u8) -> u8 {
    u8::from(core::capped(frame))
}
#[unsafe(no_mangle)]
pub extern "C" fn http2_write_queue_rust_adjust(count: i32, frame: u8, delta: i32) -> i32 {
    core::adjust(count, frame, delta)
}

/// # Safety
/// `context` and `service` must satisfy the synchronous native service contract
/// in the matching header. Priorities and indices obey the public queue API's
/// preconditions. No Rust reference to foreign state is ever constructed.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn http2_write_queue_rust_run(
    context: *mut c_void,
    service: Service,
    operation: u32,
    priorities: usize,
    a: usize,
    b: usize,
    token: usize,
) -> usize {
    let mut s = Native { context, service };
    match operation {
        0 => usize::from(core::empty(&s, priorities)),
        1 => {
            core::enqueue(&mut s, a, b as u8);
            0
        }
        2 => core::selected(&s, priorities).map_or(0, |p| p + 1),
        3 => {
            core::remove(&mut s, priorities, a, token, None);
            0
        }
        4 => {
            core::remove(&mut s, priorities, 0, 0, Some(a as u32));
            0
        }
        5 => {
            core::change(&mut s, a, b, token);
            0
        }
        6 => {
            core::clear(&mut s, priorities);
            0
        }
        7 => usize::from(core::stream_in_other_priority(&s, priorities, a, token)),
        _ => unreachable!("invalid internal queue operation"),
    }
}
