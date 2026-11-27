// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
mod alps_decoder_rust_core;
use alps_decoder_rust_core::{Header, Pair, Payload, Setting};

#[unsafe(no_mangle)]
pub extern "C" fn alps_decoder_rust_bypass(previous: i32, next: i32) -> i32 {
    alps_decoder_rust_core::bypass(previous, next)
}

#[unsafe(no_mangle)]
pub extern "C" fn alps_decoder_rust_finish(
    framing: bool,
    forbidden: bool,
    ack: bool,
    ready: bool,
    error: i32,
) -> i32 {
    alps_decoder_rust_core::finish(framing, forbidden, ack, ready, error)
}

#[unsafe(no_mangle)]
pub extern "C" fn alps_decoder_rust_common_header(forbidden: bool, frame_type: u8) -> bool {
    alps_decoder_rust_core::common_header(forbidden, frame_type)
}

#[unsafe(no_mangle)]
pub extern "C" fn alps_decoder_rust_settings_start(count: i32) -> i32 {
    alps_decoder_rust_core::settings_start(count)
}

#[unsafe(no_mangle)]
pub extern "C" fn alps_decoder_rust_setting(id: u32, value: u32) -> Setting {
    alps_decoder_rust_core::setting(id, value)
}

#[unsafe(no_mangle)]
pub extern "C" fn alps_decoder_rust_settings_ack() -> bool {
    alps_decoder_rust_core::settings_ack()
}

#[unsafe(no_mangle)]
pub extern "C" fn alps_decoder_rust_header(
    error: i32,
    enabled: extern "C" fn() -> bool,
    stream: u32,
    frame_type: u8,
    flags: u8,
) -> Header {
    // Only scalar values are live here. The feature service cannot invalidate
    // any Rust input/state borrow; Rust decides whether to query it at all.
    alps_decoder_rust_core::header(error, || enabled(), stream, frame_type, flags)
}

#[repr(C)]
pub struct PayloadView {
    owner: *mut Payload,
    pairs: *const Pair,
    count: usize,
    bypass: i32,
}

/// # Safety
/// `data` identifies `len` initialized, immutable readable bytes for this call.
/// `inline_pairs` identifies `capacity` writable Pair slots, disjoint from
/// data; those slots need not be initialized. Either pointer may be null only
/// for an empty corresponding range. No native callbacks occur. The returned
/// pair array is initialized through count and is borrowed from inline_pairs
/// when owner is null, otherwise owned until payload_free. No input borrow
/// survives.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn alps_decoder_rust_payload_into(
    data: *const u8,
    len: usize,
    inline_pairs: *mut Pair,
    capacity: usize,
    previous_bypass: i32,
) -> PayloadView {
    let data = if len == 0 {
        &[]
    } else {
        // SAFETY: the native framing callback supplies initialized input bytes.
        unsafe { std::slice::from_raw_parts(data, len) }
    };
    let mut count = 0;
    let bypass = alps_decoder_rust_core::payload_pairs(data, |pair| {
        if count < capacity {
            // SAFETY: caller provides disjoint writable slots. Raw writes do
            // not create references to their possibly uninitialized contents.
            unsafe { inline_pairs.add(count).write(pair) };
        }
        count += 1;
    });
    if count <= capacity {
        return PayloadView {
            owner: std::ptr::null_mut(),
            pairs: inline_pairs,
            count,
            bypass: alps_decoder_rust_core::bypass(previous_bypass, bypass),
        };
    }
    // The immutable input is still borrowed, with no native callbacks between
    // passes. Reserve exactly the observed count instead of growing a Vec.
    let owner = Box::new(alps_decoder_rust_core::payload_reserved(data, count));
    PayloadView {
        pairs: owner.pairs.as_ptr(),
        count: owner.pairs.len(),
        bypass: alps_decoder_rust_core::bypass(previous_bypass, owner.bypass),
        owner: Box::into_raw(owner),
    }
}

