// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
mod websocket_frame_rust_core;
use std::{ffi::c_void, mem::MaybeUninit, slice};
use websocket_frame_rust_core::{self as core, Header, Parser};

const _: () = assert!(size_of::<Header>() == 24);
const _: () = assert!(align_of::<Header>() == align_of::<u64>());

// SAFETY: Readable initialized call-scoped bytes; null is allowed for empty.
unsafe fn bytes<'a>(ptr: *const u8, len: usize) -> &'a [u8] {
    if len == 0 {
        &[]
    } else {
        unsafe { slice::from_raw_parts(ptr, len) }
    }
}
// SAFETY: Exclusive initialized writable bytes, null allowed for empty.
unsafe fn writable<'a>(ptr: *mut u8, len: usize) -> &'a mut [u8] {
    if len == 0 {
        &mut []
    } else {
        unsafe { slice::from_raw_parts_mut(ptr, len) }
    }
}

// SAFETY: Exclusive writable storage, possibly uninitialized, within one live
// allocation and no larger than isize::MAX. Null is allowed only for empty.
// Unlike payload masking, writing a header does not read the destination.
unsafe fn destination<'a>(ptr: *mut u8, len: usize) -> &'a mut [MaybeUninit<u8>] {
    if len == 0 {
        &mut []
    } else {
        unsafe { slice::from_raw_parts_mut(ptr.cast(), len) }
    }
}

fn write_bytes(destination: &mut [MaybeUninit<u8>], bytes: &[u8]) {
    for (slot, byte) in destination.iter_mut().zip(bytes) {
        slot.write(*byte);
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn rustium_websocket_frame_opcode(kind: u8, op: i32) -> bool {
    core::opcode(kind, op)
}
#[unsafe(no_mangle)]
pub extern "C" fn rustium_websocket_frame_header_size(length: u64, masked: bool) -> usize {
    core::header_size(Header { length, masked: u8::from(masked), ..Header::default() })
}

/// # Safety
/// `out` is exclusive writable storage, possibly uninitialized, null only if
/// empty; its length fits isize::MAX and lies in one live allocation. If
/// masked and the destination fits the header, `key` designates four
/// initialized readable bytes and may overlap output. No Rust output borrow
/// remains alive while the potentially aliased
/// key is read.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn rustium_websocket_frame_write(
    h: Header,
    key: *const u8,
    out: *mut u8,
    len: usize,
) -> i32 {
    let Some(prefix) = core::write_prefix(h, unsafe { destination(out, len) }) else {
        return -4;
    };
    if h.masked != 0 {
        // Snapshot AFTER the prefix is written; copy supports overlap with the
        // key destination, as does the original SpanWriter::Write.
        let k: [u8; 4] = unsafe { bytes(key, 4) }.try_into().expect("four-byte key");
        write_bytes(&mut (unsafe { destination(out, len) })[prefix..prefix + 4], &k);
    }
    core::header_size(h) as i32
}

/// # Safety
/// `key` has four readable initialized bytes; data is initialized writable
/// storage (null permitted if empty). They may overlap. For overlap, their
/// union must be one valid contiguous storage range. No simultaneous shared
/// key and mutable data borrow is constructed. `width` matches the frozen
/// platform C++ packed width (sizeof(size_t) on MSVC, 16 on GCC x86/ARM).
#[unsafe(no_mangle)]
pub unsafe extern "C" fn rustium_websocket_frame_mask(
    key: *const u8,
    offset: u64,
    data: *mut u8,
    len: usize,
    width: usize,
) {
    if len == 0 {
        return;
    }
    let d = data as usize;
    let k = key as usize;
    if k < d + len && d < k + 4 {
        let (start, base) = if k < d { (key.cast_mut(), k) } else { (data, d) };
        let end = (k + 4).max(d + len);
        core::mask_aliased(
            unsafe { writable(start, end - base) },
            k - base,
            d - base,
            len,
            d % width,
            width,
            offset,
        );
    } else {
        let key = unsafe { bytes(key, 4) }.try_into().expect("four-byte key");
        core::mask(key, offset, unsafe { writable(data, len) });
    }
}

/// # Safety
/// Readable call-scoped input; null allowed if empty. Returns code in low
/// 16 bits, close-result kind in the next byte. No pointer is retained.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn rustium_websocket_frame_close(data: *const u8, len: usize) -> u32 {
    let (code, kind) = core::close(unsafe { bytes(data, len) });
    u32::from(code) | (u32::from(kind) << 16)
}

