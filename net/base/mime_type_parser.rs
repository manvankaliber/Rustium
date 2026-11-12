// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

//! Byte-preserving MIME grammar. Plans borrow input by offset; only transformed
//! output bytes are staged. Caller state is inspected lazily before any writes.
#![forbid(unsafe_code)]

#[derive(Clone, Copy, Debug, Default, PartialEq, Eq)]
pub struct Span {
    pub start: usize,
    pub end: usize,
    pub scratch: bool,
}

impl Span {
    pub fn bytes<'a>(self, input: &'a [u8], scratch: &'a [u8]) -> &'a [u8] {
        let source = if self.scratch { scratch } else { input };
        &source[self.start..self.end]
    }
}

fn lws(b: u8) -> bool {
    matches!(b, b' ' | b'\t')
}

fn trim(mut span: Span, source: &[u8]) -> Span {
    while span.start < span.end && lws(source[span.start]) {
        span.start += 1;
    }
    while span.end > span.start && lws(source[span.end - 1]) {
        span.end -= 1;
    }
    span
}

fn mime_range(input: &[u8]) -> Option<Span> {
    let mut start = 0;
    while start < input.len() && lws(input[start]) {
        start += 1;
    }
    let mut end = start;
    let mut slash = false;
    while end < input.len() && !lws(input[end]) && !matches!(input[end], b';' | b'(') {
        slash |= input[end] == b'/';
        end += 1;
    }
    slash.then_some(Span { start, end, scratch: false })
}

#[derive(Clone, Copy)]
struct Value {
    span: Span,
    escaped: bool,
}

struct Parameters<'a> {
    input: &'a [u8],
    offset: usize,
}

impl Iterator for Parameters<'_> {
    type Item = (Span, Value);

    fn next(&mut self) -> Option<Self::Item> {
        let input = self.input;
        while let Some(semicolon) = input[self.offset..].iter().position(|&b| b == b';') {
            self.offset += semicolon + 1;
            while self.offset < input.len() && lws(input[self.offset]) {
                self.offset += 1;
            }
            let name_start = self.offset;
            while self.offset < input.len() && !matches!(input[self.offset], b';' | b'=') {
                self.offset += 1;
            }
            if self.offset == input.len() || input[self.offset] == b';' {
                continue;
            }
            let name = Span { start: name_start, end: self.offset, scratch: false };
            self.offset += 1;
            while self.offset < input.len() && lws(input[self.offset]) {
                self.offset += 1;
            }
            if self.offset == input.len() || input[self.offset] == b';' {
                continue;
            }
            let quoted = input[self.offset] == b'"';
            if quoted {
                self.offset += 1;
            }
            let start = self.offset;
            let mut escaped = false;
            if quoted {
                while self.offset < input.len() && input[self.offset] != b'"' {
                    if input[self.offset] == b'\\' && self.offset + 1 < input.len() {
                        escaped = true;
                        self.offset += 1;
                    }
                    self.offset += 1;
                }
            } else {
                while self.offset < input.len() && input[self.offset] != b';' {
                    self.offset += 1;
                }
            }
            let mut span = Span { start, end: self.offset, scratch: false };
            if !quoted {
                while span.end > span.start && lws(input[span.end - 1]) {
                    span.end -= 1;
                }
            }
            // The next search consumes a close quote and any post-quote junk.
            return Some((name, Value { span, escaped }));
        }
        None
    }
}

fn output_value(
    input: &[u8],
    value: Value,
    scratch: &mut Vec<u8>,
    trim_lws: bool,
    lower: bool,
) -> Span {
    let mut span = value.span;
    if value.escaped {
        let start = scratch.len();
        scratch.reserve(span.end - span.start);
        let mut offset = span.start;
        while offset < span.end {
            if input[offset] == b'\\' && offset + 1 < span.end {
                offset += 1;
            }
            let byte = input[offset];
            scratch.push(if lower { byte.to_ascii_lowercase() } else { byte });
            offset += 1;
        }
        span = Span { start, end: scratch.len(), scratch: true };
        if trim_lws {
            span = trim(span, scratch);
        }
        return span;
    }
    if trim_lws {
        span = trim(span, input);
    }
    let bytes = &input[span.start..span.end];
    if lower && bytes.iter().any(u8::is_ascii_uppercase) {
        let start = scratch.len();
        scratch.extend(bytes.iter().map(u8::to_ascii_lowercase));
        span = Span { start, end: scratch.len(), scratch: true };
    }
    span
}

