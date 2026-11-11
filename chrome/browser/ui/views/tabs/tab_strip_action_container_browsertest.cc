// Copyright 2024 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/tabs/tab_strip_action_container.h"

#include "base/feature_list.h"
#include "base/test/metrics/histogram_tester.h"
#include "base/test/metrics/user_action_tester.h"
#include "base/test/run_until.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/test_future.h"
#include "chrome/browser/optimization_guide/browser_test_util.h"
#include "chrome/browser/ui/browser_element_identifiers.h"
#include "chrome/browser/ui/browser_window/public/browser_window_features.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/views/controls/rich_hover_button.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/interaction/browser_elements_views.h"
#include "chrome/common/actor.mojom.h"
#include "chrome/common/chrome_features.h"
#include "chrome/grit/generated_resources.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/test/browser_test.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/events/base_event_utils.h"
#include "ui/gfx/animation/slide_animation.h"
#include "ui/views/view_utils.h"

#include "testing/gmock/include/gmock/gmock.h"

namespace {
using base::test::RunUntil;
using testing::SizeIs;

using ActorTaskNudgeState = actor::ui::ActorTaskNudgeState;

}  // namespace

class TabStripActionContainerBrowserTest : public InProcessBrowserTest {
 public:
  TabStripActionContainerBrowserTest() {
  }

  void SetUp() override {
    // This will temporarily disable preloading.

    InProcessBrowserTest::SetUp();
  }

  void TearDown() override {
    InProcessBrowserTest::TearDown();
  }

  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();
  }

  void TearDownOnMainThread() override {
    InProcessBrowserTest::TearDownOnMainThread();
  }

  void SetUpInProcessBrowserTestFixture() override {
    InProcessBrowserTest::SetUpInProcessBrowserTestFixture();
  }

  TabStripModel* tab_strip_model() { return browser()->GetTabStripModel(); }

  BrowserView* browser_view() {
    return BrowserView::GetBrowserViewForBrowser(browser());
  }

  TabStripActionContainer* tab_strip_action_container() {
    return BrowserElementsViews::From(browser())
        ->GetViewAs<TabStripActionContainer>(kTabStripActionContainerElementId);
  }

 protected:



  void ShowTabStripNudgeButton(TabStripNudgeButton* button) {
    tab_strip_action_container()->ShowTabStripNudge(button);
  }
  void HideTabStripNudgeButton(TabStripNudgeButton* button) {
    tab_strip_action_container()->HideTabStripNudge(button);
  }

  void OnTabStripNudgeButtonTimeout(TabStripNudgeButton* button) {
    tab_strip_action_container()->OnTabStripNudgeButtonTimeout(button);
  }

  gfx::SlideAnimation* GetExpansionAnimation(TabStripNudgeButton* button) {
    if (tab_strip_action_container()->ButtonOwnsAnimation(button)) {
      return button->GetExpansionAnimationForTesting();
    }
    return tab_strip_action_container()
        ->animation_session_for_testing()
        ->expansion_animation();
  }

  void SetLockedExpansionMode(LockedExpansionMode mode,
                              TabStripNudgeButton* button) {
    tab_strip_action_container()->SetLockedExpansionMode(mode, button);
  }
  void OnButtonClicked(TabStripNudgeButton* button) {
  }
  void OnButtonDismissed(TabStripNudgeButton* button) {
  }

  void ResetPrewarming() {  }

  void ResetAnimation(int value) {
    if (tab_strip_action_container()->animation_session_for_testing()) {
      tab_strip_action_container()
          ->animation_session_for_testing()
          ->ResetOpacityAnimationForTesting(value);
      if (tab_strip_action_container()->animation_session_for_testing()) {
        tab_strip_action_container()
            ->animation_session_for_testing()
            ->ResetExpansionAnimationForTesting(value);
      }
    }
  }

  void Click(views::View* clickable_view) {
    clickable_view->OnMousePressed(
        ui::MouseEvent(ui::EventType::kMousePressed, gfx::Point(), gfx::Point(),
                       ui::EventTimeForNow(), ui::EF_LEFT_MOUSE_BUTTON, 0));
    clickable_view->OnMouseReleased(ui::MouseEvent(
        ui::EventType::kMouseReleased, gfx::Point(), gfx::Point(),
        ui::EventTimeForNow(), ui::EF_LEFT_MOUSE_BUTTON, 0));
  }



 protected:
  base::test::ScopedFeatureList feature_list_;
};






// TODO(crbug.com/451697169): Fix this test for Windows and Linux.
// TODO(crbug.com/461145884): Enable on ChromeOS
// TODO(crbug.com/465247286): Fix this for Mac.
#if BUILDFLAG(IS_WIN) || BUILDFLAG(IS_LINUX) || BUILDFLAG(IS_MAC)
#define MAYBE_GlicLabelEnablementFollowsWindowActivation \
  DISABLED_GlicLabelEnablementFollowsWindowActivation
#else
#define MAYBE_GlicLabelEnablementFollowsWindowActivation \
  GlicLabelEnablementFollowsWindowActivation
#endif
