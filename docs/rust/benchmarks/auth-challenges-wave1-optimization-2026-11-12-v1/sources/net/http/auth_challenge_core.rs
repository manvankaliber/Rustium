// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#![forbid(unsafe_code)]

/// Ranges into the original challenge. None preserves the default null view.
#[derive(Debug, PartialEq, Eq)]
pub struct Challenge {
    pub scheme: std::ops::Range<usize>,
    pub params: Option<std::ops::Range<usize>>,
}
fn lws(c: u8) -> bool {
    c == b' ' || c == b'\t'
}
pub fn tokenize(input: &[u8]) -> Challenge {
    let Some(start) = input.iter().position(|&c| !lws(c)) else {
        return Challenge { scheme: 0..0, params: None };
    };
    let end = input[start..].iter().position(|&c| lws(c)).map_or(input.len(), |n| start + n);
    let mut param_start = end;
    let mut param_end = input.len();
    while param_start < param_end && lws(input[param_start]) {
        param_start += 1;
    }
    while param_start < param_end && lws(input[param_end - 1]) {
        param_end -= 1;
    }
    Challenge { scheme: start..end, params: Some(param_start..param_end) }
}
pub fn lowercase_into(input: &[u8], out: &mut [u8]) {
    for (out, &byte) in out.iter_mut().zip(input) {
        *out = byte.to_ascii_lowercase();
    }
}
pub fn lowercase_in_place(input: &mut [u8]) {
    input.make_ascii_lowercase();
}
#[allow(clippy::manual_is_multiple_of)]
pub fn encoded_len(params: &[u8]) -> usize {
    let mut len = params.len();
    while len > 0 && len % 4 != 0 && params[len - 1] == b'=' {
        len -= 1;
    }
    len
}
pub fn scheme_matches(scheme: &[u8], expected: &[u8]) -> bool {
    scheme == expected
}
pub fn first_round(scheme: &[u8], expected: &[u8], params: &[u8]) -> i32 {
    if scheme_matches(scheme, expected) && encoded_len(params) == 0 {
        0
    } else {
        3
    }
}
/// Returns (decision, encoded length). Decision 4 means decoding is required.
pub fn later_decision(scheme: &[u8], expected: &[u8], params: &[u8]) -> (i32, usize) {
    if !scheme_matches(scheme, expected) {
        return (3, 0);
    }
    let len = encoded_len(params);
    (if len == 0 { 1 } else { 4 }, len)
}
fn sextet(byte: u8) -> Option<u8> {
    match byte {
        b'A'..=b'Z' => Some(byte - b'A'),
        b'a'..=b'z' => Some(byte - b'a' + 26),
        b'0'..=b'9' => Some(byte - b'0' + 52),
        b'+' => Some(62),
        b'/' => Some(63),
        _ => None,
    }
}
/// Strict modp rules, including nonzero unused bits. On failure `out` may be
/// partially written; callers must stage it separately from observable output.
#[allow(clippy::manual_is_multiple_of)]
pub fn decode_strict_into(input: &[u8], out: &mut [u8]) -> Option<usize> {
    if input.is_empty() {
        return Some(0);
    }
    if input.len() < 4 || input.len() % 4 != 0 {
        return None;
    }
    let mut end = input.len();
    if input[end - 1] == b'=' {
        end -= 1;
        if input[end - 1] == b'=' {
            end -= 1;
        }
    }
    if end % 4 == 1 {
        return None;
    }
    let mut written = 0;
    for chunk in input[..end].chunks(4) {
        let a = sextet(chunk[0])?;
        let b = sextet(chunk[1])?;
        *out.get_mut(written)? = (a << 2) | (b >> 4);
        written += 1;
        if chunk.len() > 2 {
            let c = sextet(chunk[2])?;
            *out.get_mut(written)? = (b << 4) | (c >> 2);
            written += 1;
            if chunk.len() > 3 {
                *out.get_mut(written)? = (c << 6) | sextet(chunk[3])?;
                written += 1;
            }
        }
    }
    Some(written)
}
#[cfg(test)]
mod tests {
    use super::*;
    #[test]
    fn whitespace_padding_and_bytes() {
        assert_eq!(tokenize(b" \t"), Challenge { scheme: 0..0, params: None });
        assert_eq!(
            tokenize(b" \tNTLM\t  Ab=== \t"),
            Challenge { scheme: 2..6, params: Some(9..14) }
        );
        assert_eq!(encoded_len(b"Ab==="), 4);
        let mut out = [0; 3];
        assert_eq!(decode_strict_into(b"Zh==", &mut out), Some(1));
        assert_eq!(out[0], b'f');
        assert_eq!(decode_strict_into(b"====", &mut out), None);
        assert_eq!(later_decision(b"ntlm", b"ntlm", b""), (1, 0));
    }
}
