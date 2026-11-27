// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license.
#include "net/base/proxy_value_rust_reference_proxy_string_util.h"
#include "net/spdy/spdy_session_key_rust_original.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace net {
TEST(ProxyReferenceComponentTest, ValuesSurviveSpdyComponentCopies) {
  namespace ref = proxy_value_rust_reference;
  const ref::HostPortPair origin("origin.example", 443);
  const ref::ProxyChain proxy = ref::ProxyUriToProxyChain(
      "https://proxy.example:8443", ref::ProxyServer::SCHEME_HTTPS);
  ASSERT_TRUE(proxy.IsValid());
  ASSERT_EQ(1u, proxy.length());
  EXPECT_EQ("https://proxy.example:8443",
            ref::ProxyServerToProxyUri(proxy.First()));
  {
    // Construction/copy/comparison/destruction of keys execute in the SPDY
    // reference component; proxy out-of-line methods import the proxy owner.
    const SpdySessionKeyOriginal key(
        origin, PRIVACY_MODE_DISABLED, proxy, SessionUsage::kDestination,
        SocketTag(), NetworkAnonymizationKey(), SecureDnsPolicy::kAllow,
        false, handles::kInvalidNetworkHandle);
    const SpdySessionKeyOriginal copy(key);
    EXPECT_EQ(origin, copy.host_port_pair());
    EXPECT_EQ(proxy, copy.proxy_chain());
    EXPECT_EQ(key, copy);
    EXPECT_FALSE(key < copy);
    EXPECT_FALSE(copy < key);
    const SpdySessionKeyOriginal other(
        ref::HostPortPair("origin.example", 444), PRIVACY_MODE_DISABLED,
        proxy, SessionUsage::kDestination, SocketTag(),
        NetworkAnonymizationKey(), SecureDnsPolicy::kAllow, false,
        handles::kInvalidNetworkHandle);
    EXPECT_NE(key < other, other < key);
  }
  EXPECT_EQ("proxy.example:8443", proxy.First().host_port_pair().ToString());
}
}  // namespace net
