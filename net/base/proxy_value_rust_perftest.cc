// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Run --verify without timing. Parent alone runs --benchmark in a quiet window.
#include <array>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <random>
#include <ratio>
#include <sstream>

#include "base/check.h"
#include "base/i18n/icu_util.h"
#include "base/pickle.h"
#include "build/buildflag.h"
#include "net/base/proxy_string_util.h"
#include "net/base/proxy_value_rust_reference_proxy_string_util.h"
#include "net/net_buildflags.h"

namespace {
namespace ref = net::proxy_value_rust_reference;
struct Rust {
  using Server = net::ProxyServer;
  using Chain = net::ProxyChain;
  using Pair = net::HostPortPair;
  static Server Uri(std::string_view s) {
    return net::ProxyUriToProxyServer(s, Server::SCHEME_HTTP, true);
  }
  static Chain UriChain(std::string_view s) {
    return net::ProxyUriToProxyChain(s, Server::SCHEME_HTTP, true);
  }
  static Chain Pac(std::string_view s) {
    return net::PacResultElementToProxyChain(s);
  }
  static Chain Multi(std::string_view s) {
    return net::MultiProxyUrisToProxyChain(s, Server::SCHEME_HTTP, true);
  }
  static int Scheme(std::string_view s) {
    return net::GetSchemeFromUriScheme(s, true);
  }
  static std::string Format(const Server& s) {
    return net::ProxyServerToProxyUri(s) +
           net::ProxyServerToPacResultElement(s);
  }
};
struct Cpp {
  using Server = ref::ProxyServer;
  using Chain = ref::ProxyChain;
  using Pair = ref::HostPortPair;
  static Server Uri(std::string_view s) {
    return ref::ProxyUriToProxyServer(s, Server::SCHEME_HTTP, true);
  }
  static Chain UriChain(std::string_view s) {
    return ref::ProxyUriToProxyChain(s, Server::SCHEME_HTTP, true);
  }
  static Chain Pac(std::string_view s) {
    return ref::PacResultElementToProxyChain(s);
  }
  static Chain Multi(std::string_view s) {
    return ref::MultiProxyUrisToProxyChain(s, Server::SCHEME_HTTP, true);
  }
  static int Scheme(std::string_view s) {
    return ref::GetSchemeFromUriScheme(s, true);
  }
  static std::string Format(const Server& s) {
    return ref::ProxyServerToProxyUri(s) +
           ref::ProxyServerToPacResultElement(s);
  }
};
struct Case {
  std::string_view id;
  std::string_view input;
  int operation;
};
constexpr Case kCases[] = {
    {"scheme_lookup", "SoCkS", 0},
    {"host_dns", " Example.COM ", 1},
    {"host_ipv4", "0x7f000001", 1},
    {"host_ipv6", "2001:db8::1", 1},
    {"host_idn", "f\xc3\xbcpy", 1},
    {"host_invalid", "bad%00host", 1},
    {"port_long_zeroes", "00000000000000443", 2},
    {"port_invalid", "65536", 2},
    {"uri_http", "http://Example.COM:8080", 3},
    {"uri_socks", "socks://[::1]:1080", 3},
    {"uri_https", "https://secure.example:443", 3},
    {"uri_quic", "quic://secure.example:443", 3},
    {"uri_invalid", "http://user:pass@bad:80", 3},
    {"pac_proxy", "PROXY Example.COM:8080", 4},
    {"pac_socks", "SOCKS [::1]:1080", 4},
    {"pac_direct", " DIRECT ", 4},
#if BUILDFLAG(ENABLE_BRACKETED_PROXY_URIS)
    {"multi_https", "[https://first:443 https://second:443]", 5},
    {"multi_quic_https", "[quic://first:443 https://second:443]", 5},
    {"multi_invalid_order", "[https://first:443 quic://second:443]", 5},
#endif
    {"server_format", "https://[2001:db8::1]:8443", 6},
    {"server_order_flags", "https://a:443", 7},
    {"server_persistence", "socks5://[::1]:1080", 8},
    {"chain_ownership", "https://proxy.example:443", 9},
    {"chain_order_opaque", "https://proxy.example:443", 10},
    {"chain_persistence", "https://proxy.example:443", 11},
    {"chain_prefix_split", "https://proxy.example:443", 12},
    {"uri_direct", "direct://", 13}};
template <class A>
std::string Exercise(const Case& c) {
  using S = typename A::Server;
  using C = typename A::Chain;
  auto output = [](const S& s) {
    return s.is_valid() ? A::Format(s) : std::string("INVALID");
  };
  switch (c.operation) {
    case 0:
      return std::to_string(A::Scheme(c.input));
    case 1:
      return output(
          S::FromSchemeHostAndPort(S::SCHEME_HTTP, c.input, std::nullopt));
    case 2:
      return output(
          S::FromSchemeHostAndPort(S::SCHEME_HTTPS, "example.com", c.input));
    case 3:
      return output(A::Uri(c.input));
    case 4:
      return A::Pac(c.input).ToDebugString();
    case 5:
      return A::Multi(c.input).ToDebugString();
    case 6: {
      auto s = A::Uri(c.input);
      return output(s) + s.GetHost() + std::to_string(s.GetPort());
    }
    case 7: {
      auto s = A::Uri(c.input);
      auto t = S(S::SCHEME_HTTPS, typename A::Pair("z", 80));
      return std::to_string((s < t) * 1 + (s == t) * 2 + s.is_http() * 4 +
                            s.is_https() * 8 + s.is_socks() * 16 +
                            s.is_quic() * 32 + s.is_http_like() * 64 +
                            s.is_secure_http_like() * 128 +
                            S::GetDefaultPortForScheme(s.scheme()));
    }
    case 8: {
      auto s = A::Uri(c.input);
      base::Pickle p;
      s.Persist(&p);
      base::PickleIterator i(p);
      return output(S::CreateFromPickle(&i));
    }
    case 9: {
      C a(A::Uri(c.input));
      C b(a);
      C d(std::move(b));
      b = a;
      d = std::move(b);
      return d.ToDebugString() + d.First().GetHost() + d.Last().GetHost() +
             d.GetProxyServer(0).GetHost() +
             std::to_string(
                 d.proxy_servers().size() + d.proxy_servers_if_valid()->size() +
                 d.length() + d.is_single_proxy() + d.is_multi_proxy() +
                 d.is_direct() + d.IsValid() + d.is_get_to_proxy_allowed() +
                 d.AnyProxy([](const auto& s) { return s.is_https(); }));
    }
    case 10: {
      auto s = A::Uri(c.input);
      auto a = C::WithOpaqueData({s}, -17);
      auto b = C::WithOpaqueData({s}, 43);
      return std::to_string(a < b) + std::to_string(a == b) +
             std::to_string(*a.opaque_data()) + a.ToDebugString();
    }
    case 11: {
      C a(A::Uri(c.input));
      base::Pickle p;
      a.Persist(&p);
      base::PickleIterator i(p);
      return C::InitFromPickle(i)->ToDebugString();
    }
    case 12: {
      C a(A::Uri(c.input));
      auto split = a.SplitLast();
      return a.Prefix(1).ToDebugString() + a.Prefix(0).ToDebugString() +
             split.first.ToDebugString() + output(split.second);
    }
    case 13:
      return A::UriChain(c.input).ToDebugString();
  }
  return {};
}
uint64_t g_sink = 0;
template <class A>
double Time(const Case& c, int iterations) {
  auto start = std::chrono::steady_clock::now();
  uint64_t sum = 0;
  for (int i = 0; i < iterations; ++i) {
    auto result = Exercise<A>(c);
    sum += result.size();
    if (!result.empty()) {
      sum += static_cast<unsigned char>(result.front());
    }
  }
  auto stop = std::chrono::steady_clock::now();
  g_sink += sum;
  return std::chrono::duration<double, std::nano>(stop - start).count() /
         iterations;
}
}  // namespace
int main(int argc, char** argv) {
  CHECK(base::i18n::InitializeICU());
  bool timing = argc == 2 && std::string_view(argv[1]) == "--benchmark";
  CHECK(argc == 2 && (timing || std::string_view(argv[1]) == "--verify"));
  for (const auto& c : kCases) {
    CHECK_EQ(Exercise<Cpp>(c), Exercise<Rust>(c));
  }
  if (!timing) {
    std::cout << "Verified " << std::size(kCases)
              << " full API workload checksums; no timing.\n";
    return 0;
  }
  constexpr int iterations = 20000;
  std::mt19937 rng(0x50525859);
  std::cout << std::setprecision(17);
  for (const auto& c : kCases) {
    for (int i = 0; i < 100; ++i) {
      g_sink += Exercise<Cpp>(c).size();
      g_sink += Exercise<Rust>(c).size();
    }
    for (int pair = 0; pair < 20; ++pair) {
      bool rust_first = rng() & 1;
      double cpp, rust;
      if (rust_first) {
        rust = Time<Rust>(c, iterations);
        cpp = Time<Cpp>(c, iterations);
      } else {
        cpp = Time<Cpp>(c, iterations);
        rust = Time<Rust>(c, iterations);
      }
      std::cout << "{\"case\":\"" << c.id << "\",\"repeat\":" << pair
                << ",\"baseline_ns\":" << cpp << ",\"rust_ns\":" << rust
                << ",\"rust_first\":" << (rust_first ? "true" : "false")
                << ",\"iterations\":" << iterations << "}\n";
    }
  }
  std::cerr << "sink=" << g_sink << "\n";
}
