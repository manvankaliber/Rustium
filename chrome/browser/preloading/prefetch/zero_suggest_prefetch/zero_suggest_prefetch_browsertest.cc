// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/location.h"
#include "base/run_loop.h"
#include "base/scoped_observation.h"
#include "base/task/task_traits.h"
#include "base/test/run_until.h"
#include "base/test/scoped_feature_list.h"
#include "base/unguessable_token.h"
#include "build/build_config.h"
#include "chrome/browser/autocomplete/remote_suggestions_service_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/search_engines/template_url_service_factory.h"
#include "chrome/common/webui_url_constants.h"
#include "chrome/test/base/chrome_test_utils.h"
#include "chrome/test/base/platform_browser_test.h"
#include "chrome/test/base/search_test_utils.h"
#include "components/omnibox/browser/remote_suggestions_service.h"
#include "components/omnibox/common/omnibox_features.h"
#include "components/search_engines/template_url_service.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace {

class TestRemoteSuggestionsObserver
    : public RemoteSuggestionsService::Observer {
 public:
  explicit TestRemoteSuggestionsObserver(RemoteSuggestionsService* service) {
    observation_.Observe(service);
  }
  ~TestRemoteSuggestionsObserver() override = default;

  // RemoteSuggestionsService::Observer:
  void OnRequestCreated(const base::UnguessableToken&,
                        const network::ResourceRequest*) override {
    request_created_ = true;
  }

  bool request_created() const { return request_created_; }

 private:
  bool request_created_ = false;
  base::ScopedObservation<RemoteSuggestionsService,
                          RemoteSuggestionsService::Observer>
      observation_{this};
};

class ZeroSuggestPrefetchBrowserTest : public PlatformBrowserTest {
 public:
  ZeroSuggestPrefetchBrowserTest() {
    feature_list_.InitAndEnableFeature(
        omnibox::kZeroSuggestPrefetchOnPageLoadAndTabSwitch);
  }

 private:
  base::test::ScopedFeatureList feature_list_;
};

// Verifies that navigating to the New Tab Page (NTP) triggers zero-suggest
// prefetch once the page finishes loading across all platforms.
IN_PROC_BROWSER_TEST_F(ZeroSuggestPrefetchBrowserTest,
                       PrefetchTriggeredOnNtpLoad) {
  Profile* profile = chrome_test_utils::GetProfile(this);
  TemplateURLService* template_url_service =
      TemplateURLServiceFactory::GetForProfile(profile);
  search_test_utils::WaitForTemplateURLServiceToLoad(template_url_service);

  RemoteSuggestionsService* remote_suggestions_service =
      RemoteSuggestionsServiceFactory::GetForProfile(
          profile, /*create_if_necessary=*/true);
  ASSERT_TRUE(remote_suggestions_service);

  TestRemoteSuggestionsObserver observer(remote_suggestions_service);

  ASSERT_TRUE(chrome_test_utils::NavigateToURL(
      chrome_test_utils::GetActiveWebContents(this),
      chrome::ChromeUINewTabURLAsGURL()));

  EXPECT_TRUE(
      base::test::RunUntil([&]() { return observer.request_created(); }));
}


}  // namespace
