// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include <array>
#include <random>
#include <sstream>

#include "base/pickle.h"
#include "base/test/gtest_util.h"
#include "build/buildflag.h"
#include "net/base/proxy_chain.h"
#include "net/base/proxy_server.h"
#include "net/base/proxy_string_util.h"
#include "net/base/proxy_value_rust_reference_proxy_string_util.h"
#include "net/net_buildflags.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace net {
namespace {
namespace ref = proxy_value_rust_reference;

void SamePickle(const base::Pickle& a, const base::Pickle& b) {
  ASSERT_EQ(a.size(), b.size());
  ASSERT_EQ(a.AsBytes(), b.AsBytes());
}
void SameServer(const ProxyServer& a,
                const ref::ProxyServer& b,
                bool persist = true) {
  ASSERT_EQ(static_cast<int>(a.scheme()), static_cast<int>(b.scheme()));
  ASSERT_EQ(a.is_valid(), b.is_valid());
  ASSERT_EQ(a.is_http(), b.is_http());
  ASSERT_EQ(a.is_https(), b.is_https());
  ASSERT_EQ(a.is_socks(), b.is_socks());
  ASSERT_EQ(a.is_quic(), b.is_quic());
  ASSERT_EQ(a.is_http_like(), b.is_http_like());
  ASSERT_EQ(a.is_secure_http_like(), b.is_secure_http_like());
  ASSERT_EQ(ProxyServer::GetDefaultPortForScheme(a.scheme()),
            ref::ProxyServer::GetDefaultPortForScheme(b.scheme()));
  if (a.is_valid()) {
    ASSERT_EQ(a.GetHost(), b.GetHost());
    ASSERT_EQ(a.GetPort(), b.GetPort());
    ASSERT_EQ(a.host_port_pair().host(), b.host_port_pair().host());
    if (!a.GetHost().empty()) {
      ASSERT_EQ(ProxyServerToProxyUri(a), ref::ProxyServerToProxyUri(b));
      ASSERT_EQ(ProxyServerToPacResultElement(a),
                ref::ProxyServerToPacResultElement(b));
      std::ostringstream as, bs;
      as << a;
      bs << b;
      ASSERT_EQ(as.str(), bs.str());
    }
  }
  auto ac = a;
  auto bc = b;
  ASSERT_EQ(a == ac, b == bc);
  ASSERT_EQ((a <=> ac) == 0, (b <=> bc) == 0);
  if (persist) {
    base::Pickle ap, bp;
    a.Persist(&ap);
    b.Persist(&bp);
    SamePickle(ap, bp);
    base::PickleIterator ai(ap), bi(bp);
    SameServer(ProxyServer::CreateFromPickle(&ai),
               ref::ProxyServer::CreateFromPickle(&bi), false);
  }
}
void SameChain(const ProxyChain& a,
               const ref::ProxyChain& b,
               bool persist = true) {
  ASSERT_EQ(a.IsValid(), b.IsValid());
  ASSERT_EQ(a.length(), b.length());
  ASSERT_EQ(a.is_direct(), b.is_direct());
  ASSERT_EQ(a.is_single_proxy(), b.is_single_proxy());
  ASSERT_EQ(a.is_multi_proxy(), b.is_multi_proxy());
  ASSERT_EQ(a.opaque_data(), b.opaque_data());
  ASSERT_EQ(a.is_get_to_proxy_allowed(), b.is_get_to_proxy_allowed());
  bool printable = true;
  if (a.IsValid()) {
    for (const auto& s : a.proxy_servers()) {
      if (s.GetHost().empty()) {
        printable = false;
      }
    }
  }
  if (printable) {
    ASSERT_EQ(a.ToDebugString(), b.ToDebugString());
  }
  ASSERT_EQ(a.AnyProxy([](const auto& s) { return s.is_secure_http_like(); }),
            b.AnyProxy([](const auto& s) { return s.is_secure_http_like(); }));
  ASSERT_EQ(a.proxy_servers_if_valid().has_value(),
            b.proxy_servers_if_valid().has_value());
  auto ac = a;
  auto bc = b;
  ASSERT_EQ(a == ac, b == bc);
  ASSERT_EQ((a <=> ac) == 0, (b <=> bc) == 0);
  ac = ProxyChain();
  bc = ref::ProxyChain();
  ac = a;
  bc = b;
  ProxyChain am(std::move(ac));
  ref::ProxyChain bm(std::move(bc));
  if (printable) {
    ASSERT_EQ(am.ToDebugString(), bm.ToDebugString());
  }
  if (!a.IsValid()) {
    return;
  }
  ASSERT_EQ(&a.proxy_servers(), &*a.proxy_servers_if_valid());
  for (size_t i = 0; i < a.length(); ++i) {
    SameServer(a.GetProxyServer(i), b.GetProxyServer(i), persist);
    ASSERT_EQ(&a.GetProxyServer(i), &a.proxy_servers().at(i));
  }
  if (a.length()) {
    ASSERT_EQ(&a.First(), &a.GetProxyServer(0));
    ASSERT_EQ(&a.Last(), &a.GetProxyServer(a.length() - 1));
    auto as = a.SplitLast();
    auto bs = b.SplitLast();
    ASSERT_EQ(&as.second, &a.Last());
    SameServer(as.second, bs.second, false);
    if (printable) {
      ASSERT_EQ(as.first.ToDebugString(), bs.first.ToDebugString());
    }
    ASSERT_EQ(as.first.opaque_data(), bs.first.opaque_data());
  }
  for (size_t i = 0; i <= a.length(); ++i) {
    auto ap = a.Prefix(i);
    auto bp = b.Prefix(i);
    if (printable) {
      ASSERT_EQ(ap.ToDebugString(), bp.ToDebugString());
    }
    ASSERT_EQ(ap.opaque_data(), bp.opaque_data());
  }
  if (persist) {
    base::Pickle ap, bp;
    a.Persist(&ap);
    b.Persist(&bp);
    SamePickle(ap, bp);
    base::PickleIterator ai(ap), bi(bp);
    auto ar = ProxyChain::InitFromPickle(ai);
    auto br = ref::ProxyChain::InitFromPickle(bi);
    ASSERT_EQ(ar.has_value(), br.has_value());
    if (ar) {
      SameChain(*ar, *br, false);
    }
  }
}
void Input(std::string_view raw, uint64_t state, bool deep) {
  constexpr int schemes[] = {4, 8, 16, 32, 64};
  const auto s = static_cast<ProxyServer::Scheme>(schemes[state % 5]);
  const auto r = static_cast<ref::ProxyServer::Scheme>(s);
  const bool quic = (state & 8) != 0;
  switch ((state >> 4) % 7) {
    case 0:
      SameServer(ProxyUriToProxyServer(raw, s, quic),
                 ref::ProxyUriToProxyServer(raw, r, quic), deep);
      break;
    case 1:
      SameServer(PacResultElementToProxyServer(raw),
                 ref::PacResultElementToProxyServer(raw), deep);
      break;
    case 2:
      SameServer(ProxySchemeHostAndPortToProxyServer(s, raw),
                 ref::ProxySchemeHostAndPortToProxyServer(r, raw), deep);
      break;
    case 3:
      SameServer(ProxyServer::FromSchemeHostAndPort(
                     s, raw, std::optional<uint16_t>(state)),
                 ref::ProxyServer::FromSchemeHostAndPort(
                     r, raw, std::optional<uint16_t>(state)),
                 deep);
      break;
    case 4:
      SameChain(ProxyUriToProxyChain(raw, s, quic),
                ref::ProxyUriToProxyChain(raw, r, quic), deep);
      break;
    case 5:
      SameChain(PacResultElementToProxyChain(raw),
                ref::PacResultElementToProxyChain(raw), deep);
      break;
    case 6:
#if BUILDFLAG(ENABLE_BRACKETED_PROXY_URIS)
      SameChain(MultiProxyUrisToProxyChain(raw, s, quic),
                ref::MultiProxyUrisToProxyChain(raw, r, quic), deep);
#endif
      break;
  }
  ASSERT_EQ(static_cast<int>(GetSchemeFromUriScheme(raw, quic)),
            static_cast<int>(ref::GetSchemeFromUriScheme(raw, quic)));
}
TEST(ProxyValueRust, BoundaryCorpusAndAllBytes) {
  constexpr std::string_view seeds[] = {"",
                                        " ",
                                        "\t\r\n",
                                        "DIRECT",
                                        "direct://",
                                        "DIRECT foo",
                                        "PROXY foo:80",
                                        "SOCKS foo",
                                        "SOCKS5 foo",
                                        "HTTPS [::1]:443",
                                        "QUIC foo",
                                        "http://FoO",
                                        "https://f\xc3\xbcpy:123",
                                        "socks://1.2.3.4",
                                        "quic://foo",
                                        "http://@foo",
                                        "foo:",
                                        "foo:000000000080",
                                        "foo:65536",
                                        "foo:-1",
                                        "foo:0",
                                        "::1",
                                        "[::1]:65535",
                                        "[::ffff:1.2.3.4]:80",
                                        "[https://a https://b]",
                                        "[quic://a https://b]",
                                        "[https://a quic://b]",
                                        "[direct://]",
                                        "[direct:// https://a]",
                                        "[https://a\thttps://b]",
                                        "https://a https://b",
                                        "[]",
                                        "[ ]",
                                        "fo o:80",
                                        "foo/bar",
                                        "foo?bar",
                                        "foo#bar"};
  for (auto seed : seeds) {
    for (uint64_t k = 0; k < 160; ++k) {
      SCOPED_TRACE(seed);
      Input(seed, k, true);
    }
  }
  for (unsigned c = 0; c < 256; ++c) {
    std::string raw(1, static_cast<char>(c));
    for (uint64_t k = 0; k < 128; k += 16) {
      Input(raw, k, true);
    }
    Input("http://a" + raw + ":80", c, true);
    Input("[https://a " + raw + "]", c, true);
    for (auto port : {std::string(), std::string("0"), std::string("65535"),
                      std::string("65536"), std::string("00"), raw}) {
      SameServer(ProxyServer::FromSchemeHostAndPort(ProxyServer::SCHEME_HTTP,
                                                    "a", port),
                 ref::ProxyServer::FromSchemeHostAndPort(
                     ref::ProxyServer::SCHEME_HTTP, "a", port));
    }
  }
  Input(std::string_view(), 0, true);
  std::string backing = "https://Example.test:443";
  auto a = ProxyUriToProxyServer(backing, ProxyServer::SCHEME_HTTP);
  auto b = ref::ProxyUriToProxyServer(backing, ref::ProxyServer::SCHEME_HTTP);
  backing.assign(4096, 'z');
  SameServer(a, b);  // input lifetime is not retained.
  backing = "foo:80";
  a = ProxyUriToProxyServer(backing, ProxyServer::SCHEME_HTTP);
  a = ProxyUriToProxyServer(a.host_port_pair().host(),
                            ProxyServer::SCHEME_HTTP);
  ASSERT_EQ(a.GetHost(), "foo");  // input aliases object being replaced.
}
TEST(ProxyValueRust, RawConstructorsOrderingAndOpaque) {
  std::vector<ProxyServer> aa;
  std::vector<ref::ProxyServer> bb;
  aa.emplace_back();
  bb.emplace_back();
  for (int s : {4, 8, 16, 32, 64}) {
    for (auto host : {"z", "a", "::1"}) {
      for (uint16_t p : {0, 1, 65535}) {
        aa.emplace_back(static_cast<ProxyServer::Scheme>(s),
                        HostPortPair(host, p));
        bb.emplace_back(static_cast<ref::ProxyServer::Scheme>(s),
                        ref::HostPortPair(host, p));
        SameServer(aa.back(), bb.back());
        SameChain(ProxyChain(aa.back()), ref::ProxyChain(bb.back()));
      }
    }
  }
  for (size_t i = 0; i < aa.size(); ++i) {
    for (size_t j = 0; j < aa.size(); ++j) {
      ASSERT_EQ(aa[i] == aa[j], bb[i] == bb[j]);
      ASSERT_EQ((aa[i] <=> aa[j]) < 0, (bb[i] <=> bb[j]) < 0);
    }
  }
  std::vector<ProxyChain> chains;
  std::vector<ref::ProxyChain> refs;
  for (int n = 0; n < 80; ++n) {
    std::vector<ProxyServer> a;
    std::vector<ref::ProxyServer> b;
    for (int j = 0; j < n % 5; ++j) {
      a.push_back(aa[(n + j * 17) % aa.size()]);
      b.push_back(bb[(n + j * 17) % bb.size()]);
    }
    chains.push_back(ProxyChain::WithOpaqueData(a, n - 40));
    refs.push_back(ref::ProxyChain::WithOpaqueData(b, n - 40));
    SameChain(chains.back(), refs.back());
  }
  for (size_t i = 0; i < chains.size(); ++i) {
    for (size_t j = 0; j < chains.size(); ++j) {
      ASSERT_EQ(chains[i] == chains[j], refs[i] == refs[j]);
      ASSERT_EQ((chains[i] <=> chains[j]) < 0, (refs[i] <=> refs[j]) < 0);
    }
  }
}
TEST(ProxyValueRust, PickleFailuresAndIteratorMutation) {
  for (int scheme : {-100, 0, 1, 2, 4, 8, 16, 32, 64, 127}) {
    for (std::string_view host : {"", ":0", "foo", "foo:", "foo:0", "[::1]:80",
                                  "a:b:c", "[a]:80", "foo:65536"}) {
      // Original debug contract rejects invalid scheme + nonempty parsed pair.
      if ((scheme != 4 && scheme != 8 && scheme != 16 && scheme != 32 &&
           scheme != 64) &&
          (host == "foo:0" || host == "[::1]:80")) {
        continue;
      }
      base::Pickle pickle;
      pickle.WriteInt(scheme);
      pickle.WriteString(host);
      pickle.WriteInt(989);
      base::PickleIterator ai(pickle), bi(pickle);
      SameServer(ProxyServer::CreateFromPickle(&ai),
                 ref::ProxyServer::CreateFromPickle(&bi));
      int av = 0, bv = 0;
      ASSERT_EQ(ai.ReadInt(&av), bi.ReadInt(&bv));
      ASSERT_EQ(av, bv);
    }
  }
  for (int len : {-1, 0, 1, 2, 3, 5}) {
    for (int fields = 0; fields < 9; ++fields) {
      base::Pickle p;
      p.WriteInt(-1);
      p.WriteInt(len);
      for (int i = 0; i < fields; ++i) {
        if (i % 2 == 0) {
          p.WriteInt(4);
        } else {
          p.WriteString("a:80");
        }
      }
      base::PickleIterator ai(p), bi(p);
      auto a = ProxyChain::InitFromPickle(ai);
      auto b = ref::ProxyChain::InitFromPickle(bi);
      ASSERT_EQ(a.has_value(), b.has_value());
      if (a) {
        SameChain(*a, *b);
      }
      int av = 0, bv = 0;
      ASSERT_EQ(ai.ReadInt(&av), bi.ReadInt(&bv));
      ASSERT_EQ(av, bv);
    }
  }
}
TEST(ProxyValueRust, MillionDeterministicDifferentialInputs) {
  std::mt19937_64 rng(0x70726f787976616cULL);
  constexpr std::string_view seeds[] = {"http://foo:80",
                                        "https://FOO:443",
                                        "socks://[::1]",
                                        "PROXY foo:123",
                                        "DIRECT",
                                        "direct://",
                                        "[quic://a https://b]",
                                        "[https://a https://b]",
                                        "foo:",
                                        "foo:00000080"};
  for (size_t i = 0; i < 1000000; ++i) {
    const uint64_t state = rng();
    std::string raw;
    if (i % 3) {
      raw = seeds[state % std::size(seeds)];
      if (i % 3 == 1) {
        for (size_t j = 0; j < 1 + (state >> 8) % 3; ++j) {
          raw[(state >> (j * 11)) % raw.size()] = static_cast<char>(rng());
        }
      }
    } else {
      for (size_t j = 0; j < (state >> 8) % 40; ++j) {
        raw.push_back(static_cast<char>(rng()));
      }
    }
    Input(raw, state, i % 1024 == 0);
    if (::testing::Test::HasFatalFailure()) {
      ADD_FAILURE() << "input " << i;
      return;
    }
  }
}
TEST(ProxyValueRust, PicklePayloadTruncationAndAllBytes) {
  base::Pickle p;
  ProxyServer::FromSchemeHostAndPort(ProxyServer::SCHEME_HTTP, "example", "80")
      .Persist(&p);
  for (size_t n = 0; n <= p.payload_bytes().size(); ++n) {
    auto payload = p.payload_bytes().first(n);
    auto ai = base::PickleIterator::WithData(payload),
         bi = base::PickleIterator::WithData(payload);
    SameServer(ProxyServer::CreateFromPickle(&ai),
               ref::ProxyServer::CreateFromPickle(&bi));
    int av = 0, bv = 0;
    ASSERT_EQ(ai.ReadInt(&av), bi.ReadInt(&bv));
    ASSERT_EQ(av, bv);
  }
  for (unsigned c = 1; c < 256; ++c) {
    // Embedded NUL is a documented DFATAL for uncanonicalized HostPortPair,
    // separately checked below. All other bytes exercise the raw pickle API.
    std::string host = "x";
    host += static_cast<char>(c);
    host += "y:80";
    base::Pickle raw;
    raw.WriteInt(4);
    raw.WriteString(host);
    base::PickleIterator ai(raw), bi(raw);
    auto a = ProxyServer::CreateFromPickle(&ai);
    auto b = ref::ProxyServer::CreateFromPickle(&bi);
    // Values may be deliberately noncanonical; formatting preconditions for
    // strings starting '[' are exercised by upstream/death tests instead.
    SameServer(a, b);
  }
}
TEST(ProxyValueRust, DebugFailureContracts) {
  EXPECT_NOTREACHED_DEATH(ProxyServerToProxyUri(
      ProxyServer(static_cast<ProxyServer::Scheme>(0), HostPortPair("a", 80))));
  EXPECT_NOTREACHED_DEATH(ref::ProxyServerToProxyUri(ref::ProxyServer(
      static_cast<ref::ProxyServer::Scheme>(0), ref::HostPortPair("a", 80))));
  EXPECT_DCHECK_DEATH(
      ProxyServer(ProxyServer::SCHEME_INVALID, HostPortPair("a", 1)));
  EXPECT_DCHECK_DEATH(ProxyServer().GetHost());
  EXPECT_DCHECK_DEATH(ProxyChain().proxy_servers());
  EXPECT_CHECK_DEATH(ProxyChain::Direct().GetProxyServer(0));
  EXPECT_DCHECK_DEATH(ProxyServerToProxyUri(
      ProxyServer(ProxyServer::SCHEME_HTTP, HostPortPair("", 0))));
  EXPECT_DCHECK_DEATH(
      ProxyChain(ProxyServer(ProxyServer::SCHEME_HTTP, HostPortPair("", 0)))
          .ToDebugString());
  EXPECT_DCHECK_DEATH_WITH(
      ProxyServer(ProxyServer::SCHEME_HTTP,
                  HostPortPair(std::string_view("a\0b", 3), 0))
          .GetHost(),
      "Host has a null char");
}
}  // namespace
}  // namespace net
