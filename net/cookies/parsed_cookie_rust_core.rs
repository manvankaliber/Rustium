// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//! ParsedCookie policy. Byte slices deliberately do not require UTF-8.
#![forbid(unsafe_code)]

pub const KEYS: [&[u8]; 9] = [
    b"path",
    b"domain",
    b"expires",
    b"max-age",
    b"secure",
    b"httponly",
    b"samesite",
    b"priority",
    b"partitioned",
];
pub const NO_NAMELESS: usize = 4;

#[repr(C)]
#[derive(Default)]
pub struct Scan {
    pub next: usize,
    pub begin: usize,
    pub len: usize,
    pub found: usize,
}

#[repr(C)]
#[derive(Default)]
pub struct Pair {
    // 0: done, 1: keep, 2: overlong attribute (warn), 3: invalid (clear).
    pub action: usize,
    pub next: usize,
    pub name_begin: usize,
    pub name_len: usize,
    pub value_begin: usize,
    pub value_len: usize,
    pub attribute: usize,
    pub reason: usize,
    pub internal_htab: usize,
    pub nameless: usize,
}

#[repr(C)]
pub struct Mutation {
    // 0: reject, 1/2: name/value, 3: clear attribute, 4: set attribute.
    pub action: usize,
    pub begin: usize,
    pub len: usize,
}

#[repr(C)]
pub struct SameSite {
    pub value: i32,
    pub metric: i32,
}

fn whitespace(c: u8) -> bool {
    c == b' ' || c == b'\t'
}

pub fn terminator(input: &[u8]) -> usize {
    input.iter().position(|c| matches!(c, 0 | b'\r' | b'\n')).unwrap_or(input.len())
}

// The raw scanners do not treat CR/LF/NUL as terminators: their supplied end
// is authoritative. Only the String helpers and constructor truncate/reject.
pub fn scan(input: &[u8], token: bool) -> Scan {
    let begin = input.iter().position(|c| !whitespace(*c)).unwrap_or(input.len());
    if token && begin == input.len() {
        return Scan { next: begin, ..Scan::default() };
    }
    let next = begin
        + input[begin..]
            .iter()
            .position(|c| *c == b';' || (token && *c == b'='))
            .unwrap_or(input.len() - begin);
    let mut end = next;
    while end > begin && whitespace(input[end - 1]) {
        end -= 1;
    }
    Scan { next, begin, len: end - begin, found: 1 }
}

// Public raw scanner pointer arguments may identify the same C++ iterator
// object, including the const end reference. Simulate the original assignment
// order with safe numeric slots. All byte access remains inside the original
// input span; aliasing end can shorten the active scan boundary.
pub fn scan_aliased(input: &[u8], token: bool, aliases: usize) -> Scan {
    if aliases == 0 {
        return scan(input, token);
    }
    let start = if aliases & 1 != 0 { 0 } else { 1 };
    let finish = if aliases & 2 != 0 {
        0
    } else if aliases & 4 != 0 {
        start
    } else {
        2
    };
    let end = if aliases & 8 != 0 {
        0
    } else if aliases & 16 != 0 {
        start
    } else if aliases & 32 != 0 {
        finish
    } else {
        3
    };
    let mut slots = [0, input.len(), input.len(), input.len()];
    while slots[0] != slots[end] && whitespace(input[slots[0]]) {
        slots[0] += 1;
    }
    if token && slots[0] == slots[end] {
        return Scan { next: slots[0], ..Scan::default() };
    }
    slots[start] = slots[0];
    while slots[0] != slots[end] && input[slots[0]] != b';' && (!token || input[slots[0]] != b'=') {
        slots[0] += 1;
    }
    if token {
        let real_end = slots[0];
        if slots[0] != slots[start] {
            slots[0] -= 1;
            while slots[0] != slots[start] && whitespace(input[slots[0]]) {
                slots[0] -= 1;
            }
            slots[0] += 1;
        }
        slots[finish] = slots[0];
        slots[0] = real_end;
    } else {
        slots[finish] = slots[0];
        if slots[finish] != slots[start] {
            slots[finish] -= 1;
            while slots[finish] != slots[start] && whitespace(input[slots[finish]]) {
                slots[finish] -= 1;
            }
            slots[finish] += 1;
        }
    }
    Scan { next: slots[0], begin: slots[start], len: slots[finish] - slots[start], found: 1 }
}

pub fn string_scan(input: &[u8], token: bool) -> Scan {
    scan(&input[..terminator(input)], token)
}

pub fn valid_chars(input: &[u8], name: bool) -> bool {
    input.iter().all(|c| *c >= 0x20 && *c != 0x7f && *c != b';' && (!name || *c != b'='))
}

// Status is a private bridge code, not a CookieInclusionStatus enum value.
// The order of failures is observable and must not be combined/reordered.
pub fn valid_pair(name: &[u8], value: &[u8]) -> usize {
    if name.is_empty() && value.is_empty() {
        return 1;
    }
    if name.len().checked_add(value.len()).is_none_or(|len| len > 4096) {
        return 2;
    }
    if !valid_chars(name, true) || !valid_chars(value, false) {
        return 3;
    }
    0
}

