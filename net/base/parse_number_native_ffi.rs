// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

//! Narrow production C ABI. Grammar and numeric policy live in the unchanged
//! safe parse_number_core crate. No CXX representations cross this boundary.

use parse_number_core::{self as parser, ParseError};

#[repr(i32)]
pub enum AbiError {
    Overflow = 0,
    Underflow = 1,
    Invalid = 2,
}

// SAFETY: The exported entrypoints document the borrowed byte range and output
// storage contract. No caller-provided value is read through the output
// pointers.
#[inline(always)]
unsafe fn dispatch<T>(
    input: *const u8,
    len: usize,
    format: i32,
    output: *mut T,
    error: *mut AbiError,
    parse: impl FnOnce(&[u8], i32, bool) -> Result<T, ParseError>,
) -> bool {
    let result = {
        let bytes = if len == 0 {
            &[]
        } else {
            // SAFETY: A nonempty std::string_view is a live, initialized byte
            // range of len bytes. The range is borrowed only for this call.
            unsafe { std::slice::from_raw_parts(input, len) }
        };
        parse(bytes, format, !error.is_null())
    };
    // The shared input borrow has ended before either output write. This also
    // handles C++ output storage that was not initialized before the call.
    match result {
        Ok(value) => {
            // SAFETY: Success requires writable, aligned output storage for T,
            // exactly as the existing C++ numeric API does. No old value is
            // read.
            unsafe { output.write(value) };
            true
        }
        Err(kind) => {
            if !error.is_null() {
                let value = match kind {
                    ParseError::Overflow => AbiError::Overflow,
                    ParseError::Underflow => AbiError::Underflow,
                    ParseError::Invalid => AbiError::Invalid,
                };
                // SAFETY: The optional error pointer is null or points to
                // writable, aligned ParseIntError storage with this exact ABI.
                unsafe { error.write(value) };
            }
            false
        }
    }
}

macro_rules! export {
    ($name:ident, $ty:ty, $parse:path) => {
        /// Parse borrowed protocol-number bytes for the C++ adapter.
        ///
        /// # Safety
        /// For nonzero len, input must identify len initialized live bytes,
        /// with len <= isize::MAX. Null input is permitted when len is zero.
        /// On success, output must identify writable, aligned storage for the
        /// integer type; the old contents may be uninitialized. Error must be
        /// null or writable aligned storage for the matching repr(i32) C++ enum.
        /// The byte range may alias outputs: it is no longer borrowed at writes.
        #[unsafe(no_mangle)]
        pub unsafe extern "C" fn $name(
            input: *const u8,
            len: usize,
            format: i32,
            output: *mut $ty,
            error: *mut AbiError,
        ) -> bool {
            // SAFETY: Forward the documented foreign-call contract unchanged.
            unsafe { dispatch(input, len, format, output, error, $parse) }
        }
    };
}

export!(rustium_parse_number_native_i32, i32, parser::parse_i32);
export!(rustium_parse_number_native_i64, i64, parser::parse_i64);
export!(rustium_parse_number_native_u32, u32, parser::parse_u32);
export!(rustium_parse_number_native_u64, u64, parser::parse_u64);

const _: () = {
    assert!(std::mem::size_of::<AbiError>() == 4);
    assert!(std::mem::align_of::<AbiError>() == 4);
};
