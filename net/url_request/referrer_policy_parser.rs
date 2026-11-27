// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#![forbid(unsafe_code)]

const POLICIES: [(&[u8], u32); 8] = [
    (b"no-referrer", 7),
    (b"no-referrer-when-downgrade", 0),
    (b"origin", 4),
    (b"origin-when-cross-origin", 2),
    (b"same-origin", 5),
    (b"strict-origin", 6),
    (b"strict-origin-when-cross-origin", 1),
    (b"unsafe-url", 3),
];

/// Histogram-persisted ReferrerPolicy scalar values. Keep synchronized with the
/// public C++ enum; this parser deliberately does not use C++ optional layout.
pub fn parse(input: &[u8]) -> Option<u32> {
    for token in input.rsplit(|&byte| byte == b',') {
        let token = trim_ascii(token);
        for (name, value) in POLICIES {
            if token.eq_ignore_ascii_case(name) {
                return Some(value);
            }
        }
    }
    None
}

fn trim_ascii(mut bytes: &[u8]) -> &[u8] {
    while bytes.first().is_some_and(|b| matches!(*b, 9..=13 | 32)) {
        bytes = &bytes[1..];
    }
    while bytes.last().is_some_and(|b| matches!(*b, 9..=13 | 32)) {
        bytes = &bytes[..bytes.len() - 1];
    }
    bytes
}

#[cfg(test)]
mod tests {
    use super::parse;
    #[test]
    fn all_values_and_priority() {
        for (s, n) in [
            (b"no-referrer".as_slice(), 7),
            (b"no-referrer-when-downgrade", 0),
            (b"origin", 4),
            (b"origin-when-cross-origin", 2),
            (b"same-origin", 5),
            (b"strict-origin", 6),
            (b"strict-origin-when-cross-origin", 1),
            (b"unsafe-url", 3),
        ] {
            assert_eq!(parse(s), Some(n));
        }
        assert_eq!(parse(b"origin, nonsense, UNSAFE-URL"), Some(3));
        assert_eq!(parse(b"origin,\x0b"), Some(4));
        assert_eq!(parse(b"origin\0"), None);
        assert_eq!(parse("origin, unsafe-url".as_bytes()), Some(4));
    }
}
