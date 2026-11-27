// Copyright 2024 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/tabs/tab_strip_action_container.h"

#include <memory>
#include <vector>

#include "base/check_deref.h"
#include "base/test/scoped_feature_list.h"
#include "chrome/browser/ui/browser_window/test/mock_browser_window_interface.h"
#include "chrome/browser/ui/call_to_action/call_to_action_lock.h"
#include "chrome/browser/ui/tabs/tab_list_bridge.h"
#include "chrome/browser/ui/tabs/tab_model.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/tabs/test_tab_strip_model_delegate.h"
#include "chrome/browser/ui/views/tabs/tab_strip_nudge_button.h"
#include "chrome/common/chrome_features.h"
#include "chrome/test/base/testing_browser_process.h"
#include "chrome/test/base/testing_profile.h"
#include "chrome/test/base/testing_profile_manager.h"
#include "chrome/test/views/chrome_views_test_base.h"
#include "components/tabs/public/mock_tab_interface.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/test_renderer_host.h"
#include "content/public/test/test_web_contents_factory.h"
#include "content/public/test/web_contents_tester.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/gfx/animation/animation_test_api.h"
#include "ui/views/test/views_test_utils.h"
#include "ui/views/widget/widget.h"


// TODO(crbug.com/461140208): Re-enable failing tests on ChromeOS.
#define MAYBE(test_name) test_name

namespace {
using testing::SizeIs;
}  // namespace

class TabStripActionContainerTest : public ChromeViewsTestBase {
 public:
  explicit TabStripActionContainerTest(
      const std::vector<base::test::FeatureRef>& additional_features = {})
      : animation_mode_reset_(gfx::AnimationTestApi::SetRichAnimationRenderMode(
            gfx::Animation::RichAnimationRenderMode::FORCE_ENABLED)) {
    std::vector<base::test::FeatureRefAndParams> enabled_features = {


        };
    for (const auto& feature : additional_features) {
      enabled_features.emplace_back(*feature, base::FieldTrialParams{});
    }
    std::vector<base::test::FeatureRef> disabled_features;
    scoped_feature_list_.InitWithFeaturesAndParameters(enabled_features,
                                                       disabled_features);
  }
  TabStripActionContainerTest(const TabStripActionContainerTest&) = delete;
  TabStripActionContainerTest& operator=(const TabStripActionContainerTest&) =
      delete;
  ~TabStripActionContainerTest() override = default;

  void SetUp() override {
    raw_ptr<TestingProfileManager> testing_profile_manager =
        TestingBrowserProcess::GetGlobal()->SetUpGlobalFeaturesForTesting(
            /*profile_manager=*/true);
    ChromeViewsTestBase::SetUp();
    profile_ = testing_profile_manager->CreateTestingProfile(
        TestingProfile::kDefaultProfileUserName);
    web_contents_ = content::WebContentsTester::CreateTestWebContents(
        profile_.get(), nullptr);
  }

  void TearDown() override {
    tab_strip_action_container_.reset();
    browser_window_interface_.reset();
    tab_interface_.reset();
    tab_list_bridge_.reset();
    raw_web_contents_ = nullptr;
    tab_strip_model_.reset();

    web_contents_.reset();
    profile_ = nullptr;

    ChromeViewsTestBase::TearDown();

    TestingBrowserProcess::GetGlobal()->TearDownGlobalFeaturesForTesting();
  }

  Profile* GetProfile(bool use_otr_profile) {
    if (use_otr_profile) {
      return TestingProfile::Builder().BuildOffTheRecord(
          profile_.get(), Profile::OTRProfileID::CreateUniqueForTesting());
    }
    return profile_.get();
  }


 protected:
  std::unique_ptr<TabStripModel> tab_strip_model_;
  std::unique_ptr<TabListBridge> tab_list_bridge_;

  std::unique_ptr<tabs::MockTabInterface> tab_interface_;
  std::unique_ptr<MockBrowserWindowInterface> browser_window_interface_;
  ui::UnownedUserDataHost data_host_;
  std::unique_ptr<CallToActionLock> call_to_action_;
  TestTabStripModelDelegate tab_strip_model_delegate_;
  base::test::ScopedFeatureList scoped_feature_list_;
  std::unique_ptr<views::View> locked_expansion_view_;
  std::unique_ptr<TabStripActionContainer> tab_strip_action_container_;

  content::WebContents* web_contents() { return raw_web_contents_; }

  void SimulateActiveTabChanged() {
    auto new_contents = content::WebContentsTester::CreateTestWebContents(
        profile_.get(), nullptr);
    tab_strip_model_->AppendWebContents(std::move(new_contents),
                                        /*foreground=*/true);
  }

 private:
  // Owned by TabStrip.

  content::RenderViewHostTestEnabler render_view_host_test_enabler_;
  raw_ptr<TestingProfile> profile_ = nullptr;
  std::unique_ptr<content::WebContents> web_contents_;
  raw_ptr<content::WebContents> raw_web_contents_ = nullptr;
  gfx::AnimationTestApi::RenderModeResetter animation_mode_reset_;
  tabs::TabModel::PreventFeatureInitializationForTesting
      prevent_feature_initialization_;
};



TEST_F(TabStripActionContainerTest,
       MAYBE(OrdersButtonsCorrectlyAtConstruction)) {

// TODO(crbug.com/437141881): Fix flaky tests on Mac.
// Mac doesn't have a separator, so the children sizes are different.
#if !BUILDFLAG(IS_MAC)
  ASSERT_THAT(tab_strip_action_container_->children(), SizeIs(3));



#endif  // !BUILDFLAG(IS_MAC)
}

TEST_F(TabStripActionContainerTest, MAYBE(OrdersButtonsCorrectlyWhenShown)) {

// TODO(crbug.com/437141881): Fix flaky tests on Mac.
// Mac doesn't have a separator, so the children sizes are different.
#if !BUILDFLAG(IS_MAC)

  // Before `ShowGlicActorTaskIcon()` is called, `glic_button` is a direct
  // child `tab_strip_action_container_`. When `ShowGlicActorTaskIcon()` is
  // called, `glic_button` is moved from being a direct child of
  // `tab_strip_action_container_` to being a child of
  // `glic_actor_button_container()`.
  ASSERT_THAT(tab_strip_action_container_->children(), SizeIs(2));




#endif  // !BUILDFLAG(IS_MAC)
}



#if !BUILDFLAG(IS_MAC)
class TabStripActionContainerWithToolbarButtonTest
    : public TabStripActionContainerTest {
 public:
  TabStripActionContainerWithToolbarButtonTest()
       {}
};

TEST_F(TabStripActionContainerWithToolbarButtonTest,
       SeparatorHiddenWhenToolbarButtonEnabled) {
  ASSERT_TRUE(tab_strip_action_container_->GetSeparatorForTesting());
  EXPECT_FALSE(
      tab_strip_action_container_->GetSeparatorForTesting()->GetVisible());
  EXPECT_FALSE(
      tab_strip_action_container_->GetSeparatorForTesting()->GetVisible());
}

#endif  // !BUILDFLAG(IS_MAC)
