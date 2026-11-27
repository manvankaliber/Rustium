// Copyright 2012 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/base/host_port_rust_reference.h"

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
#include "net/base/ip_endpoint.h"
#include "net/base/url_util.h"
#include "url/gurl.h"
#include "url/scheme_host_port.h"
#include "url/url_canon.h"
#include "url/url_canon_ip.h"

namespace net {

bool HostPortRustOriginalParseHostAndPort(std::string_view input,
                                          std::string* host,
                                          int* port) {
  if (input.empty()) {
    return false;
  }

  url::Component auth_component(0, input.size());
  url::Component username_component;
  url::Component password_component;
  url::Component hostname_component;
  url::Component port_component;

  url::ParseAuthority(input, auth_component, url::ParserMode::kSpecialURL,
                      &username_component, &password_component,
                      &hostname_component, &port_component);

  // There shouldn't be a username/password.
  if (username_component.is_valid() || password_component.is_valid()) {
    return false;
  }

  if (hostname_component.is_empty()) {
    return false;  // Failed parsing.
  }

  int parsed_port_number = -1;
  if (port_component.is_nonempty()) {
    parsed_port_number = url::ParsePort(input, port_component);

    // If parsing failed, port_number will be either PORT_INVALID or
    // PORT_UNSPECIFIED, both of which are negative.
    if (parsed_port_number < 0) {
      return false;  // Failed parsing the port number.
    }
  }

  if (port_component.len == 0) {
    return false;  // Reject inputs like "foo:"
  }

  unsigned char tmp_ipv6_addr[16];

  // If the hostname starts with a bracket, it is either an IPv6 literal or
  // invalid. If it is an IPv6 literal then strip the brackets.
  if (hostname_component.len > 0 && input[hostname_component.begin] == '[') {
    if (input[hostname_component.end() - 1] == ']' &&
        url::IPv6AddressToNumber(hostname_component.AsViewOn(input),
                                 tmp_ipv6_addr)) {
      // Strip the brackets.
      hostname_component.begin++;
      hostname_component.len -= 2;
    } else {
      return false;
    }
  }

  // Pass results back to caller.
  *host = std::string(
      input.substr(hostname_component.begin, hostname_component.len));
  *port = parsed_port_number;

  return true;  // Success.
}

namespace {

// Value dictionary keys
constexpr std::string_view kValueHostKey = "host";
constexpr std::string_view kValuePortKey = "port";

}  // namespace

HostPortRustReference::HostPortRustReference() : port_(0) {}

HostPortRustReference::HostPortRustReference(std::string_view in_host,
                                             uint16_t in_port)
    : port_(in_port), host_(in_host) {}

HostPortRustReference::HostPortRustReference(const char* in_host,
                                             uint16_t in_port)
    : HostPortRustReference(std::string_view(in_host), in_port) {}

HostPortRustReference::HostPortRustReference(std::string&& in_host,
                                             uint16_t in_port)
    : port_(in_port), host_(std::move(in_host)) {}

// static
HostPortRustReference HostPortRustReference::FromURL(const GURL& url) {
  return HostPortRustReference(url.HostNoBracketsPiece(),
                               static_cast<uint16_t>(url.EffectiveIntPort()));
}

// static
HostPortRustReference HostPortRustReference::FromSchemeHostPort(
    const url::SchemeHostPort& scheme_host_port) {
  DCHECK(scheme_host_port.IsValid());

  // HostPortRustReference assumes hostnames do not have surrounding brackets
  // (as is commonly used for IPv6 literals), so strip them if present.
  std::string_view host = scheme_host_port.host();
  if (host.size() >= 2 && host.front() == '[' && host.back() == ']') {
    host = host.substr(1, host.size() - 2);
  }

  return HostPortRustReference(host, scheme_host_port.port());
}

// static
HostPortRustReference HostPortRustReference::FromIPEndPoint(
    const IPEndPoint& ipe) {
  return HostPortRustReference(ipe.ToStringWithoutPort(), ipe.port());
}

// static
HostPortRustReference HostPortRustReference::FromString(std::string_view str) {
  // Input with more than one ':' is ambiguous unless it contains an IPv6
  // literal (signified by starting with a '['). ParseHostAndPort() allows such
  // input and always uses the last ':' as the host/port delimiter, but because
  // HostPortRustReference often deals with IPv6 literals without brackets,
  // disallow such input here to prevent a common error.
  if (base::SplitStringPiece(str, ":", base::KEEP_WHITESPACE,
                             base::SPLIT_WANT_ALL)
              .size() > 2 &&
      str.front() != '[') {
    return HostPortRustReference();
  }

  std::string host;
  int port;
  if (!HostPortRustOriginalParseHostAndPort(str, &host, &port)) {
    return HostPortRustReference();
  }

  // Require a valid port.
  if (port == -1) {
    return HostPortRustReference();
  }
  DCHECK(base::IsValueInRangeForNumericType<uint16_t>(port));

  return HostPortRustReference(host, port);
}

// static
std::optional<HostPortRustReference> HostPortRustReference::FromValue(
    const base::Value& value) {
  const base::DictValue* dict = value.GetIfDict();
  if (!dict) {
    return std::nullopt;
  }

  const std::string* host = dict->FindString(kValueHostKey);
  std::optional<int> port = dict->FindInt(kValuePortKey);

  if (host == nullptr || !port.has_value() ||
      !base::IsValueInRangeForNumericType<uint16_t>(port.value())) {
    return std::nullopt;
  }

  return HostPortRustReference(*host,
                               base::checked_cast<uint16_t>(port.value()));
}

std::string HostPortRustReference::ToString() const {
  std::string ret(HostForURL());
  ret += ':';
  ret += base::NumberToString(port_);
  return ret;
}

std::string HostPortRustReference::HostForURL() const {
  // TODO(rtenneti): Add support for |host| to have '\0'.
  if (host_.find('\0') != std::string::npos) {
    std::string host_for_log(host_);
    size_t nullpos;
    while ((nullpos = host_for_log.find('\0')) != std::string::npos) {
      host_for_log.replace(nullpos, 1, "%00");
    }
    LOG(DFATAL) << "Host has a null char: " << host_for_log;
  }
  // Check to see if the host is an IPv6 address.  If so, added brackets.
  if (host_.find(':') != std::string::npos) {
    DCHECK_NE(host_[0], '[');
    return base::StringPrintf("[%s]", host_.c_str());
  }

  return host_;
}

base::Value HostPortRustReference::ToValue() const {
  base::DictValue dict;
  dict.Set(kValueHostKey, host_);
  dict.Set(kValuePortKey, port_);

  return base::Value(std::move(dict));
}

}  // namespace net
