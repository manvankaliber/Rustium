// Copyright 2019 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/test/scoped_feature_list.h"
#include "build/build_config.h"
#include "chrome/browser/policy/policy_test_utils.h"
#include "components/policy/core/common/policy_map.h"
#include "components/policy/policy_constants.h"
#include "content/public/browser/browser_accessibility_state.h"
#include "content/public/test/browser_test.h"
#include "testing/gtest/include/gtest/gtest.h"


namespace policy {


IN_PROC_BROWSER_TEST_F(PolicyTest, RendererAccessibilityEnabled) {
  content::BrowserAccessibilityState* accessibility_state =
      content::BrowserAccessibilityState::GetInstance();

  // Verify default state is enabled (AX mode changes allowed).
  EXPECT_TRUE(accessibility_state->IsAXModeChangeAllowed());

  // Disable renderer accessibility via policy.
  PolicyMap policies;
  policies.Set(key::kRendererAccessibilityEnabled, POLICY_LEVEL_MANDATORY,
               POLICY_SCOPE_USER, POLICY_SOURCE_CLOUD, base::Value(false),
               nullptr);
  UpdateProviderPolicy(policies);

  // Verify that policy disabled AX mode changes.
  EXPECT_FALSE(accessibility_state->IsAXModeChangeAllowed());

  // Re-enable policy dynamically (dynamic_refresh test).
  policies.Set(key::kRendererAccessibilityEnabled, POLICY_LEVEL_MANDATORY,
               POLICY_SCOPE_USER, POLICY_SOURCE_CLOUD, base::Value(true),
               nullptr);
  UpdateProviderPolicy(policies);

  EXPECT_TRUE(accessibility_state->IsAXModeChangeAllowed());
}

}  // namespace policy
