// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

//! Proxy value policy. URL canonicalization/authority/IP and pickle primitives
//! are explicit services; no proxy decisions are delegated back to C++.
#![forbid(unsafe_code)]

use std::cmp::Ordering;

pub const INVALID: i32 = 1;
pub const HTTP: i32 = 4;
pub const SOCKS4: i32 = 8;
pub const SOCKS5: i32 = 16;
pub const HTTPS: i32 = 32;
pub const QUIC: i32 = 64;

#[derive(Clone, Debug, PartialEq, Eq)]
pub struct Server {
    pub scheme: i32,
    pub host: Vec<u8>,
    pub port: u16,
}
impl Default for Server {
    fn default() -> Self {
        Self { scheme: INVALID, host: Vec::new(), port: 0 }
    }
}

pub struct Authority {
    pub host: Vec<u8>,
    pub port: Vec<u8>,
    pub user: bool,
    pub port_present: bool,
}
pub trait Services {
    fn canonical_host(&self, host: &[u8]) -> Vec<u8>;
    fn authority(&self, input: &[u8]) -> Authority;
    fn ipv6(&self, input: &[u8]) -> bool;
}

pub fn default_port(scheme: i32) -> i32 {
    match scheme {
        HTTP => 80,
        SOCKS4 | SOCKS5 => 1080,
        HTTPS | QUIC => 443,
        _ => -1,
    }
}
pub fn scheme_flag(scheme: i32, flag: u8) -> bool {
    match flag {
        0 => scheme != INVALID,
        1 => scheme == HTTP,
        2 => scheme == HTTPS,
        3 => matches!(scheme, SOCKS4 | SOCKS5),
        4 => scheme == QUIC,
        5 => matches!(scheme, HTTP | HTTPS | QUIC),
        6 => matches!(scheme, HTTPS | QUIC),
        _ => false,
    }
}
pub fn valid_serial_scheme(scheme: i32) -> bool {
    matches!(scheme, INVALID | HTTP | SOCKS4 | SOCKS5 | HTTPS | QUIC)
}
// Exact url::ParsePort semantics, including arbitrarily many leading zeroes.
pub fn port(input: &[u8]) -> Option<u16> {
    if input.is_empty() {
        return None;
    }
    let mut value = 0_u16;
    for &c in input {
        if !c.is_ascii_digit() {
            return None;
        }
        value = value.checked_mul(10)?.checked_add(u16::from(c - b'0'))?;
    }
    Some(value)
}
pub fn trim_ascii(input: &[u8]) -> &[u8] {
    let ws = |b: &u8| matches!(b, b' ' | b'\t' | b'\n' | b'\r' | 0x0b | 0x0c);
    let start = input.iter().take_while(|b| ws(b)).count();
    let end = input[start..].iter().rev().take_while(|b| ws(b)).count();
    &input[start..input.len() - end]
}
pub fn trim_lws(input: &[u8]) -> &[u8] {
    input.trim_ascii_start_matches_lws().trim_ascii_end_matches_lws()
}
// HTTP LWS differs from the URL/ASCII whitespace set.
trait Lws {
    fn trim_ascii_start_matches_lws(&self) -> &Self;
    fn trim_ascii_end_matches_lws(&self) -> &Self;
}
impl Lws for [u8] {
    fn trim_ascii_start_matches_lws(&self) -> &Self {
        let n = self.iter().take_while(|&&b| b == b' ' || b == b'\t').count();
        &self[n..]
    }
    fn trim_ascii_end_matches_lws(&self) -> &Self {
        let n = self.iter().rev().take_while(|&&b| b == b' ' || b == b'\t').count();
        &self[..self.len() - n]
    }
}
pub fn from_host(scheme: i32, host: &[u8], p: Option<u16>, svc: &impl Services) -> Server {
    let host = trim_ascii(host);
    let bracketed;
    let host = if !host.starts_with(b"[") && host.contains(&b':') {
        bracketed = [b"[".as_slice(), host, b"]"].concat();
        &bracketed
    } else {
        host
    };
    let mut host = svc.canonical_host(host);
    if host.is_empty() {
        return Server::default();
    }
    if host.starts_with(b"[") && host.ends_with(b"]") {
        host = host[1..host.len() - 1].to_vec();
    }
    Server { scheme, host, port: p.unwrap_or_else(|| default_port(scheme) as u16) }
}
pub fn from_host_port(scheme: i32, host: &[u8], p: &[u8], svc: &impl Services) -> Server {
    if p.is_empty() {
        return from_host(scheme, host, None, svc);
    }
    match port(p) {
        Some(p) => from_host(scheme, host, Some(p), svc),
        None => Server::default(),
    }
}
pub fn from_authority(scheme: i32, input: &[u8], svc: &impl Services) -> Server {
    if scheme == INVALID {
        return Server::default();
    }
    let a = svc.authority(trim_lws(input));
    if a.user || a.host.is_empty() || (a.port_present && a.port.is_empty()) {
        return Server::default();
    }
    from_host_port(scheme, &a.host, &a.port, svc)
}
pub fn uri_scheme(input: &[u8], quic: bool) -> i32 {
    if input.eq_ignore_ascii_case(b"http") {
        HTTP
    } else if input.eq_ignore_ascii_case(b"https") {
        HTTPS
    } else if input.eq_ignore_ascii_case(b"socks4") {
        SOCKS4
    } else if input.eq_ignore_ascii_case(b"socks") || input.eq_ignore_ascii_case(b"socks5") {
        SOCKS5
    } else if quic && input.eq_ignore_ascii_case(b"quic") {
        QUIC
    } else {
        INVALID
    }
}
pub fn from_uri(input: &[u8], scheme: i32, quic: bool, svc: &impl Services) -> Server {
    let input = trim_lws(input);
    if let Some(i) = input.iter().position(|&b| b == b':')
        && input[i..].starts_with(b"://")
    {
        return from_authority(uri_scheme(&input[..i], quic), &input[i + 3..], svc);
    }
    from_authority(scheme, input, svc)
}
fn pac_parts(input: &[u8]) -> (&[u8], &[u8]) {
    let input = trim_lws(input);
    let i = input.iter().position(|&b| b == b' ' || b == b'\t').unwrap_or(input.len());
    (&input[..i], &input[i..])
}
pub fn from_pac(input: &[u8], svc: &impl Services) -> Server {
    let (kind, authority) = pac_parts(input);
    let scheme = if kind.eq_ignore_ascii_case(b"proxy") {
        HTTP
    } else if kind.eq_ignore_ascii_case(b"socks") || kind.eq_ignore_ascii_case(b"socks4") {
        SOCKS4
    } else if kind.eq_ignore_ascii_case(b"socks5") {
        SOCKS5
    } else if kind.eq_ignore_ascii_case(b"https") {
        HTTPS
    } else {
        INVALID
    }; // Original PAC parser deliberately does not accept QUIC.
    from_authority(scheme, authority, svc)
}
pub fn host_for_url(host: &[u8]) -> Vec<u8> {
    host_port_policy_core::format_host(host).value
}
pub fn host_port(host: &[u8], port: u16) -> Vec<u8> {
    host_port_policy_core::format_pair(host, port).value
}
pub fn format_server(server: &Server, pac: bool) -> Vec<u8> {
    let prefix: &[u8] = match (server.scheme, pac) {
        (HTTP, false) => b"",
        (HTTP, true) => b"PROXY ",
        (SOCKS4, false) => b"socks4://",
        (SOCKS4, true) => b"SOCKS ",
        (SOCKS5, false) => b"socks5://",
        (SOCKS5, true) => b"SOCKS5 ",
        (HTTPS, false) => b"https://",
        (HTTPS, true) => b"HTTPS ",
        (QUIC, false) => b"quic://",
        (QUIC, true) => b"QUIC ",
        _ => return Vec::new(), // C++ adapter retains NOTREACHED precondition.
    };
    [prefix.to_vec(), host_port(&server.host, server.port)].concat()
}
pub fn compare_server(a: &Server, b: &Server) -> Ordering {
    // HostPortPair's physical field order is PORT then HOST, not host/port.
    a.scheme
        .cmp(&b.scheme)
        .then_with(|| host_port_policy_core::compare(a.port, &a.host, b.port, &b.host))
}
pub fn valid_chain(servers: &[Server], quic: bool, multi: bool) -> bool {
    match servers {
        [] => true,
        [s] => s.scheme != INVALID && (s.scheme != QUIC || quic),
        _ if !multi => false,
        _ => {
            let mut https = false;
            for s in servers {
                match s.scheme {
                    QUIC if quic && !https => (),
                    HTTPS => https = true,
                    _ => return false,
                }
            }
            true
        }
    }
}
pub fn chain_pac(input: &[u8], svc: &impl Services) -> Vec<Server> {
    let (kind, rest) = pac_parts(input);
    if kind.eq_ignore_ascii_case(b"direct") && rest.is_empty() {
        Vec::new()
    } else {
        vec![from_pac(input, svc)]
    }
}
pub fn chain_uri(input: &[u8], scheme: i32, quic: bool, svc: &impl Services) -> Vec<Server> {
    let input = trim_lws(input);
    if let Some(i) = input.windows(3).position(|w| w == b"://")
        && input[..i].eq_ignore_ascii_case(b"direct")
    {
        return if input[i + 3..].is_empty() { Vec::new() } else { vec![Server::default()] };
    }
    vec![from_uri(input, scheme, quic, svc)]
}
pub fn chain_multi(input: &[u8], scheme: i32, quic: bool, svc: &impl Services) -> Vec<Server> {
    let input = trim_lws(input);
    let bracketed = input.starts_with(b"[") && input.ends_with(b"]");
    let input = if bracketed { trim_lws(&input[1..input.len() - 1]) } else { input };
    // SplitStringPiece splits on SPACE only, then trims full ASCII whitespace.
    let parts: Vec<_> =
        input.split(|&b| b == b' ').map(trim_ascii).filter(|s| !s.is_empty()).collect();
    if parts.is_empty() || (parts.len() > 1 && !bracketed) {
        return vec![Server::default()];
    }
    let mut result = Vec::new();
    for part in &parts {
        if part.eq_ignore_ascii_case(b"direct://") {
            return if parts.len() == 1 { Vec::new() } else { vec![Server::default()] };
        }
        result.push(from_uri(part, scheme, quic, svc));
    }
    result
}
pub fn debug_chain(valid: bool, servers: &[Server], opaque: Option<i32>) -> Vec<u8> {
    if !valid {
        return b"INVALID PROXY CHAIN".to_vec();
    }
    let mut result = b"[".to_vec();
    if servers.is_empty() {
        result.extend(b"direct://");
    }
    for (i, s) in servers.iter().enumerate() {
        if i > 0 {
            result.extend(b", ");
        }
        result.extend(format_server(s, false));
    }
    result.push(b']');
    if let Some(opaque) = opaque {
        result.extend(format!(" (Opaque data {opaque})").bytes());
    }
    result
}
pub fn compare_chain(
    a: Option<&[Server]>,
    ao: Option<i32>,
    b: Option<&[Server]>,
    bo: Option<i32>,
) -> Ordering {
    match (a, b) {
        (None, None) => ao.cmp(&bo),
        (None, Some(_)) => Ordering::Less,
        (Some(_), None) => Ordering::Greater,
        (Some(a), Some(b)) => {
            for (a, b) in a.iter().zip(b) {
                let c = compare_server(a, b);
                if c != Ordering::Equal {
                    return c;
                }
            }
            a.len().cmp(&b.len()).then_with(|| ao.cmp(&bo))
        }
    }
}
pub fn persisted_server(scheme: i32, input: &[u8], svc: &impl Services) -> Server {
    let scheme = if valid_serial_scheme(scheme) { scheme } else { INVALID };
    let mut host = Vec::new();
    let mut p = 0;
    if host_port_policy_core::pair_input_allowed(input) {
        let a = svc.authority(input);
        if !a.user
            && !a.host.is_empty()
            && a.port_present
            && let Some(parsed_port) = port(&a.port)
        {
            let h = &a.host;
            if h.starts_with(b"[") {
                if h.ends_with(b"]") && svc.ipv6(h) {
                    host = h[1..h.len() - 1].to_vec();
                    p = parsed_port;
                }
            } else {
                host = h.clone();
                p = parsed_port;
            }
        }
    }
    Server { scheme, host, port: p }
}
pub fn get_allowed(len: usize, scheme: i32) -> bool {
    len == 1 && matches!(scheme, HTTP | HTTPS)
}
pub fn persisted_length(len: usize) -> i32 {
    if len > i32::MAX as usize - 1 {
        0
    } else {
        len as i32
    }
}
