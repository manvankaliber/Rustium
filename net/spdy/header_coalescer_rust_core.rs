// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#![forbid(unsafe_code)]

/// A value snapshot, not a foreign borrow. Flags use bytes for the C ABI.
#[derive(Clone, Copy, Debug)]
#[repr(C)]
pub struct State {
    pub header_list_size: usize,
    pub headers_valid: u8,
    pub error_seen: u8,
    pub regular_header_seen: u8,
}

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
#[repr(u8)]
pub enum Action {
    Ignore = 0,
    Append = 1,
    EmptyName = 2,
    PseudoAfterRegular = 3,
    InvalidName = 4,
    UppercaseName = 5,
    TooLarge = 6,
    InvalidValue = 7,
    ReleaseValid = 8,
    ReleaseInvalid = 9,
}

#[repr(C)]
pub struct Decision {
    pub state: State,
    pub action: Action,
    pub invalid_byte: u8,
}

#[derive(Clone, Copy)]
pub enum NameStart {
    Empty,
    PseudoAfterRegular,
    Pseudo,
    Regular,
}

pub fn name_start(first: Option<u8>, regular_header_seen: u8) -> NameStart {
    match first {
        None => NameStart::Empty,
        Some(b':') if regular_header_seen != 0 => NameStart::PseudoAfterRegular,
        Some(b':') => NameStart::Pseudo,
        Some(_) => NameStart::Regular,
    }
}

pub fn name_action(name: &[u8]) -> Action {
    if name.is_empty() {
        return Action::InvalidName;
    }
    let mut uppercase = false;
    for &c in name {
        // A later invalid token byte must still win over an uppercase byte.
        if !token_char(c) {
            return Action::InvalidName;
        }
        uppercase |= c.is_ascii_uppercase();
    }
    if uppercase {
        Action::UppercaseName
    } else {
        Action::Append
    }
}

pub fn next_size(size: usize, key_len: usize, value_len: usize) -> usize {
    size.wrapping_add(key_len).wrapping_add(value_len).wrapping_add(32)
}

pub fn size_action(size: usize, maximum: u32) -> Action {
    if size > maximum as usize {
        Action::TooLarge
    } else {
        Action::Append
    }
}

pub fn invalid_value(value: &[u8]) -> Option<u8> {
    value.iter().copied().find(|&c| (c < 32 && c != 9) || c == 127)
}

pub fn release_decision(headers_valid: u8) -> (Action, u8) {
    let action = if headers_valid != 0 { Action::ReleaseValid } else { Action::ReleaseInvalid };
    (action, 0)
}

impl State {
    fn decision(self, action: Action, invalid_byte: u8) -> Decision {
        Decision { state: self, action, invalid_byte }
    }

    fn fail(mut self, action: Action, invalid_byte: u8) -> Decision {
        self.error_seen = 1;
        self.decision(action, invalid_byte)
    }

    pub fn release(mut self) -> Decision {
        let (action, headers_valid) = release_decision(self.headers_valid);
        self.headers_valid = headers_valid;
        self.decision(action, 0)
    }

    pub fn header(mut self, key: &[u8], value: &[u8], maximum: u32) -> Decision {
        if self.error_seen != 0 {
            return self.decision(Action::Ignore, 0);
        }
        let name = match name_start(key.first().copied(), self.regular_header_seen) {
            NameStart::Empty => return self.fail(Action::EmptyName, 0),
            NameStart::PseudoAfterRegular => return self.fail(Action::PseudoAfterRegular, 0),
            NameStart::Pseudo => &key[1..],
            NameStart::Regular => {
                // This transition precedes name validation in the original.
                self.regular_header_seen = 1;
                key
            }
        };
        let action = name_action(name);
        if action != Action::Append {
            return self.fail(action, 0);
        }
        // C++ size_t arithmetic wraps, even with debug assertions enabled.
        // Preserve overflow behavior and the increment on size/value failure.
        self.header_list_size = next_size(self.header_list_size, key.len(), value.len());
        let action = size_action(self.header_list_size, maximum);
        if action != Action::Append {
            return self.fail(action, 0);
        }
        if let Some(byte) = invalid_value(value) {
            return self.fail(Action::InvalidValue, byte);
        }
        self.decision(Action::Append, 0)
    }
}

fn token_char(c: u8) -> bool {
    c.is_ascii_alphanumeric() || b"!#$%&'*+-.^_`|~".contains(&c)
}

#[cfg(test)]
mod tests {
    use super::*;
    fn state() -> State {
        State { header_list_size: 0, headers_valid: 1, error_seen: 0, regular_header_seen: 0 }
    }
    #[test]
    fn overflow_and_precedence() {
        let mut s = state();
        s.header_list_size = usize::MAX - 34;
        let d = s.header(b"a", b"bc", 0);
        assert_eq!(d.action, Action::Append);
        assert_eq!(d.state.header_list_size, 0);
        assert_eq!(state().header(b"A?", b"\n", 0).action, Action::InvalidName);
        assert_eq!(state().header(b"A", b"\n", 0).action, Action::UppercaseName);
        assert_eq!(state().header(b"a", b"\n", 0).action, Action::TooLarge);
        let invalid_value = state().header(b"a", b"\r\n", 35);
        assert_eq!(invalid_value.invalid_byte, 13);
        assert_eq!(invalid_value.state.header_list_size, 35);
        assert_eq!(state().header(b"a", b"", 32).state.header_list_size, 33);
    }
    #[test]
    fn transitions_and_release() {
        let d = state().header(b"?", b"", 100);
        assert_eq!(d.state.regular_header_seen, 1);
        assert_eq!(d.state.header_list_size, 0);
        assert_eq!(d.state.header(b"", b"", 100).action, Action::Ignore);
        let s = state().release().state;
        assert_eq!(s.release().action, Action::ReleaseInvalid);
        assert_eq!(s.header(b"a", b"", 100).action, Action::Append);
    }
    #[test]
    fn every_byte() {
        for c in 0..=255u8 {
            let expected = c > 32 && c < 127 && !b"()<>@,;:\\\"/[]?={}".contains(&c);
            assert_eq!(token_char(c), expected);
            let d = state().header(b"a", &[c], 34);
            assert_eq!(d.action == Action::Append, c == 9 || (c >= 32 && c != 127));
        }
    }
}
