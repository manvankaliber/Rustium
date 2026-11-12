// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

//! Allocation-free protocol integer parsing, retaining Chromium's error rules.

#![forbid(unsafe_code)]

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
#[repr(u8)]
pub enum ParseError {
    Overflow = 1,
    Underflow = 2,
    Invalid = 3,
}

// Iterator inputs may be decoded (HSTS quoted strings), whereas public scalar
// APIs borrow contiguous bytes. Keep their overflow-classification strategies
// separate without changing the common magnitude/error rules.
trait NumberBytes: Iterator<Item = u8> + Sized {
    #[inline]
    fn remaining_digits(mut self) -> bool {
        self.all(|byte| byte.is_ascii_digit())
    }
}

struct DecodedBytes<I>(I);

impl<I: Iterator<Item = u8>> Iterator for DecodedBytes<I> {
    type Item = u8;

    #[inline]
    fn next(&mut self) -> Option<u8> {
        self.0.next()
    }
}

impl<I: Iterator<Item = u8>> NumberBytes for DecodedBytes<I> {}

struct SliceBytes<'a>(std::slice::Iter<'a, u8>);

impl Iterator for SliceBytes<'_> {
    type Item = u8;

    #[inline]
    fn next(&mut self) -> Option<u8> {
        self.0.next().copied()
    }
}

impl NumberBytes for SliceBytes<'_> {
    #[inline]
    fn remaining_digits(self) -> bool {
        // A short-circuiting per-byte scan prevents vector reduction. Reduce
        // invalid-byte flags within bounded blocks instead, then short-circuit
        // between blocks. Every byte, including NUL/non-ASCII, is still
        // checked.
        let (blocks, remainder) = self.0.as_slice().as_chunks::<16>();
        for block in blocks {
            let invalid =
                block.iter().fold(0u8, |flags, byte| flags | u8::from(!byte.is_ascii_digit()));
            if invalid != 0 {
                return false;
            }
        }
        remainder.iter().all(u8::is_ascii_digit)
    }
}

#[inline]
fn accumulate<const LIMIT: u64>(
    mut value: u64,
    mut bytes: impl NumberBytes,
    report_error: bool,
) -> Result<u64, ParseError> {
    for byte in bytes.by_ref() {
        let digit = byte.wrapping_sub(b'0') as u64;
        if digit > 9 {
            return Err(ParseError::Invalid);
        }
        if value > LIMIT / 10 || (value == LIMIT / 10 && digit > LIMIT % 10) {
            // Overflow with a non-digit later in the input is a parse error.
            // Skip that classification pass when the C++ caller ignores it.
            return if !report_error || bytes.remaining_digits() {
                Err(ParseError::Overflow)
            } else {
                Err(ParseError::Invalid)
            };
        }
        // The preceding limit check proves these operations cannot wrap.
        value = value.wrapping_mul(10).wrapping_add(digit);
    }
    Ok(value)
}

#[inline]
fn parse_unsigned<const LIMIT: u64>(
    mut bytes: impl NumberBytes,
    strict: bool,
    report_error: bool,
) -> Result<u64, ParseError> {
    let first = bytes.next().ok_or(ParseError::Invalid)?;
    let digit = first.wrapping_sub(b'0');
    if digit > 9 {
        return Err(ParseError::Invalid);
    }
    if strict && digit == 0 {
        return if bytes.next().is_some() { Err(ParseError::Invalid) } else { Ok(0) };
    }
    accumulate::<LIMIT>(digit as u64, bytes, report_error)
}

/// Parse decoded decimal bytes, e.g. from an HSTS quoted-string iterator.
/// Full syntax is checked even after overflow when `report_error` is true.
#[inline]
pub fn parse_u32_iter(
    bytes: impl Iterator<Item = u8>,
    strict: bool,
    report_error: bool,
) -> Result<u32, ParseError> {
    parse_unsigned::<{ u32::MAX as u64 }>(DecodedBytes(bytes), strict, report_error)
        .map(|value| value as u32)
}

