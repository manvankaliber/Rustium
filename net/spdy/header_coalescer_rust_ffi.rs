// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
mod header_coalescer_rust_core;
use header_coalescer_rust_core::{Action, Decision, NameStart, State};

const _: () = assert!(std::mem::offset_of!(State, headers_valid) == std::mem::size_of::<usize>());
const _: () = assert!(std::mem::offset_of!(Decision, action) == std::mem::size_of::<State>());

/// # Safety
/// For nonempty inputs, pointers address initialized readable memory for the
/// corresponding length (at most isize::MAX). Inputs may overlap each other.
/// No mutable foreign references or callbacks exist during the call; neither
/// input is retained. State and return storage are passed by value.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn header_coalescer_rust_header(
    state: State,
    key: *const u8,
    key_len: usize,
    value: *const u8,
    value_len: usize,
    maximum: u32,
) -> Decision {
    // The latched-error public path must not inspect either input, even when
    // callers supply placeholder views that would not be readable otherwise.
    if state.error_seen != 0 {
        return state.header(&[], &[], maximum);
    }
    // SAFETY: public std::string_view inputs provide the readable ranges.
    // Empty views can have null data; no slice is formed from a null pointer.
    let key = if key_len == 0 { &[] } else { unsafe { std::slice::from_raw_parts(key, key_len) } };
    // SAFETY: same immutable range contract; key/value overlap is permitted.
    let value =
        if value_len == 0 { &[] } else { unsafe { std::slice::from_raw_parts(value, value_len) } };
    state.header(key, value, maximum)
}

#[unsafe(no_mangle)]
pub extern "C" fn header_coalescer_rust_release(state: State) -> Decision {
    state.release()
}

/// Action occupies bits 0..7; the next native validity byte occupies bits
/// 8..15.
#[unsafe(no_mangle)]
pub extern "C" fn header_coalescer_rust_release_fast(headers_valid: u8) -> u16 {
    let (action, next_valid) = header_coalescer_rust_core::release_decision(headers_valid);
    action as u16 | ((next_valid as u16) << 8)
}

fn packed(action: Action, invalid_byte: u8, error_seen: bool) -> u32 {
    action as u32 | ((invalid_byte as u32) << 8) | (u32::from(error_seen) << 16)
}

/// Scalar-result path for the native public adapter. Bits 0..7 are Action,
/// 8..15 are the first invalid value byte, and bit 16 is the post-log error
/// latch.
///
/// # Safety
/// Unless `error_seen != 0`, `size` is an aligned initialized writable native
/// size_t and `regular` is a distinct initialized writable one-byte flag (0/1).
/// Nonempty key/value ranges are initialized readable bytes, at most
/// isize::MAX. They may overlap each other or either state field. No references
/// are retained. There are no callbacks. Raw stores occur only between complete
/// input-borrow scopes, preserving the original order: flag before name, size
/// before value. The latched path does not access any pointer, including the
/// two state fields.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn header_coalescer_rust_header_fast(
    size: *mut usize,
    regular: *mut u8,
    error_seen: u8,
    key: *const u8,
    key_len: usize,
    value: *const u8,
    value_len: usize,
    maximum: u32,
) -> u32 {
    if error_seen != 0 {
        return packed(Action::Ignore, 0, true);
    }
    let first = if key_len == 0 {
        None
    } else {
        // SAFETY: initialized first byte by the nonempty input contract.
        Some(unsafe { key.read() })
    };
    let previous_regular = if first.is_some() {
        // SAFETY: initialized native flag; no input borrow exists yet.
        unsafe { regular.read() }
    } else {
        0
    };
    let offset = match header_coalescer_rust_core::name_start(first, previous_regular) {
        NameStart::Empty => return packed(Action::EmptyName, 0, true),
        NameStart::PseudoAfterRegular => return packed(Action::PseudoAfterRegular, 0, true),
        NameStart::Pseudo => 1,
        NameStart::Regular => {
            if previous_regular == 0 {
                // SAFETY: distinct writable native flag, before any slice.
                unsafe { regular.write(1) };
            }
            0
        }
    };
    let action = {
        // SAFETY: offset is 0 or 1 and key_len >= 1; the suffix stays in range.
        let name = unsafe { std::slice::from_raw_parts(key.add(offset), key_len - offset) };
        header_coalescer_rust_core::name_action(name)
    };
    if action != Action::Append {
        return packed(action, 0, true);
    }
    // SAFETY: aligned initialized native counter. The name borrow has ended.
    let next_size =
        header_coalescer_rust_core::next_size(unsafe { size.read() }, key_len, value_len);
    // SAFETY: no input borrow remains; commit before forming the value slice.
    unsafe { size.write(next_size) };
    let action = header_coalescer_rust_core::size_action(next_size, maximum);
    if action != Action::Append {
        return packed(action, 0, true);
    }
    let invalid = {
        // SAFETY: readable initialized range, formed after all state stores.
        let value = if value_len == 0 {
            &[]
        } else {
            unsafe { std::slice::from_raw_parts(value, value_len) }
        };
        header_coalescer_rust_core::invalid_value(value)
    };
    match invalid {
        Some(byte) => packed(Action::InvalidValue, byte, true),
        None => packed(Action::Append, 0, false),
    }
}

