// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

//! DNS response wire policy. No pointers, ownership casts, or retained packets.
#![forbid(unsafe_code)]
use crate::ffi::{BuildResult, ParseResult, Question, Record};

pub trait ParserIo {
    fn packet(&self) -> &[u8];
    fn clear_name(&mut self);
    fn name_empty(&self) -> bool;
    fn append_name(&mut self, bytes: &[u8]);
    fn field(&mut self, index: u8, value: u32);
    fn rdata(&mut self, offset: usize, len: usize);
}

// Iterative traversal uses the original byte budget, including pointer bytes.
// In skip mode the first complete pointer is sufficient, even if its target
// is invalid. That public distinction is intentional.
pub fn read_name(io: &mut impl ParserIo, initial: usize, store: bool) -> usize {
    if initial >= io.packet().len() {
        return 0;
    }
    if store {
        io.clear_name();
    }
    let mut offset = initial;
    let mut seen = 0usize;
    let mut consumed = 0;
    let mut encoded = 0;
    loop {
        match io.packet()[offset] & 0xc0 {
            0xc0 => {
                let size = io.packet().len();
                if size < 2 || offset > size - 2 {
                    return 0;
                }
                if consumed == 0 {
                    consumed = offset - initial + 2;
                    if !store {
                        return consumed;
                    }
                }
                seen += 2;
                if seen > size {
                    return 0;
                }
                offset = usize::from(
                    u16::from_be_bytes([io.packet()[offset], io.packet()[offset + 1]]) & 0x3fff,
                );
                if offset >= size {
                    return 0;
                }
            }
            0 => {
                let n = usize::from(io.packet()[offset]);
                offset += 1;
                if n == 0 {
                    return if consumed == 0 { offset - initial } else { consumed };
                }
                encoded += n + 1;
                if encoded > 255 || n >= io.packet().len() - offset {
                    return 0;
                }
                if store {
                    if !io.name_empty() {
                        io.append_name(b".");
                    }
                    // Reborrow after the append: packet bytes may alias output.
                    let mut label = [0; 63];
                    label[..n].copy_from_slice(&io.packet()[offset..offset + n]);
                    io.append_name(&label[..n]);
                }
                offset += n;
                seen += n + 1;
            }
            _ => return 0,
        }
    }
}
fn u16_at(input: &[u8], offset: usize) -> Option<u16> {
    let s = input.get(offset..offset.checked_add(2)?)?;
    Some(u16::from_be_bytes([s[0], s[1]]))
}
pub fn parse_record(io: &mut impl ParserIo, offset: usize, allowed: bool) -> usize {
    if !allowed {
        return 0;
    }
    let n = read_name(io, offset, true);
    if n == 0 {
        return 0;
    }
    let mut pos = offset + n;
    for (index, width) in [(0, 2), (1, 2), (2, 4)] {
        let value = if width == 4 {
            let Some(s) = io.packet().get(pos..pos + 4) else {
                return 0;
            };
            u32::from_be_bytes([s[0], s[1], s[2], s[3]])
        } else {
            let Some(v) = u16_at(io.packet(), pos) else {
                return 0;
            };
            u32::from(v)
        };
        io.field(index, value);
        pos += width;
    }
    let Some(len) = u16_at(io.packet(), pos) else {
        return 0;
    };
    pos += 2;
    if usize::from(len) > io.packet().len() - pos {
        return 0;
    }
    io.rdata(pos, usize::from(len));
    pos + usize::from(len) - offset
}
pub fn parse_question(io: &mut impl ParserIo, offset: usize) -> usize {
    let n = read_name(io, offset, true);
    if n == 0 || n + 4 > io.packet().len() - offset {
        return 0;
    }
    let value = u16_at(io.packet(), offset + n).unwrap();
    io.field(0, u32::from(value));
    n + 4
}

