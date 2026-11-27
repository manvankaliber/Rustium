// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#![forbid(unsafe_code)]

#[derive(Clone, Default)]
pub struct Rules {
    maps: Vec<Map>,
    exclusions: Vec<Vec<u8>>,
}
#[derive(Clone)]
struct Map {
    pattern: Vec<u8>,
    host: Vec<u8>,
    port: i32,
}
pub struct Rewrite<'a> {
    pub host: &'a [u8],
    pub port: u16,
}

fn trim(input: &[u8]) -> &[u8] {
    // base::kWhitespaceASCII includes vertical tab, unlike is_ascii_whitespace.
    let whitespace = |c: &u8| matches!(c, b' ' | b'\t' | b'\n' | b'\r' | 0x0b | 0x0c);
    let start = input.iter().position(|c| !whitespace(c)).unwrap_or(input.len());
    let end = input.iter().rposition(|c| !whitespace(c)).map_or(start, |i| i + 1);
    &input[start..end]
}
impl Rules {
    pub fn add(
        &mut self,
        input: &[u8],
        parse: impl FnOnce(&[u8]) -> Option<(Vec<u8>, i32)>,
    ) -> bool {
        let mut parts = trim(input).split(|&c| c == b' ').map(trim);
        let command = parts.next().unwrap_or_default();
        let Some(pattern) = parts.next() else { return false };
        let replacement = parts.next();
        if parts.next().is_some() {
            return false;
        }
        if command.eq_ignore_ascii_case(b"exclude") && replacement.is_none() {
            self.exclusions.push(pattern.to_ascii_lowercase());
            return true;
        }
        if command.eq_ignore_ascii_case(b"map")
            && let Some(replacement) = replacement
            && let Some((host, port)) = parse(replacement)
        {
            self.maps.push(Map { pattern: pattern.to_ascii_lowercase(), host, port });
            return true;
        }
        false
    }
    pub fn set(
        &mut self,
        input: &[u8],
        mut parse: impl FnMut(&[u8]) -> Option<(Vec<u8>, i32)>,
    ) -> Vec<(usize, usize)> {
        self.maps.clear();
        self.exclusions.clear();
        let mut invalid = Vec::new();
        let mut offset = 0;
        for piece in input.split(|&c| c == b',') {
            let rule = trim(piece);
            let leading = piece.len()
                - piece
                    .iter()
                    .skip_while(|&&c| matches!(c, b' ' | b'\t' | b'\n' | b'\r' | 0x0b | 0x0c))
                    .count();
            if !self.add(rule, &mut parse) {
                invalid.push((offset + leading, rule.len()));
            }
            offset += piece.len() + 1;
        }
        invalid
    }
    pub fn rewrite<'a>(
        &'a self,
        host: &[u8],
        port: u16,
        mut formatted: impl FnMut() -> Vec<u8>,
    ) -> Option<Rewrite<'a>> {
        // Formatting is lazy: the original calls HostForURL (and its
        // diagnostics) only when the hostname itself fails to match a mapping
        // pattern.
        for map in &self.maps {
            if !matches(host, &map.pattern) && !matches(&formatted(), &map.pattern) {
                continue;
            }
            if self.exclusions.iter().any(|p| matches(host, p)) {
                return None;
            }
            return Some(Rewrite {
                host: &map.host,
                port: if map.port == -1 { port } else { map.port as u16 },
            });
        }
        None
    }
}

/// ICU CBU8_NEXT: invalid input returns -1 after consuming its valid prefix.
/// Invalid codepoints never match literals, but wildcards can skip them.
fn next(input: &[u8], at: &mut usize) -> i32 {
    let lead = input[*at];
    *at += 1;
    if lead < 0x80 {
        return i32::from(lead);
    }
    let (length, mut value) = match lead {
        0xc2..=0xdf => (2, i32::from(lead & 0x1f)),
        0xe0..=0xef => (3, i32::from(lead & 0x0f)),
        0xf0..=0xf4 => (4, i32::from(lead & 7)),
        _ => return -1,
    };
    for n in 1..length {
        let Some(&byte) = input.get(*at) else { return -1 };
        if !(0x80..=0xbf).contains(&byte)
            || (n == 1
                && ((lead == 0xe0 && byte < 0xa0)
                    || (lead == 0xed && byte >= 0xa0)
                    || (lead == 0xf0 && byte < 0x90)
                    || (lead == 0xf4 && byte >= 0x90)))
        {
            return -1;
        }
        value = (value << 6) | i32::from(byte & 0x3f);
        *at += 1;
    }
    value
}
/// Chromium MatchPattern semantics: a run of '?' matches zero through N
/// codepoints, '*' makes that run unbounded, '\\' escapes the next codepoint.
/// Literal runs choose their earliest match, without subsequent backtracking.
pub fn matches(input: &[u8], pattern: &[u8]) -> bool {
    let (mut p, mut s) = (0, 0);
    loop {
        let mut maximum = Some(0usize);
        while p < pattern.len() && matches!(pattern[p], b'*' | b'?') {
            if pattern[p] == b'*' {
                maximum = None;
            } else {
                maximum = maximum.map(|n| n + 1);
            }
            p += 1;
        }
        let (pattern_start, mut string_start) = (p, s);
        let mut escape = false;
        loop {
            if p == pattern.len() {
                if s == input.len() {
                    return true;
                }
            } else {
                if !escape && matches!(pattern[p], b'*' | b'?') {
                    break;
                }
                if !escape && pattern[p] == b'\\' {
                    escape = true;
                    next(pattern, &mut p);
                    continue;
                }
                escape = false;
                if s == input.len() {
                    return false;
                }
                let (mut pn, mut sn) = (p, s);
                let character = next(pattern, &mut pn);
                if character == next(input, &mut sn) && character != -1 {
                    p = pn;
                    s = sn;
                    continue;
                }
            }
            if maximum == Some(0) {
                return false;
            }
            maximum = maximum.map(|n| n - 1);
            p = pattern_start;
            // Search cannot advance beyond end: a nonempty literal would have
            // returned above; at end-of-pattern we return on end-of-input.
            next(input, &mut string_start);
            s = string_start;
        }
    }
}
