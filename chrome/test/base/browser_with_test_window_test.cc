// Copyright 2012 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/test/base/browser_with_test_window_test.h"

#include <memory>
#include <vector>

#include "base/command_line.h"
#include "base/location.h"
#include "base/run_loop.h"
#include "build/build_config.h"
#include "chrome/browser/net/system_network_context_manager.h"
#include "chrome/browser/profiles/profile_destroyer.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/test/base/testing_browser_process.h"
#include "chrome/test/base/testing_profile.h"
#include "chrome/test/base/testing_profile_manager.h"
#include "components/sync_preferences/pref_service_syncable.h"
#include "content/public/browser/navigation_controller.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/navigation_simulator.h"
#include "content/public/test/test_renderer_host.h"
#include "content/public/test/test_utils.h"

#if defined(TOOLKIT_VIEWS)
#include "chrome/browser/ui/views/chrome_constrained_window_views_client.h"
#include "components/constrained_window/constrained_window_views.h"
#endif


using content::NavigationController;
using content::RenderFrameHost;
using content::RenderFrameHostTester;
using content::WebContents;

BrowserWithTestWindowTest::~BrowserWithTestWindowTest() = default;

void BrowserWithTestWindowTest::SetUp() {
  testing::Test::SetUp();

  base::CommandLine::ForCurrentProcess()->AppendSwitch(switches::kNoFirstRun);
  if (!profile_manager_) {
    SetUpProfileManager();
  }


  // This must be created after |ash_test_helper_| is set up so that it doesn't
  // create a DeviceDataManager.
  rvh_test_enabler_ = std::make_unique<content::RenderViewHostTestEnabler>();

#if defined(TOOLKIT_VIEWS)
  SetConstrainedWindowViewsClient(CreateChromeConstrainedWindowViewsClient());
#endif

  user_performance_tuning_manager_environment_.SetUp(
      TestingBrowserProcess::GetGlobal()->local_state());


  // Subclasses can provide their own Profile name.
  std::optional<std::string> profile_name = GetDefaultProfileName();
  if (profile_name) {
    profile_ = CreateProfile(*profile_name)->GetWeakPtr();
  }
}

void BrowserWithTestWindowTest::TearDown() {
  // Some tests end up posting tasks to the DB thread that must be completed
  // before the profile can be destroyed and the test safely shut down.
  base::RunLoop().RunUntilIdle();

#if defined(TOOLKIT_VIEWS)
  constrained_window::SetConstrainedWindowViewsClient(nullptr);
#endif

  // Depends on LocalState owned by |profile_manager_|.
  if (SystemNetworkContextManager::GetInstance()) {
    SystemNetworkContextManager::DeleteInstance();
  }


  user_performance_tuning_manager_environment_.TearDown();


  // Calling DeleteAllTestingProfiles() first can cause issues in some tests, if
  // they're still holding a ScopedProfileKeepAlive.
  profile_ = nullptr;
  profile_manager_.reset();

#if defined(TOOLKIT_VIEWS)
  views_test_helper_.reset();
#endif

  testing::Test::TearDown();

  // A Task is leaked if we don't destroy everything, then run all pending
  // tasks. This includes backend tasks which could otherwise be affected by the
  // deletion of the temp dir.
  task_environment_->RunUntilIdle();
}

void BrowserWithTestWindowTest::SetUpProfileManager(
    const base::FilePath& profiles_path,
    std::unique_ptr<ProfileManager> profile_manager) {
  profile_manager_ = std::make_unique<TestingProfileManager>(
      TestingBrowserProcess::GetGlobal());
  ASSERT_TRUE(
      profile_manager_->SetUp(profiles_path, std::move(profile_manager)));
}

gfx::NativeWindow BrowserWithTestWindowTest::GetContext() {
#if defined(TOOLKIT_VIEWS)
  return views_test_helper_->GetContext();
#else
  return nullptr;
#endif
}

void BrowserWithTestWindowTest::CommitPendingLoad(
    NavigationController* controller) {
  if (!controller->GetPendingEntry()) {
    return;  // Nothing to commit.
  }

  RenderFrameHostTester::CommitPendingLoad(controller);
}

void BrowserWithTestWindowTest::NavigateAndCommit(WebContents* web_contents,
                                                  const GURL& url) {
  content::NavigationSimulator::NavigateAndCommitFromBrowser(web_contents, url);
}

void BrowserWithTestWindowTest::NavigateAndCommitActiveTabWithTitle(
    BrowserWindowInterface* navigating_browser,
    const GURL& url,
    const std::u16string& title) {
  WebContents* contents =
      navigating_browser->GetTabStripModel()->GetActiveWebContents();
  NavigateAndCommit(contents, url);
  contents->UpdateTitleForEntry(contents->GetController().GetActiveEntry(),
                                title);
}

std::optional<std::string> BrowserWithTestWindowTest::GetDefaultProfileName() {
  return TestingProfile::kDefaultProfileUserName;
}

TestingProfile* BrowserWithTestWindowTest::CreateProfile(
    const std::string& profile_name) {
  auto* profile = profile_manager_->CreateTestingProfile(
      profile_name, /*prefs=*/nullptr, /*user_name=*/std::u16string(),
      /*avatar_id=*/0, GetTestingFactories());
  return profile;
}

void BrowserWithTestWindowTest::DeleteProfile(const std::string& profile_name) {
  if (profile_name == GetDefaultProfileName()) {
    profile_ = nullptr;
  }
  profile_manager_->DeleteTestingProfile(profile_name);
}

TestingProfile::TestingFactories
BrowserWithTestWindowTest::GetTestingFactories() {
  return {};
}


BrowserWithTestWindowTest::BrowserWithTestWindowTest(
    std::unique_ptr<content::BrowserTaskEnvironment> task_environment,
    BrowserWindowInterface::Type browser_type,
    bool hosted_app)
    : task_environment_(std::move(task_environment)) {}
