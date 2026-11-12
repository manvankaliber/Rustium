// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

mod referrer_policy_parser;

/// Parse a synchronous borrowed header; -1 denotes no recognized policy.
///
/// # Safety
/// For nonzero len, data identifies len initialized live bytes, with len no
/// greater than isize::MAX. Null data is permitted for zero length. No input
/// reference is retained, and this function never writes foreign memory.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn rustium_referrer_policy_parse(data: *const u8, len: usize) -> i32 {
    let bytes = if len == 0 {
        &[]
    } else {
        // SAFETY: Forward the documented foreign borrowed-range contract.
        unsafe { std::slice::from_raw_parts(data, len) }
    };
    referrer_policy_parser::parse(bytes).map_or(-1, |policy| policy as i32)
}

#[cfg(test)]
mod tests {
    use super::rustium_referrer_policy_parse;

    #[test]
    fn foreign_borrow_and_nullable_empty() {
        // SAFETY: Empty input may be null. The nonempty slices below remain
        // initialized and alive for the complete synchronous call.
        unsafe {
            assert_eq!(rustium_referrer_policy_parse(std::ptr::null(), 0), -1);
            for (input, expected) in
                [(b"origin".as_slice(), 4), (b"unknown", -1), (b"\tORIGIN,unsafe-url\r\n", 3)]
            {
                assert_eq!(rustium_referrer_policy_parse(input.as_ptr(), input.len()), expected);
            }
        }
    }
}