struct NameIo<'a> {
    packet: &'a [u8],
    name: Vec<u8>,
}
impl ParserIo for NameIo<'_> {
    fn packet(&self) -> &[u8] {
        self.packet
    }
    fn clear_name(&mut self) {
        self.name.clear();
    }
    fn name_empty(&self) -> bool {
        self.name.is_empty()
    }
    fn append_name(&mut self, b: &[u8]) {
        self.name.extend_from_slice(b);
    }
    fn field(&mut self, _: u8, _: u32) {}
    fn rdata(&mut self, _: usize, _: usize) {}
}
// Uncompressed query-name conversion accepts an omitted final zero, exactly
// as NetworkToDottedName(require_complete=false) does.
fn query_name(input: &[u8]) -> Option<Vec<u8>> {
    let mut result = Vec::new();
    let mut pos = 0;
    let mut encoded = 0;
    while pos < input.len() {
        let n = usize::from(input[pos]);
        if n & 0xc0 == 0xc0 || n > 63 || pos + 1 + n > input.len() {
            return None;
        }
        pos += 1;
        if n == 0 {
            return Some(result);
        }
        encoded += n + 1;
        if encoded > 255 {
            return None;
        }
        if !result.is_empty() {
            result.push(b'.');
        }
        result.extend_from_slice(&input[pos..pos + n]);
        pos += n;
    }
    Some(result)
}
pub fn header(input: &[u8], index: usize) -> u16 {
    u16_at(input, index * 2).unwrap_or(0)
}
// Public accessors read the mutable backing every time; no cached header.
pub fn field_value(input: &[u8], index: usize) -> u16 {
    match index {
        1 => header(input, 1) & !15,
        6 => header(input, 1) & 15,
        _ => header(input, index),
    }
}
pub fn init_parse(
    input: &[u8],
    question: &[u8],
    qname: &[u8],
    query_id: u16,
    qtype: u16,
    matching: bool,
) -> ParseResult {
    let mut result = ParseResult {
        success: false,
        id_available: false,
        reset_parser: false,
        offset: 0,
        records: 0,
        questions: Vec::new(),
    };
    if input.len() < 12 || (matching && input.len() < 12 + question.len()) {
        return result;
    }
    result.id_available = true;
    if matching && header(input, 0) != query_id {
        return result;
    }
    if header(input, 1) & 0x8000 == 0 {
        return result;
    }
    result.records = usize::from(header(input, 3))
        + usize::from(header(input, 4))
        + usize::from(header(input, 5));
    if matching {
        if header(input, 2) != 1 || &input[12..12 + question.len()] != question {
            return result;
        }
        let Some(name) = query_name(qname) else {
            return result;
        };
        result.questions.push(Question { name, qtype });
        result.offset = 12 + question.len();
    } else {
        result.reset_parser = true;
        let mut io = NameIo { packet: input, name: Vec::new() };
        let mut offset = 12;
        for _ in 0..header(input, 2) {
            let n = read_name(&mut io, offset, true);
            if n == 0 || n + 4 > input.len() - offset {
                return result;
            }
            result.questions.push(Question {
                name: std::mem::take(&mut io.name),
                qtype: u16_at(input, offset + n).unwrap(),
            });
            offset += n + 4;
        }
        result.offset = offset;
    }
    result.success = true;
    result
}
pub fn record_size(name: &[u8], kind: u16, owned: usize, borrowed: usize) -> usize {
    let n =
        if kind == 41 { 1 } else { name.len() + if name.last() == Some(&b'.') { 1 } else { 2 } };
    n + 10 + if owned == 0 { borrowed } else { owned }
}
fn hostname(input: &[u8]) -> bool {
    if input.is_empty() || input.len() > 254 || (input.len() == 254 && input.last() != Some(&b'.'))
    {
        return false;
    }
    let mut in_component = false;
    let mut started = false;
    let mut size = 0;
    for &c in input {
        size += 1;
        let alnum = c.is_ascii_lowercase() || c.is_ascii_digit();
        if !in_component {
            started = alnum;
            if !alnum && c != b'-' && c != b'_' {
                return false;
            }
            in_component = true;
        } else if c == b'.' {
            in_component = false;
            if size > 64 || size == 1 {
                return false;
            }
            size = 0;
        } else if !alnum && c != b'-' && c != b'_' {
            return false;
        }
    }
    size <= 63 && started
}
fn wire_name(input: &[u8], internet: bool) -> Option<Vec<u8>> {
    if internet && !hostname(input) {
        return None;
    }
    let mut result = Vec::new();
    let mut pos = 0;
    while pos < input.len() {
        let end = input[pos..].iter().position(|&c| c == b'.').map_or(input.len(), |n| pos + n);
        let n = end - pos;
        if n == 0 || n > 63 || result.len() + n + 2 > 255 {
            return None;
        }
        result.push(n as u8);
        result.extend_from_slice(&input[pos..end]);
        pos = end + 1;
    }
    if result.is_empty() {
        return None;
    }
    result.push(0);
    Some(result)
}
fn valid_size(kind: u16, n: usize) -> bool {
    match kind {
        1 => n == 4,
        28 => n == 16,
        33 => n >= 6,
        65 => n >= 3,
        _ => true,
    }
}
// Debug-only caller contract, evaluated in C++'s DCHECK mode rather than
// depending on a potentially different Rust debug-assertion configuration.
pub fn valid_rcode(rcode: u8) -> bool {
    rcode & !15 == 0
}
fn put16(out: &mut Vec<u8>, v: u16) {
    out.extend_from_slice(&v.to_be_bytes());
}
#[allow(clippy::too_many_arguments)]
pub fn build(
    id: u16,
    authoritative: bool,
    rcode: u8,
    query_id: u16,
    qtype: u16,
    question: &[u8],
    has_query: bool,
    records: &[Record],
    answers: usize,
    authority: usize,
    validate: bool,
    internet: bool,
) -> BuildResult {
    let mut bytes = Vec::new();
    for v in [
        id,
        0x8000 | if authoritative { 0x400 } else { 0 } | u16::from(rcode),
        u16::from(has_query),
        answers as u16,
        authority as u16,
        (records.len() - answers - authority) as u16,
    ] {
        put16(&mut bytes, v);
    }
    if has_query {
        bytes.extend_from_slice(question);
    }
    let mut success = !has_query || id == query_id;
    let mut allocated = bytes.len();
    for (index, r) in records.iter().enumerate() {
        allocated += record_size(&r.name, r.kind, r.data.len(), r.data.len());
        if (validate && !valid_size(r.kind, r.data.len()))
            || (validate && has_query && index < answers && r.kind != qtype && r.kind != 5)
        {
            success = false;
            continue;
        }
        let name = if r.kind == 41 { Some(vec![0]) } else { wire_name(&r.name, internet) };
        let Some(name) = name else {
            success = false;
            continue;
        };
        bytes.extend_from_slice(&name);
        put16(&mut bytes, r.kind);
        put16(&mut bytes, r.klass);
        bytes.extend_from_slice(&r.ttl.to_be_bytes());
        put16(&mut bytes, r.data.len() as u16);
        bytes.extend_from_slice(&r.data);
    }
    // The original size formula may overestimate only for invalid names, which
    // already fail. Initialize every byte, even in validation-disabled builds.
    bytes.resize(allocated.max(bytes.len()), 0);
    BuildResult { success, bytes }
}

