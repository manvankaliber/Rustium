// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license in LICENSE.
#include "net/dns/dns_client.h"
#include "net/dns/dns_client_rust_services.h"
#include "net/dns/dns_request_family_ffi.h"

#include "base/test/task_environment.h"
#include "net/dns/dns_config.h"
#include "net/dns/dns_test_util.h"
#include "net/log/net_log.h"
#include "net/test/test_with_task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace net {
namespace {
class DnsRequestFamilyNativeTest : public TestWithTaskEnvironment {};

TEST_F(DnsRequestFamilyNativeTest, ProductionProviderSharesCapabilityPolicy) {
  auto client = DnsClient::CreateClient(NetLog::Get());
  auto provider = client->MakeRustPolicyServices(nullptr);
  ASSERT_TRUE(provider);
  auto primitive = [](void* context, uint32_t op, DnsResolverWords input) {
    return static_cast<DnsClientRustServices*>(context)->Dispatch(op, input);
  };
  const auto result = rustium_dns_request_family_run(
      3, static_cast<uint32_t>(dns_request_family::ClientEvent::kInsecure),
      dns_request_family::W(-1), provider.get(), primitive);
  EXPECT_EQ(!!result.x, client->CanUseInsecureDnsTransactions(std::nullopt));
}

TEST_F(DnsRequestFamilyNativeTest, CustomProviderRetainsVirtualFallback) {
  MockDnsClient custom(DnsConfig(), MockDnsClientRuleList{});
  EXPECT_FALSE(custom.MakeRustPolicyServices(nullptr));
  // Existing MockDnsClient virtual methods remain the fallback path. No cast,
  // registry lookup or imported old-DLL vtable is used to identify this object.
}
}  // namespace
}  // namespace net
