from pathlib import Path
p=Path('net/dns/dns_names_core.rs')
p.write_text('''// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

//! Reusable byte-oriented DNS policy with caller-owned bounded output.
#![forbid(unsafe_code)]

pub const MAX_NAME: usize = 255;
pub trait HostServices {
    fn compliant(&mut self, input: &[u8]) -> bool;
    fn localhost(&mut self, input: &[u8]) -> bool;
    fn ip_literal(&mut self, input: &[u8]) -> bool;
    fn url_address(&mut self, input: &[u8]) -> bool;
}

// A shared scan keeps validity and conversion acceptance identical, without
// allocating a wire name for callers which need only a validity result.
fn labels(input: &[u8], mut write: impl FnMut(usize, &[u8])) -> Option<usize> {
    let mut total = 0;
    let mut start = 0;
    while start < input.len() {
        let end = input[start..].iter().position(|&c| c == b'.')
            .map_or(input.len(), |offset| start + offset);
        let len = end - start;
        if len == 0 || len > 63 || total + len + 2 > MAX_NAME {
            return None;
        }
        write(total, &input[start..end]);
        total += len + 1;
        start = end + 1;
    }
    if total == 0 { None } else { Some(total + 1) }
}

pub fn dotted(input: &[u8], output: &mut [u8; MAX_NAME]) -> Option<usize> {
    let len = labels(input, |offset, label| {
        output[offset] = label.len() as u8;
        output[offset + 1..offset + 1 + label.len()].copy_from_slice(label);
    })?;
    output[len - 1] = 0;
    Some(len)
}

pub fn valid(input: &[u8]) -> bool {
    labels(input, |_, _| {}).is_some()
}

pub fn dotted_with_services(
    input: &[u8], require_hostname: bool, services: &mut impl HostServices,
    output: &mut [u8; MAX_NAME],
) -> Option<usize> {
    if require_hostname && !services.compliant(input) { return None; }
    dotted(input, output)
}

pub fn valid_record(input: &[u8], services: &mut impl HostServices) -> bool {
    valid(input) && !services.localhost(input) && !services.ip_literal(input)
        && !services.url_address(input)
}

/// Returns payload offset and length; failure consumes nothing.
pub fn length_prefixed(input: &[u8], wide: bool) -> Option<(usize, usize)> {
    let (prefix, len) = if wide {
        (2, usize::from(u16::from_be_bytes([*input.first()?, *input.get(1)?])))
    } else {
        (1, usize::from(*input.first()?))
    };
    (len <= input.len() - prefix).then_some((prefix, len))
}

/// `remaining` lasts until the next mutable reader call. `take` receives at
/// most 256 bytes, bounded by remaining length. `taken` exposes those bytes
/// after the cursor commit, so the cursor object may itself be input storage.
pub trait NameReader {
    fn remaining(&self) -> &[u8];
    fn take(&mut self, count: usize);
    fn taken(&self) -> &[u8];
}

struct SliceReader<'a> {
    remaining: &'a [u8],
    taken: &'a [u8],
}
impl NameReader for SliceReader<'_> {
    fn remaining(&self) -> &[u8] { self.remaining }
    fn take(&mut self, count: usize) {
        (self.taken, self.remaining) = self.remaining.split_at(count);
    }
    fn taken(&self) -> &[u8] { self.taken }
}

/// Contiguous parsing borrows only input and separate initialized output.
/// Caller commits the returned consumed length after both borrows have ended.
pub fn network(input: &[u8], complete: bool, output: &mut [u8; MAX_NAME])
    -> (usize, Option<usize>) {
    network_reader(&mut SliceReader { remaining: input, taken: &[] }, complete, output)
}

/// Completed reads consume bytes even on label/name limit failure. Compression
/// rejection and truncated reads leave the current label untouched.
pub fn network_reader(reader: &mut impl NameReader, complete: bool,
    output: &mut [u8; MAX_NAME]) -> (usize, Option<usize>) {
    let mut len = 0;
    let mut consumed = 0;
    let mut octets = 0;
    loop {
        let label_len = {
            let input = reader.remaining();
            if input.is_empty() { break; }
            if input[0] & 0xc0 == 0xc0 { return (consumed, None); }
            let Some((_, n)) = length_prefixed(input, false) else {
                return (consumed, None);
            };
            n
        };
        reader.take(label_len + 1);
        consumed += label_len + 1;
        if label_len != 0 { octets += label_len + 1; }
        if label_len > 63 || octets > MAX_NAME { return (consumed, None); }
        if label_len == 0 { return (consumed, Some(len)); }
        if len != 0 { output[len] = b'.'; len += 1; }
        output[len..len + label_len].copy_from_slice(&reader.taken()[1..label_len + 1]);
        len += label_len;
    }
    (consumed, (!complete).then_some(len))
}

pub fn less(lhs: &[u8], rhs: &[u8]) -> bool {
    for (&left, &right) in lhs.iter().zip(rhs) {
        if left != right {
            let left = left.to_ascii_lowercase();
            let right = right.to_ascii_lowercase();
            if left != right { return left < right; }
        }
    }
    lhs.len() < rhs.len()
}

pub trait Canonicalizer {
    // External URL service writes to its own C++ output; true means BROKEN.
    fn canonicalize(&mut self, input: &[u8]) -> bool;
    fn use_original(&mut self, input: &[u8]);
}
pub fn canonicalize(input: &[u8], service: &mut impl Canonicalizer) {
    if service.canonicalize(input) { service.use_original(input); }
}
''')
p=Path('net/dns/dns_names_ffi.rs')
p.write_text('''// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

mod dns_names_core;

#[cxx::bridge(namespace = "net::dns_names_internal")]
mod ffi {
    struct NameBuffer { bytes: [u8; 255] }
    struct NameResult { consumed: usize, len: usize, valid: bool }
    struct ReadResult { offset: usize, len: usize, valid: bool }
    struct LabelChunk { bytes: [u8; 256] }
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
        fn dotted_name(input: &[u8], require_hostname: bool, output: &mut NameBuffer) -> NameResult;
        fn valid_name(input: &[u8]) -> bool;
        fn valid_record(input: &[u8]) -> bool;
        fn network_name(input: &[u8], complete: bool, output: &mut NameBuffer) -> NameResult;
        fn network_name_reader(reader: Pin<&mut NameReader>, complete: bool,
            output: &mut NameBuffer) -> NameResult;
        fn read_length(input: &[u8], wide: bool) -> ReadResult;
        fn canonical_name(input: &[u8], output: Pin<&mut CanonicalName>);
    }
}

// Host services never retain or mutate input. Outputs below are initialized,
// separately owned objects. CXX generates shared-struct layout assertions.
struct Services;
impl dns_names_core::HostServices for Services {
    fn compliant(&mut self, input: &[u8]) -> bool { ffi::host_compliant(input) }
    fn localhost(&mut self, input: &[u8]) -> bool { ffi::host_localhost(input) }
    fn ip_literal(&mut self, input: &[u8]) -> bool { ffi::host_ip_literal(input) }
    fn url_address(&mut self, input: &[u8]) -> bool { ffi::host_url_address(input) }
}
fn result(consumed: usize, len: Option<usize>) -> ffi::NameResult {
    ffi::NameResult { consumed, valid: len.is_some(), len: len.unwrap_or_default() }
}
fn dotted_name(input: &[u8], require_hostname: bool, output: &mut ffi::NameBuffer) -> ffi::NameResult {
    result(0, dns_names_core::dotted_with_services(input, require_hostname, &mut Services,
        &mut output.bytes))
}
fn valid_name(input: &[u8]) -> bool { dns_names_core::valid(input) }
fn valid_record(input: &[u8]) -> bool { dns_names_core::valid_record(input, &mut Services) }
fn network_name(input: &[u8], complete: bool, output: &mut ffi::NameBuffer) -> ffi::NameResult {
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
    fn remaining(&self) -> &[u8] { ffi::reader_remaining(&self.reader) }
    fn take(&mut self, count: usize) {
        self.chunk = ffi::reader_take(self.reader.as_mut(), count);
    }
    fn taken(&self) -> &[u8] { &self.chunk.bytes }
}
fn network_name_reader(reader: std::pin::Pin<&mut ffi::NameReader>, complete: bool,
    output: &mut ffi::NameBuffer) -> ffi::NameResult {
    let mut reader = Reader { reader, chunk: ffi::LabelChunk { bytes: [0; 256] } };
    let (consumed, len) = dns_names_core::network_reader(&mut reader, complete, &mut output.bytes);
    result(consumed, len)
}
fn read_length(input: &[u8], wide: bool) -> ffi::ReadResult {
    match dns_names_core::length_prefixed(input, wide) {
        Some((offset, len)) => ffi::ReadResult { offset, len, valid: true },
        None => ffi::ReadResult { offset: 0, len: 0, valid: false },
    }
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
pub unsafe extern "C" fn net_dns_names_less(lhs: *const u8, lhs_len: usize,
    rhs: *const u8, rhs_len: usize) -> bool {
    let lhs = if lhs_len == 0 { &[] } else {
        // SAFETY: The private C++ caller supplies its live string_view range.
        unsafe { std::slice::from_raw_parts(lhs, lhs_len) }
    };
    let rhs = if rhs_len == 0 { &[] } else {
        // SAFETY: Same contract for rhs; the immutable ranges may overlap.
        unsafe { std::slice::from_raw_parts(rhs, rhs_len) }
    };
    dns_names_core::less(lhs, rhs)
}
''')
p=Path('net/dns/dns_names_ffi.h');s=p.read_text().replace('#include <cstdint>', '#include <cstddef>\n#include <cstdint>\n#include <string>');s=s.replace('struct CanonicalHostResult;\n','');s=s.replace('struct NameReader {', '''struct CanonicalName {
  STACK_ALLOCATED();
 public:
  const raw_ref<std::string> output;
};
// Input ranges are borrowed for the call, never retained or modified. Empty
// ranges may have null pointers. Only DomainNameLess calls this private ABI.
extern "C" bool net_dns_names_less(const uint8_t* lhs, size_t lhs_len,
                                    const uint8_t* rhs, size_t rhs_len);
struct NameReader {''');s=s.replace('CanonicalHostResult host_canonicalize(rust::Slice<const uint8_t> input);', '''bool host_canonicalize(rust::Slice<const uint8_t> input, CanonicalName& output);
void canonical_use_original(rust::Slice<const uint8_t> input, CanonicalName& output);''');p.write_text(s)
