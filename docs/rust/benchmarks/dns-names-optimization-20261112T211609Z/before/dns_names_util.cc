// Copyright 2022 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/dns/dns_names_util.h"

#include "base/check.h"
#include "base/compiler_specific.h"
#include "base/containers/span_rust.h"
#include "base/strings/string_view_util.h"
#include "net/base/ip_address.h"
#include "net/base/url_util.h"
#include "net/dns/dns_names_ffi.rs.h"
#include "url/third_party/mozilla/url_parse.h"
#include "url/url_canon.h"
#include "url/url_canon_stdstring.h"

namespace net::dns_names_internal {
namespace {
std::string_view View(rust::Slice<const uint8_t> input) {
  if (input.empty()) {
    return {};
  }
  // SAFETY: A nonempty CXX slice borrows this initialized range for the call.
  // Empty slices are handled above because their data pointer may be dangling.
  return base::as_string_view(
      UNSAFE_BUFFERS(base::span(input.data(), input.size())));
}
}  // namespace

bool host_compliant(rust::Slice<const uint8_t> input) {
  return IsCanonicalizedHostCompliant(View(input));
}
bool host_localhost(rust::Slice<const uint8_t> input) {
  return HostStringIsLocalhost(View(input));
}
bool host_ip_literal(rust::Slice<const uint8_t> input) {
  IPAddress address;
  return address.AssignFromIPLiteral(View(input));
}
bool host_url_address(rust::Slice<const uint8_t> input) {
  IPAddress address;
  return ParseURLHostnameToAddress(View(input), &address);
}
rust::Slice<const uint8_t> reader_remaining(const NameReader& context) {
  return base::SpanToRustSlice(context.reader->remaining_span());
}
LabelChunk reader_take(NameReader& context, size_t count) {
  auto input = context.reader->remaining_span().first(count);
  auto inner = *context.reader;
  CHECK(inner.Skip(count));
  *context.reader = inner;
  LabelChunk result{};
  base::span(result.bytes).first(count).copy_from(input);
  return result;
}
CanonicalHostResult host_canonicalize(rust::Slice<const uint8_t> input) {
  // Only URL canonicalization lives here. Rust chooses whether to use its
  // result or preserve the original bytes for a BROKEN host.
  std::string canonicalized;
  url::StdStringCanonOutput output(&canonicalized);
  url::CanonHostInfo host_info;
  url::CanonicalizeHostVerbose(View(input), url::Component(0, input.size()),
                               &output, &host_info);
  output.Complete();
  CanonicalHostResult result;
  result.broken = host_info.family == url::CanonHostInfo::Family::BROKEN;
  result.bytes.reserve(canonicalized.size());
  for (uint8_t byte : canonicalized) {
    result.bytes.push_back(byte);
  }
  return result;
}
}  // namespace net::dns_names_internal

namespace net::dns_names_util {
namespace {
rust::Slice<const uint8_t> Bytes(std::string_view input) {
  return base::SpanToRustSlice(base::as_byte_span(input));
}
}  // namespace

bool IsValidDnsName(std::string_view input) {
  return dns_names_internal::valid_name(Bytes(input));
}
bool IsValidDnsRecordName(std::string_view input) {
  return dns_names_internal::valid_record(Bytes(input));
}
std::optional<std::vector<uint8_t>> DottedNameToNetwork(
    std::string_view input,
    bool require_valid_internet_hostname) {
  auto result = dns_names_internal::dotted_name(
      Bytes(input), require_valid_internet_hostname);
  if (!result.valid) {
    return std::nullopt;
  }
  return std::vector<uint8_t>(result.bytes.begin(), result.bytes.end());
}
std::optional<std::string> NetworkToDottedName(base::span<const uint8_t> input,
                                               bool require_complete) {
  auto reader = base::SpanReader(input);
  return NetworkToDottedName(reader, require_complete);
}
std::optional<std::string> NetworkToDottedName(
    base::SpanReader<const uint8_t>& reader,
    bool require_complete) {
  // Rust owns the decoding loop. Each input borrow ends before the callback
  // commits a cursor change. This also preserves cursor/input object aliases.
  dns_names_internal::NameReader context{raw_ref(reader)};
  auto result = dns_names_internal::network_name(context, require_complete);
  if (!result.valid) {
    return std::nullopt;
  }
  return std::string(result.bytes.begin(), result.bytes.end());
}
namespace {
bool ReadLengthPrefixed(base::SpanReader<const uint8_t>& reader,
                        base::span<const uint8_t>* out,
                        bool wide) {
  const auto input = reader.remaining_span();
  const auto result =
      dns_names_internal::read_length(base::SpanToRustSlice(input), wide);
  if (!result.valid) {
    return false;
  }
  // Match the old output-before-reader commit order. Neither the output nor
  // reader is exposed to Rust, so uninitialized output spans are supported.
  auto inner_reader = reader;
  CHECK(inner_reader.Skip(result.offset + result.len));
  *out = input.subspan(result.offset, result.len);
  reader = inner_reader;
  return true;
}
}  // namespace
bool ReadU8LengthPrefixed(base::SpanReader<const uint8_t>& reader,
                          base::span<const uint8_t>* out) {
  return ReadLengthPrefixed(reader, out, false);
}
bool ReadU16LengthPrefixed(base::SpanReader<const uint8_t>& reader,
                           base::span<const uint8_t>* out) {
  return ReadLengthPrefixed(reader, out, true);
}
std::string UrlCanonicalizeNameIfAble(std::string_view input) {
  auto result = dns_names_internal::canonical_name(Bytes(input));
  return std::string(result.begin(), result.end());
}
bool DomainNameLess(std::string_view lhs, std::string_view rhs) {
  return dns_names_internal::name_less(Bytes(lhs), Bytes(rhs));
}
}  // namespace net::dns_names_util
