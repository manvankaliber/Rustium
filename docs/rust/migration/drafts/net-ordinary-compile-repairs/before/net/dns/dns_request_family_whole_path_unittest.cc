// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license in LICENSE.
#include "net/dns/dns_request_family_async_workloads.h"
#include "net/dns/dns_request_family_http_workloads.h"
#include "net/dns/dns_request_family_platform_workloads.h"
#include "net/test/test_with_task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"
namespace net::dns_request_family_test {
namespace {
class DnsRequestWholePathTest : public testing::TestWithParam<unsigned> {
 protected:
  NetTaskEnvironment environment{base::test::TaskEnvironment::MainThreadType::IO,
                                 base::test::TaskEnvironment::TimeSource::MOCK_TIME};
};
TEST_P(DnsRequestWholePathTest, ClassicPairedLifecycle) {
  SCOPED_TRACE(kAsyncCases.at(GetParam()));
  EXPECT_EQ(AsyncWorkload<Production>(GetParam(),1,environment),
            AsyncWorkload<Original>(GetParam(),1,environment));
}
TEST_P(DnsRequestWholePathTest, DohAndProbePairedLifecycle) {
  SCOPED_TRACE(kHttpCases.at(GetParam()));
  EXPECT_EQ(HttpWorkload<Production>(GetParam(),1,environment),
            HttpWorkload<Original>(GetParam(),1,environment));
}
INSTANTIATE_TEST_SUITE_P(AllPaths,DnsRequestWholePathTest,testing::Range(0u,10u));
class DnsPlatformWholePathTest : public testing::TestWithParam<unsigned> {
 protected:
  NetTaskEnvironment environment{base::test::TaskEnvironment::MainThreadType::IO,
                                 base::test::TaskEnvironment::TimeSource::MOCK_TIME};
};
TEST_P(DnsPlatformWholePathTest, SeparatelyTypedProviderLifecycle) {
  SCOPED_TRACE(kPlatformCases.at(GetParam()));
  EXPECT_EQ(PlatformWorkload<PlatformProduction>(GetParam(),1,environment),
            PlatformWorkload<PlatformOriginal>(GetParam(),1,environment));
}
INSTANTIATE_TEST_SUITE_P(AllPaths,DnsPlatformWholePathTest,testing::Range(0u,5u));
}  // namespace
}  // namespace net::dns_request_family_test
