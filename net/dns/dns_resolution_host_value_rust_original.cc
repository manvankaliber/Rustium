// Exact original Host value body; HPP provider is independently frozen.
#include "net/dns/dns_resolution_host_value_rust_original.h"
#include <utility>
#include "base/check.h"
namespace net::dns_resolution_original {
DnsResolverHostOriginal::DnsResolverHostOriginal(std::variant<url::SchemeHostPort, HostPortPair> host)
    : host_(std::move(host)) {
#if DCHECK_IS_ON()
  if (std::holds_alternative<url::SchemeHostPort>(host_)) {
    DCHECK(std::get<url::SchemeHostPort>(host_).IsValid());
  } else {
    DCHECK(std::holds_alternative<HostPortPair>(host_));
    DCHECK(!std::get<HostPortPair>(host_).IsEmpty());
  }
#endif  // DCHECK_IS_ON()
}

DnsResolverHostOriginal::~DnsResolverHostOriginal() = default;

DnsResolverHostOriginal::DnsResolverHostOriginal(const DnsResolverHostOriginal&) = default;

DnsResolverHostOriginal& DnsResolverHostOriginal::operator=(const DnsResolverHostOriginal&) = default;

DnsResolverHostOriginal::DnsResolverHostOriginal(DnsResolverHostOriginal&&) = default;

DnsResolverHostOriginal& DnsResolverHostOriginal::operator=(DnsResolverHostOriginal&&) = default;

bool DnsResolverHostOriginal::HasScheme() const {
  return std::holds_alternative<url::SchemeHostPort>(host_);
}

const std::string& DnsResolverHostOriginal::GetScheme() const {
  DCHECK(std::holds_alternative<url::SchemeHostPort>(host_));
  return std::get<url::SchemeHostPort>(host_).scheme();
}

std::string DnsResolverHostOriginal::GetHostname() const {
  if (std::holds_alternative<url::SchemeHostPort>(host_)) {
    return std::get<url::SchemeHostPort>(host_).host();
  } else {
    DCHECK(std::holds_alternative<HostPortPair>(host_));
    return std::get<HostPortPair>(host_).HostForURL();
  }
}

std::string_view DnsResolverHostOriginal::GetHostnameWithoutBrackets() const {
  if (std::holds_alternative<url::SchemeHostPort>(host_)) {
    std::string_view hostname = std::get<url::SchemeHostPort>(host_).host();
    if (hostname.size() > 2 && hostname.front() == '[' &&
        hostname.back() == ']') {
      return hostname.substr(1, hostname.size() - 2);
    } else {
      return hostname;
    }
  } else {
    DCHECK(std::holds_alternative<HostPortPair>(host_));
    return std::get<HostPortPair>(host_).host();
  }
}

uint16_t DnsResolverHostOriginal::GetPort() const {
  if (std::holds_alternative<url::SchemeHostPort>(host_)) {
    return std::get<url::SchemeHostPort>(host_).port();
  } else {
    DCHECK(std::holds_alternative<HostPortPair>(host_));
    return std::get<HostPortPair>(host_).port();
  }
}

std::string DnsResolverHostOriginal::ToString() const {
  if (std::holds_alternative<url::SchemeHostPort>(host_)) {
    return std::get<url::SchemeHostPort>(host_).Serialize();
  } else {
    DCHECK(std::holds_alternative<HostPortPair>(host_));
    return std::get<HostPortPair>(host_).ToString();
  }
}

const url::SchemeHostPort& DnsResolverHostOriginal::AsSchemeHostPort() const {
  const url::SchemeHostPort* scheme_host_port =
      std::get_if<url::SchemeHostPort>(&host_);
  DCHECK(scheme_host_port);
  return *scheme_host_port;
}

const HostPortPair& DnsResolverHostOriginal::AsHostPortPair() const {
  const HostPortPair* host_port_pair = std::get_if<HostPortPair>(&host_);
  DCHECK(host_port_pair);
  return *host_port_pair;
}


}