/// # Safety
/// `data` identifies `len` initialized readable bytes, at most isize::MAX, for
/// this synchronous call. It may be null only when len is zero. No callbacks
/// occur. The returned owner must be freed once after consuming its pair array.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn alps_decoder_rust_payload(data: *const u8, len: usize) -> PayloadView {
    let data = if len == 0 {
        &[]
    } else {
        // SAFETY: native QUICHE callback supplies initialized payload bytes.
        unsafe { std::slice::from_raw_parts(data, len) }
    };
    let owner = Box::new(alps_decoder_rust_core::payload(data));
    // Only offsets survive this call; no Rust borrow of input survives output
    // mutation in C++. No mutable slice is formed over foreign storage.
    PayloadView {
        pairs: owner.pairs.as_ptr(),
        count: owner.pairs.len(),
        bypass: owner.bypass,
        owner: Box::into_raw(owner),
    }
}

/// # Safety
/// `owner` is null or a live owner returned by payload/payload_into, freed once
/// after the caller stops accessing the pair array. No input ownership
/// transfers.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn alps_decoder_rust_payload_free(owner: *mut Payload) {
    if owner.is_null() {
        return;
    }
    // SAFETY: reclaim precisely the Box transferred to native code.
    drop(unsafe { Box::from_raw(owner) });
}

#[cfg(test)]
mod tests {
    use super::*;
    use std::mem::MaybeUninit;

    #[test]
    fn caller_slots_overflow_and_sticky_partial_commit() {
        let canary = Pair { origin: 91, origin_len: 92, value: 93, value_len: 94 };
        for count in [0, 1, 8, 9, 64] {
            for capacity in [0, 1, 8] {
                for malformed in [false, true] {
                    let mut data = Vec::new();
                    for _ in 0..count {
                        data.extend_from_slice(&[0, 3, b'a', 0, 0xff, 0, 3, 0x80, 0, b'z']);
                    }
                    if malformed {
                        data.push(0);
                    }
                    let mut slots = [MaybeUninit::<Pair>::uninit(); 9];
                    slots[capacity].write(canary);
                    let output = slots.as_mut_ptr().cast::<Pair>();
                    // SAFETY: initialized immutable data and distinct writable
                    // slots. Only the prefix selected by capacity may be used.
                    let view = unsafe {
                        alps_decoder_rust_payload_into(
                            data.as_ptr(),
                            data.len(),
                            output,
                            capacity,
                            5,
                        )
                    };
                    assert_eq!(view.count, count);
                    assert_eq!(view.bypass, if malformed { 7 } else { 5 });
                    assert_eq!(view.owner.is_null(), count <= capacity);
                    if count <= capacity {
                        assert_eq!(view.pairs, output.cast_const());
                    }
                    let pairs = if count == 0 {
                        &[]
                    } else {
                        // SAFETY: result documents initialized count elements.
                        unsafe { std::slice::from_raw_parts(view.pairs, view.count) }
                    };
                    for (index, pair) in pairs.iter().enumerate() {
                        assert_eq!(
                            *pair,
                            Pair {
                                origin: index * 10 + 2,
                                origin_len: 3,
                                value: index * 10 + 7,
                                value_len: 3,
                            }
                        );
                    }
                    // SAFETY: the initialized guard is outside output capacity.
                    assert_eq!(unsafe { slots[capacity].assume_init() }, canary);
                    // SAFETY: all array reads ended; null is a valid no-op.
                    unsafe { alps_decoder_rust_payload_free(view.owner) };
                }
            }
        }
        // SAFETY: both zero-length ranges may be null and allocate no owner.
        let empty = unsafe {
            alps_decoder_rust_payload_into(std::ptr::null(), 0, std::ptr::null_mut(), 0, 7)
        };
        assert!(empty.owner.is_null());
        assert!(empty.pairs.is_null());
        assert_eq!(empty.count, 0);
        assert_eq!(empty.bypass, 7);
    }
}
