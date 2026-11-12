// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

//! The three legacy HTTP range-header grammars, without foreign ownership.

#![forbid(unsafe_code)]

use parse_number_core::parse_i64;

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct Range {
    pub first: i64,
    pub last: i64,
    pub suffix: i64,
}

impl Default for Range {
    fn default() -> Self {
        Self { first: -1, last: -1, suffix: -1 }
    }
}

impl Range {
    fn valid(&self) -> bool {
        self.suffix > 0 || (self.first >= 0 && (self.last == -1 || self.last >= self.first))
    }
}

fn lws(byte: u8) -> bool {
    byte == b' ' || byte == b'\t'
}

fn trim(mut input: &[u8]) -> &[u8] {
    while input.first().is_some_and(|&byte| lws(byte)) {
        input = &input[1..];
    }
    while input.last().is_some_and(|&byte| lws(byte)) {
        input = &input[..input.len() - 1];
    }
    input
}

#[inline]
fn find(byte: u8, input: &[u8]) -> Option<usize> {
    if input.len() <= 16 {
        input.iter().position(|&candidate| candidate == byte)
    } else {
        memchr::memchr(byte, input)
    }
}

// The legacy grammar accepts leading zeros. Compare complete zero blocks
// without numeric accumulation, leaving at least one byte so an all-zero
// number stays nonempty. The remaining parser still validates every byte.
#[inline]
fn compact_zeros(mut input: &[u8]) -> &[u8] {
    while input.len() > 16 && input.starts_with(&[b'0'; 16]) {
        input = &input[16..];
    }
    input
}

// base::StringToInt64 allows one leading '+', but rejects leading whitespace.
// It also reports failure on overflow; its partial output is not used here.
#[inline]
fn legacy_i64(input: &[u8]) -> Option<i64> {
    if let Some(rest) = input.strip_prefix(b"+") {
        if !rest.first().is_some_and(u8::is_ascii_digit) {
            return None;
        }
        parse_i64(compact_zeros(rest), 0, false).ok()
    } else {
        parse_i64(compact_zeros(input), 1, false).ok()
    }
}

pub fn parse_ranges(input: &[u8], mut emit: impl FnMut(Range)) -> bool {
    let Some(equal) = input.iter().position(|&byte| byte == b'=') else {
        return false;
    };
    if !trim(&input[..equal]).eq_ignore_ascii_case(b"bytes") {
        return false;
    }
    let mut pos = equal + 1;
    while pos < input.len() {
        let start = pos;
        let mut quote = false;
        let mut escaped = false;
        while pos < input.len() {
            let byte = input[pos];
            if quote {
                if escaped {
                    escaped = false;
                } else if byte == b'\\' {
                    escaped = true;
                } else if byte == b'"' {
                    quote = false;
                }
            } else if byte == b',' {
                break;
            } else if byte == b'"' {
                quote = true;
            }
            pos += 1;
        }
        let token = trim(&input[start..pos]);
        if pos < input.len() {
            pos += 1;
        }
        if token.is_empty() {
            continue;
        }
        let Some(minus) = token.iter().position(|&byte| byte == b'-') else {
            return false;
        };
        let mut range = Range::default();
        let first = trim(&token[..minus]);
        if !first.is_empty() {
            let Some(value) = legacy_i64(first) else { return false };
            range.first = value;
        }
        let last = trim(&token[minus + 1..]);
        if !last.is_empty() {
            let Some(value) = legacy_i64(last) else { return false };
            if range.first != -1 {
                range.last = value;
            } else {
                range.suffix = value;
            }
        } else if range.first == -1 {
            return false;
        }
        if !range.valid() {
            return false;
        }
        emit(range);
    }
    true
}

fn delimiter(input: &[u8], pos: &mut usize, expected: u8, whitespace: bool) -> bool {
    if whitespace {
        while input.get(*pos).is_some_and(|&byte| lws(byte)) {
            *pos += 1;
        }
    }
    if input.get(*pos) != Some(&expected) {
        return false;
    }
    *pos += 1;
    if whitespace {
        while input.get(*pos).is_some_and(|&byte| lws(byte)) {
            *pos += 1;
        }
    }
    true
}

fn fetch_decimal(input: &[u8], pos: &mut usize) -> Option<i64> {
    let start = *pos;
    while input.get(*pos).is_some_and(u8::is_ascii_digit) {
        *pos += 1;
    }
    if *pos == start {
        None
    } else {
        // This slice contains only digits, so overflow is its only parse error.
        Some(parse_i64(&input[start..*pos], 0, false).unwrap_or(i64::MAX))
    }
}

pub fn parse_fetch(input: &[u8], whitespace: bool) -> Option<Range> {
    if !input.starts_with(b"bytes") {
        return None;
    }
    let mut pos = 5;
    if !delimiter(input, &mut pos, b'=', whitespace) {
        return None;
    }
    let first = fetch_decimal(input, &mut pos);
    if !delimiter(input, &mut pos, b'-', whitespace) {
        return None;
    }
    let last = fetch_decimal(input, &mut pos);
    if pos != input.len() {
        return None;
    }
    // Fetch deliberately returns syntactically valid but semantically invalid
    // ranges such as bytes=-0 or bytes=10-5. Do not call Range::valid here.
    match (first, last) {
        (None, Some(suffix)) => Some(Range { suffix, ..Range::default() }),
        (Some(first), None) => Some(Range { first, ..Range::default() }),
        (Some(first), Some(last)) => Some(Range { first, last, ..Range::default() }),
        (None, None) => None,
    }
}

/// Input and scalar effects used by Content-Range's sequential legacy API.
/// Each byte borrow ends before a scalar write. Implementations may therefore
/// support input/output overlap without retaining a shared slice across writes.
pub trait ContentSource {
    fn with_bytes<R>(&self, read: impl FnOnce(&[u8]) -> R) -> R;
    fn write(&mut self, slot: usize, value: i64);
    fn read(&self, slot: usize) -> i64;
    fn reset(&mut self) {
        for slot in (0..3).rev() {
            self.write(slot, -1);
        }
    }
}

pub fn parse_content(source: &mut impl ContentSource) -> bool {
    let bounds = source.with_bytes(|input| {
        let begin = input.iter().position(|&byte| !lws(byte)).unwrap_or(input.len());
        let end = input.iter().rposition(|&byte| !lws(byte)).map_or(begin, |index| index + 1);
        let space = begin + find(b' ', &input[begin..end])?;
        if !trim(&input[begin..space]).eq_ignore_ascii_case(b"bytes") {
            return None;
        }
        let minus = space + 1 + find(b'-', &input[space + 1..end])?;
        let slash = minus + 1 + find(b'/', &input[minus + 1..end])?;
        Some((space, minus, slash, end))
    });
    let Some((space, minus, slash, end)) = bounds else { return false };
    let first = source.with_bytes(|input| legacy_i64(trim(&input[space + 1..minus])));
    let Some(first) = first else { return false };
    // This field ends at the first '-', so a parsed first position cannot
    // be negative. Scalar aliases can change it after subsequent writes.
    source.write(0, first);
    let success = (|| {
        let last = source.with_bytes(|input| legacy_i64(trim(&input[minus + 1..slash])));
        let Some(last) = last else { return false };
        source.write(1, last);
        if source.read(1) < source.read(0) {
            return false;
        }
        let length = source.with_bytes(|input| legacy_i64(trim(&input[slash + 1..end])));
        let Some(length) = length else { return false };
        source.write(2, length);
        source.read(2) > source.read(1)
    })();
    if !success {
        source.reset();
    }
    success
}
