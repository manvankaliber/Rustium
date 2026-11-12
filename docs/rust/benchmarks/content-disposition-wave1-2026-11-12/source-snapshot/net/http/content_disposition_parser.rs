// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

//! Content-Disposition compatibility policy. External services only convert
//! complete byte strings; tokenization, decoding and fallback choices live
//! here.
#![forbid(unsafe_code)]

pub trait CharsetServices {
    // 0: ICU UTF-8 with substitutions; 1: strict UTF-16 then UTF-8;
    // 2: native multibyte to wide then UTF-8; 3: ICU normalized UTF-8.
    fn convert(&self, input: &[u8], charset: &[u8], mode: u8) -> Option<Vec<u8>>;
}

#[derive(Debug, Default, PartialEq, Eq)]
pub struct Disposition {
    pub attachment: bool,
    pub flags: u32,
    pub filename: Vec<u8>,
}

fn lws(input: &[u8]) -> &[u8] {
    let start = input.iter().position(|c| !matches!(c, b' ' | b'\t')).unwrap_or(input.len());
    let end = input.iter().rposition(|c| !matches!(c, b' ' | b'\t')).map_or(start, |i| i + 1);
    &input[start..end]
}

// Delimiters inside double quotes are ignored, including unterminated quotes.
// Backslash only escapes within a quote. Each token starts with fresh state.
fn next_value<'a>(input: &mut &'a [u8], delimiter: u8) -> &'a [u8] {
    let mut quoted = false;
    let mut escaped = false;
    let mut end = input.len();
    for (i, &c) in input.iter().enumerate() {
        if quoted {
            if escaped {
                escaped = false;
            } else if c == b'\\' {
                escaped = true;
            } else if c == b'"' {
                quoted = false;
            }
        } else if c == delimiter {
            end = i;
            break;
        } else if c == b'"' {
            quoted = true;
        }
    }
    let value = lws(&input[..end]);
    *input = &input[(end + usize::from(end < input.len()))..];
    value
}

fn token(c: u8) -> bool {
    c > 0x20 && c < 0x7f && !b"()<>@,;:\\\"/[]?={}".contains(&c)
}

fn unquote(value: &[u8]) -> Vec<u8> {
    if !value.starts_with(b"\"") {
        return value.to_vec();
    }
    if value.len() == 1 || !value.ends_with(b"\"") {
        return value[1..].to_vec();
    }
    let mut output = Vec::with_capacity(value.len() - 2);
    let mut escaped = false;
    for &c in &value[1..value.len() - 1] {
        if c == b'\\' && !escaped {
            escaped = true;
            continue;
        }
        escaped = false;
        output.push(c);
    }
    output
}

fn hex(c: u8) -> Option<u8> {
    match c {
        b'0'..=b'9' => Some(c - b'0'),
        b'a'..=b'f' => Some(c - b'a' + 10),
        b'A'..=b'F' => Some(c - b'A' + 10),
        _ => None,
    }
}

fn unescape(input: &[u8]) -> Vec<u8> {
    let mut out = Vec::with_capacity(input.len());
    let mut i = 0;
    while i < input.len() {
        if input[i] == b'%'
            && i + 2 < input.len()
            && let (Some(a), Some(b)) = (hex(input[i + 1]), hex(input[i + 2]))
        {
            out.push(a * 16 + b);
            i += 3;
            continue;
        }
        out.push(input[i]);
        i += 1;
    }
    out
}

fn q_decode(input: &[u8]) -> Option<Vec<u8>> {
    let mut out = Vec::with_capacity(input.len());
    let mut i = 0;
    while i < input.len() {
        match input[i] {
            b'_' => out.push(b' '),
            b'=' => {
                let a = hex(*input.get(i + 1)?)?;
                let b = hex(*input.get(i + 2)?)?;
                out.push(a * 16 + b);
                i += 2;
            }
            c if c > 0x20 && c < 0x7f && c != b'?' => out.push(c),
            _ => return None,
        }
        i += 1;
    }
    Some(out)
}

fn base64_digit(c: u8) -> Option<u32> {
    match c {
        b'A'..=b'Z' => Some(u32::from(c - b'A')),
        b'a'..=b'z' => Some(u32::from(c - b'a') + 26),
        b'0'..=b'9' => Some(u32::from(c - b'0') + 52),
        b'+' => Some(62),
        b'/' => Some(63),
        _ => None,
    }
}

// modp strict policy requires padding/length but accepts nonzero discarded
// bits.
fn b_decode(input: &[u8]) -> Option<Vec<u8>> {
    if !input.len().is_multiple_of(4) {
        return None;
    }
    let mut out = Vec::with_capacity(input.len() / 4 * 3);
    for (i, chunk) in input.as_chunks::<4>().0.iter().enumerate() {
        let a = base64_digit(chunk[0])?;
        let b = base64_digit(chunk[1])?;
        out.push(((a << 2) | (b >> 4)) as u8);
        if chunk[2] == b'=' {
            if chunk[3] != b'=' || i + 1 != input.len() / 4 {
                return None;
            }
        } else {
            let c = base64_digit(chunk[2])?;
            out.push(((b << 4) | (c >> 2)) as u8);
            if chunk[3] == b'=' {
                if i + 1 != input.len() / 4 {
                    return None;
                }
            } else {
                let d = base64_digit(chunk[3])?;
                out.push(((c << 6) | d) as u8);
            }
        }
    }
    Some(out)
}

// Chromium IsStringUTF8 excludes Unicode noncharacters as well as malformed
// UTF-8.
fn valid_utf8(input: &[u8]) -> bool {
    std::str::from_utf8(input).is_ok_and(|s| {
        s.chars().all(|c| {
            let c = c as u32;
            !(0xfdd0..=0xfdef).contains(&c) && c & 0xffff != 0xfffe && c & 0xffff != 0xffff
        })
    })
}

