// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

//! NetLog representation policy. Value ownership lives in the C++ sink.
#![forbid(unsafe_code)]

use std::borrow::Cow;

pub enum Value<'a> {
    Int(i32),
    Double(f64),
    Bool(bool),
    Text(Cow<'a, [u8]>),
    Decimal { digits: [u8; 20], start: usize },
}

const MAX_SAFE_INTEGER: u64 = (1 << 53) - 1;

fn decimal(mut magnitude: u64, negative: bool) -> Value<'static> {
    let mut digits = [0; 20];
    let mut start = digits.len();
    loop {
        start -= 1;
        digits[start] = b'0' + (magnitude % 10) as u8;
        magnitude /= 10;
        if magnitude == 0 {
            break;
        }
    }
    if negative {
        start -= 1;
        digits[start] = b'-';
    }
    Value::Decimal { digits, start }
}

pub fn signed(value: i64) -> Value<'static> {
    if let Ok(value) = i32::try_from(value) {
        Value::Int(value)
    } else if value.unsigned_abs() <= MAX_SAFE_INTEGER {
        Value::Double(value as f64)
    } else {
        decimal(value.unsigned_abs(), value < 0)
    }
}

pub fn unsigned(value: u64) -> Value<'static> {
    if let Ok(value) = i32::try_from(value) {
        Value::Int(value)
    } else if value <= MAX_SAFE_INTEGER {
        Value::Double(value as f64)
    } else {
        decimal(value, false)
    }
}

pub fn boolean(value: bool) -> Value<'static> {
    Value::Bool(value)
}

// Dictionary strings deliberately bypass NetLog's byte-string escaping. The
// base::Value sink retains its original UTF-8 validation/diagnostic contract.
pub fn literal(raw: &[u8]) -> Value<'_> {
    Value::Text(Cow::Borrowed(raw))
}

pub fn string(raw: &[u8]) -> Value<'_> {
    if raw.is_ascii() {
        return literal(raw);
    }
    const HEX: &[u8; 16] = b"0123456789ABCDEF";
    let mut text = b"%ESCAPED:\xE2\x80\x8B ".to_vec();
    text.reserve(raw.len());
    for &byte in raw {
        if byte >= 128 || byte == b'%' {
            text.extend_from_slice(&[b'%', HEX[(byte >> 4) as usize], HEX[(byte & 15) as usize]]);
        } else {
            text.push(byte);
        }
    }
    Value::Text(Cow::Owned(text))
}

pub fn binary(raw: &[u8]) -> Value<'static> {
    const ALPHABET: &[u8; 64] = b"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    let mut text = Vec::with_capacity(raw.len().div_ceil(3).checked_mul(4).expect("base64 length"));
    for block in raw.chunks(3) {
        let a = block[0];
        let b = block.get(1).copied().unwrap_or(0);
        let c = block.get(2).copied().unwrap_or(0);
        text.push(ALPHABET[(a >> 2) as usize]);
        text.push(ALPHABET[(((a & 3) << 4) | (b >> 4)) as usize]);
        text.push(if block.len() > 1 {
            ALPHABET[(((b & 15) << 2) | (c >> 6)) as usize]
        } else {
            b'='
        });
        text.push(if block.len() > 2 { ALPHABET[(c & 63) as usize] } else { b'=' });
    }
    Value::Text(Cow::Owned(text))
}
