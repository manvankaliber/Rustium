// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//! Complete byte-range value policy, directly usable by a Rust Range parser.
#![forbid(unsafe_code)]

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
#[repr(C)]
pub struct State {
    pub first: i64,
    pub last: i64,
    pub suffix: i64,
    pub computed: bool,
}
impl Default for State {
    fn default() -> Self {
        Self::new()
    }
}
impl State {
    pub const fn new() -> Self {
        Self { first: -1, last: -1, suffix: -1, computed: false }
    }
    pub const fn bounded(first: i64, last: i64) -> Self {
        Self { first, last, ..Self::new() }
    }
    pub const fn right_unbounded(first: i64) -> Self {
        Self::bounded(first, -1)
    }
    pub const fn suffix(suffix: i64) -> Self {
        Self { suffix, ..Self::new() }
    }
    pub const fn is_suffix(&self) -> bool {
        self.suffix != -1
    }
    pub const fn has_first(&self) -> bool {
        self.first != -1
    }
    pub const fn has_last(&self) -> bool {
        self.last != -1
    }
    pub const fn is_valid(&self) -> bool {
        self.suffix > 0 || (self.first >= 0 && (self.last == -1 || self.last >= self.first))
    }
    /// Negative size and repeated calls do not mutate. All other attempts latch
    /// computed, including invalid/unsatisfiable ranges. Setters do not reset
    /// it. Original C++ overflows for some contradictory negative suffix
    /// states; those have no defined C++ result. Rust deterministically wraps
    /// that one subtraction, preserving every defined original result without
    /// panicking.
    pub fn compute_bounds(&mut self, size: i64) -> bool {
        if size < 0 || self.computed {
            return false;
        }
        self.computed = true;
        if !self.has_first() && !self.has_last() && !self.is_suffix() {
            self.first = 0;
            self.last = size - 1;
            return true;
        }
        if !self.is_valid() {
            return false;
        }
        if self.is_suffix() {
            self.first = size.wrapping_sub(size.min(self.suffix));
            self.last = size - 1;
            return true;
        }
        if self.first < size {
            self.last = if self.has_last() { (size - 1).min(self.last) } else { size - 1 };
            return true;
        }
        false
    }
    /// Formatting is total; C++ retains its original validity DCHECK contract.
    /// Decimal output is stack-only, including i64::MIN via unsigned_abs().
    pub fn header(&self) -> Header {
        let mut out = Header { bytes: [0; 48], len: 0 };
        out.append(b"bytes=");
        if self.is_suffix() {
            out.append(b"-");
            out.number(self.suffix);
        } else {
            out.number(self.first);
            out.append(b"-");
            if self.has_last() {
                out.number(self.last);
            }
        }
        out
    }
}
#[derive(Clone, Copy)]
#[repr(C)]
pub struct Header {
    pub bytes: [u8; 48],
    pub len: u8,
}
impl Header {
    pub fn as_bytes(&self) -> &[u8] {
        &self.bytes[..usize::from(self.len)]
    }
    fn append(&mut self, text: &[u8]) {
        let start = usize::from(self.len);
        self.bytes[start..start + text.len()].copy_from_slice(text);
        self.len += text.len() as u8;
    }
    fn number(&mut self, value: i64) {
        if value < 0 {
            self.append(b"-");
        }
        let mut magnitude = value.unsigned_abs();
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
        self.append(&digits[start..]);
    }
}
