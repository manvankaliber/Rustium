// Copyright 2024 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/page_info/core/merchant_trust_service.h"

#include <memory>
#include <optional>
#include <utility>

#include "base/memory/raw_ptr.h"
#include "base/test/metrics/histogram_tester.h"
#include "base/test/metrics/user_action_tester.h"
#include "base/test/task_environment.h"
#include "base/test/test_future.h"
#include "components/page_info/core/page_info_types.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace page_info {
namespace {

using testing::_;
using testing::Return;

class MockMerchantTrustServiceDelegate : public MerchantTrustService::Delegate {
 public:
  MOCK_METHOD(double, GetSiteEngagementScore, (const GURL url), (override));
};

class MerchantTrustServiceTest : public testing::Test {
 public:
  MerchantTrustServiceTest();
  ~MerchantTrustServiceTest() override;

  void SetUp() override {
    auto delegate = std::make_unique<MockMerchantTrustServiceDelegate>();
    delegate_ = delegate.get();
    service_ = std::make_unique<MerchantTrustService>(
        std::move(delegate), /*optimization_guide_decider=*/nullptr,
        /*is_off_the_record=*/false, /*prefs=*/nullptr);
  }

 protected:
  MerchantTrustService* service() { return service_.get(); }
  MockMerchantTrustServiceDelegate* delegate() { return delegate_; }

 private:
  base::test::TaskEnvironment task_environment_;
  std::unique_ptr<MerchantTrustService> service_;
  raw_ptr<MockMerchantTrustServiceDelegate> delegate_ = nullptr;
};

MerchantTrustServiceTest::MerchantTrustServiceTest() = default;
MerchantTrustServiceTest::~MerchantTrustServiceTest() = default;

TEST_F(MerchantTrustServiceTest, GetMerchantTrustInfoReturnsNoData) {
  const GURL url("https://foo.com");
  base::test::TestFuture<GURL, std::optional<MerchantData>> future;
  service()->GetMerchantTrustInfo(
      url, future.GetCallback<const GURL&, std::optional<MerchantData>>());

  EXPECT_EQ(future.Get<0>(), url);
  EXPECT_FALSE(future.Get<1>().has_value());
}

TEST_F(MerchantTrustServiceTest, RecordMerchantTrustInteractionFamiliarSite) {
  base::HistogramTester t;
  base::UserActionTester user_action_tester;

  EXPECT_CALL(*delegate(), GetSiteEngagementScore(_))
      .WillRepeatedly(Return(kMerchantFamiliarityThreshold));
  service()->RecordMerchantTrustInteraction(
      GURL("https://foo.com"), MerchantTrustInteraction::kPageInfoRowShown);

  EXPECT_EQ(1, user_action_tester.GetActionCount(
                   "MerchantTrust.PageInfoRowSeen.FamiliarSite"));
  t.ExpectUniqueSample(
      "Security.PageInfo.MerchantTrustInteraction.FamiliarSite",
      MerchantTrustInteraction::kPageInfoRowShown, 1);
  t.ExpectBucketCount(
      "Security.PageInfo.MerchantTrustEngagement.PageInfoRowShown",
      kMerchantFamiliarityThreshold, 1);
}

TEST_F(MerchantTrustServiceTest, RecordMerchantTrustInteractionUnfamiliarSite) {
  base::HistogramTester t;
  base::UserActionTester user_action_tester;

  EXPECT_CALL(*delegate(), GetSiteEngagementScore(_))
      .WillRepeatedly(Return(kMerchantFamiliarityThreshold - 0.1));
  service()->RecordMerchantTrustInteraction(
      GURL("https://foo.com"),
      MerchantTrustInteraction::kBubbleOpenedFromPageInfo);

  EXPECT_EQ(1, user_action_tester.GetActionCount(
                   "MerchantTrust.BubbleOpenedFromPageInfo.UnfamiliarSite"));
  t.ExpectUniqueSample(
      "Security.PageInfo.MerchantTrustInteraction.UnfamiliarSite",
      MerchantTrustInteraction::kBubbleOpenedFromPageInfo, 1);
  t.ExpectBucketCount("Security.PageInfo.MerchantTrustEngagement.BubbleOpened",
                      kMerchantFamiliarityThreshold - 1, 1);
}

}  // namespace
}  // namespace page_info
