// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

//! Incremental gzip-header parsing with the legacy Chromium acceptance rules.
//! No payload decoding, allocations, or references retained between calls.

#![forbid(unsafe_code)]

const EXTRA: u8 = 0x04;
const NAME: u8 = 0x08;
const COMMENT: u8 = 0x10;
const CRC: u8 = 0x02;
const OPTIONAL: u8 = EXTRA | NAME | COMMENT | CRC;

#[derive(Clone, Copy, Debug, Default, PartialEq, Eq)]
#[repr(C)]
pub struct State {
    pub phase: i32,
    pub flags: u8,
    pub extra_length: u16,
}

// Keep the FFI layout explicit and checked on both sides of the boundary.
const _: () = {
    assert!(std::mem::size_of::<State>() == 8);
    assert!(std::mem::align_of::<State>() == 4);
    assert!(std::mem::offset_of!(State, flags) == 4);
    assert!(std::mem::offset_of!(State, extra_length) == 6);
};

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
#[repr(u8)]
pub enum Status {
    Incomplete = 0,
    Complete = 1,
    Invalid = 2,
}

impl State {
    // Keep this inside the bridge so the small local state is optimized away.
    #[inline]
    pub fn read_more(&mut self, input: &[u8], header_end: &mut usize) -> Status {
        let mut pos = 0;
        // Network streams can deliver the fixed prefix one byte at a time.
        // Handle that bounded transition without a general-loop round trip.
        if input.len() == 1 && (0..=9).contains(&self.phase) {
            if self.phase < 3 {
                if input[0] != [0x1f, 0x8b, 8][self.phase as usize] {
                    return Status::Invalid;
                }
            } else if self.phase == 3 {
                self.flags = input[0] & OPTIONAL;
            }
            self.phase += 1;
            return if self.phase == 10 && self.flags == 0 {
                *header_end = 1;
                Status::Complete
            } else {
                Status::Incomplete
            };
        }
        // Common full-buffer case: validate the fixed prefix without dispatching
        // the state machine once per byte. Preserve progress on each error.
        if self.phase == 0 && input.len() >= 4 {
            if input[0] != 0x1f {
                return Status::Invalid;
            }
            self.phase = 1;
            if input[1] != 0x8b {
                return Status::Invalid;
            }
            self.phase = 2;
            if input[2] != 8 {
                return Status::Invalid;
            }
            self.flags = input[3] & OPTIONAL;
            self.phase = 4;
            pos = 4;
        }

        while pos < input.len() {
            match self.phase {
                0..=2 => {
                    let expected = [0x1f, 0x8b, 8][self.phase as usize];
                    if input[pos] != expected {
                        return Status::Invalid;
                    }
                    pos += 1;
                    self.phase += 1;
                }
                3 => {
                    // FTEXT and reserved bits were ignored by the C++ reader.
                    // Do not turn this migration into stricter format checking.
                    self.flags = input[pos] & OPTIONAL;
                    pos += 1;
                    self.phase = 4;
                }
                4..=9 => {
                    // MTIME, XFL, and OS are opaque. Skip only bytes present in
                    // this chunk; phase records progress across split headers.
                    let count = (10 - self.phase as usize).min(input.len() - pos);
                    self.phase += count as i32;
                    pos += count;
                }
                10 => {
                    if self.flags & EXTRA == 0 {
                        self.phase = 13;
                    } else {
                        self.extra_length = input[pos] as u16;
                        pos += 1;
                        self.phase = 11;
                    }
                }
                11 => {
                    // The previous phase stored only the low byte.
                    self.extra_length |= (input[pos] as u16) << 8;
                    pos += 1;
                    self.phase = 12;
                    // C++ intentionally falls through here, even at the end
                    // of a chunk: zero-length FEXTRA must finish immediately.
                    self.skip_extra(input.len(), &mut pos);
                }
                12 => self.skip_extra(input.len(), &mut pos),
                13 | 14 => {
                    let flag = if self.phase == 13 { NAME } else { COMMENT };
                    if self.flags & flag == 0 {
                        self.phase += 1;
                    } else {
                        let rest = &input[pos..];
                        let end = if rest.len() < 16 {
                            rest.iter().position(|&byte| byte == 0)
                        } else {
                            memchr::memchr(0, rest)
                        };
                        if let Some(end) = end {
                            pos += end + 1;
                            self.flags &= !flag;
                            self.phase += 1;
                        } else {
                            pos = input.len();
                        }
                    }
                }
                15 => {
                    if self.flags & CRC == 0 {
                        self.phase = 17;
                    } else {
                        pos += 1;
                        self.phase = 16;
                    }
                }
                16 => {
                    // The old parser skipped FHCRC without validating it.
                    pos += 1;
                    self.flags &= !CRC;
                    self.phase = 17;
                }
                17 => {
                    *header_end = pos;
                    return Status::Complete;
                }
                // The public C++ API never constructs other states. Still
                // reject invalid bridge metadata instead of looping forever.
                _ => return Status::Invalid,
            }
        }

        // Match completion at the exact chunk boundary, including the legacy
        // state left after a fixed header with no optional fields. Empty calls
        // must retain the same completion behavior.
        if self.phase > 9 && self.flags == 0 {
            *header_end = pos;
            Status::Complete
        } else {
            Status::Incomplete
        }
    }

    #[inline]
    fn skip_extra(&mut self, input_len: usize, pos: &mut usize) {
        let count = usize::from(self.extra_length).min(input_len - *pos);
        *pos += count;
        self.extra_length -= count as u16;
        if self.extra_length == 0 {
            self.flags &= !EXTRA;
            self.phase = 13;
        }
    }
}

pub fn has_header(input: &[u8]) -> bool {
    State::default().read_more(input, &mut 0) == Status::Complete
}
