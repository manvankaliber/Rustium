// Prepared full factory/params/jobs/common-pool/handle comparisons; not executed.
#include "net/socket/connection_jobs_family_flow_fixture.h"
#include "base/test/scoped_feature_list.h"
#include "testing/gtest/include/gtest/gtest.h"
namespace net::connection_jobs_family_flow {
class ConnectionFamilyFlowTest:public testing::TestWithParam<unsigned> {
 protected:base::test::TaskEnvironment environment{base::test::TaskEnvironment::TimeSource::MOCK_TIME};
};
TEST_P(ConnectionFamilyFlowTest, FactoryThroughHandleMatchesOriginal) {
 const auto kind=static_cast<ScriptCase>(GetParam()%16);
 if(!connection_jobs_job_workloads::Supported(kind))GTEST_SKIP()<<"Configured QUIC proxy feature unavailable";
 base::test::ScopedFeatureList features;
 if(GetParam()>=16)features.InitAndEnableFeature(net::features::kHappyEyeballsV2);
 else features.InitAndDisableFeature(net::features::kHappyEyeballsV2);
 auto original=::net::connection_jobs_family_flow::Run<Original>(kind,environment);
 auto rust=::net::connection_jobs_family_flow::Run<Production>(kind,environment);
 EXPECT_EQ(original.events,rust.events);
 EXPECT_EQ(original.checksum,rust.checksum);
}
INSTANTIATE_TEST_SUITE_P(AllProtocolsAndTransportFamilies,ConnectionFamilyFlowTest,testing::Range(0u,32u));
TEST(ConnectionFamilyFlowTest, FactoryAsyncCompletionAllowsHandleReset) {
 base::test::TaskEnvironment environment{base::test::TaskEnvironment::TimeSource::MOCK_TIME};
 base::test::ScopedFeatureList features;features.InitAndDisableFeature(net::features::kHappyEyeballsV2);
 EXPECT_EQ(::net::connection_jobs_family_flow::Run<Original>(ScriptCase::kTlsAsync,environment,false,true).events,
           ::net::connection_jobs_family_flow::Run<Production>(ScriptCase::kTlsAsync,environment,false,true).events);
}
TEST(ConnectionFamilyFlowTest, CancellingPendingFactoryJobSuppressesCompletion) {
 base::test::TaskEnvironment environment{base::test::TaskEnvironment::TimeSource::MOCK_TIME};
 base::test::ScopedFeatureList features;features.InitAndDisableFeature(net::features::kHappyEyeballsV2);
 EXPECT_EQ(::net::connection_jobs_family_flow::Run<Original>(ScriptCase::kTlsAsync,environment,true).events,
           ::net::connection_jobs_family_flow::Run<Production>(ScriptCase::kTlsAsync,environment,true).events);
}
TEST(ConnectionFamilyFlowTest, FactoryCompletionAllowsTerminalHandleDeletion) {
 base::test::TaskEnvironment environment{base::test::TaskEnvironment::TimeSource::MOCK_TIME};
 base::test::ScopedFeatureList features;features.InitAndDisableFeature(net::features::kHappyEyeballsV2);
 const auto original=::net::connection_jobs_family_flow::Run<Original>(ScriptCase::kTlsAsync,environment,false,false,true);
 const auto rust=::net::connection_jobs_family_flow::Run<Production>(ScriptCase::kTlsAsync,environment,false,false,true);
 EXPECT_EQ(original.events,rust.events);
 EXPECT_EQ(original.checksum,rust.checksum);
 ASSERT_FALSE(original.events.empty());
 EXPECT_EQ(19,original.events.back().kind);
}
}
