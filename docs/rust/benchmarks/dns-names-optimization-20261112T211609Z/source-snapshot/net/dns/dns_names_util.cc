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
bool host_canonicalize(rust::Slice<const uint8_t> input,
                       CanonicalName& context) {
  // URL/ICU writes directly into the eventual C++ result. Rust owns the
  // decision to keep this result or request original-byte fallback.
  url::StdStringCanonOutput output(&*context.output);
  url::CanonHostInfo host_info;
  url::CanonicalizeHostVerbose(View(input), url::Component(0, input.size()),
                               &output, &host_info);
  const bool broken = host_info.family == url::CanonHostInfo::Family::BROKEN;
  if (!broken) {
    output.Complete();
  }
  return broken;
}
void canonical_use_original(rust::Slice<const uint8_t> input,
                            CanonicalName& context) {
  context.output->assign(View(input));
}
}  // namespace net::dns_names_internal

namespace net::dns_names_util {
namespace {
rust::Slice<const uint8_t> Bytes(std::string_view input) {
  // Byte reinterpretation only; CXX handles the null-empty slice convention.
  // Avoid a separate std::string_view -> span adapter on this call path.
  return {reinterpret_cast<const uint8_t*>(input.data()), input.size()};
}
struct NameResult {
  size_t consumed;
  size_t len;
  bool valid;
};
NameResult DecodeResult(uint64_t value) {
  const uint32_t length_plus_one = static_cast<uint32_t>(value);
  return {static_cast<size_t>(value >> 32),
          length_plus_one ? length_plus_one - 1u : 0u, length_plus_one != 0};
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
  dns_names_internal::NameBuffer output{};
  const auto result = DecodeResult(dns_names_internal::dotted_name(
      Bytes(input), require_valid_internet_hostname, output));
  if (!result.valid) {
    return std::nullopt;
  }
  auto bytes = base::span(output.bytes).first(result.len);
  return std::vector<uint8_t>(bytes.begin(), bytes.end());
}
std::optional<std::string> NetworkToDottedName(base::span<const uint8_t> input,
                                               bool require_complete) {
  dns_names_internal::NameBuffer output{};
  const auto result = DecodeResult(dns_names_internal::network_name(
      base::SpanToRustSlice(input), require_complete, output));
  if (!result.valid) {
    return std::nullopt;
  }
  return std::string(
      base::as_string_view(base::span(output.bytes).first(result.len)));
}
std::optional<std::string> NetworkToDottedName(
    base::SpanReader<const uint8_t>& reader,
    bool require_complete) {
  dns_names_internal::NameBuffer output{};
  NameResult result;
  const auto input = reader.remaining_span();
  const auto start = reinterpret_cast<uintptr_t>(input.data());
  const auto cursor = reinterpret_cast<uintptr_t>(&reader);
  // Only an ownership overlap check lives here, never DNS parsing. Subtraction
  // avoids endpoint overflow. Rust never sees the private SpanReader layout.
  const bool aliases_cursor =
      !input.empty() && (start <= cursor ? cursor - start < input.size()
                                         : start - cursor < sizeof(reader));
  if (aliases_cursor) {
    // Every input borrow ends before the cursor callback mutates its storage;
    // payload bytes are copied after that commit, matching the original.
    dns_names_internal::NameReader context{raw_ref(reader)};
    result = DecodeResult(dns_names_internal::network_name_reader(
        context, require_complete, output));
  } else {
    result = DecodeResult(dns_names_internal::network_name(
        base::SpanToRustSlice(input), require_complete, output));
    CHECK(reader.Skip(result.consumed));
  }
  if (!result.valid) {
    return std::nullopt;
  }
  return std::string(
      base::as_string_view(base::span(output.bytes).first(result.len)));
}
namespace {
bool ReadLengthPrefixed(base::SpanReader<const uint8_t>& reader,
                        base::span<const uint8_t>* out,
                        bool wide) {
  const auto input = reader.remaining_span();
  const auto result =
      dns_names_internal::read_length(base::SpanToRustSlice(input), wide);
  if (!result) {
    return false;
  }
  const size_t offset = result & 3u;
  const size_t len = result >> 2;
  // Match the old output-before-reader commit order. Neither the output nor
  // reader is exposed to Rust, so uninitialized output spans are supported.
  auto inner_reader = reader;
  CHECK(inner_reader.Skip(offset + len));
  *out = input.subspan(offset, len);
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
  std::string result;
  dns_names_internal::CanonicalName context{raw_ref(result)};
  dns_names_internal::canonical_name(Bytes(input), context);
  return result;
}
bool DomainNameLess(std::string_view lhs, std::string_view rhs) {
  return dns_names_internal::net_dns_names_less(
      reinterpret_cast<const uint8_t*>(lhs.data()), lhs.size(),
      reinterpret_cast<const uint8_t*>(rhs.data()), rhs.size());
}
}  // namespace net::dns_names_util
