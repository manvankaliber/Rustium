// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//! Raw pointers and callbacks are confined to this ABI. Policy is safe Rust.
mod http_request_headers_rust_core;
use http_request_headers_rust_core as core;
use std::{ffi::c_void, slice};

#[repr(C)]
pub struct Span {
    data: *const u8,
    len: usize,
}
#[repr(C)]
pub struct Reader {
    context: *const c_void,
    count: usize,
    field: unsafe extern "C" fn(*const c_void, usize, bool) -> Span,
}
#[repr(C)]
pub struct Plan {
    action: u32,
    index: usize,
}
#[repr(C)]
pub struct Parsed {
    error: u32,
    key_len: usize,
    value_begin: usize,
    value_len: usize,
}

// SAFETY: nonempty ranges are initialized/readable and valid through the call.
// Zero-length ranges may be null. No reference obtained here survives a
// callback capable of mutating C++ strings. Reader callbacks are strictly
// read-only.
unsafe fn bytes<'a>(data: *const u8, len: usize) -> &'a [u8] {
    if len == 0 {
        &[]
    } else {
        unsafe { slice::from_raw_parts(data, len) }
    }
}
impl core::Headers for Reader {
    fn len(&self) -> usize {
        self.count
    }
    fn field(&self, index: usize, value: bool) -> &[u8] {
        assert!(index < self.count);
        // SAFETY: Reader is constructed only by the C++ read service; the
        // callback returns an immutable field valid until this Reader call
        // returns.
        let span = unsafe { (self.field)(self.context, index, value) };
        unsafe { bytes(span.data, span.len) }
    }
}

/// # Safety
/// Reader is a valid read service; key is readable for len bytes, null if
/// empty.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn rustium_request_headers_find(
    reader: Reader,
    key: *const u8,
    len: usize,
) -> usize {
    core::find(&reader, unsafe { bytes(key, len) })
}
/// # Safety
/// Same read-only contract as find. No storage mutation occurs inside this
/// call.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn rustium_request_headers_plan(
    reader: Reader,
    key: *const u8,
    len: usize,
    mode: u32,
) -> Plan {
    let (action, index) = core::mutation(&reader, unsafe { bytes(key, len) }, mode);
    Plan { action, index }
}
/// # Safety
/// Input is readable for len bytes, null if empty. kind: name/value/CRLF.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn rustium_request_headers_validate(
    data: *const u8,
    len: usize,
    kind: u32,
) -> bool {
    let input = unsafe { bytes(data, len) };
    match kind {
        0 => core::valid_name(input),
        1 => core::valid_value(input),
        _ => core::crlf_index(input) != usize::MAX,
    }
}
/// # Safety
/// Input is readable for len bytes, null if empty.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn rustium_request_headers_parse(data: *const u8, len: usize) -> Parsed {
    let (error, key_len, value_begin, value_len) = core::parse(unsafe { bytes(data, len) });
    Parsed { error, key_len, value_begin, value_len }
}

/// # Safety
/// Input is readable for len bytes, null if empty.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn rustium_request_headers_crlf(data: *const u8, len: usize) -> usize {
    core::crlf_index(unsafe { bytes(data, len) })
}

/// # Safety
/// key/value are readable call-scoped ranges, null if empty. output is null for
/// a sizing query or a fresh initialized buffer of key_len + value_len + 2
/// bytes disjoint from both inputs. No borrow is retained after this call.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn rustium_request_headers_format(
    key: *const u8,
    key_len: usize,
    value: *const u8,
    value_len: usize,
    output: *mut u8,
) -> usize {
    let len = key_len + value_len + 2;
    if !output.is_null() {
        core::format_pair(
            unsafe { bytes(key, key_len) },
            unsafe { bytes(value, value_len) },
            unsafe { slice::from_raw_parts_mut(output, len) },
        );
    }
    len
}
/// # Safety
/// Input remains valid for every referenced range until that range's callback.
/// Callback/context are valid and synchronous; no Rust input borrow crosses a
/// callback. As with the original split-view parser, invalidating a later
/// line's backing storage during an earlier mutation is outside the public
/// contract.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn rustium_request_headers_block(
    data: *const u8,
    len: usize,
    context: *mut c_void,
    visit: unsafe extern "C" fn(*mut c_void, usize, usize),
) {
    let ranges = core::block(unsafe { bytes(data, len) });
    for (begin, len) in ranges {
        unsafe { visit(context, begin, len) };
    }
}
/// # Safety
/// Valid immutable read service; output is fresh initialized disjoint C++
/// storage with exactly serialized_len bytes. Empty outputs may be null.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn rustium_request_headers_serialize(
    reader: Reader,
    output: *mut u8,
    len: usize,
) -> usize {
    if output.is_null() {
        return core::serialized_len(&reader);
    }
    let out = unsafe { slice::from_raw_parts_mut(output, len) };
    core::serialize(&reader, out);
    len
}
#[unsafe(no_mangle)]
pub extern "C" fn rustium_request_headers_step(started: bool, at_end: bool) -> u32 {
    core::iterator_step(started, at_end)
}
/// # Safety
/// Valid synchronous callback/context for count indices. This holds no
/// references to source storage while callbacks perform SetHeader or external
/// NetLog work.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn rustium_request_headers_each(
    count: usize,
    context: *mut c_void,
    visit: unsafe extern "C" fn(*mut c_void, usize),
) {
    core::indices(count, |i| unsafe { visit(context, i) });
}
/// # Safety
/// output has 32 initialized writable bytes. Flag bits mean
/// gzip/deflate/brotli/zstd support.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn rustium_request_headers_encoding(
    mode: u32,
    supported: u32,
    advanced: bool,
    brotli: bool,
    zstd: bool,
    output: *mut u8,
) -> usize {
    core::encoding(mode, supported, advanced, brotli, zstd, unsafe {
        slice::from_raw_parts_mut(output, 32)
    })
}

/// # Safety
/// Reader obeys the immutable, synchronous read-service contract.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn rustium_request_headers_encoding_mode(reader: Reader) -> u32 {
    core::encoding_mode(&reader)
}
