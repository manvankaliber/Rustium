// Copyright 2022 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/dns/dns_names_reference.h"

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "base/check.h"
#include "base/containers/span.h"
#include "base/containers/span_reader.h"
#include "base/strings/string_view_util.h"
#include "net/base/ip_address.h"
#include "net/base/url_util.h"
#include "net/dns/public/dns_protocol.h"
#include "url/third_party/mozilla/url_parse.h"
#include "url/url_canon.h"
#include "url/url_canon_stdstring.h"

namespace net::dns_names_reference {
namespace {
// Frozen dependency policy. Only URL's parser/canonicalizer remains external.
bool IsHostCharAlphanumeric(char c) {
  // We can just check lowercase because uppercase characters have already been
  // normalized.
  return ((c >= 'a') && (c <= 'z')) || ((c >= '0') && (c <= '9'));
}
bool IsCanonicalizedHostCompliant(std::string_view host) {
  if (host.empty() || host.size() > 254 ||
      (host.back() != '.' && host.size() == 254)) {
    return false;
  }

  bool in_component = false;
  bool most_recent_component_started_alphanumeric = false;
  size_t label_size = 0;

  for (char c : host) {
    ++label_size;
    if (!in_component) {
      most_recent_component_started_alphanumeric = IsHostCharAlphanumeric(c);
      if (!most_recent_component_started_alphanumeric && (c != '-') &&
          (c != '_')) {
        return false;
      }
      in_component = true;
    } else if (c == '.') {
      in_component = false;
      if (label_size > 64 || label_size == 1) {
        // Label should not be empty or longer than 63 characters (+1 for '.'
        // character included in `label_size`).
        return false;
      } else {
        label_size = 0;
      }
    } else if (!IsHostCharAlphanumeric(c) && (c != '-') && (c != '_')) {
      return false;
    }
  }

  // Check for too-long label when not ended with final '.'.
  if (label_size > 63) {
    return false;
  }

  return most_recent_component_started_alphanumeric;
}
constexpr bool ParseIPLiteralToBytes(std::string_view ip_literal,
                                     IPAddressBytes* bytes) {
  // |ip_literal| could be either an IPv4 or an IPv6 literal. If it contains
  // a colon however, it must be an IPv6 address.
  if (ip_literal.find(':') != std::string_view::npos) {
    // GURL expects IPv6 hostnames to be surrounded with brackets.
    // Not using base::StrCat() because it is not constexpr.
    std::string host_with_brackets;
    host_with_brackets.reserve(ip_literal.size() + 2);
    host_with_brackets.push_back('[');
    host_with_brackets.append(ip_literal);
    host_with_brackets.push_back(']');

    // Try parsing the hostname as an IPv6 literal.
    bytes->Resize(16);  // 128 bits.
    return url::IPv6AddressToNumber(host_with_brackets, bytes->span());
  }

  // Otherwise the string is an IPv4 address.
  bytes->Resize(4);  // 32 bits.
  int num_components;
  url::CanonHostInfo::Family family =
      url::IPv4AddressToNumber(ip_literal, bytes->span(), &num_components);
  return family == url::CanonHostInfo::IPV4;
}
class FrozenIPAddress {
 public:
  bool AssignFromIPLiteral(std::string_view text) {
    bool ok = ParseIPLiteralToBytes(text, &bytes_);
    if (!ok) {
      bytes_.Resize(0);
    }
    return ok;
  }
  bool IsIPv4() const { return bytes_.size() == 4; }
  bool IsIPv6() const { return bytes_.size() == 16; }
  bool IsLoopback() const {
    if (IsIPv4()) {
      return bytes_[0] == 127;
    }
    if (IsIPv6()) {
      for (size_t i = 0; i + 1 < bytes_.size(); ++i) {
        if (bytes_[i] != 0) {
          return false;
        }
      }
      return bytes_.back() == 1;
    }
    return false;
  }