#[cfg(test)]
mod fast_tests {
    use super::*;

    #[test]
    fn release_matches_value_abi() {
        for headers_valid in 0..=255 {
            let old = header_coalescer_rust_release(State {
                header_list_size: usize::MAX,
                headers_valid,
                error_seen: 1,
                regular_header_seen: 1,
            });
            let code = header_coalescer_rust_release_fast(headers_valid);
            assert_eq!(code as u8, old.action as u8);
            assert_eq!((code >> 8) as u8, old.state.headers_valid);
        }
    }

    #[test]
    fn independent_inputs_match_value_abi() {
        let keys: &[&[u8]] = &[b"", b":", b"A?", b"A", b"a", b":path"];
        let values: &[&[u8]] = &[b"", b"\r\n", b"\t", b"\x80\xff", b"ok"];
        for initial_size in [0, 33, usize::MAX - 34, usize::MAX] {
            for initial_regular in [0, 1] {
                for error in [0, 1] {
                    for &key in keys {
                        for &value in values {
                            for maximum in [0, 34, u32::MAX] {
                                let state = State {
                                    header_list_size: initial_size,
                                    headers_valid: 1,
                                    error_seen: error,
                                    regular_header_seen: initial_regular,
                                };
                                let old = state.header(key, value, maximum);
                                let mut size = initial_size;
                                let mut regular = initial_regular;
                                // SAFETY: independent initialized input/field
                                // storage.
                                let code = unsafe {
                                    header_coalescer_rust_header_fast(
                                        &mut size,
                                        &mut regular,
                                        error,
                                        key.as_ptr(),
                                        key.len(),
                                        value.as_ptr(),
                                        value.len(),
                                        maximum,
                                    )
                                };
                                assert_eq!(
                                    code,
                                    packed(old.action, old.invalid_byte, old.state.error_seen != 0)
                                );
                                assert_eq!(size, old.state.header_list_size);
                                assert_eq!(regular, old.state.regular_header_seen);
                            }
                        }
                    }
                }
            }
        }
    }

    #[test]
    fn latched_error_touches_no_pointer() {
        // SAFETY: the latched path has no readable/writable-pointer
        // requirement.
        let code = unsafe {
            header_coalescer_rust_header_fast(
                std::ptr::null_mut(),
                std::ptr::null_mut(),
                1,
                std::ptr::dangling(),
                usize::MAX,
                std::ptr::dangling(),
                usize::MAX,
                0,
            )
        };
        assert_eq!(code, packed(Action::Ignore, 0, true));
    }

    #[test]
    fn aliased_counter_is_committed_before_value_read() {
        let mut size = 256usize;
        let size_ptr = std::ptr::addr_of_mut!(size);
        let offset = if cfg!(target_endian = "little") { 0 } else { size_of::<usize>() - 1 };
        let mut regular = 0;
        // SAFETY: value observes one initialized counter byte; all native field
        // pointers are disjoint. The implementation ends borrows before writes.
        let code = unsafe {
            header_coalescer_rust_header_fast(
                size_ptr,
                &mut regular,
                0,
                b"a".as_ptr(),
                1,
                size_ptr.cast::<u8>().add(offset),
                1,
                u32::MAX,
            )
        };
        assert_eq!(code, packed(Action::Append, 0, false));
        assert_eq!(size, 290);
        assert_eq!(regular, 1);
    }

    #[test]
    fn aliased_flag_is_committed_before_value_read() {
        let mut size = 0usize;
        let mut regular = 0u8;
        let regular_ptr = std::ptr::addr_of_mut!(regular);
        // SAFETY: immutable value view is formed after the raw flag write.
        let code = unsafe {
            header_coalescer_rust_header_fast(
                &mut size,
                regular_ptr,
                0,
                b"a".as_ptr(),
                1,
                regular_ptr,
                1,
                u32::MAX,
            )
        };
        assert_eq!(code, packed(Action::InvalidValue, 1, true));
        assert_eq!(size, 34);
        assert_eq!(regular, 1);
    }
}
