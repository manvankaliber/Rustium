// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

mod dns_query_rust_core;
use dns_query_rust_core as core;
use std::mem::MaybeUninit;
use std::pin::Pin;

#[cxx::bridge(namespace = "net::dns_query_rust")]
mod ffi {
    struct Construction {
        size: usize,
        has_opt: bool,
    }
    unsafe extern "C++" {
        include!("net/dns/dns_query_rust_ffi.h");
        type OptContext;
        fn opt_clone_or_create(context: Pin<&mut OptContext>, clone_input: bool) -> bool;
        fn opt_contains(context: &OptContext, code: u16) -> bool;
        fn opt_add_ede(context: Pin<&mut OptContext>);
        fn opt_add_padding(context: Pin<&mut OptContext>, len: u16);
        fn opt_len(context: &OptContext) -> usize;
        fn opt_count(context: &OptContext) -> usize;
    }
    extern "Rust" {
        fn prepare_query(
            name: &[u8],
            has_input: bool,
            padded: bool,
            structured_errors: bool,
            context: Pin<&mut OptContext>,
        ) -> Construction;
        unsafe fn write_query(
            output: *mut u8,
            size: usize,
            id: u16,
            name: &[u8],
            qtype: u16,
            has_opt: bool,
            opt: &[u8],
        );
        fn parse_query(bytes: &[u8]) -> usize;
        fn read_u16(bytes: &[u8]) -> u16;
        fn question_size(name_size: usize) -> usize;
        unsafe fn set_flags(output: *mut u8, flags: u16);
        unsafe fn copy_query(
            source: *const u8,
            output: *mut u8,
            size: usize,
            replace_id: bool,
            id: u16,
        );
    }
}

struct Services<'a>(Pin<&'a mut ffi::OptContext>);
impl core::OptServices for Services<'_> {
    fn clone_or_create(&mut self, clone_input: bool) -> bool {
        ffi::opt_clone_or_create(self.0.as_mut(), clone_input)
    }
    fn contains(&self, code: u16) -> bool {
        ffi::opt_contains(&self.0, code)
    }
    fn add_ede(&mut self) {
        ffi::opt_add_ede(self.0.as_mut());
    }
    fn add_padding(&mut self, len: u16) {
        ffi::opt_add_padding(self.0.as_mut(), len);
    }
    fn len(&self) -> usize {
        ffi::opt_len(&self.0)
    }
    fn count(&self) -> usize {
        ffi::opt_count(&self.0)
    }
}

fn prepare_query(
    name: &[u8],
    has_input: bool,
    padded: bool,
    structured_errors: bool,
    context: Pin<&mut ffi::OptContext>,
) -> ffi::Construction {
    let (size, has_opt) =
        core::prepare_opt(name, has_input, padded, structured_errors, &mut Services(context));
    ffi::Construction { size, has_opt }
}

// SAFETY: the C++ caller owns an exclusively writable allocation of `size`
// bytes. Name/OPT inputs are initialized, separately allocated and remain
// alive through the call. No byte borrow is retained after this function.
unsafe fn write_query(
    output: *mut u8,
    size: usize,
    id: u16,
    name: &[u8],
    qtype: u16,
    has_opt: bool,
    opt: &[u8],
) {
    // MaybeUninit permits the IOBuffer's freshly allocated uninitialized bytes.
    let output = unsafe { std::slice::from_raw_parts_mut(output.cast::<MaybeUninit<u8>>(), size) };
    core::write_query(output, id, name, qtype, has_opt, opt);
}

fn parse_query(bytes: &[u8]) -> usize {
    // 0: invalid; usize::MAX: original VLOG question-count diagnostic.
    // All other results are a nonzero wire-name length. C++ commits only a
    // successful result, preserving preexisting qname_size_ on failure.
    match core::parse(bytes) {
        core::ParseResult::Invalid => 0,
        core::ParseResult::QuestionCount => usize::MAX,
        core::ParseResult::Name(size) => size,
    }
}

fn read_u16(bytes: &[u8]) -> u16 {
    core::read_u16(bytes)
}
fn question_size(name_size: usize) -> usize {
    core::question_size(name_size)
}

// SAFETY: two writable bytes beginning at the flags field, potentially
// uninitialized on a raw-packet object. Never construct an initialized slice.
unsafe fn set_flags(output: *mut u8, flags: u16) {
    let output = unsafe { std::slice::from_raw_parts_mut(output.cast::<MaybeUninit<u8>>(), 2) };
    core::set_flags(output, flags);
}

// SAFETY: source/output cover `size` bytes; output is exclusively writable.
// Input can contain uninitialized tail capacity: ptr::copy preserves it
// without manufacturing a &[u8]. memmove semantics permit overlapping ranges.
// Neither input nor output pointers are retained; no reference spans the copy.
unsafe fn copy_query(source: *const u8, output: *mut u8, size: usize, replace_id: bool, id: u16) {
    let count = core::copy_size(size);
    if count != 0 {
        unsafe { std::ptr::copy(source, output, count) };
    }
    if replace_id {
        assert!(count >= 2);
        let output = unsafe { std::slice::from_raw_parts_mut(output.cast::<MaybeUninit<u8>>(), 2) };
        core::clone_id(output, id);
    }
}
