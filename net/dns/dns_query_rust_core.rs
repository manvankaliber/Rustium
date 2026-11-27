// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

//! DnsQuery policy. No IOBuffer, feature registry, or OPT object ownership.
#![forbid(unsafe_code)]

use std::mem::MaybeUninit;

pub const HEADER_SIZE: usize = 12;
const OPT_FIXED_SIZE: usize = 11;
const OPTION_HEADER_SIZE: usize = 4;
const EDE: u16 = 15;
const PADDING: u16 = 12;

pub trait OptServices {
    fn clone_or_create(&mut self, clone_input: bool) -> bool;
    fn contains(&self, code: u16) -> bool;
    fn add_ede(&mut self);
    fn add_padding(&mut self, len: u16);
    fn len(&self) -> usize;
    fn count(&self) -> usize;
}

pub fn question_size(name_size: usize) -> usize {
    name_size + 4
}

// Same debug precondition as NetworkToDottedName: label <=63, total label
// octets <=255 (excluding root), nonempty dotted result, ignore root suffix.
pub fn constructor_name_valid(name: &[u8]) -> bool {
    let mut pos = 0;
    let mut nonempty = false;
    while let Some(&len) = name.get(pos) {
        pos += 1;
        if len == 0 {
            return nonempty;
        }
        if len > 63 {
            return false;
        }
        pos += usize::from(len);
        if pos > name.len() || pos > 255 {
            return false;
        }
        nonempty = true;
    }
    false
}

// All selection and sequencing belongs here; services only operate on the
// unchanged OptRecordRdata objects. In particular an empty *present* OPT still
// selects a record (and triggers the original debug OptCount precondition).
pub fn prepare_opt(
    name: &[u8],
    has_input: bool,
    padded: bool,
    structured_errors: bool,
    services: &mut impl OptServices,
) -> (usize, bool) {
    debug_assert!(constructor_name_valid(name));
    let no_opt_size = HEADER_SIZE + question_size(name.len());
    if !has_input && !padded && !structured_errors {
        return (no_opt_size, false);
    }
    // A failed clone was already a DCHECK followed by a null dereference in
    // C++. Fail closed instead of treating a service failure as absent OPT.
    assert!(services.clone_or_create(has_input), "invalid OPT object");
    if structured_errors && !services.contains(EDE) {
        services.add_ede();
    }
    let unpadded = no_opt_size + OPT_FIXED_SIZE + services.len();
    let padding_size = if padded {
        OPTION_HEADER_SIZE + (128 - (unpadded + OPTION_HEADER_SIZE) % 128) % 128
    } else {
        0
    };
    if padding_size > 0 {
        debug_assert!(!services.contains(PADDING));
        services.add_padding((padding_size - OPTION_HEADER_SIZE) as u16);
    }
    debug_assert_ne!(services.count(), 0);
    (no_opt_size + OPT_FIXED_SIZE + services.len(), true)
}

fn put(out: &mut [MaybeUninit<u8>], bytes: &[u8]) {
    for (slot, byte) in out.iter_mut().zip(bytes) {
        slot.write(*byte);
    }
}

// Destination may be uninitialized. Every byte of the complete query is
// initialized here, including the header's reserved fields and OPT TTL.
pub fn write_query(
    out: &mut [MaybeUninit<u8>],
    id: u16,
    name: &[u8],
    qtype: u16,
    has_opt: bool,
    opt: &[u8],
) {
    let question_end = HEADER_SIZE + question_size(name.len());
    assert_eq!(out.len(), question_end + if has_opt { OPT_FIXED_SIZE + opt.len() } else { 0 });
    put(&mut out[..HEADER_SIZE], &[0; HEADER_SIZE]);
    put(&mut out[..2], &id.to_be_bytes());
    put(&mut out[2..4], &0x0100_u16.to_be_bytes());
    put(&mut out[4..6], &1_u16.to_be_bytes());
    put(&mut out[HEADER_SIZE..HEADER_SIZE + name.len()], name);
    put(&mut out[question_end - 4..question_end - 2], &qtype.to_be_bytes());
    put(&mut out[question_end - 2..question_end], &1_u16.to_be_bytes());
    if has_opt {
        put(&mut out[10..12], &1_u16.to_be_bytes());
        let record = &mut out[question_end..];
        put(&mut record[..9], &[0, 0, 41, 16, 0, 0, 0, 0, 0]);
        // SpanWriter's original U16 conversion truncates large RDATA sizes.
        put(&mut record[9..11], &(opt.len() as u16).to_be_bytes());
        put(&mut record[11..], opt);
    }
}

#[derive(Debug, PartialEq)]
pub enum ParseResult {
    Invalid,
    QuestionCount,
    Name(usize),
}

pub fn parse(bytes: &[u8]) -> ParseResult {
    if bytes.len() < HEADER_SIZE || bytes[2] & 0x80 != 0 {
        return ParseResult::Invalid;
    }
    if bytes[4..6] != [0, 1] {
        return ParseResult::QuestionCount;
    }
    let mut pos = HEADER_SIZE;
    loop {
        let Some(&len) = bytes.get(pos) else {
            return ParseResult::Invalid;
        };
        pos += 1;
        if len == 0 {
            break;
        }
        // Preserve ReadName's deliberately different policy: it accepts
        // length bytes 64..254, even pointer-looking bytes, if enough data
        // remains. The root is excluded from the 255-byte limit.
        if pos - HEADER_SIZE + usize::from(len) > 255 {
            return ParseResult::Invalid;
        }
        pos += usize::from(len);
        if pos > bytes.len() {
            return ParseResult::Invalid;
        }
    }
    if bytes.len() - pos < 4 || bytes[pos + 2..pos + 4] != [0, 1] {
        return ParseResult::Invalid;
    }
    ParseResult::Name(pos - HEADER_SIZE)
}

pub fn read_u16(bytes: &[u8]) -> u16 {
    u16::from_be_bytes([bytes[0], bytes[1]])
}

pub fn set_flags(out: &mut [MaybeUninit<u8>], flags: u16) {
    // This public API historically assigns directly to the packed header.
    // It accepts a native representation, unlike the constructor and ID.
    put(out, &flags.to_ne_bytes());
}

pub fn clone_id(out: &mut [MaybeUninit<u8>], id: u16) {
    put(out, &id.to_be_bytes());
}

pub fn copy_size(allocation_size: usize) -> usize {
    // Include the entire backing allocation, even trailing/unknown records
    // and uninitialized capacity beyond a previously parsed valid prefix.
    allocation_size
}
