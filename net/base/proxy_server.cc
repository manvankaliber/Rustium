// Copyright 2010 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include "net/base/proxy_server.h"

#include "base/check_op.h"
#include "base/logging.h"
#include "base/pickle.h"
#include "net/base/proxy_string_util.h"
#include "net/base/proxy_value_rust_ffi.h"
#include "net/base/proxy_value_rust_ffi.rs.h"

namespace net {
namespace pv = proxy_value_rust;

ProxyServer::ProxyServer(Scheme scheme, const HostPortPair& pair)
    : scheme_(scheme), host_port_pair_(pair) {
  if (!is_valid()) {
    DCHECK(pair.host().empty() && pair.port() == 0);
    host_port_pair_ = HostPortPair();
  }
}
ProxyServer ProxyServer::FromSchemeHostAndPort(Scheme scheme,
                                               std::string_view host,
                                               std::string_view port) {
  DCHECK_NE(scheme, SCHEME_INVALID);
  return pv::Restore(pv::parse_host(scheme, pv::Bytes(host), pv::Bytes(port)));
}
ProxyServer ProxyServer::FromSchemeHostAndPort(Scheme scheme,
                                               std::string_view host,
                                               std::optional<uint16_t> port) {
  DCHECK_NE(scheme, SCHEME_INVALID);
  return pv::Restore(pv::parse_host_number(scheme, pv::Bytes(host),
                                           port.value_or(0), port.has_value()));
}
ProxyServer ProxyServer::CreateFromPickle(base::PickleIterator* iterator) {
  pv::Reader reader{iterator};
  return pv::Restore(pv::read_server(reader));
}
void ProxyServer::Persist(base::Pickle* pickle) const {
  pv::Writer writer{pickle};
  pv::persist_server(writer, scheme_, pv::Bytes(host_port_pair_.host()),
                     host_port_pair_.port());
}
std::string ProxyServer::GetHost() const {
  const auto& host = host_port_pair().host();
  pv::check_host(pv::Bytes(host), false);
  return pv::String(pv::format_host(pv::Bytes(host)));
}
uint16_t ProxyServer::GetPort() const {
  return host_port_pair().port();
}
const HostPortPair& ProxyServer::host_port_pair() const {
  DCHECK(is_valid());
  return host_port_pair_;
}
int ProxyServer::GetDefaultPortForScheme(Scheme scheme) {
  return pv::default_port(scheme);
}
bool ProxyServer::is_valid() const {
  return pv::scheme_flag(scheme_, 0);
}
bool ProxyServer::is_http() const {
  return pv::scheme_flag(scheme_, 1);
}
bool ProxyServer::is_https() const {
  return pv::scheme_flag(scheme_, 2);
}
bool ProxyServer::is_socks() const {
  return pv::scheme_flag(scheme_, 3);
}
bool ProxyServer::is_quic() const {
  return pv::scheme_flag(scheme_, 4);
}
bool ProxyServer::is_http_like() const {
  return pv::scheme_flag(scheme_, 5);
}
bool ProxyServer::is_secure_http_like() const {
  return pv::scheme_flag(scheme_, 6);
}
std::strong_ordering operator<=>(const ProxyServer& a, const ProxyServer& b) {
  const int c = pv::compare_server(
      a.scheme_, pv::Bytes(a.host_port_pair_.host()), a.host_port_pair_.port(),
      b.scheme_, pv::Bytes(b.host_port_pair_.host()), b.host_port_pair_.port());
  return c < 0   ? std::strong_ordering::less
         : c > 0 ? std::strong_ordering::greater
                 : std::strong_ordering::equal;
}
bool operator==(const ProxyServer& a, const ProxyServer& b) {
  return (a <=> b) == 0;
}
std::ostream& operator<<(std::ostream& os, const ProxyServer& s) {
  return os << ProxyServerToPacResultElement(s);
}
}  // namespace net