#[derive(Default)]
pub struct MimePlan {
    pub mime: Span,
    pub scratch: Vec<u8>,
}

// The bridge collects parameter descriptors directly into its final vector.
// Names and ordinary values have no owned Rust byte allocation.
pub fn parse(
    input: &[u8],
    want_parameters: bool,
    mut parameter: impl FnMut(Span, Span),
) -> Option<MimePlan> {
    let mime = mime_range(input)?;
    let mut result = MimePlan { mime, scratch: Vec::new() };
    if want_parameters {
        for (name, value) in (Parameters { input, offset: mime.end }) {
            let value = output_value(input, value, &mut result.scratch, false, false);
            parameter(name, value);
        }
    }
    Some(result)
}

fn ascii_whitespace(b: u8) -> bool {
    matches!(b, b' ' | b'\t' | b'\n' | b'\x0b' | b'\x0c' | b'\r')
}

fn token(input: &[u8]) -> bool {
    !input.is_empty()
        && input.iter().all(|&b| b > 0x20 && b < 0x7f && !b"()<>@,;:\\\"/[]?={}".contains(&b))
}

pub fn without_parameters(input: &[u8]) -> Option<(Span, Span)> {
    let slash = input.iter().position(|&b| b == b'/')?;
    let mut top = Span { start: 0, end: slash, scratch: false };
    let mut sub = Span { start: slash + 1, end: input.len(), scratch: false };
    while top.start < top.end && ascii_whitespace(input[top.start]) {
        top.start += 1;
    }
    while sub.end > sub.start && ascii_whitespace(input[sub.end - 1]) {
        sub.end -= 1;
    }
    (token(top.bytes(input, &[])) && token(sub.bytes(input, &[]))).then_some((top, sub))
}

#[derive(Default)]
pub struct ContentTypePlan {
    pub write_mime: bool,
    pub mime: Span,
    pub write_charset: bool,
    pub charset: Span,
    pub write_boundary: bool,
    pub boundary: Span,
    pub scratch: Vec<u8>,
}

pub fn content_type<'a>(
    input: &[u8],
    current_mime: impl FnOnce() -> &'a [u8],
    had_charset: impl FnOnce() -> bool,
    want_boundary: bool,
    boundary_is_mime: bool,
) -> ContentTypePlan {
    let Some(mime) = mime_range(input) else {
        return ContentTypePlan::default();
    };
    if input == b"*/*" {
        return ContentTypePlan::default();
    }
    let mut charset = None;
    let mut boundary = None;
    for (name, value) in (Parameters { input, offset: mime.end }) {
        let name = name.bytes(input, &[]);
        if charset.is_none() && name.eq_ignore_ascii_case(b"charset") {
            charset = Some(value);
        } else if want_boundary && boundary.is_none() && name.eq_ignore_ascii_case(b"boundary") {
            boundary = Some(value);
        }
        if charset.is_some() && (!want_boundary || boundary.is_some()) {
            break;
        }
    }
    let mut result = ContentTypePlan::default();
    if let Some(value) = boundary {
        result.write_boundary = true;
        result.boundary = output_value(input, value, &mut result.scratch, true, false);
    }
    // Original code compares MIME after committing the boundary. Model that
    // write here when those outputs alias, without mutating borrowed input.
    let equal = if boundary_is_mime && result.write_boundary {
        mime.bytes(input, &[]).eq_ignore_ascii_case(result.boundary.bytes(input, &result.scratch))
    } else {
        mime.bytes(input, &[]).eq_ignore_ascii_case(current_mime())
    };
    let had = !equal && had_charset();
    if !equal {
        result.write_mime = true;
        result.mime = output_value(
            input,
            Value { span: mime, escaped: false },
            &mut result.scratch,
            false,
            true,
        );
    }
    if let Some(value) = charset {
        result.write_charset = true;
        result.charset = output_value(input, value, &mut result.scratch, true, true);
    } else {
        result.write_charset = had;
    }
    result
}
