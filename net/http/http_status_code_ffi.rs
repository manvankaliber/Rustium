// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license.
// No incoming pointers or Rust borrows cross the ABI. Returned bytes are
// static byte records, never freed. Null means unknown. Each record has a
// one-byte length followed by that many phrase bytes and a trailing NUL.
// This explicit schema does not expose any Rust object layout.
// unsafe is restricted to edition-2024 symbol export attributes.
mod http_status_code_core;
#[unsafe(no_mangle)]
pub extern "C" fn rustium_http_status_reason_record(code: i32) -> *const u8 {
    http_status_code_core::reason_record(code).map_or(std::ptr::null(), |bytes| bytes.as_ptr())
}
#[unsafe(no_mangle)]
pub extern "C" fn rustium_http_status_parse(code: i32) -> i32 {
    if http_status_code_core::known(code) {
        code
    } else {
        -1
    }
}