 private:
  IPAddressBytes bytes_;
};
bool IsNormalizedLocalhostTLD(std::string_view host) {
  return base::EndsWith(host, ".localhost",
                        base::CompareCase::INSENSITIVE_ASCII);
}
bool IsLocalHostname(std::string_view host) {
  // Remove any trailing '.'.
  if (!host.empty() && *host.rbegin() == '.') {
    host.remove_suffix(1);
  }

  return base::EqualsCaseInsensitiveASCII(host, "localhost") ||
         IsNormalizedLocalhostTLD(host);
}
bool HostStringIsLocalhost(std::string_view host) {
  FrozenIPAddress ip_address;
  if (ip_address.AssignFromIPLiteral(host)) {
    return ip_address.IsLoopback();
  }
  return IsLocalHostname(host);
}
bool ParseURLHostnameToAddress(std::string_view hostname,
                               FrozenIPAddress* ip_address) {
  if (hostname.size() >= 2 && hostname.front() == '[' &&
      hostname.back() == ']') {
    // Strip the square brackets that surround IPv6 literals.
    auto ip_literal = std::string_view(hostname).substr(1, hostname.size() - 2);
    return ip_address->AssignFromIPLiteral(ip_literal) && ip_address->IsIPv6();
  }

  return ip_address->AssignFromIPLiteral(hostname) && ip_address->IsIPv4();
}
}  // namespace

bool IsValidDnsName(std::string_view dotted_form_name) {
  return DottedNameToNetwork(dotted_form_name,
                             /*require_valid_internet_hostname=*/false)
      .has_value();
}

bool IsValidDnsRecordName(std::string_view dotted_form_name) {
  FrozenIPAddress ip_address;
  return IsValidDnsName(dotted_form_name) &&
         !HostStringIsLocalhost(dotted_form_name) &&
         !ip_address.AssignFromIPLiteral(dotted_form_name) &&
         !ParseURLHostnameToAddress(dotted_form_name, &ip_address);
}

// Based on DJB's public domain code.
std::optional<std::vector<uint8_t>> DottedNameToNetwork(
    std::string_view dotted_form_name,
    bool require_valid_internet_hostname) {
  // Use full IsCanonicalizedHostCompliant() validation if not
  // `is_unrestricted`. All subsequent validity checks should not apply unless
  // `is_unrestricted` because IsCanonicalizedHostCompliant() is expected to be
  // more strict than any validation here.
  if (require_valid_internet_hostname &&
      !IsCanonicalizedHostCompliant(dotted_form_name)) {
    return std::nullopt;
  }

  std::vector<uint8_t> name;
  name.reserve(dns_protocol::kMaxNameLength);

  auto iter = dotted_form_name.begin();
  while (iter != dotted_form_name.end()) {
    auto pos = std::find(iter, dotted_form_name.end(), '.');
    size_t labellen = std::distance(iter, pos);
    // Don't allow empty labels per http://crbug.com/456391.
    if (!labellen) {
      DCHECK(!require_valid_internet_hostname);
      return std::nullopt;
    }
    // `2` includes the length byte and the terminating '\0' byte.
    if (name.size() + labellen + 2 > dns_protocol::kMaxNameLength ||
        labellen > dns_protocol::kMaxLabelLength) {
      DCHECK(!require_valid_internet_hostname);
      return std::nullopt;
    }
    // This cast is safe because kMaxLabelLength < 255.
    name.push_back(static_cast<uint8_t>(labellen));
    name.insert(name.end(), iter, pos);
    if (pos == dotted_form_name.end()) {
      break;
    }
    iter = pos + 1;
  }

  if (name.empty()) {  // Empty names e.g. "", "." are not valid.
    DCHECK(!require_valid_internet_hostname);
    return std::nullopt;
  }
  name.push_back(0);  // This is the root label (of length 0).

  return name;
}

std::optional<std::string> NetworkToDottedName(base::span<const uint8_t> span,
                                               bool require_complete) {
  auto reader = base::SpanReader(span);
  return NetworkToDottedName(reader, require_complete);
}

std::optional<std::string> NetworkToDottedName(
    base::SpanReader<const uint8_t>& reader,
    bool require_complete) {
  std::string ret;
  size_t octets_read = 0u;
  while (reader.remaining() > 0u) {
    // DNS name compression not allowed because it does not make sense without
    // the context of a full DNS message.
    if ((reader.remaining_span()[0u] & dns_protocol::kLabelMask) ==
        dns_protocol::kLabelPointer) {
      return std::nullopt;
    }

    base::span<const uint8_t> label;
    if (!ReadU8LengthPrefixed(reader, &label)) {
      return std::nullopt;
    }

    // Final zero-length label not included in size enforcement.
    if (!label.empty()) {
      octets_read += label.size() + 1u;
    }

    if (label.size() > dns_protocol::kMaxLabelLength) {
      return std::nullopt;
    }
    if (octets_read > dns_protocol::kMaxNameLength) {
      return std::nullopt;
    }

    if (label.empty()) {
      return ret;
    }

    if (!ret.empty()) {
      ret.append(".");
    }

    ret.append(base::as_string_view(label));
  }

  if (require_complete) {
    return std::nullopt;
  }

  // If terminating zero-length label was not included in the input, no need to
  // recheck against max name length because terminating zero-length label does
  // not count against the limit.

  return ret;
}

bool ReadU8LengthPrefixed(base::SpanReader<const uint8_t>& reader,
                          base::span<const uint8_t>* out) {
  base::SpanReader<const uint8_t> inner_reader = reader;
  uint8_t len;
  if (!inner_reader.ReadU8BigEndian(len)) {
    return false;
  }
  std::optional<base::span<const uint8_t>> bytes = inner_reader.Read(len);
  if (!bytes) {
    return false;
  }
  *out = *bytes;
  reader = inner_reader;
  return true;
}

bool ReadU16LengthPrefixed(base::SpanReader<const uint8_t>& reader,
                           base::span<const uint8_t>* out) {
  base::SpanReader<const uint8_t> inner_reader = reader;
  uint16_t len;
  if (!inner_reader.ReadU16BigEndian(len)) {
    return false;
  }
  std::optional<base::span<const uint8_t>> bytes = inner_reader.Read(len);
  if (!bytes) {
    return false;
  }
  *out = *bytes;
  reader = inner_reader;
  return true;
}

std::string UrlCanonicalizeNameIfAble(std::string_view name) {
  std::string canonicalized;
  url::StdStringCanonOutput output(&canonicalized);
  url::CanonHostInfo host_info;
  url::CanonicalizeHostVerbose(name, url::Component(0, name.size()), &output,
                               &host_info);

  if (host_info.family == url::CanonHostInfo::Family::BROKEN) {
    return std::string(name);
  }

  output.Complete();
  return canonicalized;
}

}  // namespace net::dns_names_reference
