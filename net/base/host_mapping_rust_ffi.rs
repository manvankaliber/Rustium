// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
mod host_mapping_rust_core;
use host_mapping_rust_core::Rules;
use host_port_policy_core as policy;
#[cxx::bridge(namespace = "net::host_mapping_rust")]
mod ffi {
    struct Authority {
        username_len: i32,
        password_len: i32,
        host_begin: i32,
        host_len: i32,
        port_begin: i32,
        port_len: i32,
    }
    struct BadRule {
        begin: usize,
        len: usize,
    }
    struct Rewrite {
        matched: bool,
        host: Vec<u8>,
        port: u16,
    }
    unsafe extern "C++" {
        include!("net/base/host_mapping_rust_ffi.h");
        fn authority(input: &[u8]) -> Authority;
        fn parse_port(input: &[u8], begin: i32, len: i32) -> i32;
        fn ipv6(input: &[u8]) -> bool;
        fn report_format_diagnostics(diagnostic: &[u8], has_null: bool, bad_brackets: bool);
    }
    extern "Rust" {
        type Rules;
        fn new_rules() -> Box<Rules>;
        fn clone_rules(rules: &Rules) -> Box<Rules>;
        fn add(rules: &mut Rules, input: &[u8]) -> bool;
        fn set(rules: &mut Rules, input: &[u8]) -> Vec<BadRule>;
        fn rewrite(rules: &Rules, host: &[u8], port: u16) -> Rewrite;
        fn match_pattern(input: &[u8], pattern: &[u8]) -> bool;
        fn format_for_match(host: &[u8], port: u16) -> Vec<u8>;
        fn format_url_host(host: &[u8]) -> Vec<u8>;
    }
}
fn parse(input: &[u8]) -> Option<(Vec<u8>, i32)> {
    let (range, port) = policy::parse_host_and_port(
        input,
        |s| {
            let a = ffi::authority(s);
            policy::Authority {
                username: policy::Component { begin: 0, len: a.username_len },
                password: policy::Component { begin: 0, len: a.password_len },
                host: policy::Component { begin: a.host_begin.max(0) as usize, len: a.host_len },
                port: policy::Component { begin: a.port_begin.max(0) as usize, len: a.port_len },
            }
        },
        |s, c| ffi::parse_port(s, c.begin as i32, c.len),
        ffi::ipv6,
    )?;
    Some((input[range].to_vec(), port))
}
fn new_rules() -> Box<Rules> {
    Box::default()
}
fn clone_rules(rules: &Rules) -> Box<Rules> {
    Box::new(rules.clone())
}
fn add(rules: &mut Rules, input: &[u8]) -> bool {
    rules.add(input, parse)
}
fn set(rules: &mut Rules, input: &[u8]) -> Vec<ffi::BadRule> {
    rules.set(input, parse).into_iter().map(|(begin, len)| ffi::BadRule { begin, len }).collect()
}
fn rewrite(rules: &Rules, host: &[u8], port: u16) -> ffi::Rewrite {
    match rules.rewrite(host, port, || {
        let formatted = policy::format_pair(host, port);
        report_diagnostics(&formatted);
        formatted.value
    }) {
        Some(value) => ffi::Rewrite { matched: true, host: value.host.to_vec(), port: value.port },
        None => ffi::Rewrite { matched: false, host: Vec::new(), port },
    }
}
fn match_pattern(input: &[u8], pattern: &[u8]) -> bool {
    host_mapping_rust_core::matches(input, pattern)
}
fn format_for_match(host: &[u8], port: u16) -> Vec<u8> {
    policy::format_pair(host, port).value
}
fn format_url_host(host: &[u8]) -> Vec<u8> {
    let formatted = policy::format_host(host);
    report_diagnostics(&formatted);
    formatted.value
}
fn report_diagnostics(formatted: &policy::FormattedHost) {
    if formatted.diagnostic.is_some() || formatted.bracketed_input {
        ffi::report_format_diagnostics(
            formatted.diagnostic.as_deref().unwrap_or_default(),
            formatted.diagnostic.is_some(),
            formatted.bracketed_input,
        );
    }
}
