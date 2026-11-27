// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

//! Reusable byte-oriented DNS policy with caller-owned bounded output.
#![forbid(unsafe_code)]

pub const MAX_NAME: usize = 255;
pub trait HostServices {
    fn compliant(&mut self, input: &[u8]) -> bool;
    fn localhost(&mut self, input: &[u8]) -> bool;
    fn ip_literal(&mut self, input: &[u8]) -> bool;
    fn url_address(&mut self, input: &[u8]) -> bool;
}

// A shared scan keeps validity and conversion acceptance identical, without
// allocating a wire name for callers which need only a validity result.
fn labels(input: &[u8], mut write: impl FnMut(usize, &[u8])) -> Option<usize> {
    let mut total = 0;
    let mut start = 0;
    while start < input.len() {
        let end = input[start..]
            .iter()
            .position(|&c| c == b'.')
            .map_or(input.len(), |offset| start + offset);
        let len = end - start;
        if len == 0 || len > 63 || total + len + 2 > MAX_NAME {
            return None;
        }
        write(total, &input[start..end]);
        total += len + 1;
        start = end + 1;
    }
    if total == 0 {
        None
    } else {
        Some(total + 1)
    }
}

pub fn dotted(input: &[u8], output: &mut [u8; MAX_NAME]) -> Option<usize> {
    let len = labels(input, |offset, label| {
        output[offset] = label.len() as u8;
        output[offset + 1..offset + 1 + label.len()].copy_from_slice(label);
    })?;
    output[len - 1] = 0;
    Some(len)
}

pub fn valid(input: &[u8]) -> bool {
    labels(input, |_, _| {}).is_some()
}

pub fn dotted_with_services(
    input: &[u8],
    require_hostname: bool,
    services: &mut impl HostServices,
    output: &mut [u8; MAX_NAME],
) -> Option<usize> {
    if require_hostname && !services.compliant(input) {
        return None;
    }
    dotted(input, output)
}

pub fn valid_record(input: &[u8], services: &mut impl HostServices) -> bool {
    valid(input)
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

/// `remaining` lasts until the next mutable reader call. `take` receives at
/// most 256 bytes, bounded by remaining length. `taken` exposes those bytes
/// after the cursor commit, so the cursor object may itself be input storage.
pub trait NameReader {
    fn remaining(&self) -> &[u8];
    fn take(&mut self, count: usize);
    fn taken(&self) -> &[u8];
}

struct SliceReader<'a> {
    remaining: &'a [u8],
    taken: &'a [u8],
}
impl NameReader for SliceReader<'_> {
    fn remaining(&self) -> &[u8] {
        self.remaining
    }
    fn take(&mut self, count: usize) {
        (self.taken, self.remaining) = self.remaining.split_at(count);
    }
    fn taken(&self) -> &[u8] {
        self.taken
    }
}

/// Contiguous parsing borrows only input and separate initialized output.
/// Caller commits the returned consumed length after both borrows have ended.
pub fn network(
    input: &[u8],
    complete: bool,
    output: &mut [u8; MAX_NAME],
) -> (usize, Option<usize>) {
    network_reader(&mut SliceReader { remaining: input, taken: &[] }, complete, output)
}

/// Completed reads consume bytes even on label/name limit failure. Compression
/// rejection and truncated reads leave the current label untouched.
pub fn network_reader(
    reader: &mut impl NameReader,
    complete: bool,
    output: &mut [u8; MAX_NAME],
) -> (usize, Option<usize>) {
    let mut len = 0;
    let mut consumed = 0;
    let mut octets = 0;
    loop {
        let label_len = {
            let input = reader.remaining();
            if input.is_empty() {
                break;
            }
            if input[0] & 0xc0 == 0xc0 {
                return (consumed, None);
            }
            let Some((_, n)) = length_prefixed(input, false) else {
                return (consumed, None);
            };
            n
        };
        reader.take(label_len + 1);
        consumed += label_len + 1;
        if label_len != 0 {
            octets += label_len + 1;
        }
        if label_len > 63 || octets > MAX_NAME {
            return (consumed, None);
        }
        if label_len == 0 {
            return (consumed, Some(len));
        }
        if len != 0 {
            output[len] = b'.';
            len += 1;
        }
        output[len..len + label_len].copy_from_slice(&reader.taken()[1..label_len + 1]);
        len += label_len;
    }
    (consumed, (!complete).then_some(len))
}

pub fn less(lhs: &[u8], rhs: &[u8]) -> bool {
    for (&left, &right) in lhs.iter().zip(rhs) {
        if left != right {
            let left = left.to_ascii_lowercase();
            let right = right.to_ascii_lowercase();
            if left != right {
                return left < right;
            }
        }
    }
    lhs.len() < rhs.len()
}

pub trait Canonicalizer {
    // External URL service writes to its own C++ output; true means BROKEN.
    fn canonicalize(&mut self, input: &[u8]) -> bool;
    fn use_original(&mut self, input: &[u8]);
}
pub fn canonicalize(input: &[u8], service: &mut impl Canonicalizer) {
    if service.canonicalize(input) {
        service.use_original(input);
    }
}
