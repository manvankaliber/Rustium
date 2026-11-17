// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#![forbid(unsafe_code)]

use std::cmp::Ordering;

/// A component from url::ParseAuthority. An absent component has len == -1.
#[derive(Clone, Copy, Debug)]
pub struct Component {
    pub begin: usize,
    pub len: i32,
}

#[derive(Clone, Copy, Debug)]
pub struct Authority {
    pub username: Component,
    pub password: Component,
    pub host: Component,
    pub port: Component,
}

/// Implements the local ParseHostAndPort acceptance policy. The authority,
/// port and IPv6 functions are URL library services, not acceptance policy.
pub fn parse_host_and_port(
    input: &[u8],
    authority: impl FnOnce(&[u8]) -> Authority,
    parse_port: impl FnOnce(&[u8], Component) -> i32,
    ipv6: impl FnOnce(&[u8]) -> bool,
) -> Option<(std::ops::Range<usize>, i32)> {
    if input.is_empty() {
        return None;
    }
    let parts = authority(input);
    if parts.username.len >= 0 || parts.password.len >= 0 || parts.host.len <= 0 {
        return None;
    }
    let port = if parts.port.len > 0 {
        let port = parse_port(input, parts.port);
        if port < 0 {
            return None;
        }
        port
    } else {
        -1
    };
    if parts.port.len == 0 {
        return None;
    }
    let start = parts.host.begin;
    let end = start.checked_add(parts.host.len as usize)?;
    let host = input.get(start..end)?;
    if host.first() == Some(&b'[') {
        if host.last() != Some(&b']') || !ipv6(host) {
            return None;
        }
        Some((start + 1..end - 1, port))
    } else {
        Some((start..end, port))
    }
}

pub fn pair_input_allowed(input: &[u8]) -> bool {
    input.first() == Some(&b'[') || input.iter().filter(|&&c| c == b':').take(2).count() < 2
}

pub fn pair_port_valid(port: i32) -> bool {
    (0..=u16::MAX as i32).contains(&port)
}

pub fn strip_brackets(host: &[u8]) -> &[u8] {
    if host.len() >= 2 && host.first() == Some(&b'[') && host.last() == Some(&b']') {
        &host[1..host.len() - 1]
    } else {
        host
    }
}

/// C++ defaulted comparison follows declaration order: port, then string.
pub fn compare(a_port: u16, a_host: &[u8], b_port: u16, b_host: &[u8]) -> Ordering {
    a_port.cmp(&b_port).then_with(|| a_host.cmp(b_host))
}

pub struct FormattedHost {
    pub value: Vec<u8>,
    pub diagnostic: Option<Vec<u8>>,
    pub bracketed_input: bool,
}

pub fn format_host(host: &[u8]) -> FormattedHost {
    let diagnostic = host.contains(&0).then(|| {
        let mut out = Vec::new();
        for &c in host {
            if c == 0 {
                out.extend_from_slice(b"%00");
            } else {
                out.push(c);
            }
        }
        out
    });
    let colon = host.contains(&b':');
    let mut value = Vec::new();
    if colon {
        value.push(b'[');
        // StringPrintf("[%s]", host.c_str()) stops at the first NUL.
        value.extend_from_slice(host.split(|&c| c == 0).next().unwrap_or_default());
        value.push(b']');
    } else {
        value.extend_from_slice(host);
    }
    FormattedHost { value, diagnostic, bracketed_input: colon && host.first() == Some(&b'[') }
}

pub fn format_pair(host: &[u8], port: u16) -> FormattedHost {
    let mut result = format_host(host);
    result.value.push(b':');
    result.value.extend_from_slice(port.to_string().as_bytes());
    result
}

#[cfg(test)]
mod tests {
    use super::*;
    const NONE: Component = Component { begin: 0, len: -1 };
    fn parse(
        input: &[u8],
        host: Component,
        port: Component,
        number: i32,
        valid_ipv6: bool,
    ) -> Option<(std::ops::Range<usize>, i32)> {
        parse_host_and_port(
            input,
            |_| Authority { username: NONE, password: NONE, host, port },
            |_, _| number,
            |_| valid_ipv6,
        )
    }
    #[test]
    fn parsing_and_checks() {
        let host = Component { begin: 0, len: 3 };
        assert_eq!(
            parse(b"abc:80", host, Component { begin: 4, len: 2 }, 80, false),
            Some((0..3, 80))
        );
        assert_eq!(parse(b"abc", host, NONE, 0, false), Some((0..3, -1)));
        assert_eq!(parse(b"abc:", host, Component { begin: 4, len: 0 }, 0, false), None);
        assert_eq!(parse(b"abc:x", host, Component { begin: 4, len: 1 }, -2, false), None);
        let h = Component { begin: 0, len: 5 };
        assert_eq!(parse(b"[::1]", h, NONE, 0, true), Some((1..4, -1)));
        assert_eq!(parse(b"[::1]", h, NONE, 0, false), None);
        assert!(!pair_input_allowed(b"a:b:c"));
        assert!(pair_input_allowed(b"[::1]:80"));
        assert_eq!(strip_brackets(b"[]"), b"");
        assert!(pair_port_valid(65535));
        assert!(!pair_port_valid(65536));
    }
    #[test]
    fn formatting_and_order() {
        let out = format_pair(b"a\0:b", 123);
        assert_eq!(out.value, b"[a]:123");
        assert_eq!(out.diagnostic.unwrap(), b"a%00:b");
        assert_eq!(format_host(b"a\0b").value, b"a\0b");
        assert_eq!(compare(1, b"z", 2, b"a"), Ordering::Less);
        assert_eq!(compare(1, &[0xff], 1, &[0x7f]), Ordering::Greater);
    }
}
