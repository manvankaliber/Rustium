// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#![forbid(unsafe_code)]

use std::mem::MaybeUninit;

#[repr(C)]
#[derive(Clone, Copy, Default, Debug, PartialEq, Eq)]
pub struct Header {
    pub length: u64,
    pub key: [u8; 4],
    pub opcode: i32,
    pub final_bit: u8,
    pub reserved1: u8,
    pub reserved2: u8,
    pub reserved3: u8,
    pub masked: u8,
}

fn first_byte(h: Header) -> u8 {
    (if h.final_bit != 0 { 128 } else { 0 })
        | (if h.reserved1 != 0 { 64 } else { 0 })
        | (if h.reserved2 != 0 { 32 } else { 0 })
        | (if h.reserved3 != 0 { 16 } else { 0 })
        | ((h.opcode & 15) as u8)
}

pub fn opcode(kind: u8, op: i32) -> bool {
    match kind {
        0 => (0..=2).contains(&op),
        1 => (8..=10).contains(&op),
        2 => (3..=7).contains(&op),
        _ => (11..=15).contains(&op),
    }
}

pub fn header_size(h: Header) -> usize {
    2 + if h.length <= 125 {
        0
    } else if h.length <= 65535 {
        2
    } else {
        8
    } + if h.masked != 0 { 4 } else { 0 }
}

// Only writes the prefix: the ABI reads the potentially aliased masking key
// after these writes, matching SpanWriter's observable read order.
pub fn write_prefix(h: Header, out: &mut [MaybeUninit<u8>]) -> Option<usize> {
    if out.len() < header_size(h) {
        return None;
    }
    out[0].write(first_byte(h));
    let (tag, count) = if h.length <= 125 {
        (h.length as u8, 0)
    } else if h.length <= 65535 {
        (126, 2)
    } else {
        (127, 8)
    };
    out[1].write(tag | if h.masked != 0 { 128 } else { 0 });
    if count != 0 {
        for (slot, byte) in out[2..2 + count].iter_mut().zip(&h.length.to_be_bytes()[8 - count..]) {
            slot.write(*byte);
        }
    }
    Some(2 + count)
}

pub fn mask(key: [u8; 4], offset: u64, data: &mut [u8]) {
    let start = offset as usize & 3;
    let k = std::array::from_fn::<_, 16, _>(|i| key[(start + i) & 3]);
    let packed = u128::from_ne_bytes(k);
    let (chunks, remainder) = data.as_chunks_mut::<16>();
    for chunk in chunks {
        let value = u128::from_ne_bytes(*chunk);
        chunk.copy_from_slice(&(value ^ packed).to_ne_bytes());
    }
    for (i, b) in remainder.iter_mut().enumerate() {
        *b ^= k[i];
    }
}

// The key may overlap payload. A single mutable union slice, indexed by
// offsets, avoids constructing aliased Rust borrows. Match the frozen C++
// scalar prefix / packed body / scalar tail read order, including live key
// mutations in scalar sections. Packed width is 8 on MSVC x64, 16 on GCC x64.
pub fn mask_aliased(
    storage: &mut [u8],
    key: usize,
    begin: usize,
    len: usize,
    address_modulus: usize,
    packed_width: usize,
    offset: u64,
) {
    let scalar = |s: &mut [u8], from: usize, count: usize, phase: usize| {
        for i in 0..count {
            let k = s[key + ((phase + i) & 3)];
            s[begin + from + i] ^= k;
        }
    };
    let phase = offset as usize & 3;
    if len <= packed_width * 2 {
        scalar(storage, 0, len, phase);
        return;
    }
    let prefix = (packed_width - address_modulus) % packed_width;
    scalar(storage, 0, prefix, phase);
    let rotated = std::array::from_fn::<_, 4, _>(|i| storage[key + ((phase + prefix + i) & 3)]);
    let end = prefix + (len - prefix) / packed_width * packed_width;
    mask(rotated, 0, &mut storage[begin + prefix..begin + end]);
    scalar(storage, end, len - end, (phase + end) & 3);
}

// kind: 0 success/no reason, 1 broken size, 2 reserved code, 3 UTF-8 error,
// 4 success with reason borrowed from input. Noncharacters are valid UTF-8.
pub fn close(payload: &[u8]) -> (u16, u8) {
    match payload.len() {
        0 => (1005, 0),
        1 => (1002, 1),
        _ => {
            let code = u16::from_be_bytes([payload[0], payload[1]]);
            if matches!(code, 1005 | 1006 | 1015) {
                return (1002, 2);
            }
            if payload.len() == 2 {
                return (code, 0);
            }
            if std::str::from_utf8(&payload[2..]).is_ok() {
                (code, 4)
            } else {
                (1002, 3)
            }
        }
    }
}

