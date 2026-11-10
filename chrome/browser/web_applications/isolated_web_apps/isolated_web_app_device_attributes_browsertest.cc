// Copyright 2025 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <array>
#include <string>
#include <tuple>

#include "base/strings/string_util.h"
#include "base/test/scoped_feature_list.h"
#include "build/build_config.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/web_applications/test/isolated_web_app_test_utils.h"
#include "chrome/browser/web_applications/isolated_web_apps/install/isolated_web_app_install_source.h"
#include "chrome/browser/web_applications/isolated_web_apps/isolated_web_app_url_info.h"
#include "chrome/browser/web_applications/isolated_web_apps/test/isolated_web_app_builder.h"
#include "chrome/common/chrome_features.h"
#include "content/public/common/content_features.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "services/network/public/mojom/permissions_policy/permissions_policy_feature.mojom.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/public/common/features.h"


namespace web_app {
namespace {

using ::testing::HasSubstr;

constexpr std::array<const char*, 5> kDeviceAttributeNames = {
    "AnnotatedAssetId", "AnnotatedLocation", "DirectoryId", "Hostname",
    "SerialNumber"};

content::EvalJsResult CallDeviceAttributesApi(
    const content::ToRenderFrameHost& frame,
    const std::string& attribute_name) {
  return content::EvalJs(
      frame, base::ReplaceStringPlaceholders("navigator.managed.get$1()",
                                             {attribute_name}, nullptr));
}

constexpr char kPermissionsPolicyError[] =
    "Permissions-Policy: device-attributes are disabled.";

constexpr char kNotSupportedPlatformErrorMessage[] =
    "This web API is not supported on the current platform.";

}  // namespace


class IsolatedWebAppDeviceAttributesDefaultFeaturesNonChromeOSBrowserTest
    : public IsolatedWebAppBrowserTestHarness {
 private:
  base::test::ScopedFeatureList feature_list_{features::kIsolatedWebAppDevMode};
};

IN_PROC_BROWSER_TEST_F(
    IsolatedWebAppDeviceAttributesDefaultFeaturesNonChromeOSBrowserTest,
    DevModeIwaWithPermissionsPolicyLaunchesWithoutCrash) {
  auto manifest_builder = ManifestBuilder().AddPermissionsPolicy(
      network::mojom::PermissionsPolicyFeature::kDeviceAttributes,
      /*self=*/true, {});
  std::unique_ptr<ScopedBundledIsolatedWebApp> bundle =
      IsolatedWebAppBuilder(manifest_builder).BuildBundle();
  bundle->TrustSigningKey();
  IsolatedWebAppUrlInfo url_info =
      bundle
          ->InstallWithSource(profile(),
                              &IsolatedWebAppInstallSource::FromDevUi)
          .value();

  content::RenderFrameHost* app_frame = OpenApp(url_info.app_id());
  ASSERT_NE(app_frame, nullptr);
  EXPECT_EQ(
      "undefined",
      content::EvalJs(app_frame, "typeof navigator.managed.getSerialNumber"));
}

class IsolatedWebAppDeviceAttributesNonChromeOSBrowserTest
    : public IsolatedWebAppBrowserTestHarness {
 public:
  IsolatedWebAppDeviceAttributesNonChromeOSBrowserTest() {
    feature_list_.InitWithFeatures(
        {features::kIsolatedWebAppDevMode, blink::features::kDeviceAttributes},
        {});
  }

 protected:
  enum class InstallType { kDevMode, kPolicy };

  IsolatedWebAppUrlInfo InstallIwa(InstallType install_type,
                                   bool grant_permissions_policy) {
    auto manifest_builder = ManifestBuilder();
    if (grant_permissions_policy) {
      manifest_builder.AddPermissionsPolicy(
          network::mojom::PermissionsPolicyFeature::kDeviceAttributes,
          /*self=*/true, {});
    }
    std::unique_ptr<ScopedBundledIsolatedWebApp> bundle =
        IsolatedWebAppBuilder(manifest_builder).BuildBundle();
    bundle->TrustSigningKey();
    switch (install_type) {
      case InstallType::kDevMode:
        return bundle
            ->InstallWithSource(profile(),
                                &IsolatedWebAppInstallSource::FromDevUi)
            .value();
      case InstallType::kPolicy:
        return bundle
            ->InstallWithSource(
                profile(), &IsolatedWebAppInstallSource::FromExternalPolicy)
            .value();
    }
  }

 private:
  base::test::ScopedFeatureList feature_list_;
};

IN_PROC_BROWSER_TEST_F(
    IsolatedWebAppDeviceAttributesNonChromeOSBrowserTest,
    DevModeIwaWithPermissionsPolicyReturnsNotSupportedError) {
  IsolatedWebAppUrlInfo url_info =
      InstallIwa(InstallType::kDevMode, /*grant_permissions_policy=*/true);

  content::RenderFrameHost* app_frame = OpenApp(url_info.app_id());
  ASSERT_NE(app_frame, nullptr);

  for (const char* attribute_name : kDeviceAttributeNames) {
    EXPECT_THAT(CallDeviceAttributesApi(app_frame, attribute_name),
                content::EvalJsResult::ErrorIs(
                    HasSubstr(kNotSupportedPlatformErrorMessage)));
  }
}

IN_PROC_BROWSER_TEST_F(
    IsolatedWebAppDeviceAttributesNonChromeOSBrowserTest,
    DevModeIwaWithoutPermissionsPolicyReturnsPermissionsPolicyError) {
  IsolatedWebAppUrlInfo url_info =
      InstallIwa(InstallType::kDevMode, /*grant_permissions_policy=*/false);

  content::RenderFrameHost* app_frame = OpenApp(url_info.app_id());
  ASSERT_NE(app_frame, nullptr);

  for (const char* attribute_name : kDeviceAttributeNames) {
    EXPECT_THAT(
        CallDeviceAttributesApi(app_frame, attribute_name),
        content::EvalJsResult::ErrorIs(HasSubstr(kPermissionsPolicyError)));
  }
}

IN_PROC_BROWSER_TEST_F(
    IsolatedWebAppDeviceAttributesNonChromeOSBrowserTest,
    PolicyInstalledIwaWithPermissionsPolicyReturnsNotSupportedError) {
  IsolatedWebAppUrlInfo url_info =
      InstallIwa(InstallType::kPolicy, /*grant_permissions_policy=*/true);

  content::RenderFrameHost* app_frame = OpenApp(url_info.app_id());
  ASSERT_NE(app_frame, nullptr);

  for (const char* attribute_name : kDeviceAttributeNames) {
    EXPECT_THAT(CallDeviceAttributesApi(app_frame, attribute_name),
                content::EvalJsResult::ErrorIs(
                    HasSubstr(kNotSupportedPlatformErrorMessage)));
  }
}


}  // namespace web_app
