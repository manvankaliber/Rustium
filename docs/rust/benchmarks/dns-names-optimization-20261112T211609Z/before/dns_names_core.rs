// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

//! Byte-oriented DNS policy. URL/IP/localhost services stay in their modules.
#![forbid(unsafe_code)]

pub trait HostServices {
    fn compliant(&mut self, input: &[u8]) -> bool;
    fn localhost(&mut self, input: &[u8]) -> bool;
    fn ip_literal(&mut self, input: &[u8]) -> bool;
    fn url_address(&mut self, input: &[u8]) -> bool;
    fn canonicalize(&mut self, input: &[u8]) -> (bool, Vec<u8>);
}

pub fn dotted(input: &[u8]) -> Option<Vec<u8>> {
    let mut output = Vec::with_capacity(255);
    let mut start = 0;
    while start < input.len() {
        let end = input[start..]
            .iter()
            .position(|&c| c == b'.')
            .map_or(input.len(), |offset| start + offset);
        let len = end - start;
        if len == 0 || len > 63 || output.len() + len + 2 > 255 {
            return None;
        }
        output.push(len as u8);
        output.extend_from_slice(&input[start..end]);
        start = end + 1;
    }
    if output.is_empty() {
        return None;
    }
    output.push(0);
    Some(output)
}

pub fn dotted_with_services(
    input: &[u8],
    require_hostname: bool,
    services: &mut impl HostServices,
) -> Option<Vec<u8>> {
    if require_hostname && !services.compliant(input) {
        return None;
    }
    dotted(input)
}

pub fn valid_record(input: &[u8], services: &mut impl HostServices) -> bool {
    dotted(input).is_some()
        && !services.localhost(input)
        && !services.ip_literal(input)
        && !services.url_address(input)
}

/// Returns payload offset and length; failure consumes nothing.
pub fn length_prefixed(input: &[u8], wide: bool) -> Option<(usize, usize)> {
    let (prefix, len) = if wide {
        (2, usize::from(u16::from_be_bytes([*input.first()?, *input.get(1)?])))
    } else {
        (1, usize::from(*input.first()?))
    };
    (len <= input.len() - prefix).then_some((prefix, len))
}

/// The returned view lasts until the next mutable reader call. `take` receives
/// at most 256 bytes and never more than `remaining().len()`.
pub trait NameReader {
    fn remaining(&self) -> &[u8];
    // Advance first, then copy the consumed bytes: input can alias the cursor.
    fn take(&mut self, count: usize) -> [u8; 256];
}

/// `consumed` includes completed reads, even if their label/name limit fails.
/// Compression rejection and truncated reads leave the current label untouched.
pub fn network(reader: &mut impl NameReader, complete: bool) -> (usize, Option<Vec<u8>>) {
    let mut output = Vec::new();
    let mut consumed = 0;
    let mut octets = 0;
    while !reader.remaining().is_empty() {
        if reader.remaining()[0] & 0xc0 == 0xc0 {
            return (consumed, None);
        }
        let Some((prefix, len)) = length_prefixed(reader.remaining(), false) else {
            return (consumed, None);
        };
        let label = reader.take(prefix + len);
        consumed += prefix + len;
        if len != 0 {
            octets += len + 1;
        }
        if len > 63 || octets > 255 {
            return (consumed, None);
        }
        if len == 0 {
            return (consumed, Some(output));
        }
        if !output.is_empty() {
            output.push(b'.');
        }
        output.extend_from_slice(&label[prefix..prefix + len]);
    }
    (consumed, (!complete).then_some(output))
}

pub fn less(lhs: &[u8], rhs: &[u8]) -> bool {
    lhs.iter().map(u8::to_ascii_lowercase).cmp(rhs.iter().map(u8::to_ascii_lowercase)).is_lt()
}

pub fn canonicalize(input: &[u8], services: &mut impl HostServices) -> Vec<u8> {
    let (broken, output) = services.canonicalize(input);
    if broken {
        input.to_vec()
    } else {
        output
    }
}