#[inline]
fn parse<const POSITIVE_LIMIT: u64, const NEGATIVE_LIMIT: u64>(
    mut bytes: impl NumberBytes,
    format: i32,
    report_error: bool,
) -> Result<(u64, bool), ParseError> {
    let first = bytes.next().ok_or(ParseError::Invalid)?;
    let non_negative = format == 0 || format == 2;
    let strict = format == 2 || format == 3;
    let negative = first == b'-';
    let first_digit = if first.is_ascii_digit() {
        first
    } else if negative && !non_negative {
        bytes.next().ok_or(ParseError::Invalid)?
    } else {
        return Err(ParseError::Invalid);
    };
    if strict && first_digit == b'0' {
        return if negative || bytes.next().is_some() {
            Err(ParseError::Invalid)
        } else {
            Ok((0, false))
        };
    }
    // Leading zeros carry no magnitude. Avoid checked multiplication for long
    // non-strict fields, while strict zero/-zero rules remain above.
    let first_digit = if first_digit == b'0' {
        loop {
            match bytes.next() {
                Some(b'0') => {}
                Some(byte) => break byte,
                None => return Ok((0, negative)),
            }
        }
    } else {
        first_digit
    };
    let digit = first_digit.wrapping_sub(b'0') as u64;
    if digit > 9 {
        return Err(ParseError::Invalid);
    }
    if negative {
        accumulate::<NEGATIVE_LIMIT>(digit, bytes, report_error).map(|value| (value, true)).map_err(
            |error| if error == ParseError::Overflow { ParseError::Underflow } else { error },
        )
    } else {
        accumulate::<POSITIVE_LIMIT>(digit, bytes, report_error).map(|value| (value, false))
    }
}

// Positive slice fields need neither negative-limit accumulation nor sign
// restoration. This applies equally to strict, optional-sign and legacy unknown
// signed formats. Negative inputs retain the original format/underflow path;
// zero-prefixed inputs retain the existing leading-zero skip.
#[inline]
fn parse_slice<const POSITIVE_LIMIT: u64, const NEGATIVE_LIMIT: u64>(
    input: &[u8],
    format: i32,
    report_error: bool,
) -> Result<(u64, bool), ParseError> {
    let bytes = SliceBytes(input.iter());
    if matches!(input.first(), Some(b'-' | b'0')) {
        parse::<POSITIVE_LIMIT, NEGATIVE_LIMIT>(bytes, format, report_error)
    } else {
        parse_unsigned::<POSITIVE_LIMIT>(bytes, format == 2 || format == 3, report_error)
            .map(|value| (value, false))
    }
}

#[inline]
pub fn parse_i32(input: &[u8], format: i32, report_error: bool) -> Result<i32, ParseError> {
    parse_slice::<{ i32::MAX as u64 }, { i32::MAX as u64 + 1 }>(input, format, report_error).map(
        |(value, negative)| if negative { (value as i32).wrapping_neg() } else { value as i32 },
    )
}

#[inline]
pub fn parse_i64(input: &[u8], format: i32, report_error: bool) -> Result<i64, ParseError> {
    parse_slice::<{ i64::MAX as u64 }, { i64::MAX as u64 + 1 }>(input, format, report_error).map(
        |(value, negative)| if negative { (value as i64).wrapping_neg() } else { value as i64 },
    )
}

/// Parse signed protocol bytes from a borrowed or decoded iterator.
#[inline]
pub fn parse_i64_iter(
    bytes: impl Iterator<Item = u8>,
    format: i32,
    report_error: bool,
) -> Result<i64, ParseError> {
    parse::<{ i64::MAX as u64 }, { i64::MAX as u64 + 1 }>(DecodedBytes(bytes), format, report_error)
        .map(
            |(value, negative)| if negative { (value as i64).wrapping_neg() } else { value as i64 },
        )
}

#[inline]
pub fn parse_u32(input: &[u8], format: i32, report_error: bool) -> Result<u32, ParseError> {
    parse_unsigned::<{ u32::MAX as u64 }>(SliceBytes(input.iter()), format == 2, report_error)
        .map(|value| value as u32)
}

#[inline]
pub fn parse_u64(input: &[u8], format: i32, report_error: bool) -> Result<u64, ParseError> {
    parse_unsigned::<{ u64::MAX }>(SliceBytes(input.iter()), format == 2, report_error)
}
