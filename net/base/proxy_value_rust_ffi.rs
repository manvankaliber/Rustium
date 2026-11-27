// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
mod proxy_value_rust_core;
use proxy_value_rust_core as core;

#[cxx::bridge(namespace = "net::proxy_value_rust")]
mod ffi {
    struct Server {
        scheme: i32,
        host: Vec<u8>,
        port: u16,
    }
    struct Authority {
        host: Vec<u8>,
        port: Vec<u8>,
        user: bool,
        port_present: bool,
    }
    struct Chain {
        valid: bool,
        servers: Vec<Server>,
    }
    struct ReadInt {
        ok: bool,
        value: i32,
    }
    unsafe extern "C++" {
        include!("net/base/proxy_value_rust_ffi.h");
        type Reader;
        type Writer;
        fn canonical_host(input: &[u8]) -> Vec<u8>;
        fn parse_authority(input: &[u8]) -> Authority;
        fn is_ipv6(input: &[u8]) -> bool;
        fn read_int(reader: Pin<&mut Reader>) -> ReadInt;
        fn read_length(reader: Pin<&mut Reader>) -> ReadInt;
        fn read_string(reader: Pin<&mut Reader>) -> Vec<u8>;
        fn write_int(writer: Pin<&mut Writer>, value: i32);
        fn write_string(writer: Pin<&mut Writer>, value: &[u8]);
        fn check_invalid_record(host: &[u8], port: u16);
        fn check_host(host: &[u8], require_nonempty: bool);
        fn check_format_scheme(recognized: bool);
    }
    extern "Rust" {
        fn default_port(scheme: i32) -> i32;
        fn scheme_flag(scheme: i32, flag: u8) -> bool;
        fn parse_host(scheme: i32, host: &[u8], port: &[u8]) -> Server;
        fn parse_host_number(scheme: i32, host: &[u8], port: u16, has_port: bool) -> Server;
        fn parse_server(input: &[u8], scheme: i32, quic: bool, kind: u8) -> Server;
        fn get_uri_scheme(input: &[u8], quic: bool) -> i32;
        fn format_server(scheme: i32, host: &[u8], port: u16, pac: bool) -> Vec<u8>;
        fn format_host(host: &[u8]) -> Vec<u8>;
        fn compare_server(
            a_scheme: i32,
            a_host: &[u8],
            a_port: u16,
            b_scheme: i32,
            b_host: &[u8],
            b_port: u16,
        ) -> i8;
        fn valid_chain(servers: &[Server], quic: bool, multi: bool) -> bool;
        fn get_allowed(len: usize, scheme: i32) -> bool;
        fn parse_chain(input: &[u8], scheme: i32, quic: bool, multi: bool, kind: u8) -> Chain;
        fn debug_chain(valid: bool, servers: &[Server], has_opaque: bool, opaque: i32) -> Vec<u8>;
        #[allow(clippy::too_many_arguments)] // Existing optional C++ fields.
        fn compare_chain(
            a_valid: bool,
            a: &[Server],
            a_has_opaque: bool,
            a_opaque: i32,
            b_valid: bool,
            b: &[Server],
            b_has_opaque: bool,
            b_opaque: i32,
        ) -> i8;
        fn read_server(reader: Pin<&mut Reader>) -> Server;
        fn read_chain(reader: Pin<&mut Reader>, quic: bool, multi: bool) -> Chain;
        fn persist_server(writer: Pin<&mut Writer>, scheme: i32, host: &[u8], port: u16);
        fn persist_chain(writer: Pin<&mut Writer>, servers: &[Server]);
    }
}
struct Services;
impl core::Services for Services {
    fn canonical_host(&self, input: &[u8]) -> Vec<u8> {
        ffi::canonical_host(input)
    }
    fn authority(&self, input: &[u8]) -> core::Authority {
        let a = ffi::parse_authority(input);
        core::Authority { host: a.host, port: a.port, user: a.user, port_present: a.port_present }
    }
    fn ipv6(&self, input: &[u8]) -> bool {
        ffi::is_ipv6(input)
    }
}
impl From<core::Server> for ffi::Server {
    fn from(s: core::Server) -> Self {
        Self { scheme: s.scheme, host: s.host, port: s.port }
    }
}
fn server(scheme: i32, host: &[u8], port: u16) -> core::Server {
    core::Server { scheme, host: host.to_vec(), port }
}
fn servers(input: &[ffi::Server]) -> Vec<core::Server> {
    input.iter().map(|s| server(s.scheme, &s.host, s.port)).collect()
}
fn default_port(scheme: i32) -> i32 {
    core::default_port(scheme)
}
fn scheme_flag(scheme: i32, flag: u8) -> bool {
    core::scheme_flag(scheme, flag)
}
fn parse_host(scheme: i32, host: &[u8], port: &[u8]) -> ffi::Server {
    core::from_host_port(scheme, host, port, &Services).into()
}
fn parse_host_number(scheme: i32, host: &[u8], port: u16, has_port: bool) -> ffi::Server {
    core::from_host(scheme, host, has_port.then_some(port), &Services).into()
}
fn parse_server(input: &[u8], scheme: i32, quic: bool, kind: u8) -> ffi::Server {
    match kind {
        0 => core::from_uri(input, scheme, quic, &Services),
        1 => core::from_pac(input, &Services),
        _ => core::from_authority(scheme, input, &Services),
    }
    .into()
}
fn get_uri_scheme(input: &[u8], quic: bool) -> i32 {
    core::uri_scheme(input, quic)
}
fn format_server(scheme: i32, host: &[u8], port: u16, pac: bool) -> Vec<u8> {
    ffi::check_format_scheme(core::valid_serial_scheme(scheme) && scheme != core::INVALID);
    ffi::check_host(host, true);
    core::format_server(&server(scheme, host, port), pac)
}
fn format_host(host: &[u8]) -> Vec<u8> {
    core::host_for_url(host)
}
fn compare_server(
    a_scheme: i32,
    a_host: &[u8],
    a_port: u16,
    b_scheme: i32,
    b_host: &[u8],
    b_port: u16,
) -> i8 {
    core::compare_server(&server(a_scheme, a_host, a_port), &server(b_scheme, b_host, b_port)) as i8
}
fn valid_chain(input: &[ffi::Server], quic: bool, multi: bool) -> bool {
    core::valid_chain(&servers(input), quic, multi)
}
fn get_allowed(len: usize, scheme: i32) -> bool {
    core::get_allowed(len, scheme)
}
fn make_chain(input: Vec<core::Server>, quic: bool, multi: bool) -> ffi::Chain {
    let valid = core::valid_chain(&input, quic, multi);
    ffi::Chain {
        valid,
        servers: if valid { input.into_iter().map(Into::into).collect() } else { Vec::new() },
    }
}
fn parse_chain(input: &[u8], scheme: i32, quic: bool, multi: bool, kind: u8) -> ffi::Chain {
    let result = match kind {
        0 => core::chain_uri(input, scheme, quic, &Services),
        1 => core::chain_pac(input, &Services),
        _ => core::chain_multi(input, scheme, quic, &Services),
    };
    // C++ Restore validates the retained value against the QUIC build flag;
    // `quic` here only controls explicit URI scheme recognition. In particular,
    // default-scheme QUIC does not depend on the caller's is_quic_allowed.
    make_chain(result, true, multi)
}
fn debug_chain(valid: bool, input: &[ffi::Server], has_opaque: bool, opaque: i32) -> Vec<u8> {
    if valid {
        for s in input {
            ffi::check_format_scheme(
                core::valid_serial_scheme(s.scheme) && s.scheme != core::INVALID,
            );
            ffi::check_host(&s.host, true);
        }
    }
    core::debug_chain(valid, &servers(input), has_opaque.then_some(opaque))
}
#[allow(clippy::too_many_arguments)] // CXX transports existing optional C++
                                     // fields.