fn decode_word(
    input: &[u8],
    referrer: &[u8],
    flags: &mut u32,
    services: &impl CharsetServices,
) -> Option<(Vec<u8>, bool)> {
    if input.is_empty() {
        return Some((Vec::new(), false));
    }
    if !input.is_ascii() {
        let output = if valid_utf8(input) {
            input.to_vec()
        } else {
            let converted =
                if referrer.is_empty() { None } else { services.convert(input, referrer, 1) };
            converted.or_else(|| services.convert(input, b"", 2))?
        };
        *flags |= 1 << 4;
        return Some((output, false));
    }
    let mut is_rfc = true;
    let mut charset = &b""[..];
    let mut b_encoding = false;
    let mut decoded = Vec::new();
    // The legacy tokenizer skips empty '?' fields. Preserve even its unusual
    // acceptance of truncated forms such as '=' and '=?UTF-8?='.
    for (index, part) in input.split(|c| *c == b'?').filter(|p| !p.is_empty()).enumerate() {
        match index {
            0 => {
                if part != b"=" {
                    is_rfc = false;
                    break;
                }
            }
            1 => charset = part,
            2 => {
                if part.len() != 1 || !b"bBqQ".contains(&part[0]) {
                    is_rfc = false;
                    break;
                }
                b_encoding = matches!(part[0], b'b' | b'B');
            }
            3 => {
                let bytes = if b_encoding { b_decode(part)? } else { q_decode(part)? };
                decoded = if bytes.is_empty() {
                    Vec::new()
                } else {
                    services.convert(&bytes, charset, 0)?
                };
            }
            4 => {
                if part != b"=" {
                    return None;
                }
            }
            _ => return None,
        }
    }
    if is_rfc {
        if !input.ends_with(b"=") {
            return None;
        }
        *flags |= 1 << 6;
        return Some((decoded, true));
    }
    let decoded = unescape(input);
    if decoded != input {
        *flags |= 1 << 5;
    }
    valid_utf8(&decoded).then_some((decoded, false))
}

fn decode_filename(
    input: &[u8],
    referrer: &[u8],
    services: &impl CharsetServices,
) -> Option<(Vec<u8>, u32)> {
    let mut flags = 0;
    let mut out = Vec::new();
    let mut previous_rfc = true;
    let mut i = 0;
    while i < input.len() {
        if b" \t\n\r".contains(&input[i]) {
            if !previous_rfc {
                out.push(b' ');
            }
            i += 1;
            continue;
        }
        let start = i;
        while i < input.len() && !b" \t\n\r".contains(&input[i]) {
            i += 1;
        }
        let (decoded, rfc) = decode_word(&input[start..i], referrer, &mut flags, services)?;
        out.extend(decoded);
        previous_rfc = rfc;
    }
    if out.is_empty() {
        flags = 0;
    }
    Some((out, flags))
}

fn decode_extended(input: &[u8], services: &impl CharsetServices) -> Option<Vec<u8>> {
    if input.contains(&b'"') {
        return None;
    }
    let parts: Vec<_> = input.split(|c| *c == b'\'').collect();
    if parts.len() != 3 || parts[0].is_empty() || parts[2].is_empty() {
        return None;
    }
    if !parts[2].is_ascii() {
        return Some(Vec::new());
    }
    services.convert(&unescape(parts[2]), parts[0], 3)
}

pub fn parse(
    mut input: &[u8],
    referrer: &[u8],
    first_value: bool,
    services: &impl CharsetServices,
) -> Disposition {
    if first_value {
        input = next_value(&mut input, b',');
    }
    let mut result = Disposition::default();
    let delimiter = input.iter().position(|c| *c == b';').unwrap_or(input.len());
    let kind = lws(&input[..delimiter]);
    if !kind.is_empty() && kind.iter().copied().all(token) {
        result.flags |= 1;
        if !kind.eq_ignore_ascii_case(b"inline") {
            result.attachment = true;
            if !kind.eq_ignore_ascii_case(b"attachment") {
                result.flags |= 2;
            }
        }
        input = &input[(delimiter + usize::from(delimiter < input.len()))..];
    }
    let mut filename = Vec::new();
    let mut extended = Vec::new();
    while !input.is_empty() {
        let pair = next_value(&mut input, b';');
        if pair.is_empty() {
            continue;
        }
        let Some(equals) = pair.iter().position(|c| *c == b'=') else {
            break;
        };
        if equals == 0 {
            break;
        }
        let name = lws(&pair[..equals]);
        if name.contains(&b'"') {
            break;
        }
        let value = lws(&pair[equals + 1..]);
        if value.is_empty() {
            break;
        }
        if filename.is_empty() && name.eq_ignore_ascii_case(b"filename") {
            if let Some((decoded, flags)) = decode_filename(&unquote(value), referrer, services) {
                filename = decoded;
                result.flags |= flags;
            }
            if !filename.is_empty() {
                result.flags |= 4;
                if filename[0] == b'\'' {
                    result.flags |= 128;
                }
            }
        } else if extended.is_empty() && name.eq_ignore_ascii_case(b"filename*") {
            // Mismatched opening quotes are removed from raw_value by the
            // legacy NameValuePairsIterator, even for extended attributes.
            let raw = if value.starts_with(b"\"") && (value.len() == 1 || !value.ends_with(b"\"")) {
                &value[1..]
            } else {
                value
            };
            extended = decode_extended(raw, services).unwrap_or_default();
            if !extended.is_empty() {
                result.flags |= 8;
            }
        }
    }
    result.filename = if extended.is_empty() { filename } else { extended };
    result
}
