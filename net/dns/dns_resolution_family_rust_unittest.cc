// Copyright 2026 The Chromium Authors
// BSD-style license; see LICENSE.
#include "net/dns/dns_resolution_family_rust_test_support.h"
#include "base/test/task_environment.h"
#include "net/test/test_with_task_environment.h"
#include "net/base/mock_network_change_notifier.h"
#include "testing/gtest/include/gtest/gtest.h"
namespace net::dns_resolution_family_test {
TEST(DnsResolutionFamilyRust, CompleteLocalRequestPaths){
  NetTaskEnvironment environment(base::test::TaskEnvironment::MainThreadType::IO, base::test::TaskEnvironment::TimeSource::MOCK_TIME);
  test::ScopedMockNetworkChangeNotifier notifier;
  for(size_t kind=0;kind<kCases.size();++kind){
    SCOPED_TRACE(kCases.at(kind));Fixture<Original> original(kind);Fixture<Production> production(kind);
    const auto before=original.Once();const auto after=production.Once();EXPECT_EQ(before,after);
    EXPECT_EQ(original.Once(),production.Once());
    if(kind==3||kind==4||kind==9||kind==10||kind==12||kind==14){EXPECT_NE(OK,after.error);}
    else{EXPECT_EQ(OK,after.error);EXPECT_FALSE(after.endpoints.empty());}
  }
}
TEST(DnsResolutionFamilyRust, RepeatedCachePathsKeepResultOwnership){
  NetTaskEnvironment environment(base::test::TaskEnvironment::MainThreadType::IO, base::test::TaskEnvironment::TimeSource::MOCK_TIME);
  test::ScopedMockNetworkChangeNotifier notifier;
  for(size_t kind=11;kind<kCases.size();++kind){
    SCOPED_TRACE(kCases.at(kind));Fixture<Original> original(kind);Fixture<Production> production(kind);
    for(unsigned call=0;call<8;++call)EXPECT_EQ(original.Once(),production.Once());
  }
}
}
