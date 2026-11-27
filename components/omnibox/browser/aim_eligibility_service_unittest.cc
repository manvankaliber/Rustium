// Copyright 2025 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/omnibox/browser/aim_eligibility_service.h"

#include "base/test/scoped_feature_list.h"
#include "components/omnibox/browser/omnibox_pref_names.h"
#include "components/omnibox/common/omnibox_features.h"
#include "components/prefs/testing_pref_service.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {
class RemovedAimEligibilityService : public AimEligibilityService {
 public:
  explicit RemovedAimEligibilityService(PrefService& prefs)
      : AimEligibilityService(prefs, nullptr, nullptr, nullptr, "en-US", {}) {}

 protected:
  std::string GetLocaleImpl() const override { return "en-US"; }
  variations::VariationsService* GetVariationsService() const override {
    return nullptr;
  }
};

TEST(AimEligibilityServiceTest, RemovedEvenWhenFlagsAndPoliciesAllowIt) {
  base::test::ScopedFeatureList features;
  features.InitAndEnableFeature(omnibox::kAimEnabled);
  TestingPrefServiceSimple prefs;
  AimEligibilityService::RegisterProfilePrefs(prefs.registry());
  prefs.SetInteger(omnibox::kAIModeSettings, 0);
  prefs.SetInteger(omnibox::kThirdPartyAiChatSettings, 0);
  RemovedAimEligibilityService service(prefs);
  EXPECT_FALSE(service.IsServerEligibilityEnabled());
  EXPECT_FALSE(service.IsAimEligible());
  EXPECT_FALSE(service.IsAimAllowedByFeatureAndPolicy());
  EXPECT_FALSE(service.IsCobrowseEligible());
  EXPECT_FALSE(service.IsFuseboxEligible());
  EXPECT_FALSE(service.IsCsbEligible());
  EXPECT_FALSE(AimEligibilityService::IsAimAllowedByPolicy(&prefs));
  EXPECT_FALSE(AimEligibilityService::IsAimAllowedByThirdPartyPolicy(&prefs));
  EXPECT_FALSE(AimEligibilityService::GenericKillSwitchFeatureCheck(
      &service, omnibox::kAimEnabled));
  EXPECT_FALSE(service.IsAimUrl(GURL("https://www.google.com/search?udm=50"),
                                std::nullopt));
  EXPECT_EQ(nullptr, service.GetSearchboxConfig());
  EXPECT_EQ(nullptr, service.GetLhsRailConfig());
  service.FetchEligibility(AimEligibilityService::RequestSource::kUser);
  service.StartServerEligibilityRequestForDebugging();
  EXPECT_FALSE(service.SetEligibilityResponseForDebugging("CAE="));
  EXPECT_EQ(0u, service.GetMostRecentResponse().ByteSizeLong());
}

}  // namespace
