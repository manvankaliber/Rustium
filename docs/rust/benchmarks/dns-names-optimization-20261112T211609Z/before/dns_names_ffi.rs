// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

mod dns_names_core;

#[cxx::bridge(namespace = "net::dns_names_internal")]
mod ffi {
    struct NameResult {
        bytes: Vec<u8>,
        consumed: usize,
        valid: bool,
    }
    struct ReadResult {
        offset: usize,
        len: usize,
        valid: bool,
    }
    struct CanonicalHostResult {
        bytes: Vec<u8>,
        broken: bool,
    }
    struct LabelChunk {
        bytes: [u8; 256],
    }
    unsafe extern "C++" {
        include!("net/dns/dns_names_ffi.h");
        fn host_compliant(input: &[u8]) -> bool;
        fn host_localhost(input: &[u8]) -> bool;
        fn host_ip_literal(input: &[u8]) -> bool;
        fn host_url_address(input: &[u8]) -> bool;
        fn host_canonicalize(input: &[u8]) -> CanonicalHostResult;
        type NameReader;
        fn reader_remaining(reader: &NameReader) -> &[u8];
        fn reader_take(reader: Pin<&mut NameReader>, count: usize) -> LabelChunk;
    }
    extern "Rust" {
        fn dotted_name(input: &[u8], require_hostname: bool) -> NameResult;
        fn valid_name(input: &[u8]) -> bool;
        fn valid_record(input: &[u8]) -> bool;
        fn network_name(reader: Pin<&mut NameReader>, complete: bool) -> NameResult;
        fn read_length(input: &[u8], wide: bool) -> ReadResult;
        fn name_less(lhs: &[u8], rhs: &[u8]) -> bool;
        fn canonical_name(input: &[u8]) -> Vec<u8>;
    }
}

// Host-service callbacks borrow input only for their call. C++ returns owned
// bytes and never writes or retains this input. No foreign output pointers or
// references to uninitialized objects cross this bridge. CXX generates layout
// assertions for shared structs; its generated glue is the only unsafe code.
struct Services;
impl dns_names_core::HostServices for Services {
    fn compliant(&mut self, input: &[u8]) -> bool {
        ffi::host_compliant(input)
    }
    fn localhost(&mut self, input: &[u8]) -> bool {
        ffi::host_localhost(input)
    }
    fn ip_literal(&mut self, input: &[u8]) -> bool {
        ffi::host_ip_literal(input)
    }
    fn url_address(&mut self, input: &[u8]) -> bool {
        ffi::host_url_address(input)
    }
    fn canonicalize(&mut self, input: &[u8]) -> (bool, Vec<u8>) {
        let result = ffi::host_canonicalize(input);
        (result.broken, result.bytes)
    }
}
fn result(consumed: usize, bytes: Option<Vec<u8>>) -> ffi::NameResult {
    ffi::NameResult { consumed, valid: bytes.is_some(), bytes: bytes.unwrap_or_default() }
}
fn dotted_name(input: &[u8], require_hostname: bool) -> ffi::NameResult {
    result(0, dns_names_core::dotted_with_services(input, require_hostname, &mut Services))
}
fn valid_name(input: &[u8]) -> bool {
    dns_names_core::dotted(input).is_some()
}
fn valid_record(input: &[u8]) -> bool {
    dns_names_core::valid_record(input, &mut Services)
}
struct Reader<'a>(std::pin::Pin<&'a mut ffi::NameReader>);
impl dns_names_core::NameReader for Reader<'_> {
    // This borrow ends before `take` mutates the C++ cursor, whose object
    // representation may itself be part of the input.
    fn remaining(&self) -> &[u8] {
        ffi::reader_remaining(&self.0)
    }
    fn take(&mut self, count: usize) -> [u8; 256] {
        ffi::reader_take(self.0.as_mut(), count).bytes
    }
}
fn network_name(reader: std::pin::Pin<&mut ffi::NameReader>, complete: bool) -> ffi::NameResult {
    let (consumed, bytes) = dns_names_core::network(&mut Reader(reader), complete);
    result(consumed, bytes)
}
fn read_length(input: &[u8], wide: bool) -> ffi::ReadResult {
    match dns_names_core::length_prefixed(input, wide) {
        Some((offset, len)) => ffi::ReadResult { offset, len, valid: true },
        None => ffi::ReadResult { offset: 0, len: 0, valid: false },
    }
}
fn name_less(lhs: &[u8], rhs: &[u8]) -> bool {
    dns_names_core::less(lhs, rhs)
}
fn canonical_name(input: &[u8]) -> Vec<u8> {
    dns_names_core::canonicalize(input, &mut Services)
}
