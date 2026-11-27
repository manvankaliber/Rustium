// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include "base/check.h"
#include "base/logging.h"
#include "base/notreached.h"
#include "base/pickle.h"
#include "build/buildflag.h"
#include "net/base/proxy_chain.h"
#include "net/base/proxy_server.h"
#include "net/base/proxy_value_rust_ffi.h"
#include "net/base/proxy_value_rust_ffi.rs.h"
#include "net/net_buildflags.h"
#include "url/third_party/mozilla/url_parse.h"
#include "url/url_canon.h"
#include "url/url_canon_ip.h"
#include "url/url_canon_stdstring.h"

namespace net::proxy_value_rust {
rust::Vec<uint8_t> Copy(std::string_view input) {
  rust::Vec<uint8_t> out;
  out.reserve(input.size());
  for (unsigned char c : input) {
    out.push_back(c);
  }
  return out;
}
rust::Vec<uint8_t> canonical_host(rust::Slice<const uint8_t> input) {
  auto view = View(input);
  std::string result;
  url::StdStringCanonOutput output(&result);
  url::Component component;
  if (!url::CanonicalizeHost(view, url::Component(0, view.size()), &output,
                             &component) ||
      component.is_empty()) {
    return {};
  }
  output.Complete();
  return Copy(result);
}
Authority parse_authority(rust::Slice<const uint8_t> input) {
  auto s = View(input);
  url::Component user, password, host, port;
  url::ParseAuthority(s, url::Component(0, s.size()),
                      url::ParserMode::kSpecialURL, &user, &password, &host,
                      &port);
  return {host.is_nonempty() ? Copy(host.AsViewOn(s)) : rust::Vec<uint8_t>(),
          port.is_nonempty() ? Copy(port.AsViewOn(s)) : rust::Vec<uint8_t>(),
          user.is_valid() || password.is_valid(), port.is_valid()};
}
bool is_ipv6(rust::Slice<const uint8_t> input) {
  unsigned char bytes[16];
  return url::IPv6AddressToNumber(View(input), bytes);
}
ReadInt read_int(Reader& r) {
  int value = 0;
  bool ok = r.iterator->ReadInt(&value);
  return {ok, value};
}
ReadInt read_length(Reader& r) {
  size_t value = 0;
  bool ok = r.iterator->ReadLength(&value);
  return {ok, static_cast<int32_t>(value)};
}
rust::Vec<uint8_t> read_string(Reader& r) {
  std::string value;
  if (!r.iterator->ReadString(&value)) {
    return {};
  }
  return Copy(value);
}
void write_int(Writer& w, int32_t value) {
  w.pickle->WriteInt(value);
}
void write_string(Writer& w, rust::Slice<const uint8_t> value) {
  w.pickle->WriteString(View(value));
}
void check_invalid_record(rust::Slice<const uint8_t> host, uint16_t port) {
  DCHECK(host.empty() && port == 0);
}
void check_format_scheme(bool recognized) {
  if (!recognized) {
    NOTREACHED();
  }
}
void check_host(rust::Slice<const uint8_t> input, bool require_nonempty) {
  const auto host = View(input);
  if (host.find('\0') != std::string_view::npos) {
    std::string log;
    for (char c : host) {
      if (!c) {
        log += "%00";
      } else {
        log += c;
      }
    }
    LOG(DFATAL) << "Host has a null char: " << log;
  }
  DCHECK(host.find(':') == std::string_view::npos || host.front() != '[');
  DCHECK(!require_nonempty || !host.empty());
}
Server Snapshot(const ProxyServer& s) {
  return s.is_valid() ? Server{static_cast<int>(s.scheme()),
                               Copy(s.host_port_pair().host()),
                               s.host_port_pair().port()}
                      : Server{1, {}, 0};
}
rust::Vec<Server> Snapshot(const ProxyChain& c) {
  rust::Vec<Server> result;
  if (c.proxy_servers_if_valid()) {
    result.reserve(c.length());
    for (const auto& s : *c.proxy_servers_if_valid()) {
      result.push_back(Snapshot(s));
    }
  }
  return result;
}
ProxyServer Restore(const Server& s) {
  return ProxyServer(static_cast<ProxyServer::Scheme>(s.scheme),
                     HostPortPair(String(s.host), s.port));
}
ProxyChain Restore(const Chain& c) {
  if (!c.valid) {
    return ProxyChain();
  }
  std::vector<ProxyServer> result;
  result.reserve(c.servers.size());
  for (const auto& s : c.servers) {
    result.push_back(Restore(s));
  }
  return ProxyChain(std::move(result));
}
bool QuicEnabled() {
  return BUILDFLAG(ENABLE_QUIC_PROXY_SUPPORT);
}
bool MultiEnabled() {
  return BUILDFLAG(ENABLE_BRACKETED_PROXY_URIS);
}
}  // namespace net::proxy_value_rust
