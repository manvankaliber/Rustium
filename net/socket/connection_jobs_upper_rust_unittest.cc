// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include "net/socket/connection_jobs_upper_rust_test_support.h"
#include "base/test/scoped_feature_list.h"
#include "net/base/features.h"
#include "testing/gtest/include/gtest/gtest.h"
namespace net::connection_jobs_upper_test {
class UpperFamilyTest:public testing::TestWithParam<unsigned>{
 protected:
  base::test::TaskEnvironment environment{base::test::TaskEnvironment::TimeSource::MOCK_TIME};
};
TEST_P(UpperFamilyTest, PublicLifecycleAgainstIndependentOriginal){
  base::test::ScopedFeatureList features;
  features.InitAndEnableFeature(net::features::kPartitionConnectionsByNetworkIsolationKey);
  const auto kind=static_cast<Case>(GetParam());
  EXPECT_EQ(Run<Original>(kind,true),Run<Production>(kind,true));
}
INSTANTIATE_TEST_SUITE_P(All,UpperFamilyTest,testing::Range(0u,21u));
}  // namespace net::connection_jobs_upper_test
