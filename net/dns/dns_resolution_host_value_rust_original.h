// Exact original Host value body; HPP provider is independently frozen.
#ifndef NET_DNS_DNS_RESOLUTION_HOST_VALUE_RUST_ORIGINAL_H_
#define NET_DNS_DNS_RESOLUTION_HOST_VALUE_RUST_ORIGINAL_H_
#include <string>
#include <string_view>
#include <variant>
#include "base/component_export.h"
#include "net/base/host_port_rust_reference.h"
#include "url/scheme_host_port.h"
namespace net::dns_resolution_original {
using HostPortPair=::net::HostPortRustReference;
class COMPONENT_EXPORT(DNS_RESOLUTION_ORIGINAL) DnsResolverHostOriginal {
   public:
    explicit DnsResolverHostOriginal(std::variant<url::SchemeHostPort, HostPortPair> host);
    ~DnsResolverHostOriginal();

    DnsResolverHostOriginal(const DnsResolverHostOriginal&);
    DnsResolverHostOriginal& operator=(const DnsResolverHostOriginal&);
    DnsResolverHostOriginal(DnsResolverHostOriginal&&);
    DnsResolverHostOriginal& operator=(DnsResolverHostOriginal&&);

    bool HasScheme() const;
    const std::string& GetScheme() const;
    std::string GetHostname() const;  // With brackets for IPv6 literals.
    std::string_view GetHostnameWithoutBrackets() const;
    uint16_t GetPort() const;

    std::string ToString() const;

    // Returns the requested type. HasScheme() must return true to use the first
    // method, second to use the second.
    const url::SchemeHostPort& AsSchemeHostPort() const;
    const HostPortPair& AsHostPortPair() const;

    bool operator==(const DnsResolverHostOriginal& other) const { return host_ == other.host_; }

    bool operator<(const DnsResolverHostOriginal& other) const { return host_ < other.host_; }

   private:
    std::variant<url::SchemeHostPort, HostPortPair> host_;
  };
}
#endif
