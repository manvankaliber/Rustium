// Copyright 2024 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/facilitated_payments/ui/chrome_facilitated_payments_client.h"

#include <memory>

#include "base/functional/bind.h"
#include "base/functional/callback_helpers.h"
#include "base/test/scoped_feature_list.h"
#include "build/build_config.h"
#include "chrome/test/base/chrome_render_view_host_test_harness.h"
#include "components/facilitated_payments/core/features/features.h"
#include "components/optimization_guide/core/hints/mock_optimization_guide_decider.h"
#include "components/tabs/public/mock_tab_interface.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/origin.h"


using ::testing::_;
using ::testing::Return;


class ChromeFacilitatedPaymentsClientTest
    : public ChromeRenderViewHostTestHarness {
 public:
  void SetUp() override {
    ChromeRenderViewHostTestHarness::SetUp();
    client_ = std::make_unique<ChromeFacilitatedPaymentsClient>(
        mock_tab_, web_contents(), &optimization_guide_decider_);
  }

  // The client holds a `raw_ref` to `web_contents()` and observes it, so it
  // must be destroyed before the harness destroys the `WebContents`.
  void TearDown() override {
    client_.reset();
    ChromeRenderViewHostTestHarness::TearDown();
  }

  auto& base_client() {
    return static_cast<payments::facilitated::FacilitatedPaymentsClient&>(
        *client_);
  }


 protected:
  tabs::MockTabInterface mock_tab_;
  optimization_guide::MockOptimizationGuideDecider optimization_guide_decider_;
  std::unique_ptr<ChromeFacilitatedPaymentsClient> client_;
};

TEST_F(ChromeFacilitatedPaymentsClientTest, GetPaymentsDataManager) {
  EXPECT_NE(nullptr, base_client().GetPaymentsDataManager());
}

TEST_F(ChromeFacilitatedPaymentsClientTest,
       GetFacilitatedPaymentsNetworkInterface) {
  EXPECT_NE(nullptr, base_client().GetFacilitatedPaymentsNetworkInterface());
}

// Test the client correctly returns whether or not the tab is active.
TEST_F(ChromeFacilitatedPaymentsClientTest, IsWebContentsVisibleOrOccluded) {
  web_contents()->UpdateWebContentsVisibility(content::Visibility::VISIBLE);
  EXPECT_TRUE(base_client().IsWebContentsVisibleOrOccluded());

  web_contents()->UpdateWebContentsVisibility(content::Visibility::OCCLUDED);
  EXPECT_TRUE(base_client().IsWebContentsVisibleOrOccluded());

  web_contents()->UpdateWebContentsVisibility(content::Visibility::HIDDEN);
  EXPECT_FALSE(base_client().IsWebContentsVisibleOrOccluded());
}

TEST_F(ChromeFacilitatedPaymentsClientTest,
       IsInChromeCustomTabMode_NullCallback) {
  EXPECT_FALSE(base_client().IsInChromeCustomTabMode());
}

TEST_F(ChromeFacilitatedPaymentsClientTest,
       IsInChromeCustomTabMode_CallbackReturnsTrue) {
  client_.reset();
  client_ = std::make_unique<ChromeFacilitatedPaymentsClient>(
      mock_tab_, web_contents(), &optimization_guide_decider_,
      base::BindRepeating([](content::WebContents*) { return true; }));

  EXPECT_TRUE(base_client().IsInChromeCustomTabMode());
}

TEST_F(ChromeFacilitatedPaymentsClientTest,
       IsInChromeCustomTabMode_CallbackReturnsFalse) {
  client_.reset();
  client_ = std::make_unique<ChromeFacilitatedPaymentsClient>(
      mock_tab_, web_contents(), &optimization_guide_decider_,
      base::BindRepeating([](content::WebContents*) { return false; }));

  EXPECT_FALSE(base_client().IsInChromeCustomTabMode());
}

// Tests that only the payment QR code allowlist is registered on Desktop when
// QR code detection is enabled, since the Pix and payment link flows that use
// the other allowlists are Android-only.
TEST_F(ChromeFacilitatedPaymentsClientTest,
       RegisterAllowlists_Desktop_QrCodeDetectionEnabled) {
  base::test::ScopedFeatureList feature_list(
      payments::facilitated::kEnableDesktopQrCodeDetection);
  EXPECT_CALL(optimization_guide_decider_, RegisterOptimizationTypes).Times(0);
  EXPECT_CALL(optimization_guide_decider_,
              RegisterOptimizationTypes(testing::ElementsAre(
                  optimization_guide::proto::
                      PAYMENT_QR_CODE_MERCHANT_URL_REGEX_ALLOWLIST)))
      .Times(1);

  // Re-create the client; it should register the allowlist.
  client_.reset();
  client_ = std::make_unique<ChromeFacilitatedPaymentsClient>(
      mock_tab_, web_contents(), &optimization_guide_decider_);
}

// Tests that no allowlist is registered on Desktop when QR code detection is
// disabled.
TEST_F(ChromeFacilitatedPaymentsClientTest,
       RegisterAllowlists_Desktop_QrCodeDetectionDisabled) {
  base::test::ScopedFeatureList feature_list;
  feature_list.InitAndDisableFeature(
      payments::facilitated::kEnableDesktopQrCodeDetection);
  EXPECT_CALL(optimization_guide_decider_, RegisterOptimizationTypes).Times(0);

  // Re-create the client; it should not register any allowlist.
  client_.reset();
  client_ = std::make_unique<ChromeFacilitatedPaymentsClient>(
      mock_tab_, web_contents(), &optimization_guide_decider_);
}

// Tests that the client reports Desktop defaults for the device queries that
// are only meaningful for the Android payment UI.
TEST_F(ChromeFacilitatedPaymentsClientTest, DeviceQueries_DesktopDefaults) {
  EXPECT_FALSE(base_client().IsInLandscapeMode());
  EXPECT_FALSE(base_client().IsFoldable());
  EXPECT_EQ(nullptr, base_client().GetDeviceDelegate());
}