pub fn attribute(name: &[u8]) -> usize {
    KEYS.iter().position(|key| name.eq_ignore_ascii_case(key)).map_or(0, |i| i + 1)
}

pub fn lower(input: &mut [u8]) {
    input.make_ascii_lowercase();
}

pub fn next_pair(input: &[u8], position: usize, first: bool) -> Pair {
    let mut result = Pair { next: input.len(), nameless: NO_NAMELESS, ..Pair::default() };
    if first {
        let reason = if terminator(input) != input.len() {
            3
        } else if input.is_empty() {
            1
        } else {
            0
        };
        if reason != 0 {
            result.action = 3;
            result.reason = reason;
            return result;
        }
    }
    if position == input.len() {
        return result;
    }
    let token = scan(&input[position..], true);
    if token.found == 0 && !first {
        return result;
    }
    let mut it = position + token.next;
    let mut name_begin = position + token.begin;
    let mut name_len = token.len;
    let separator = input.get(it) == Some(&b'=');
    if !separator && first {
        it = if token.found == 0 { 0 } else { name_begin };
        name_begin = 0;
        name_len = 0;
    } else if separator {
        it += 1;
    }
    let value = scan(&input[it..], false);
    result.name_begin = name_begin;
    result.name_len = name_len;
    result.value_begin = it + value.begin;
    result.value_len = value.len;
    result.next = (it + value.next).saturating_add(1).min(input.len());
    let name = &input[name_begin..name_begin + name_len];
    let value = &input[result.value_begin..result.value_begin + result.value_len];
    result.action = 1;
    if first {
        result.internal_htab = usize::from(name.contains(&b'\t') || value.contains(&b'\t'));
        result.reason = valid_pair(name, value);
        if result.reason == 0 && name.is_empty() {
            result.nameless = if separator {
                if value.contains(&b'=') {
                    3
                } else {
                    2
                }
            } else {
                usize::from(attribute(value) != 0)
            };
        }
    } else {
        if !valid_chars(name, true) || !valid_chars(value, false) {
            result.reason = 3;
        } else if value.len() > 1024 {
            result.action = 2;
        }
        result.attribute = attribute(name);
    }
    if result.reason != 0 {
        result.action = 3;
    }
    result
}

// All mutation validation and decisions happen before the C++ storage adapter
// writes anything. Borrowed inputs may alias any current name/value/attribute.
pub fn mutation(field: usize, input: &[u8], name: &[u8], value: &[u8], enabled: bool) -> Mutation {
    let reject = Mutation { action: 0, begin: 0, len: 0 };
    if field < 2 {
        if !valid_chars(input, field == 0) {
            return reject;
        }
        let parsed = string_scan(input, field == 0);
        let bytes = &input[parsed.begin..parsed.begin + parsed.len];
        let reason = if field == 0 { valid_pair(bytes, value) } else { valid_pair(name, bytes) };
        if reason != 0 {
            return reject;
        }
        return Mutation { action: field + 1, begin: parsed.begin, len: parsed.len };
    }
    let boolean = matches!(field, 6 | 7 | 10);
    let parsed = if boolean {
        Scan::default()
    } else {
        if !valid_chars(input, false) {
            return reject;
        }
        let parsed = string_scan(input, false);
        if parsed.len > 1024 {
            return reject;
        }
        parsed
    };
    if (boolean && !enabled) || (!boolean && parsed.len == 0) {
        return Mutation { action: 3, begin: 0, len: 0 };
    }
    // A valid object always has a nonempty name or value. Setter-created
    // objects retain invalid-constructor metrics, matching the original
    // implementation.
    if name.is_empty() && value.is_empty() {
        return reject;
    }
    Mutation { action: 4, begin: parsed.begin, len: parsed.len }
}

// Remove only the selected final duplicate. Earlier duplicates remain stored
// but do not become selected again. This intentionally is not SetupAttributes.
pub fn clear_indices(indices: &mut [usize], removed: usize) {
    if removed == 0 {
        return;
    }
    for index in indices {
        if *index == removed {
            *index = 0;
        } else if *index > removed {
            *index -= 1;
        }
    }
}

pub fn emit_value(name: &[u8], first: bool) -> bool {
    first || !matches!(name, b"secure" | b"httponly" | b"partitioned")
}

pub fn serialization_flags(name: &[u8], first: bool, output_empty: bool) -> usize {
    usize::from(emit_value(name, first)) | (usize::from(!output_empty) << 1)
}

pub fn same_site(input: &[u8], present: bool) -> SameSite {
    let (value, metric) = if !present {
        (-1, 0)
    } else if input.eq_ignore_ascii_case(b"none") {
        (0, 5)
    } else if input.eq_ignore_ascii_case(b"lax") {
        (1, 3)
    } else if input.eq_ignore_ascii_case(b"strict") {
        (2, 4)
    } else if input.eq_ignore_ascii_case(b"extended") {
        (-1, 6)
    } else if input.is_empty() {
        (-1, 1)
    } else {
        (-1, 2)
    };
    SameSite { value, metric }
}

pub fn priority(input: &[u8]) -> i32 {
    if input.eq_ignore_ascii_case(b"high") {
        2
    } else if input.eq_ignore_ascii_case(b"low") {
        0
    } else {
        1
    }
}
