// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#![forbid(unsafe_code)]

pub trait Headers {
    fn len(&self) -> usize;
    fn field(&self, index: usize, value: bool) -> &[u8];
}

pub fn find(headers: &impl Headers, key: &[u8]) -> usize {
    (0..headers.len())
        .find(|&i| key.eq_ignore_ascii_case(headers.field(i, false)))
        .unwrap_or(headers.len())
}

// Storage actions: 0 = no-op, 1 = append, 2 = replace, 3 = erase.
pub fn mutation(headers: &impl Headers, key: &[u8], mode: u32) -> (u32, usize) {
    let index = find(headers, key);
    let found = index != headers.len();
    let action = match (mode, found) {
        (0, false) | (1, false) => 1,
        (0, true) => 2,
        (2, true) => 3,
        _ => 0,
    };
    (action, index)
}

pub fn valid_name(name: &[u8]) -> bool {
    !name.is_empty()
        && name.iter().all(|&c| c > 0x20 && c < 0x7f && !b"()<>@,;:\\\"/[]?={}".contains(&c))
}

pub fn valid_value(value: &[u8]) -> bool {
    !value.iter().any(|c| matches!(c, 0 | b'\r' | b'\n'))
}

pub fn crlf_index(line: &[u8]) -> usize {
    line.windows(2).position(|w| w == b"\r\n").unwrap_or(usize::MAX)
}

// Offsets preserve the original view boundaries, including an empty value.
pub fn parse(line: &[u8]) -> (u32, usize, usize, usize) {
    let Some(colon) = line.iter().position(|&c| c == b':') else {
        return (1, 0, 0, 0);
    };
    if colon == 0 {
        return (2, 0, 0, 0);
    }
    if !valid_name(&line[..colon]) {
        return (3, 0, 0, 0);
    }
    let mut begin = colon + 1;
    let mut end = line.len();
    while begin < end && matches!(line[begin], b' ' | b'\t') {
        begin += 1;
    }
    while begin < end && matches!(line[end - 1], b' ' | b'\t') {
        end -= 1;
    }
    if !valid_value(&line[begin..end]) {
        return (4, 0, 0, 0);
    }
    (0, colon, begin, end - begin)
}

// base::TRIM_WHITESPACE uses ASCII whitespace (including VT and FF), while
// HttpUtil::TrimLWS on an individual line removes only SP and HTAB.
pub fn block(input: &[u8]) -> Vec<(usize, usize)> {
    let mut lines = Vec::new();
    let mut cursor = 0;
    while cursor < input.len() {
        let end = input[cursor..]
            .windows(2)
            .position(|w| w == b"\r\n")
            .map_or(input.len(), |i| cursor + i);
        let mut begin = cursor;
        let mut trimmed_end = end;
        while begin < trimmed_end && ascii_whitespace(input[begin]) {
            begin += 1;
        }
        while begin < trimmed_end && ascii_whitespace(input[trimmed_end - 1]) {
            trimmed_end -= 1;
        }
        if begin != trimmed_end {
            lines.push((begin, trimmed_end - begin));
        }
        cursor = end.saturating_add(2);
    }
    lines
}

fn ascii_whitespace(c: u8) -> bool {
    matches!(c, b' ' | b'\t' | b'\r' | b'\n' | 0x0b | 0x0c)
}

pub fn serialized_len(headers: &impl Headers) -> usize {
    (0..headers.len())
        .fold(2, |len, i| len + headers.field(i, false).len() + headers.field(i, true).len() + 4)
}

pub fn serialize(headers: &impl Headers, output: &mut [u8]) {
    let mut cursor = 0;
    for i in 0..headers.len() {
        for piece in [headers.field(i, false), b": ", headers.field(i, true), b"\r\n"] {
            output[cursor..cursor + piece.len()].copy_from_slice(piece);
            cursor += piece.len();
        }
    }
    output[cursor..cursor + 2].copy_from_slice(b"\r\n");
}

pub fn format_pair(key: &[u8], value: &[u8], output: &mut [u8]) {
    output[..key.len()].copy_from_slice(key);
    output[key.len()..key.len() + 2].copy_from_slice(b": ");
    output[key.len() + 2..].copy_from_slice(value);
}

// Preserve the iterator's initial/current/end state without owning C++
// iterators.
pub fn iterator_step(started: bool, at_end: bool) -> u32 {
    if at_end {
        0
    } else if started {
        2
    } else {
        1
    }
}

pub fn indices(count: usize, mut visit: impl FnMut(usize)) {
    for i in 0..count {
        visit(i);
    }
}

// Native service queries are requested only after these Rust policy decisions.
pub fn encoding_mode(headers: &impl Headers) -> u32 {
    if find(headers, b"Accept-Encoding") != headers.len() {
        return 0;
    }
    if find(headers, b"Range") != headers.len() {
        return 1;
    }
    2
}

pub fn encoding(
    mode: u32,
    supported: u32,
    advanced: bool,
    brotli: bool,
    zstd: bool,
    output: &mut [u8],
) -> usize {
    if mode == 1 {
        output[..8].copy_from_slice(b"identity");
        return 8;
    }
    let mut len = 0;
    for (bit, allowed, name) in [
        (1, true, b"gzip".as_slice()),
        (2, true, b"deflate".as_slice()),
        (4, brotli && advanced, b"br".as_slice()),
        (8, zstd && advanced, b"zstd".as_slice()),
    ] {
        if supported & bit != 0 && allowed {
            if len != 0 {
                output[len..len + 2].copy_from_slice(b", ");
                len += 2;
            }
            output[len..len + name.len()].copy_from_slice(name);
            len += name.len();
        }
    }
    len
}
