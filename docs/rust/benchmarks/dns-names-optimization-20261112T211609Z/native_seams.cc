#include "net/dns/public/dns_protocol.h"
#include "base/numerics/byte_conversions.h"
#include "net/base/url_util.h"
#include "net/base/ip_address.h"
#include "net/dns/dns_util.h"
#include "base/strings/string_util.h"
namespace net {
bool IsHostCharAlphanumeric(char c) {
  // We can just check lowercase because uppercase characters have already been
  // normalized.
  return ((c >= 'a') && (c <= 'z')) || ((c >= '0') && (c <= '9'));
}
bool IsNormalizedLocalhostTLD(std::string_view host) {
  return base::EndsWith(host, ".localhost",
                        base::CompareCase::INSENSITIVE_ASCII);
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
  if (label_size > 63)
    return false;

  return most_recent_component_started_alphanumeric;
}
bool HostStringIsLocalhost(std::string_view host) {
  IPAddress ip_address;
  if (ip_address.AssignFromIPLiteral(host))
    return ip_address.IsLoopback();
  return IsLocalHostname(host);
}
bool IsLocalHostname(std::string_view host) {
  // Remove any trailing '.'.
  if (!host.empty() && *host.rbegin() == '.')
    host.remove_suffix(1);

  return base::EqualsCaseInsensitiveASCII(host, "localhost") ||
         IsNormalizedLocalhostTLD(host);
}
bool IPAddress::IsLoopback() const {
  // 127.0.0.1/8
  if (IsIPv4())
    return ip_address_[0] == 127;

  // ::1
  if (IsIPv6()) {
    for (size_t i = 0; i + 1 < ip_address_.size(); ++i) {
      if (ip_address_[i] != 0)
        return false;
    }
    return ip_address_.back() == 1;
  }

  return false;
}
bool ParseURLHostnameToAddress(std::string_view hostname,
                               IPAddress* ip_address) {
  if (hostname.size() >= 2 && hostname.front() == '[' &&
      hostname.back() == ']') {
    // Strip the square brackets that surround IPv6 literals.
    auto ip_literal = std::string_view(hostname).substr(1, hostname.size() - 2);
    return ip_address->AssignFromIPLiteral(ip_literal) && ip_address->IsIPv6();
  }

  return ip_address->AssignFromIPLiteral(hostname) && ip_address->IsIPv4();
}
std::array<uint8_t, 2> CreateNamePointer(uint16_t offset) {
  DCHECK_EQ(offset & ~dns_protocol::kOffsetMask, 0);
  std::array<uint8_t, 2> buf = base::U16ToBigEndian(offset);
  buf[0u] |= dns_protocol::kLabelPointer;
  return buf;
}
}
