// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#![forbid(unsafe_code)]

// These IDs are persisted by the native API. Keep error selection here as well
// as extension parsing: QUICHE supplies framing facts, not ALPS policy.
pub const NO_ERROR: i32 = 0;
pub const MALFORMED: i32 = 7;

pub fn bypass(previous: i32, next: i32) -> i32 {
    if next == NO_ERROR {
        previous
    } else {
        next
    }
}

pub fn finish(framing: bool, forbidden: bool, ack: bool, ready: bool, error: i32) -> i32 {
    if framing {
        1
    } else if forbidden {
        2
    } else if ack {
        4
    } else if !ready {
        3
    } else {
        error
    }
}

pub fn common_header(forbidden: bool, frame_type: u8) -> bool {
    forbidden || matches!(frame_type, 0..=3 | 5..=9)
}

// The original counter has signed-overflow UB after INT_MAX SETTINGS starts.
// All defined original counts are identical; wrapping keeps Rust total too.
pub fn settings_start(count: i32) -> i32 {
    count.wrapping_add(1)
}

#[repr(C)]
pub struct Setting {
    pub id: u32,
    pub value: u32,
}

pub fn setting(id: u32, value: u32) -> Setting {
    // Every setting, including unknown IDs and duplicates, requests a native
    // map upsert. No allowlist, validation or first-value-wins filter applies.
    Setting { id, value }
}

pub fn settings_ack() -> bool {
    true
}

#[repr(C)]
pub struct Header {
    pub error: i32,
    pub accept: bool,
}

pub fn header(
    error: i32,
    enabled: impl FnOnce() -> bool,
    stream: u32,
    frame_type: u8,
    flags: u8,
) -> Header {
    if error != NO_ERROR {
        return Header { error, accept: false };
    }
    let enabled = enabled();
    let error = if !enabled || frame_type != 0x89 {
        error
    } else if stream != 0 {
        5
    } else if flags != 0 {
        6
    } else {
        NO_ERROR
    };
    Header { error, accept: error == NO_ERROR && enabled && frame_type == 0x89 }
}

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
#[repr(C)]
pub struct Pair {
    pub origin: usize,
    pub origin_len: usize,
    pub value: usize,
    pub value_len: usize,
}

pub struct Payload {
    pub pairs: Vec<Pair>,
    pub bypass: i32,
}

fn prefixed(data: &[u8], cursor: &mut usize) -> Option<(usize, usize)> {
    let prefix = data.get(*cursor..)?.get(..2)?;
    // C++ first casts its (possibly signed) high char to uint16_t, promotes
    // before shifting, then truncates the sum back to uint16_t. The resulting
    // low 16 bits are exactly big endian for all 65,536 prefixes.
    let len = usize::from(u16::from_be_bytes([prefix[0], prefix[1]]));
    *cursor += 2;
    let begin = *cursor;
    data.get(begin..)?.get(..len)?;
    *cursor += len;
    Some((begin, len))
}

pub fn payload_pairs(data: &[u8], mut emit: impl FnMut(Pair)) -> i32 {
    let mut cursor = 0;
    while cursor < data.len() {
        let pair = prefixed(data, &mut cursor).and_then(|(origin, origin_len)| {
            prefixed(data, &mut cursor).map(|(value, value_len)| Pair {
                origin,
                origin_len,
                value,
                value_len,
            })
        });
        match pair {
            Some(pair) => emit(pair),
            None => return MALFORMED,
        }
    }
    NO_ERROR
}

pub fn payload_reserved(data: &[u8], capacity: usize) -> Payload {
    let mut pairs = Vec::with_capacity(capacity);
    let bypass = payload_pairs(data, |pair| pairs.push(pair));
    Payload { pairs, bypass }
}

pub fn payload(data: &[u8]) -> Payload {
    payload_reserved(data, 0)
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn all_prefixes_and_partial_commit() {
        for len in 0..=u16::MAX {
            let mut data = len.to_be_bytes().to_vec();
            data.resize(usize::from(len) + 2, 0xff);
            data.extend_from_slice(&[0, 0]);
            let result = payload(&data);
            assert_eq!(result.bypass, 0);
            assert_eq!(result.pairs.len(), 1);
            assert_eq!(result.pairs[0].origin_len, usize::from(len));
        }
        for data in [&b"\0"[..], &b"\0\0"[..], &b"\0\0\0"[..], &b"\0\x02x"[..]] {
            assert_eq!(payload(data).bypass, MALFORMED);
        }
        let result = payload(b"\0\0\0\0x");
        assert_eq!(result.pairs.len(), 1);
        assert_eq!(result.bypass, MALFORMED);
    }

    #[test]
    fn policy_precedence() {
        assert_eq!(finish(true, true, true, false, 5), 1);
        assert_eq!(finish(false, true, true, false, 5), 2);
        assert_eq!(finish(false, false, true, false, 5), 4);
        assert_eq!(finish(false, false, false, false, 5), 3);
        assert_eq!(finish(false, false, false, true, 5), 5);
        assert_eq!(
            header(6, || panic!("sticky error must skip feature query"), 1, 0x89, 1).error,
            6
        );
        assert_eq!(header(0, || false, 1, 0x89, 1).error, 0);
        assert_eq!(header(0, || true, 1, 0x89, 1).error, 5);
        assert_eq!(header(0, || true, 0, 0x89, 1).error, 6);
        assert!(header(0, || true, 0, 0x89, 0).accept);
        assert!(!header(0, || true, 0, 42, 0).accept);
        assert!(common_header(false, 0));
        assert!(!common_header(false, 4));
        assert!(common_header(true, 4));
        assert_eq!(settings_start(i32::MAX), i32::MIN);
        assert_eq!(setting(0xffff, 42).value, 42);
        assert!(settings_ack());
    }
}
