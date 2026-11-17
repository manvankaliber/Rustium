// Copyright 2012 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/base/host_port_pair.h"

#include <optional>
#include <string_view>

#include "base/logging.h"
#include "base/numerics/safe_conversions.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "base/trace_event/memory_usage_estimator.h"
#include "base/values.h"
#include "net/base/host_port_rust_ffi.h"
#include "net/base/host_port_rust_ffi.rs.h"
#include "net/base/ip_endpoint.h"
#include "net/base/url_util.h"
#include "url/gurl.h"
#include "url/scheme_host_port.h"
#include "url/url_canon_ip.h"

namespace net {

namespace {
rust::Slice<const uint8_t> Bytes(std::string_view input) {
  // SAFETY: The input view remains live and unchanged throughout each call.
  return input.empty() ? rust::Slice<const uint8_t>()
                       : UNSAFE_BUFFERS(rust::Slice<const uint8_t>(
                             reinterpret_cast<const uint8_t*>(input.data()),
                             input.size()));
}
std::string AsString(const rust::Vec<uint8_t>& bytes) {
  // SAFETY: CXX vector storage is initialized and remains live during copy.
  return bytes.empty()
             ? std::string()
             : UNSAFE_BUFFERS(std::string(
                   reinterpret_cast<const char*>(bytes.data()), bytes.size()));
}
std::string Format(std::string_view host, uint16_t port, bool with_port) {
  auto result = host_port_rust::format(Bytes(host), port, with_port);
  if (result.has_null) {
    LOG(DFATAL) << "Host has a null char: " << AsString(result.diagnostic);
  }
  if (result.bad_brackets) {
    DCHECK_NE(host.front(), '[');
  }
  return AsString(result.value);
}
}  // namespace

namespace {

// Value dictionary keys
constexpr std::string_view kValueHostKey = "host";
constexpr std::string_view kValuePortKey = "port";

}  // namespace

HostPortPair::HostPortPair() : port_(0) {}

HostPortPair::HostPortPair(std::string_view in_host, uint16_t in_port)
    : port_(in_port), host_(in_host) {}

HostPortPair::HostPortPair(const char* in_host, uint16_t in_port)
    : HostPortPair(std::string_view(in_host), in_port) {}

HostPortPair::HostPortPair(std::string&& in_host, uint16_t in_port)
    : port_(in_port), host_(std::move(in_host)) {}

// static
HostPortPair HostPortPair::FromURL(const GURL& url) {
  return HostPortPair(url.HostNoBracketsPiece(),
                      static_cast<uint16_t>(url.EffectiveIntPort()));
}

// static
HostPortPair HostPortPair::FromSchemeHostPort(
    const url::SchemeHostPort& scheme_host_port) {
  DCHECK(scheme_host_port.IsValid());

  // HostPortPair assumes hostnames do not have surrounding brackets (as is
  // commonly used for IPv6 literals), so strip them if present.
  std::string_view host = scheme_host_port.host();
  if (host_port_rust::strip(Bytes(host))) {
    host = host.substr(1, host.size() - 2);
  }

  return HostPortPair(host, scheme_host_port.port());
}

// static
HostPortPair HostPortPair::FromIPEndPoint(const IPEndPoint& ipe) {
  return HostPortPair(ipe.ToStringWithoutPort(), ipe.port());
}

// static
HostPortPair HostPortPair::FromString(std::string_view str) {
  if (!host_port_rust::pair_allowed(Bytes(str))) {
    return HostPortPair();
  }

  std::string host;
  int port;
  if (!host_port_rust::ParseHostAndPort(str, &host, &port)) {
    return HostPortPair();
  }

  // Require a valid port.
  if (!host_port_rust::valid_port(port)) {
    return HostPortPair();
  }
  DCHECK(base::IsValueInRangeForNumericType<uint16_t>(port));

  return HostPortPair(host, port);
}

// static
std::optional<HostPortPair> HostPortPair::FromValue(const base::Value& value) {
  const base::DictValue* dict = value.GetIfDict();
  const std::string* host = dict ? dict->FindString(kValueHostKey) : nullptr;
  std::optional<int> port = dict ? dict->FindInt(kValuePortKey) : std::nullopt;
  if (!host_port_rust::value_valid(dict != nullptr, host != nullptr,
                                   port.has_value(), port.value_or(0))) {
    return std::nullopt;
  }

  return HostPortPair(*host, base::checked_cast<uint16_t>(port.value()));
}

bool HostPortPair::IsEmpty() const {
  return host_port_rust::empty(Bytes(host_), port_);
}
bool operator==(const HostPortPair& a, const HostPortPair& b) {
  return host_port_rust::compare(a.port(), Bytes(a.host()), b.port(),
                                 Bytes(b.host())) == 0;
}
std::strong_ordering operator<=>(const HostPortPair& a, const HostPortPair& b) {
  const int order = host_port_rust::compare(a.port(), Bytes(a.host()), b.port(),
                                            Bytes(b.host()));
  return order < 0   ? std::strong_ordering::less
         : order > 0 ? std::strong_ordering::greater
                     : std::strong_ordering::equal;
}
std::string HostPortPair::ToString() const {
  return Format(host_, port_, true);
}
std::string HostPortPair::HostForURL() const {
  return Format(host_, port_, false);
}

base::Value HostPortPair::ToValue() const {
  base::DictValue dict;
  dict.Set(kValueHostKey, host_);
  dict.Set(kValuePortKey, port_);

  return base::Value(std::move(dict));
}

}  // namespace net