#[cfg(test)]
mod tests {
    use super::*;
    #[test]
    fn name_validation_and_serialization_failures() {
        assert_eq!(u16_at(&[], usize::MAX), None);
        assert!(valid_rcode(15));
        assert!(!valid_rcode(16));
        assert!(!hostname(b"a?b"));
        let mut long = vec![b'a'; 64];
        long.push(b'.');
        long.push(b'a');
        assert!(!hostname(&long));
        let mut long_query = Vec::new();
        for _ in 0..4 {
            long_query.push(63);
            long_query.extend_from_slice(&[b'a'; 63]);
        }
        assert!(query_name(&long_query).is_none());
        for input in
            [b"".as_slice(), b".", b"A", b"a..b", b"a.-b", b"a.\xff", b"a.\0", b"-a", b"_a", b"a."]
        {
            let _ = hostname(input);
            let _ = wire_name(input, true);
            let _ = wire_name(input, false);
        }
        for n in [63, 64, 253, 254, 255, 256] {
            let _ = wire_name(&vec![b'a'; n], true);
            let _ = wire_name(&vec![b'a'; n], false);
        }
        for labels in [vec![63, 63, 63, 61], vec![63, 63, 63, 62]] {
            let name = labels.into_iter().map(|n| vec![b'a'; n]).collect::<Vec<_>>().join(&b'.');
            let _ = wire_name(&name, true);
        }
        for kind in [1, 28, 33, 65, 41, 5, 999] {
            for n in [0, 1, 3, 4, 6, 16] {
                let record = Record {
                    name: if kind == 41 { vec![] } else { b"a".to_vec() },
                    kind,
                    klass: 1,
                    ttl: 0,
                    data: vec![0; n],
                };
                let _ = build(1, false, 0, 1, 1, &[], true, &[record], 1, 0, true, false);
            }
        }
        let invalid = Record { name: b"a..b".to_vec(), kind: 5, klass: 1, ttl: 0, data: vec![] };
        assert!(!build(1, false, 0, 2, 1, &[], true, &[invalid], 0, 1, false, false).success);
        assert_eq!(record_size(b"name.", 1, 0, 7), 23);
    }
    #[test]
    fn query_matching_boundaries() {
        let mut packet = vec![0x12, 0x34, 0x80, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0];
        for name in [vec![0xc0, 0], vec![64], vec![1], vec![1, b'a'], vec![], vec![0]] {
            let _ = query_name(&name);
            let _ = init_parse(&packet, &[0], &name, 0x1234, 1, true);
        }
        assert!(!init_parse(&packet, &[1], &[0], 0x1234, 1, true).success);
        packet[5] = 2;
        assert!(!init_parse(&packet, &[0], &[0], 0x1234, 1, true).success);
        assert_eq!(header(&[], 0), 0);
    }
}
