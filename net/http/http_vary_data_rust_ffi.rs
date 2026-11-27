// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
mod http_vary_data_rust_core;
use http_vary_data_rust_core::{Crypto, Headers, Pickle, State};
use std::ffi::c_void;

#[repr(C)]
pub struct Reply {
    data: *const u8,
    len: usize,
    value: usize,
}
type Callback = unsafe extern "C" fn(*mut c_void, u32, *const u8, usize, usize) -> Reply;
struct Services {
    context: *mut c_void,
    call: Callback,
}
impl Services {
    fn invoke(&self, op: u32, input: &[u8], arg: usize) -> Reply {
        // SAFETY: the public adapter supplies a live, call-scoped context and
        // callback. The callback does not retain input or reenter this API.
        unsafe { (self.call)(self.context, op, input.as_ptr(), input.len(), arg) }
    }
    fn view(&self, reply: Reply) -> &[u8] {
        if reply.len == 0 {
            &[]
        } else {
            // SAFETY: headers remain immutable for this entire synchronous
            // call; read/hash results are copied before another callback. All
            // nonempty ranges are initialized and readable for reply.len.
            unsafe { std::slice::from_raw_parts(reply.data, reply.len) }
        }
    }
}
impl Headers for Services {
    fn wildcard(&self) -> bool {
        self.invoke(0, &[], 0).value != 0
    }
    fn next(&self, cursor: &mut usize) -> Option<&[u8]> {
        let reply = self.invoke(1, &[], *cursor);
        *cursor = reply.value;
        if reply.data.is_null() {
            None
        } else {
            Some(self.view(reply))
        }
    }
    fn request(&self, name: &[u8]) -> &[u8] {
        self.view(self.invoke(2, name, 0))
    }
}
impl Crypto for Services {
    fn begin(&self, kind: i32) {
        self.invoke(3, &[], kind as usize);
    }
    fn update(&self, bytes: &[u8]) {
        self.invoke(10, bytes, 0);
    }
    fn finish(&self, kind: i32) -> [u8; 32] {
        let reply = self.invoke(11, &[], kind as usize);
        let mut digest = [0; 32];
        digest[..reply.len].copy_from_slice(self.view(reply));
        digest
    }
}
impl Pickle for Services {
    fn reset_probe(&self) {
        self.invoke(4, &[], 0);
    }
    fn probe_read(&self, size: usize) -> bool {
        self.invoke(5, &[], size).value != 0
    }
    fn read_int(&self) -> Option<i32> {
        let reply = self.invoke(6, &[], 0);
        (reply.len != 0).then_some(reply.value as i32)
    }
    fn read(&self, out: &mut [u8]) -> bool {
        let reply = self.invoke(7, &[], out.len());
        if reply.data.is_null() {
            return false;
        }
        out.copy_from_slice(self.view(reply));
        true
    }
    fn write_int(&self, value: i32) {
        self.invoke(8, &[], value as usize);
    }
    fn write(&self, bytes: &[u8]) {
        self.invoke(9, bytes, 0);
    }
}

/// # Safety
/// `context` and `call` implement the service ABI in http_vary_data_rust_ffi.h.
/// Every view is initialized and valid through its documented callback scope.
/// State is passed by value; no Rust reference aliases C++ state or Pickle
/// writes. Header views never alias a writable service destination.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn rustium_http_vary_data(
    mut state: State,
    context: *mut c_void,
    call: Callback,
    operation: u32,
    requested: i32,
) -> State {
    let services = Services { context, call };
    match operation {
        0 => {
            http_vary_data_rust_core::init(&mut state, &services, requested);
        }
        1 => {
            state.valid = u32::from(http_vary_data_rust_core::matches(&state, &services));
        }
        2 => {
            http_vary_data_rust_core::load(&mut state, &services);
        }
        3 => http_vary_data_rust_core::persist(&state, &services),
        _ => unreachable!("private Vary operation"),
    }
    state
}
