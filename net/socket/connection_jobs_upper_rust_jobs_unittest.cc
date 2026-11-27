// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include "net/socket/connection_jobs_upper_rust_job_workloads.h"
#include "base/test/scoped_feature_list.h"
#include "net/base/features.h"
#include "testing/gtest/include/gtest/gtest.h"
namespace net::connection_jobs_job_workloads {
class JobsFamilyTest:public testing::TestWithParam<unsigned>{
 protected:base::test::TaskEnvironment environment{base::test::TaskEnvironment::TimeSource::MOCK_TIME};
};
TEST_P(JobsFamilyTest, CompletePublicScriptedLifecycleMatchesOriginal){
 const auto kind=static_cast<Case>(GetParam());
 if(!Supported(kind)){GTEST_SKIP()<<"QUIC proxy provider disabled by configured build flag";}
 base::test::ScopedFeatureList features;
 features.InitAndDisableFeature(net::features::kHappyEyeballsV2);
 uint64_t original=0,rust=0;
 {Fixture<Original> fixture(kind,environment);fixture.Prepare(1);original=fixture.Run(1,true);}
 {Fixture<Production> fixture(kind,environment);fixture.Prepare(1);rust=fixture.Run(1,true);}
 EXPECT_EQ(original,rust);
}
INSTANTIATE_TEST_SUITE_P(All,JobsFamilyTest,testing::Range(0u,16u));
}  // namespace net::connection_jobs_job_workloads
