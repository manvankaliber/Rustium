// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include <stdint.h>

#include <array>
#include <string_view>

#include "base/check.h"
#include "base/containers/span.h"
#include "base/i18n/icu_util.h"
#include "base/no_destructor.h"
#include "base/pickle.h"
#include "build/buildflag.h"
#include "net/base/proxy_string_util.h"
#include "net/base/proxy_value_rust_reference_proxy_string_util.h"
#include "net/net_buildflags.h"

namespace {
namespace ref = net::proxy_value_rust_reference;
void Server(const net::ProxyServer& a, const ref::ProxyServer& b) {
  CHECK_EQ(static_cast<int>(a.scheme()), static_cast<int>(b.scheme()));
  CHECK_EQ(a.is_valid(), b.is_valid());
  CHECK_EQ(a.is_http(), b.is_http());
  CHECK_EQ(a.is_https(), b.is_https());
  CHECK_EQ(a.is_socks(), b.is_socks());
  CHECK_EQ(a.is_quic(), b.is_quic());
  CHECK_EQ(a.is_http_like(), b.is_http_like());
  CHECK_EQ(a.is_secure_http_like(), b.is_secure_http_like());
  if (a.is_valid()) {
    CHECK_EQ(a.GetHost(), b.GetHost());
    CHECK_EQ(a.GetPort(), b.GetPort());
    CHECK_EQ(a.host_port_pair().host(), b.host_port_pair().host());
    CHECK_EQ(net::ProxyServerToProxyUri(a), ref::ProxyServerToProxyUri(b));
    CHECK_EQ(net::ProxyServerToPacResultElement(a),
             ref::ProxyServerToPacResultElement(b));
  }
  base::Pickle ap, bp;
  a.Persist(&ap);
  b.Persist(&bp);
  CHECK(ap.AsBytes() == bp.AsBytes());
  base::PickleIterator ai(ap), bi(bp);
  auto ar = net::ProxyServer::CreateFromPickle(&ai);
  auto br = ref::ProxyServer::CreateFromPickle(&bi);
  CHECK_EQ(static_cast<int>(ar.scheme()), static_cast<int>(br.scheme()));
  if (ar.is_valid()) {
    CHECK_EQ(ar.GetHost(), br.GetHost());
    CHECK_EQ(ar.GetPort(), br.GetPort());
  }
}
void Chain(const net::ProxyChain& a, const ref::ProxyChain& b) {
  CHECK_EQ(a.IsValid(), b.IsValid());
  CHECK_EQ(a.length(), b.length());
  CHECK_EQ(a.ToDebugString(), b.ToDebugString());
  CHECK_EQ(a.is_single_proxy(), b.is_single_proxy());
  CHECK_EQ(a.is_multi_proxy(), b.is_multi_proxy());
  CHECK_EQ(a.is_direct(), b.is_direct());
  CHECK_EQ(a.is_get_to_proxy_allowed(), b.is_get_to_proxy_allowed());
  CHECK_EQ(a.AnyProxy([](const auto& s) { return s.is_https(); }),
           b.AnyProxy([](const auto& s) { return s.is_https(); }));
  auto ac = a;
  auto bc = b;
  CHECK_EQ(a == ac, b == bc);
  CHECK_EQ((a <=> ac) < 0, (b <=> bc) < 0);
  if (!a.IsValid()) {
    return;
  }
  CHECK_EQ(&a.proxy_servers(), &*a.proxy_servers_if_valid());
  for (size_t i = 0; i < a.length(); ++i) {
    Server(a.GetProxyServer(i), b.GetProxyServer(i));
    CHECK_EQ(&a.GetProxyServer(i), &a.proxy_servers().at(i));
  }
  if (a.length()) {
    CHECK_EQ(&a.First(), &a.GetProxyServer(0));
    CHECK_EQ(&a.Last(), &a.GetProxyServer(a.length() - 1));
    auto as = a.SplitLast();
    auto bs = b.SplitLast();
    CHECK_EQ(&as.second, &a.Last());
    CHECK_EQ(as.first.ToDebugString(), bs.first.ToDebugString());
  }
  for (size_t i = 0; i <= a.length(); ++i) {
    CHECK_EQ(a.Prefix(i).ToDebugString(), b.Prefix(i).ToDebugString());
  }
  base::Pickle ap, bp;
  a.Persist(&ap);
  b.Persist(&bp);
  CHECK(ap.AsBytes() == bp.AsBytes());
  base::PickleIterator ai(ap), bi(bp);
  auto ar = net::ProxyChain::InitFromPickle(ai);
  auto br = ref::ProxyChain::InitFromPickle(bi);
  CHECK_EQ(ar.has_value(), br.has_value());
  if (ar) {
    CHECK_EQ(ar->ToDebugString(), br->ToDebugString());
  }
}
}  // namespace
extern "C" int LLVMFuzzerInitialize(int*, char***) {
  CHECK(base::i18n::InitializeICU());
  return 0;
}
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  // SAFETY: libFuzzer supplies initialized storage for size bytes; the span is
  // borrowed synchronously, never retained, and empty input avoids null views.
  auto bytes = UNSAFE_BUFFERS(base::span(data, size));
  if (bytes.empty()) {
    return 0;
  }
  const uint8_t control = bytes[0];
  bytes = bytes.subspan(1u);
  const auto input =
      bytes.empty()
          ? std::string_view()
          : UNSAFE_BUFFERS(std::string_view(
                reinterpret_cast<const char*>(bytes.data()), bytes.size()));
  constexpr std::array schemes = {4, 8, 16, 32, 64};
  const auto s =
      static_cast<net::ProxyServer::Scheme>(schemes[control % schemes.size()]);
  const auto r = static_cast<ref::ProxyServer::Scheme>(s);
  const bool quic = control & 8;
  switch ((control >> 4) % 7) {
    case 0:
      Server(net::ProxyUriToProxyServer(input, s, quic),
             ref::ProxyUriToProxyServer(input, r, quic));
      break;
    case 1:
      Server(net::PacResultElementToProxyServer(input),
             ref::PacResultElementToProxyServer(input));
      break;
    case 2:
      Server(net::ProxySchemeHostAndPortToProxyServer(s, input),
             ref::ProxySchemeHostAndPortToProxyServer(r, input));
      break;
    case 3:
      Server(net::ProxyServer::FromSchemeHostAndPort(s, input, std::nullopt),
             ref::ProxyServer::FromSchemeHostAndPort(r, input, std::nullopt));
      break;
    case 4:
      Chain(net::ProxyUriToProxyChain(input, s, quic),
            ref::ProxyUriToProxyChain(input, r, quic));
      break;
    case 5:
      Chain(net::PacResultElementToProxyChain(input),
            ref::PacResultElementToProxyChain(input));
      break;
    case 6:
#if BUILDFLAG(ENABLE_BRACKETED_PROXY_URIS)
      Chain(net::MultiProxyUrisToProxyChain(input, s, quic),
            ref::MultiProxyUrisToProxyChain(input, r, quic));
#endif
      break;
  }
  CHECK_EQ(static_cast<int>(net::GetSchemeFromUriScheme(input, quic)),
           static_cast<int>(ref::GetSchemeFromUriScheme(input, quic)));
  return 0;
}
