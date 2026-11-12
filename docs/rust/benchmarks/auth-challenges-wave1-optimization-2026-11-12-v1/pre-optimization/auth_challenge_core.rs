// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#![forbid(unsafe_code)]

/// Byte offsets into the original challenge. `None` preserves the default
/// null empty parameter view for an input containing only HTTP whitespace.
#[derive(Debug, PartialEq, Eq)]
pub struct Challenge {
    pub scheme: Vec<u8>,
    pub params: Option<std::ops::Range<usize>>,
}

fn lws(c: u8) -> bool {
    c == b' ' || c == b'\t'
}

pub fn tokenize(input: &[u8]) -> Challenge {
    let start = input.iter().position(|&c| !lws(c));
    let Some(start) = start else {
        return Challenge { scheme: Vec::new(), params: None };
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
    Challenge {
        scheme: input[start..end].iter().map(u8::to_ascii_lowercase).collect(),
        params: Some(param_start..param_end),
    }
}

#[allow(clippy::manual_is_multiple_of)]
pub fn encoded_len(params: &[u8]) -> usize {
    let mut len = params.len();
    while len > 0 && len % 4 != 0 && params[len - 1] == b'=' {
        len -= 1;
    }
    len
}

#[derive(Debug, PartialEq, Eq)]
pub enum RoundResult {
    Accept,
    Invalid,
}

pub fn first_round(scheme: &[u8], expected: &[u8], encoded: &[u8]) -> RoundResult {
    if scheme == expected && encoded.is_empty() {
        RoundResult::Accept
    } else {
        RoundResult::Invalid
    }
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

/// Mirrors the strict modp decoder, including acceptance of nonzero unused
/// bits. Failure returns no bytes, so the caller can leave its output intact.
#[allow(clippy::manual_is_multiple_of)]
pub fn decode_strict(input: &[u8]) -> Option<Vec<u8>> {
    if input.is_empty() {
        return Some(Vec::new());
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
    let mut out = Vec::with_capacity(end / 4 * 3 + 2);
    for chunk in input[..end].chunks(4) {
        let a = sextet(chunk[0])?;
        let b = sextet(chunk[1])?;
        out.push((a << 2) | (b >> 4));
        if chunk.len() > 2 {
            let c = sextet(chunk[2])?;
            out.push((b << 4) | (c >> 2));
            if chunk.len() > 3 {
                out.push((c << 6) | sextet(chunk[3])?);
            }
        }
    }
    Some(out)
}

pub fn later_decision(scheme: &[u8], expected: &[u8], encoded_len: usize) -> i32 {
    if scheme != expected {
        3
    } else if encoded_len == 0 {
        1
    } else {
        4
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    #[test]
    fn whitespace_padding_and_bytes() {
        assert_eq!(tokenize(b" \t"), Challenge { scheme: vec![], params: None });
        assert_eq!(
            tokenize(b" \tNTLM\t  Ab=== \t"),
            Challenge { scheme: b"ntlm".to_vec(), params: Some(9..14) }
        );
        assert_eq!(tokenize(b"\x80A\rX").scheme, b"\x80a\rx");
        assert_eq!(encoded_len(b"Ab==="), 4);
        assert_eq!(encoded_len(b"="), 0);
        assert_eq!(decode_strict(b"Zh=="), Some(b"f".to_vec()));
        assert_eq!(decode_strict(b"Zg="), None);
        assert_eq!(decode_strict(b"===="), None);
        assert_eq!(later_decision(b"ntlm", b"ntlm", 0), 1);
    }
}
