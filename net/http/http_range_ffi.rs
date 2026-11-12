// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

mod http_range_parser;

// Explicit C ABI records; no standard-library private layout is shared.
#[derive(Clone, Copy)]
#[repr(C)]
struct BorrowedBytes {
    pub data: *const u8,
    pub len: usize,
}

#[repr(C)]
pub struct ParsedRange {
    pub first: i64,
    pub last: i64,
    pub suffix: i64,
}

#[repr(C)]
pub struct RangesBuffer {
    pub data: *mut ParsedRange,
    pub len: usize,
    pub capacity: usize,
}

const _: () = {
    assert!(std::mem::size_of::<BorrowedBytes>() == 2 * std::mem::size_of::<usize>());
    assert!(std::mem::offset_of!(BorrowedBytes, len) == std::mem::size_of::<usize>());
    assert!(std::mem::size_of::<ParsedRange>() == 24);
    assert!(std::mem::align_of::<ParsedRange>() == std::mem::align_of::<i64>());
    assert!(std::mem::offset_of!(ParsedRange, last) == 8);
    assert!(std::mem::offset_of!(ParsedRange, suffix) == 16);
    assert!(std::mem::size_of::<RangesBuffer>() == 3 * std::mem::size_of::<usize>());
    assert!(std::mem::offset_of!(RangesBuffer, len) == std::mem::size_of::<usize>());
    assert!(std::mem::offset_of!(RangesBuffer, capacity) == 2 * std::mem::size_of::<usize>());
};

fn wire(range: http_range_parser::Range) -> ParsedRange {
    ParsedRange { first: range.first, last: range.last, suffix: range.suffix }
}

/// Borrow the live bytes supplied by the C++ adapter.
///
/// # Safety
/// Nonzero len requires that data identify len initialized live bytes, with
/// len <= isize::MAX, immutable for the resulting borrow. Zero length may use
/// a null data pointer. The caller limits the returned reference's lifetime.
unsafe fn bytes<'a>(view: BorrowedBytes) -> &'a [u8] {
    if view.len == 0 {
        &[]
    } else {
        // SAFETY: This is the documented, synchronous std::string_view range.
        unsafe { std::slice::from_raw_parts(view.data, view.len) }
    }
}

/// Parse a list, returning bit 0 for valid syntax and bit 1 for owned entries.
/// A valid prefix is returned even if a later entry is malformed. An empty
/// result never accesses output, including malformed/empty input.
///
/// # Safety
/// The byte range obeys bytes()'s contract. When bit 1 is returned, output must
/// identify aligned writable RangesBuffer storage. Its buffer is released
/// exactly once through rustium_http_range_list_drop, without changing the
/// pointer/length/capacity. Borrowed entries cannot outlive that release.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn rustium_http_range_list(
    data: *const u8,
    len: usize,
    output: *mut RangesBuffer,
) -> u8 {
    let view = BorrowedBytes { data, len };
    let mut ranges = Vec::new();
    let success = {
        // SAFETY: No output or input writes occur while this borrow exists.
        let input = unsafe { bytes(view) };
        http_range_parser::parse_ranges(input, |range| ranges.push(wire(range)))
    };
    if ranges.is_empty() {
        return u8::from(success);
    }
    let buffer =
        RangesBuffer { data: ranges.as_mut_ptr(), len: ranges.len(), capacity: ranges.capacity() };
    // SAFETY: Transfer initialized metadata only after the input borrow ends.
    unsafe { output.write(buffer) };
    std::mem::forget(ranges);
    2 | u8::from(success)
}

/// Release one buffer previously returned by rustium_http_range_list.
///
/// # Safety
/// The record is unchanged since its transfer, has not been released, and
/// no references to its elements remain. This uses Vec's documented raw-parts
/// ownership API; C++ never frees Rust memory or relies on Vec's layout.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn rustium_http_range_list_drop(buffer: RangesBuffer) {
    // SAFETY: The pointer, initialized length and allocation capacity came
    // from this same Vec<ParsedRange>, whose ownership was transferred once.
    unsafe { drop(Vec::from_raw_parts(buffer.data, buffer.len, buffer.capacity)) };
}

/// Parse Fetch's single-range grammar into uninitialized or initialized output.
///
/// # Safety
/// The byte range obeys bytes()'s contract. On success, output must be writable
/// aligned storage for ParsedRange. Input may alias output: the input borrow
/// ends before the only output write. On failure output is never accessed.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn rustium_http_range_fetch(
    data: *const u8,
    len: usize,
    whitespace: bool,
    output: *mut ParsedRange,
) -> bool {
    let view = BorrowedBytes { data, len };
    let result = {
        // SAFETY: Forward the foreign-call range contract. Retain no borrow.
        let input = unsafe { bytes(view) };
        http_range_parser::parse_fetch(input, whitespace)
    };
    if let Some(range) = result {
        // SAFETY: Write initialized data after the shared input borrow ends.
        unsafe { output.write(wire(range)) };
        true
    } else {
        false
    }
}

struct ForeignContent {
    view: BorrowedBytes,
    outputs: [*mut i64; 3],
}

impl http_range_parser::ContentSource for ForeignContent {
    #[inline]
    fn with_bytes<R>(&self, read: impl FnOnce(&[u8]) -> R) -> R {
        // SAFETY: The input is live for the call. This callback-scoped shared
        // borrow cannot escape in R, and ends before any write() call.
        read(unsafe { bytes(self.view) })
    }

    #[inline]
    fn write(&mut self, slot: usize, value: i64) {
        // SAFETY: All slots are the caller's writable aligned i64 storage.
        // No with_bytes callback is active during these raw scalar writes.
        unsafe { self.outputs[slot].write(value) };
    }

    #[inline]
    fn read(&self, slot: usize) -> i64 {
        // SAFETY: Every output was initialized before parsing. Raw reads
        // preserve equal scalar pointers and observe earlier writes exactly.
        unsafe { self.outputs[slot].read() }
    }
}

/// Parse Content-Range preserving initialization, scalar aliases and byte
/// overlap.
///
/// # Safety
/// All three pointers identify writable aligned i64 storage; old contents may
/// be uninitialized and pointers may be equal. The byte range obeys bytes()'s
/// contract after the initial output writes. It may overlap outputs: every
/// input borrow is scoped between real foreign-memory writes.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn rustium_http_range_content(
    data: *const u8,
    len: usize,
    first: *mut i64,
    last: *mut i64,
    length: *mut i64,
) -> bool {
    let mut source =
        ForeignContent { view: BorrowedBytes { data, len }, outputs: [first, last, length] };
    use http_range_parser::ContentSource;
    source.reset();
    http_range_parser::parse_content(&mut source)
}