#[derive(Default)]
pub struct Parser {
    pending: [u8; 14],
    pending_len: usize,
    current: Option<Header>,
    offset: u64,
    pub error: u16,
}

impl Parser {
    pub fn new() -> Self {
        Self { error: 1000, ..Self::default() }
    }

    fn header(&mut self, data: &[u8]) -> usize {
        if data.len() < 2 {
            return 0;
        }
        let mut length = u64::from(data[1] & 127);
        let mut consumed = 2;
        if length == 126 {
            if data.len() < 4 {
                return 0;
            }
            length = u64::from(u16::from_be_bytes([data[2], data[3]]));
            consumed = 4;
            if length <= 125 {
                self.error = 1002;
                return 0;
            }
        } else if length == 127 {
            if data.len() < 10 {
                return 0;
            }
            length = u64::from_be_bytes(data[2..10].try_into().expect("8-byte length"));
            consumed = 10;
            if length <= 65535 || length > i64::MAX as u64 {
                self.error = 1002;
                return 0;
            }
            if length > i32::MAX as u64 {
                self.error = 1009;
                return 0;
            }
        }
        let masked = u8::from(data[1] & 128 != 0);
        let mut key = [0; 4];
        if masked != 0 {
            if data.len() < consumed + 4 {
                return 0;
            }
            key.copy_from_slice(&data[consumed..consumed + 4]);
            consumed += 4;
        }
        self.current = Some(Header {
            length,
            key,
            opcode: i32::from(data[0] & 15),
            final_bit: u8::from(data[0] & 128 != 0),
            reserved1: u8::from(data[0] & 64 != 0),
            reserved2: u8::from(data[0] & 32 != 0),
            reserved3: u8::from(data[0] & 16 != 0),
            masked,
        });
        consumed
    }

    // Emit payload offsets, never borrowed storage. The caller owns input and
    // chunk allocations; empty payloads have the original null-span semantics.
    // Complete chunks preceding a malformed header remain emitted on failure.
    pub fn decode(
        &mut self,
        input: &[u8],
        mut emit: impl FnMut(Option<Header>, bool, usize, usize),
    ) -> bool {
        if self.error != 1000 {
            return false;
        }
        if input.is_empty() {
            return true;
        }
        let mut pos = 0;
        let mut first = false;
        if self.pending_len != 0 {
            let old = self.pending_len;
            let take = input.len().min(14 - old);
            self.pending[old..old + take].copy_from_slice(&input[..take]);
            self.pending_len += take;
            let pending = self.pending;
            let consumed = self.header(&pending[..self.pending_len]);
            if self.error != 1000 {
                return false;
            }
            if self.current.is_none() {
                return true;
            }
            pos = consumed - old;
            self.pending_len = 0;
            first = true;
        }
        while pos < input.len() || first {
            if self.current.is_none() {
                pos += self.header(&input[pos..]);
                if self.error != 1000 {
                    return false;
                }
                if self.current.is_none() {
                    self.pending_len = input.len() - pos;
                    self.pending[..self.pending_len].copy_from_slice(&input[pos..]);
                    return true;
                }
                first = true;
            }
            let h = self.current.expect("parsed header");
            let size = ((input.len() - pos) as u64).min(h.length - self.offset) as usize;
            self.offset += size as u64;
            let final_chunk = self.offset == h.length;
            emit(if first { Some(h) } else { None }, final_chunk, pos, size);
            first = false;
            pos += size;
            if final_chunk {
                self.current = None;
                self.offset = 0;
            }
        }
        true
    }
}
// Header/output byte aliasing is legal. Read fields at the same stages as
// the original writer, with no live output borrow during a metadata read.
// The write callback follows SpanWriter: failed writes leave cursor unchanged.
pub fn write_live_prefix(
    initial: Header,
    capacity: usize,
    mut read: impl FnMut() -> Header,
    mut write: impl FnMut(usize, &[u8]),
) -> Option<(usize, bool)> {
    if capacity < header_size(initial) {
        return None;
    }
    write(0, &[first_byte(initial)]);
    let h = read();
    let (tag, count) = if h.length <= 125 {
        (h.length as u8, 0)
    } else if h.length <= 65535 {
        (126, 2)
    } else {
        (127, 8)
    };
    write(1, &[tag | if h.masked != 0 { 128 } else { 0 }]);
    let mut cursor = 2;
    if count != 0 && count <= capacity - cursor {
        write(cursor, &read().length.to_be_bytes()[8 - count..]);
        cursor += count;
    }
    Some((cursor, read().masked != 0))
}
