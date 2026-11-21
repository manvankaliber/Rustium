// Copyright 2026 The Chromium Authors
mod proxy_matching_rust_core;
use proxy_matching_rust_core as core;
#[allow(clippy::too_many_arguments)]
#[cxx::bridge(namespace = "net::proxy_matching_rust")]
mod ffi {
    struct Evaluation {
        result: u8,
        index: usize,
        custom: bool,
    }
    struct TextStep {
        bytes: Vec<u8>,
        index: usize,
        custom: bool,
    }
    struct Authority {
        host: Vec<u8>,
        port: Vec<u8>,
        user: bool,
        has_port: bool,
    }
    unsafe extern "C++" {
        include!("net/base/proxy_matching_rust_ffi.h");
        fn ip_service(raw: &[u8]) -> Vec<u8>;
        fn canonical_service(ip: &[u8]) -> Vec<u8>;
        fn authority_service(raw: &[u8]) -> Authority;

    }
    extern "Rust" {
        type Rule;
        type Matcher;
        fn parse_rule(raw: &[u8], proxy: bool) -> Box<Rule>;
        fn rule_memory(r: &Rule) -> usize;
        fn matcher_memory(m: &Matcher) -> usize;
        fn rule_kind(r: &Rule) -> u8;
        fn new_rule(
            kind: u8,
            scheme: &[u8],
            host: &[u8],
            port: i32,
            ip: &[u8],
            bits: usize,
        ) -> Box<Rule>;
        fn rule_suffix(r: &Rule) -> Box<Rule>;
        fn rule_serialize(r: &Rule) -> Vec<u8>;
        fn rule_evaluate(
            r: &Rule,
            host: &[u8],
            scheme: &[u8],
            port: i32,
            is_ip: bool,
            windows: bool,
            unspecified: bool,
        ) -> u8;
        fn parse_list(raw: &[u8], proxy: bool) -> Box<Matcher>;
        fn new_matcher() -> Box<Matcher>;
        fn matcher_size(m: &Matcher) -> usize;
        fn matcher_rule(m: &Matcher, index: usize) -> Box<Rule>;
        fn matcher_insert(m: &mut Matcher, index: usize, r: &Rule);
        fn matcher_insert_custom(m: &mut Matcher, index: usize);
        fn matcher_replace(m: &mut Matcher, index: usize, r: &Rule);
        fn matcher_replace_custom(m: &mut Matcher, index: usize);
        fn matcher_clear(m: &mut Matcher);
        fn matcher_step(
            m: &Matcher,
            start: usize,
            host: &[u8],
            scheme: &[u8],
            port: i32,
            is_ip: bool,
            windows: bool,
            unspecified: bool,
        ) -> Evaluation;
        fn matcher_text_step(
            m: &Matcher,
            start: usize,
            previous: &[u8],
            has_previous: bool,
        ) -> TextStep;
        fn matcher_compare_step(
            m: &Matcher,
            n: &Matcher,
            start: usize,
            a: &[u8],
            b: &[u8],
            has_previous: bool,
        ) -> Evaluation;
        fn matches_implicit(host: &[u8], windows: bool, unspecified: bool) -> bool;
        fn matches_proxy(
            result: u8,
            reverse: bool,
            host: &[u8],
            windows: bool,
            unspecified: bool,
        ) -> bool;
    }
}
pub struct Rule(core::Rule);
pub struct Matcher(core::Matcher);
struct Services;
impl core::Services for Services {
    fn ip(&self, raw: &[u8]) -> Vec<u8> {
        ffi::ip_service(raw)
    }
    fn canonical(&self, ip: &[u8]) -> Vec<u8> {
        ffi::canonical_service(ip)
    }
    fn authority(&self, raw: &[u8]) -> core::Authority {
        let a = ffi::authority_service(raw);
        let mut host = a.host;
        let port = if a.has_port { core::port_number(&a.port) } else { Some(-1) };
        let mut valid = !a.user && !host.is_empty() && port.is_some();
        if host.starts_with(b"[") {
            if host.ends_with(b"]") && self.ip(&host[1..host.len() - 1]).len() == 16 {
                host = host[1..host.len() - 1].to_vec();
            } else {
                valid = false;
            }
        }
        core::Authority { host, port: port.unwrap_or(-2), valid }
    }
}
fn parse_rule(raw: &[u8], proxy: bool) -> Box<Rule> {
    Box::new(Rule(core::parse(raw, proxy, &Services)))
}
fn rule_kind(r: &Rule) -> u8 {
    r.0.kind
}
fn new_rule(kind: u8, scheme: &[u8], host: &[u8], port: i32, ip: &[u8], bits: usize) -> Box<Rule> {
    Box::new(Rule(match kind {
        1 => core::hostname(scheme, host, port),
        2 => core::ip_host(scheme, host, port),
        3 => core::Rule {
            kind,
            scheme: scheme.to_vec(),
            host: host.to_vec(),
            port,
            ip: ip.to_vec(),
            bits,
        },
        _ => core::special(kind),
    }))
}
fn rule_suffix(r: &Rule) -> Box<Rule> {
    Box::new(Rule(core::suffix(&r.0)))
}
fn rule_serialize(r: &Rule) -> Vec<u8> {
    core::serialize(&r.0)
}
#[allow(clippy::too_many_arguments)]
fn rule_evaluate(
    r: &Rule,
    host: &[u8],
    scheme: &[u8],
    port: i32,
    is_ip: bool,
    windows: bool,
    unspecified: bool,
) -> u8 {
    core::evaluate(&r.0, &core::Url { host, scheme, port, is_ip, windows, unspecified }, &Services)
}
fn parse_list(raw: &[u8], proxy: bool) -> Box<Matcher> {
    let rules = core::tokens(raw)
        .map(|r| core::parse(r, proxy, &Services))
        .filter(|r| r.kind != 0)
        .map(Some)
        .collect();
    Box::new(Matcher(core::Matcher { rules }))
}
fn new_matcher() -> Box<Matcher> {
    Box::new(Matcher(core::Matcher::default()))
}
fn matcher_size(m: &Matcher) -> usize {
    m.0.rules.len()
}
fn matcher_rule(m: &Matcher, index: usize) -> Box<Rule> {
    Box::new(Rule(m.0.rules[index].as_ref().unwrap().clone()))
}
fn matcher_insert(m: &mut Matcher, index: usize, r: &Rule) {
    m.0.insert(index, Some(r.0.clone()));
}
fn matcher_insert_custom(m: &mut Matcher, index: usize) {
    m.0.insert(index, None);
}
fn matcher_replace(m: &mut Matcher, index: usize, r: &Rule) {
    m.0.replace(index, Some(r.0.clone()));
}
fn matcher_replace_custom(m: &mut Matcher, index: usize) {
    m.0.replace(index, None);
}
fn matcher_clear(m: &mut Matcher) {
    m.0.rules.clear();
}
// These continuation APIs return before C++ invokes arbitrary virtual rules.
// No Rust reference or GURL byte slice crosses a user callback. Replacement of
// another rule and URL mutation through external aliases remain observable.
#[allow(clippy::too_many_arguments)]
fn matcher_step(
    m: &Matcher,
    start: usize,
    host: &[u8],
    scheme: &[u8],
    port: i32,
    is_ip: bool,
    windows: bool,
    unspecified: bool,
) -> ffi::Evaluation {
    let e =
        m.0.step(start, &core::Url { host, scheme, port, is_ip, windows, unspecified }, &Services);
    ffi::Evaluation { result: e.result, index: e.index, custom: e.custom }
}
fn matcher_text_step(
    m: &Matcher,
    start: usize,
    previous: &[u8],
    has_previous: bool,
) -> ffi::TextStep {
    let e = m.0.text_step(start, previous, has_previous);
    ffi::TextStep { bytes: e.bytes, index: e.index, custom: e.custom }
}
fn matcher_compare_step(
    m: &Matcher,
    n: &Matcher,
    start: usize,
    a: &[u8],
    b: &[u8],
    has_previous: bool,
) -> ffi::Evaluation {
    let e = m.0.compare_step(&n.0, start, a, b, has_previous);
    ffi::Evaluation { result: e.result, index: e.index, custom: e.custom }
}
fn matches_implicit(host: &[u8], windows: bool, unspecified: bool) -> bool {
    core::implicit(
        &core::Url { host, scheme: b"", port: -1, is_ip: false, windows, unspecified },
        &Services,
    )
}
fn matches_proxy(result: u8, reverse: bool, host: &[u8], windows: bool, unspecified: bool) -> bool {
    core::proxy_result(result, reverse, || matches_implicit(host, windows, unspecified))
}

fn rule_memory(r: &Rule) -> usize {
    std::mem::size_of::<Rule>() + r.0.scheme.capacity() + r.0.host.capacity() + r.0.ip.capacity()
}
fn matcher_memory(m: &Matcher) -> usize {
    std::mem::size_of::<Matcher>()
        + m.0.rules.capacity() * std::mem::size_of::<Option<core::Rule>>()
        + m.0
            .rules
            .iter()
            .flatten()
            .map(|r| r.scheme.capacity() + r.host.capacity() + r.ip.capacity())
            .sum::<usize>()
}
