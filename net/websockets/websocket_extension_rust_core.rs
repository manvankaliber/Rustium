// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#![forbid(unsafe_code)]

use std::borrow::Cow;

#[derive(Clone, Copy, PartialEq, Eq)]
pub struct Parameter<'a> {
    pub name: &'a [u8],
    pub value: &'a [u8],
}

pub struct Extension<'a> {
    pub name: &'a [u8],
    pub parameters: Vec<(&'a [u8], Cow<'a, [u8]>)>,
}

pub fn token_char(c: u8) -> bool {
    matches!(c, b'!' | b'#'..=b'\'' | b'*' | b'+' | b'-' | b'.' | b'0'..=b'9' | b'A'..=b'Z' | b'^'..=b'z' | b'|' | b'~')
}

pub fn token(s: &[u8]) -> bool {
    !s.is_empty() && s.iter().copied().all(token_char)
}

struct Parser<'a> {
    data: &'a [u8],
    pos: usize,
}
impl<'a> Parser<'a> {
    fn spaces(&mut self) {
        while self.data.get(self.pos).is_some_and(|c| matches!(c, b' ' | b'\t')) {
            self.pos += 1;
        }
    }
    fn consume(&mut self, c: u8) -> bool {
        let saved = self.pos;
        self.spaces();
        if self.data.get(self.pos) == Some(&c) {
            self.pos += 1;
            true
        } else {
            self.pos = saved;
            false
        }
    }
    fn token(&mut self) -> Option<&'a [u8]> {
        self.spaces();
        let start = self.pos;
        while self.data.get(self.pos).is_some_and(|&c| token_char(c)) {
            self.pos += 1;
        }
        (self.pos > start).then_some(&self.data[start..self.pos])
    }
    fn value(&mut self) -> Option<Cow<'a, [u8]>> {
        if !self.consume(b'"') {
            return self.token().map(Cow::Borrowed);
        }
        let start = self.pos;
        let mut decoded: Option<Vec<u8>> = None;
        loop {
            let mut c = *self.data.get(self.pos)?;
            if c == b'"' {
                let end = self.pos;
                self.pos += 1;
                let value =
                    decoded.map_or_else(|| Cow::Borrowed(&self.data[start..end]), Cow::Owned);
                return (!value.is_empty()).then_some(value);
            }
            if c == b'\\' {
                decoded.get_or_insert_with(|| self.data[start..self.pos].to_vec());
                self.pos += 1;
                c = *self.data.get(self.pos)?;
            }
            if !token_char(c) {
                return None;
            }
            if let Some(out) = &mut decoded {
                out.push(c);
            }
            self.pos += 1;
        }
    }
}

// Commit no results until the complete header has passed the grammar.
pub fn parse(data: &[u8]) -> Option<Vec<Extension<'_>>> {
    let mut p = Parser { data, pos: 0 };
    let mut extensions = Vec::new();
    loop {
        let name = p.token()?;
        let mut parameters = Vec::new();
        while p.consume(b';') {
            let key = p.token()?;
            let value = if p.consume(b'=') { p.value()? } else { Cow::Borrowed(&[][..]) };
            parameters.push((key, value));
        }
        extensions.push(Extension { name, parameters });
        p.spaces();
        if !p.consume(b',') {
            return (p.pos == data.len()).then_some(extensions);
        }
    }
}

pub fn equivalent(name: &[u8], a: &[Parameter<'_>], other: &[u8], b: &[Parameter<'_>]) -> bool {
    if name != other || a.len() != b.len() {
        return false;
    }
    if a == b {
        return true;
    }
    // Sort references rather than owned strings; stable ordering preserves the
    // sequence of values for duplicate keys, including absent values.
    let mut a: Vec<_> = a.iter().collect();
    let mut b: Vec<_> = b.iter().collect();
    a.sort_by_key(|p| p.name);
    b.sort_by_key(|p| p.name);
    a == b
}

pub fn serialize(name: &[u8], parameters: &[Parameter<'_>]) -> Vec<u8> {
    if name.is_empty() {
        return Vec::new();
    }
    let mut out = Vec::with_capacity(
        name.len()
            + parameters
                .iter()
                .map(|p| 2 + p.name.len() + usize::from(!p.value.is_empty()) + p.value.len())
                .sum::<usize>(),
    );
    out.extend_from_slice(name);
    for p in parameters {
        out.extend_from_slice(b"; ");
        out.extend_from_slice(p.name);
        if !p.value.is_empty() {
            out.push(b'=');
            out.extend_from_slice(p.value);
        }
    }
    out
}