#[unsafe(no_mangle)]
pub extern "C" fn rustium_websocket_frame_parser_new() -> *mut Parser {
    Box::into_raw(Box::new(Parser::new()))
}
/// # Safety
/// `parser` is the unique live handle returned by parser_new, destroyed once.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn rustium_websocket_frame_parser_free(parser: *mut Parser) {
    drop(unsafe { Box::from_raw(parser) });
}
/// # Safety
/// Unique live parser; readable initialized input for the call (empty may be
/// null). Context belongs to C++; callback cannot unwind, reenter the parser,
/// mutate the input or retain header pointers. It copies header synchronously.
/// Payload is returned as offsets; no mutable input borrow crosses a callback.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn rustium_websocket_frame_decode(
    parser: *mut Parser,
    data: *const u8,
    len: usize,
    context: *mut c_void,
    emit: unsafe extern "C" fn(*mut c_void, *const Header, bool, usize, usize),
) -> u16 {
    let parser = unsafe { &mut *parser };
    parser.decode(unsafe { bytes(data, len) }, |h, final_chunk, start, size| {
        let ptr = h.as_ref().map_or(std::ptr::null(), |h| h as *const Header);
        unsafe { emit(context, ptr, final_chunk, start, size) };
    });
    parser.error
}
/// # Safety
/// Same byte/key contract as write. `context` is the live C++ header;
/// read synchronously snapshots its fields at each original read boundary.
/// It cannot unwind or retain output. Callback and writes never overlap.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn rustium_websocket_frame_write_live(
    initial: Header,
    context: *const c_void,
    read: unsafe extern "C" fn(*const c_void) -> Header,
    key: *const u8,
    out: *mut u8,
    len: usize,
) -> i32 {
    let result = core::write_live_prefix(
        initial,
        len,
        || unsafe { read(context) },
        |offset, bytes| {
            write_bytes(
                &mut (unsafe { destination(out, len) })[offset..offset + bytes.len()],
                bytes,
            )
        },
    );
    let Some((cursor, masked)) = result else {
        return -4;
    };
    if masked && len - cursor >= 4 {
        let k: [u8; 4] = unsafe { bytes(key, 4) }.try_into().expect("four-byte key");
        write_bytes(&mut (unsafe { destination(out, len) })[cursor..cursor + 4], &k);
    }
    core::header_size(initial) as i32
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn uninitialized_output_and_payload_tail() {
        let header =
            Header { length: 126, opcode: 2, final_bit: 1, masked: 1, ..Header::default() };
        let key = [1, 2, 3, 4];
        let mut output = [MaybeUninit::<u8>::uninit(); 256];
        // SAFETY: output is exclusive writable storage and key is initialized.
        // Only the returned written prefix is subsequently read.
        let size = unsafe {
            rustium_websocket_frame_write(
                header,
                key.as_ptr(),
                output.as_mut_ptr().cast(),
                output.len(),
            )
        };
        assert_eq!(size, 8);
        let bytes = output[..size as usize]
            .iter()
            .map(|byte| unsafe { byte.assume_init() })
            .collect::<Vec<_>>();
        assert_eq!(bytes, [0x82, 0xfe, 0, 126, 1, 2, 3, 4]);
    }

    #[test]
    fn insufficient_output_preserves_initialized_sentinels() {
        let header = Header { length: 126, masked: 1, ..Header::default() };
        let mut output = [MaybeUninit::<u8>::uninit(); 32];
        output[0].write(0xa5);
        output[31].write(0x5a);
        // SAFETY: capacity is valid. Insufficient space must fail without
        // accessing the null key or any destination byte.
        let size = unsafe {
            rustium_websocket_frame_write(header, std::ptr::null(), output.as_mut_ptr().cast(), 1)
        };
        assert_eq!(size, -4);
        assert_eq!(unsafe { output[0].assume_init() }, 0xa5);
        assert_eq!(unsafe { output[31].assume_init() }, 0x5a);
    }

    #[test]
    fn live_writer_accepts_uninitialized_output() {
        unsafe extern "C" fn read(context: *const c_void) -> Header {
            // SAFETY: context is the live immutable Header below.
            unsafe { *context.cast::<Header>() }
        }
        let header = Header { length: 65536, opcode: 1, final_bit: 1, ..Header::default() };
        let mut output = [MaybeUninit::<u8>::uninit(); 128];
        // SAFETY: context/callback match, storage is writable; the unmasked
        // path does not access the null key.
        let size = unsafe {
            rustium_websocket_frame_write_live(
                header,
                (&header as *const Header).cast(),
                read,
                std::ptr::null(),
                output.as_mut_ptr().cast(),
                output.len(),
            )
        };
        assert_eq!(size, 10);
        let bytes = output[..size as usize]
            .iter()
            .map(|byte| unsafe { byte.assume_init() })
            .collect::<Vec<_>>();
        assert_eq!(bytes, [0x81, 0x7f, 0, 0, 0, 0, 0, 1, 0, 0]);
    }
}
