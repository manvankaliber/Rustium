// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license.
// No incoming pointers or Rust borrows cross the ABI. Returned bytes are
// static, immutable UTF-8, never freed. Null/zero means unknown (never a known
// empty). unsafe is restricted to symbol export attributes required by edition
// 2024.
mod http_status_code_core;
#[repr(C)]
pub struct Reason {
    pub data: *const u8,
    pub len: usize,
}
const _: () = assert!(size_of::<Reason>() == 2 * size_of::<usize>());
const _: () = assert!(align_of::<Reason>() == align_of::<usize>());
const _: () = assert!(std::mem::offset_of!(Reason, len) == size_of::<usize>());
#[unsafe(no_mangle)]
pub extern "C" fn rustium_http_status_reason(code: i32) -> Reason {
    match http_status_code_core::reason(code) {
        Some(text) => Reason { data: text.as_ptr(), len: text.len() },
        None => Reason { data: std::ptr::null(), len: 0 },
    }
}
#[unsafe(no_mangle)]
pub extern "C" fn rustium_http_status_known(code: i32) -> bool {
    http_status_code_core::reason(code).is_some()
}
