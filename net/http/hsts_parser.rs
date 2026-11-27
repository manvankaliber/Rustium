// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

//! Allocation-free Strict-Transport-Security parsing with Chromium's existing
//! acceptance rules. Input bytes are borrowed only for the current call.

#![forbid(unsafe_code)]

use parse_number_core::{parse_u32, parse_u32_iter, ParseError};

#[derive(Debug, PartialEq, Eq)]
pub struct Policy {
    pub max_age: u32,
    pub include_subdomains: bool,
}

fn is_lws(byte: u8) -> bool {
    byte == b' ' || byte == b'\t'
}

fn trim(mut bytes: &[u8]) -> &[u8] {
    while bytes.first().is_some_and(|&byte| is_lws(byte)) {
        bytes = &bytes[1..];
    }
    while bytes.last().is_some_and(|&byte| is_lws(byte)) {
        bytes = &bytes[..bytes.len() - 1];
    }
    bytes
}

fn is_token(bytes: &[u8]) -> bool {
    !bytes.is_empty()
        && bytes.iter().all(|&byte| {
            byte > 0x20
                && byte < 0x7f
                && !matches!(
                    byte,
                    b'(' | b')'
                        | b'<'
                        | b'>'
                        | b'@'
                        | b','
                        | b';'
                        | b':'
                        | b'\\'
                        | b'"'
                        | b'/'
                        | b'['
                        | b']'
                        | b'?'
                        | b'='
                        | b'{'
                        | b'}'
                )
        })
}

// ValuesIterator recognizes quotes anywhere in a directive, and escapes only
// within quotes. Semicolons within such quotes do not split the directive.
fn directive_end(bytes: &[u8]) -> usize {
    let mut quoted = false;
    let mut escaped = false;
    for (index, &byte) in bytes.iter().enumerate() {
        if quoted {
            if escaped {
                escaped = false;
            } else if byte == b'\\' {
                escaped = true;
            } else if byte == b'"' {
                quoted = false;
            }
        } else if byte == b';' {
            return index;
        } else if byte == b'"' {
            quoted = true;
        }
    }
    bytes.len()
}

struct Value<'a> {
    bytes: &'a [u8],
    quoted: bool,
    escaped: bool,
}

impl<'a> Value<'a> {
    fn new(raw: &'a [u8]) -> Option<Self> {
        if raw.first() != Some(&b'"') {
            return Some(Self { bytes: raw, quoted: false, escaped: false });
        }
        if raw.len() < 2 || raw.last() != Some(&b'"') {
            return None;
        }
        let bytes = &raw[1..raw.len() - 1];
        let mut escaped = false;
        let mut has_escape = false;
        for &byte in bytes {
            if escaped {
                escaped = false;
            } else if byte == b'\\' {
                escaped = true;
                has_escape = true;
            } else if byte == b'"' {
                return None;
            }
        }
        // An escape immediately before the terminal quote makes it unmatched.
        if escaped {
            return None;
        }
        Some(Self { bytes, quoted: true, escaped: has_escape })
    }

    fn max_age(&self, limit: u32) -> Option<u32> {
        let parsed = if self.escaped {
            let mut iter = self.bytes.iter().copied();
            parse_u32_iter(
                std::iter::from_fn(|| {
                    let byte = iter.next()?;
                    if byte == b'\\' {
                        iter.next()
                    } else {
                        Some(byte)
                    }
                }),
                false,
                true,
            )
        } else {
            parse_u32(self.bytes, 0, true)
        };
        match parsed {
            Ok(value) => Some(value.min(limit)),
            Err(ParseError::Overflow) => Some(limit),
            Err(_) => None,
        }
    }
}

pub fn parse(mut input: &[u8], limit: u32) -> Option<Policy> {
    let mut max_age = None;
    let mut include_subdomains = false;
    while !input.is_empty() {
        let end = directive_end(input);
        let directive = trim(&input[..end]);
        input = if end < input.len() { &input[end + 1..] } else { &[] };
        if directive.is_empty() {
            continue;
        }
        let equals = directive.iter().position(|&byte| byte == b'=');
        let name = trim(&directive[..equals.unwrap_or(directive.len())]);
        let value = if let Some(equals) = equals {
            if name.contains(&b'"') {
                return None;
            }
            let raw = trim(&directive[equals + 1..]);
            if raw.is_empty() {
                return None;
            }
            Value::new(raw)?
        } else {
            Value { bytes: &[], quoted: false, escaped: false }
        };
        if name.eq_ignore_ascii_case(b"max-age") {
            if max_age.is_some() {
                return None;
            }
            max_age = Some(value.max_age(limit)?);
        } else if name.eq_ignore_ascii_case(b"includesubdomains") {
            if include_subdomains || !value.bytes.is_empty() || value.quoted {
                return None;
            }
            include_subdomains = true;
        } else if !is_token(name)
            || (!value.bytes.is_empty() && !value.quoted && !is_token(value.bytes))
        {
            return None;
        }
    }
    Some(Policy { max_age: max_age?, include_subdomains })
}