fn compare_chain(
    a_valid: bool,
    a: &[ffi::Server],
    a_has_opaque: bool,
    a_opaque: i32,
    b_valid: bool,
    b: &[ffi::Server],
    b_has_opaque: bool,
    b_opaque: i32,
) -> i8 {
    core::compare_chain(
        a_valid.then_some(servers(a)).as_deref(),
        a_has_opaque.then_some(a_opaque),
        b_valid.then_some(servers(b)).as_deref(),
        b_has_opaque.then_some(b_opaque),
    ) as i8
}
fn read_server(mut reader: std::pin::Pin<&mut ffi::Reader>) -> ffi::Server {
    let scheme = ffi::read_int(reader.as_mut());
    let host = ffi::read_string(reader);
    let result = core::persisted_server(
        if scheme.ok { scheme.value } else { core::INVALID },
        &host,
        &Services,
    );
    if result.scheme == core::INVALID {
        ffi::check_invalid_record(&result.host, result.port);
    }
    result.into()
}
fn read_chain(mut reader: std::pin::Pin<&mut ffi::Reader>, quic: bool, multi: bool) -> ffi::Chain {
    if !ffi::read_int(reader.as_mut()).ok {
        return ffi::Chain { valid: false, servers: Vec::new() };
    }
    let len = ffi::read_length(reader.as_mut());
    if !len.ok || len.value < 0 {
        return ffi::Chain { valid: false, servers: Vec::new() };
    }
    let mut result = Vec::new();
    // Deliberately retain sequential primitive reads: failure advances the
    // PickleIterator exactly like the original, including partial input.
    for _ in 0..len.value {
        result.push(read_server(reader.as_mut()));
    }
    let result: Vec<_> = result.iter().map(|s| server(s.scheme, &s.host, s.port)).collect();
    make_chain(result, quic, multi)
}
fn persist_server(
    mut writer: std::pin::Pin<&mut ffi::Writer>,
    scheme: i32,
    host: &[u8],
    port: u16,
) {
    ffi::write_int(writer.as_mut(), scheme);
    ffi::check_host(host, false);
    ffi::write_string(writer, &core::host_port(host, port));
}
fn persist_chain(mut writer: std::pin::Pin<&mut ffi::Writer>, input: &[ffi::Server]) {
    ffi::write_int(writer.as_mut(), -1); // Removed IP protection ID, ABI
                                         // frozen.
    let len = core::persisted_length(input.len());
    ffi::write_int(writer.as_mut(), len);
    for s in input.iter().take(len as usize) {
        persist_server(writer.as_mut(), s.scheme, &s.host, s.port);
    }
}
