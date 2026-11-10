// Copyright 2022 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_PROFILES_PROFILE_TESTING_HELPER_H_
#define CHROME_BROWSER_PROFILES_PROFILE_TESTING_HELPER_H_

#include "base/memory/raw_ptr.h"
#include "base/test/scoped_feature_list.h"
#include "chrome/test/base/testing_profile_manager.h"
#include "content/public/test/browser_task_environment.h"

class TestingProfile;
class Profile;

// Testing helper class to provide easy access to Profile Types (both Original
// and Off The Record), Regular, Guest and System.
class ProfileTestingHelper {
 public:
  ProfileTestingHelper();
  ~ProfileTestingHelper();

  void SetUp();

  TestingProfile* regular_profile() { return regular_profile_; }
  Profile* incognito_profile() { return incognito_profile_; }

  TestingProfile* isolated_mode_parent_profile() {
    return isolated_mode_parent_profile_;
  }
  Profile* isolated_mode_profile() { return isolated_mode_profile_; }

  TestingProfile* guest_profile() { return guest_profile_; }
  Profile* guest_profile_otr() { return guest_profile_otr_; }

  TestingProfile* system_profile() { return system_profile_; }
  Profile* system_profile_otr() { return system_profile_otr_; }


 private:
  base::test::ScopedFeatureList scoped_feature_list_;
  content::BrowserTaskEnvironment task_environment_;
  TestingProfileManager manager_;

  raw_ptr<TestingProfile> regular_profile_ = nullptr;
  raw_ptr<Profile> incognito_profile_ = nullptr;

  raw_ptr<TestingProfile> isolated_mode_parent_profile_ = nullptr;
  raw_ptr<Profile> isolated_mode_profile_ = nullptr;

  raw_ptr<TestingProfile> guest_profile_ = nullptr;
  raw_ptr<Profile> guest_profile_otr_ = nullptr;

  raw_ptr<TestingProfile> system_profile_ = nullptr;
  raw_ptr<Profile> system_profile_otr_ = nullptr;

};

#endif  // CHROME_BROWSER_PROFILES_PROFILE_TESTING_HELPER_H_
