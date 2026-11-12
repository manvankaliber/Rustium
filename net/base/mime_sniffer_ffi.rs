// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

//! Private standard-C boundary for borrowed byte views and static MIME results.
//! The parser module itself forbids unsafe code.
//! Nonempty inputs are initialized ranges within one live allocation, no longer
//! than isize::MAX; they remain unchanged during the call. Empty inputs may be
//! null. Only immutable static MIME pointers escape the adapter. C++ writes its
//! output after return, when all input borrows have ended.

#![deny(unsafe_op_in_unsafe_fn)]

mod mime_sniffer_parser;

#[repr(C)]
pub struct RawSniff {
    mime_data: *const u8,
    mime_size: usize,
    flags: u8,
}
const _: () = {
    assert!(std::mem::size_of::<bool>() == 1);
    assert!(std::mem::size_of::<RawSniff>() == 3 * std::mem::size_of::<usize>());
    assert!(std::mem::align_of::<RawSniff>() == std::mem::align_of::<usize>());
    assert!(std::mem::offset_of!(RawSniff, mime_size) == std::mem::size_of::<usize>());
    assert!(std::mem::offset_of!(RawSniff, flags) == 2 * std::mem::size_of::<usize>());
};

// Only this adapter interprets foreign pointers. The caller owns a live,
// initialized byte range for the call; empty ranges may have a null pointer.
// No reference produced here escapes a call or overlaps a Rust mutation.
unsafe fn bytes<'a>(data: *const u8, size: usize) -> &'a [u8] {
    if size == 0 {
        &[]
    } else {
        // SAFETY: the caller supplied a live initialized range of size bytes.
        unsafe { std::slice::from_raw_parts(data, size) }
    }
}

fn result(mime: Option<&'static str>, complete: bool) -> RawSniff {
    match mime {
        Some(mime) => RawSniff {
            mime_data: mime.as_ptr(),
            mime_size: mime.len(),
            flags: u8::from(complete) * 2,
        },
        None => RawSniff {
            mime_data: std::ptr::null(),
            mime_size: 0,
            flags: 1 | (u8::from(complete) * 2),
        },
    }
}

/// # Safety
/// Each nonempty pointer/size pair is a live initialized byte range for this
/// call. Returned MIME pointers reference static data, never any input buffer.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn rust_mime_sniff(
    content: *const u8,
    content_size: usize,
    path: *const u8,
    path_size: usize,
    hint: *const u8,
    hint_size: usize,
    file_url: u8,
    force_html: u8,
    hint_alias: u8,
    content_offset: usize,
) -> RawSniff {
    // SAFETY: all views satisfy the public C++ wrapper's std::string_view
    // preconditions and are borrowed only until this call returns.
    let parsed = unsafe {
        mime_sniffer_parser::sniff(
            bytes(content, content_size),
            bytes(path, path_size),
            bytes(hint, hint_size),
            file_url != 0,
            force_html != 0,
            hint_alias != 0,
            content_offset,
        )
    };
    result(parsed.mime, parsed.complete)
}

/// # Safety
/// A nonempty pointer/size pair is a live initialized range for this call.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn rust_mime_sniff_local(content: *const u8, size: usize) -> RawSniff {
    // SAFETY: the input view is valid for this call and is never retained.
    let mime = unsafe { mime_sniffer_parser::sniff_local(bytes(content, size)) };
    result(mime, mime.is_some())
}

/// # Safety
/// A nonempty pointer/size pair is a live initialized range for this call.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn rust_mime_looks_binary(content: *const u8, size: usize) -> bool {
    if size == 0 {
        return false;
    }
    // SAFETY: a nonempty input owns at least this first initialized byte. The
    // scalar Rust classifier can decide binary content before creating a slice.
    if mime_sniffer_parser::binary_byte(unsafe { *content }) {
        return true;
    }
    // SAFETY: the input view is valid for this call and is never retained.
    unsafe { mime_sniffer_parser::looks_binary(bytes(content, size)) }
}

/// # Safety
/// Nonempty pointer/size pairs are live initialized ranges for this call.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn rust_mime_should_sniff(
    mime: *const u8,
    mime_size: usize,
    scheme: u8,
    options: *const u8,
    options_size: usize,
) -> bool {
    // SAFETY: both input views are valid for this call and are never retained.
    unsafe {
        mime_sniffer_parser::should_sniff(
            bytes(mime, mime_size),
            scheme,
            bytes(options, options_size),
        )
    }
}
