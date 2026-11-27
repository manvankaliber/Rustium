// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/profiles/profile.h"

#include "build/build_config.h"
#include "chrome/browser/profiles/profile_testing_helper.h"
#include "chrome/test/base/testing_profile.h"
#include "testing/gtest/include/gtest/gtest.h"

using ProfileTest = testing::Test;

TEST_F(ProfileTest, AllowsBrowserWindows) {
  ProfileTestingHelper helper;
  helper.SetUp();

  EXPECT_TRUE(helper.regular_profile()->AllowsBrowserWindows());
  EXPECT_TRUE(helper.incognito_profile()->AllowsBrowserWindows());

  EXPECT_FALSE(helper.guest_profile()->AllowsBrowserWindows());
  EXPECT_TRUE(helper.guest_profile_otr()->AllowsBrowserWindows());

  EXPECT_FALSE(helper.system_profile()->AllowsBrowserWindows());
  EXPECT_FALSE(helper.system_profile_otr()->AllowsBrowserWindows());

}
