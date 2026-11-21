// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
mod host_port_rust_core;
use host_port_rust_core as core;

#[cxx::bridge(namespace = "net::host_port_rust")]
mod ffi {
    struct Authority {
        username_len: i32,
        password_len: i32,
        host_begin: i32,
        host_len: i32,
        port_begin: i32,
        port_len: i32,
    }
    struct ParseResult {
        ok: bool,
        start: usize,
        len: usize,
        port: i32,
    }
    struct FormatResult {
        value: Vec<u8>,
        diagnostic: Vec<u8>,
        has_null: bool,
        bad_brackets: bool,
    }
    unsafe extern "C++" {
        include!("net/base/host_port_rust_ffi.h");
        fn authority(input: &[u8]) -> Authority;
        fn parse_port(input: &[u8], begin: i32, len: i32) -> i32;
        fn ipv6(input: &[u8]) -> bool;
    }
    extern "Rust" {
        fn parse(input: &[u8]) -> ParseResult;
        fn pair_allowed(input: &[u8]) -> bool;
        fn valid_port(port: i32) -> bool;
        fn value_valid(dict: bool, host: bool, port: bool, number: i32) -> bool;
        fn empty(host: &[u8], port: u16) -> bool;
        fn strip(host: &[u8]) -> bool;
        fn compare(ap: u16, ah: &[u8], bp: u16, bh: &[u8]) -> i8;
        fn format(host: &[u8], port: u16, include_port: bool) -> FormatResult;
    }
}
fn parse(input: &[u8]) -> ffi::ParseResult {
    let result = core::parse_host_and_port(
        input,
        |s| {
            let a = ffi::authority(s);
            core::Authority {
                username: core::Component { begin: 0, len: a.username_len },
                password: core::Component { begin: 0, len: a.password_len },
                host: core::Component { begin: a.host_begin.max(0) as usize, len: a.host_len },
                port: core::Component { begin: a.port_begin.max(0) as usize, len: a.port_len },
            }
        },
        |s, c| ffi::parse_port(s, c.begin as i32, c.len),
        ffi::ipv6,
    );
    match result {
        Some((range, port)) => {
            ffi::ParseResult { ok: true, start: range.start, len: range.len(), port }
        }
        None => ffi::ParseResult { ok: false, start: 0, len: 0, port: 0 },
    }
}
fn pair_allowed(input: &[u8]) -> bool {
    core::pair_input_allowed(input)
}
fn valid_port(port: i32) -> bool {
    core::pair_port_valid(port)
}
fn value_valid(dict: bool, host: bool, port: bool, number: i32) -> bool {
    dict && host && port && core::pair_port_valid(number)
}
fn empty(host: &[u8], port: u16) -> bool {
    host.is_empty() && port == 0
}
fn strip(host: &[u8]) -> bool {
    core::strip_brackets(host).len() != host.len()
}
fn compare(ap: u16, ah: &[u8], bp: u16, bh: &[u8]) -> i8 {
    match core::compare(ap, ah, bp, bh) {
        std::cmp::Ordering::Less => -1,
        std::cmp::Ordering::Equal => 0,
        std::cmp::Ordering::Greater => 1,
    }
}
fn format(host: &[u8], port: u16, include_port: bool) -> ffi::FormatResult {
    let result = if include_port { core::format_pair(host, port) } else { core::format_host(host) };
    ffi::FormatResult {
        value: result.value,
        has_null: result.diagnostic.is_some(),
        diagnostic: result.diagnostic.unwrap_or_default(),
        bad_brackets: result.bracketed_input,
    }
}
