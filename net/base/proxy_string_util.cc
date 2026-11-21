// Copyright 2021 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include "net/base/proxy_string_util.h"

#include "base/notreached.h"
#include "net/base/proxy_value_rust_ffi.h"
#include "net/base/proxy_value_rust_ffi.rs.h"

namespace net {
namespace pv = proxy_value_rust;
ProxyChain PacResultElementToProxyChain(std::string_view input) {
  return pv::Restore(pv::parse_chain(pv::Bytes(input), ProxyServer::SCHEME_HTTP,
                                     false, pv::MultiEnabled(), 1));
}
ProxyServer PacResultElementToProxyServer(std::string_view input) {
  return pv::Restore(
      pv::parse_server(pv::Bytes(input), ProxyServer::SCHEME_HTTP, false, 1));
}
ProxyChain ProxyUriToProxyChain(std::string_view input,
                                ProxyServer::Scheme scheme,
                                bool quic) {
  return pv::Restore(pv::parse_chain(pv::Bytes(input), scheme,
                                     quic && pv::QuicEnabled(),
                                     pv::MultiEnabled(), 0));
}
ProxyServer ProxyUriToProxyServer(std::string_view input,
                                  ProxyServer::Scheme scheme,
                                  bool quic) {
  return pv::Restore(
      pv::parse_server(pv::Bytes(input), scheme, quic && pv::QuicEnabled(), 0));
}
ProxyServer ProxySchemeHostAndPortToProxyServer(ProxyServer::Scheme scheme,
                                                std::string_view input) {
  return pv::Restore(pv::parse_server(pv::Bytes(input), scheme, false, 2));
}
ProxyServer::Scheme GetSchemeFromUriScheme(std::string_view input, bool quic) {
  return static_cast<ProxyServer::Scheme>(
      pv::get_uri_scheme(pv::Bytes(input), quic && pv::QuicEnabled()));
}
ProxyChain MultiProxyUrisToProxyChain(std::string_view input,
                                      ProxyServer::Scheme scheme,
                                      bool quic) {
  if (!pv::MultiEnabled()) {
    NOTREACHED();
  }
  return pv::Restore(pv::parse_chain(pv::Bytes(input), scheme,
                                     quic && pv::QuicEnabled(),
                                     pv::MultiEnabled(), 2));
}
std::string ProxyServerToProxyUri(const ProxyServer& s) {
  if (!s.is_valid()) {
    NOTREACHED();
  }
  return pv::String(pv::format_server(
      s.scheme(), pv::Bytes(s.host_port_pair().host()), s.GetPort(), false));
}
std::string ProxyServerToPacResultElement(const ProxyServer& s) {
  if (!s.is_valid()) {
    NOTREACHED();
  }
  return pv::String(pv::format_server(
      s.scheme(), pv::Bytes(s.host_port_pair().host()), s.GetPort(), true));
}
}  // namespace net
