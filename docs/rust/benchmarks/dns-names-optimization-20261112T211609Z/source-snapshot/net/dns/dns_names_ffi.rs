// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

pub mod dns_names_core;

#[cxx::bridge(namespace = "net::dns_names_internal")]
mod ffi {
    struct NameBuffer {
        bytes: [u8; 255],
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
        type CanonicalName;
        fn host_canonicalize(input: &[u8], output: Pin<&mut CanonicalName>) -> bool;
        fn canonical_use_original(input: &[u8], output: Pin<&mut CanonicalName>);
        type NameReader;
        fn reader_remaining(reader: &NameReader) -> &[u8];
        fn reader_take(reader: Pin<&mut NameReader>, count: usize) -> LabelChunk;
    }
    extern "Rust" {
        fn dotted_name(input: &[u8], require_hostname: bool, output: &mut NameBuffer) -> u64;
        fn valid_name(input: &[u8]) -> bool;
        fn valid_record(input: &[u8]) -> bool;
        fn network_name(input: &[u8], complete: bool, output: &mut NameBuffer) -> u64;
        fn network_name_reader(
            reader: Pin<&mut NameReader>,
            complete: bool,
            output: &mut NameBuffer,
        ) -> u64;
        fn read_length(input: &[u8], wide: bool) -> u32;
        fn canonical_name(input: &[u8], output: Pin<&mut CanonicalName>);
    }
}

// Host services never retain or mutate input. Outputs below are initialized,
// separately owned objects. CXX generates shared-struct layout assertions.
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
}
// Scalar return metadata avoids an owned aggregate or hidden result pointer.
// The low word is length + 1 (zero means invalid); the high word is consumed.
// DNS parsing stops after at most 255 valid octets plus one <=192-byte read,
// so consumed always fits the high word. Output lengths are at most 255.
fn result(consumed: usize, len: Option<usize>) -> u64 {
    ((consumed as u64) << 32) | len.map_or(0, |n| n as u64 + 1)
}
fn dotted_name(input: &[u8], require_hostname: bool, output: &mut ffi::NameBuffer) -> u64 {
    result(
        0,
        dns_names_core::dotted_with_services(
            input,
            require_hostname,
            &mut Services,
            &mut output.bytes,
        ),
    )
}
fn valid_name(input: &[u8]) -> bool {
    dns_names_core::valid(input)
}
fn valid_record(input: &[u8]) -> bool {
    dns_names_core::valid_record(input, &mut Services)
}
fn network_name(input: &[u8], complete: bool, output: &mut ffi::NameBuffer) -> u64 {
    let (consumed, len) = dns_names_core::network(input, complete, &mut output.bytes);
    result(consumed, len)
}
struct Reader<'a> {
    reader: std::pin::Pin<&'a mut ffi::NameReader>,
    chunk: ffi::LabelChunk,
}
impl dns_names_core::NameReader for Reader<'_> {
    // This borrow ends before `take` mutates the C++ cursor. The latter's
    // representation can itself be input, hence the post-commit owned chunk.
    fn remaining(&self) -> &[u8] {
        ffi::reader_remaining(&self.reader)
    }
    fn take(&mut self, count: usize) {
        self.chunk = ffi::reader_take(self.reader.as_mut(), count);
    }
    fn taken(&self) -> &[u8] {
        &self.chunk.bytes
    }
}
fn network_name_reader(
    reader: std::pin::Pin<&mut ffi::NameReader>,
    complete: bool,
    output: &mut ffi::NameBuffer,
) -> u64 {
    let mut reader = Reader { reader, chunk: ffi::LabelChunk { bytes: [0; 256] } };
    let (consumed, len) = dns_names_core::network_reader(&mut reader, complete, &mut output.bytes);
    result(consumed, len)
}
fn read_length(input: &[u8], wide: bool) -> u32 {
    // Prefix width is 1 or 2; payload lengths fit 16 bits. Zero is failure.
    dns_names_core::length_prefixed(input, wide)
        .map_or(0, |(offset, len)| ((len as u32) << 2) | offset as u32)
}
struct Canonical<'a>(std::pin::Pin<&'a mut ffi::CanonicalName>);
impl dns_names_core::Canonicalizer for Canonical<'_> {
    fn canonicalize(&mut self, input: &[u8]) -> bool {
        ffi::host_canonicalize(input, self.0.as_mut())
    }
    fn use_original(&mut self, input: &[u8]) {
        ffi::canonical_use_original(input, self.0.as_mut());
    }
}
fn canonical_name(input: &[u8], output: std::pin::Pin<&mut ffi::CanonicalName>) {
    dns_names_core::canonicalize(input, &mut Canonical(output));
}

/// Private scalar ABI avoids constructing CXX slice objects on the tiny public
/// comparator path. The exported C++ DomainNameLess remains the consumer API.
///
/// # Safety
/// Each nonempty range must point to initialized readable bytes for this call,
/// with length <= isize::MAX. Null/dangling pointers are allowed for empty
/// ranges. Both inputs may alias; neither is mutated or retained.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn net_dns_names_less(
    lhs: *const u8,
    lhs_len: usize,
    rhs: *const u8,
    rhs_len: usize,
) -> bool {
    let lhs = if lhs_len == 0 {
        &[]
    } else {
        // SAFETY: The private C++ caller supplies its live string_view range.
        unsafe { std::slice::from_raw_parts(lhs, lhs_len) }
    };
    let rhs = if rhs_len == 0 {
        &[]
    } else {
        // SAFETY: Same contract for rhs; the immutable ranges may overlap.
        unsafe { std::slice::from_raw_parts(rhs, rhs_len) }
    };
    dns_names_core::less(lhs, rhs)
}
