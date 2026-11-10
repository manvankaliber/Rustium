// Copyright 2020 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/browsing_data/browsing_data_history_observer_service.h"

#include <set>
#include <utility>

#include "base/test/metrics/histogram_tester.h"
#include "base/test/scoped_feature_list.h"
#include "base/time/time.h"
#include "build/build_config.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/test/base/testing_profile.h"
#include "components/history/core/browser/history_types.h"
#include "components/safe_browsing/core/browser/suspicious_site_warning_allowlist.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"


class BrowsingDataHistoryObserverServiceTest : public testing::Test {
 public:
  BrowsingDataHistoryObserverServiceTest() = default;

 protected:
  content::BrowserTaskEnvironment task_environment_;
  base::test::ScopedFeatureList scoped_feature_list_;
};


TEST_F(BrowsingDataHistoryObserverServiceTest,
       SuspiciousSiteWarningAllowlist_AllHistoryCleared) {
  std::unique_ptr<TestingProfile> profile = TestingProfile::Builder().Build();
  BrowsingDataHistoryObserverService service(profile.get());
  HostContentSettingsMap* hcsm =
      HostContentSettingsMapFactory::GetForProfile(profile.get());
  safe_browsing::SuspiciousSiteWarningAllowlist allowlist(hcsm);

  allowlist.AllowSiteForHost("suspicious.test");
  ASSERT_TRUE(allowlist.IsSiteAllowedForHost("suspicious.test"));

  history::DeletionInfo deletion_info = history::DeletionInfo::ForAllHistory();
  service.OnHistoryDeletions(/*history_service=*/nullptr, deletion_info);

  EXPECT_FALSE(allowlist.IsSiteAllowedForHost("suspicious.test"));
}

TEST_F(BrowsingDataHistoryObserverServiceTest,
       SuspiciousSiteWarningAllowlist_OriginUrlsDeleted) {
  std::unique_ptr<TestingProfile> profile = TestingProfile::Builder().Build();
  BrowsingDataHistoryObserverService service(profile.get());
  HostContentSettingsMap* hcsm =
      HostContentSettingsMapFactory::GetForProfile(profile.get());
  safe_browsing::SuspiciousSiteWarningAllowlist allowlist(hcsm);

  allowlist.AllowSiteForHost("deleted.test");
  allowlist.AllowSiteForHost("retained.test");
  allowlist.AllowSiteForHost("unrelated.test");
  ASSERT_TRUE(allowlist.IsSiteAllowedForHost("deleted.test"));
  ASSERT_TRUE(allowlist.IsSiteAllowedForHost("retained.test"));
  ASSERT_TRUE(allowlist.IsSiteAllowedForHost("unrelated.test"));

  // deleted.test has 0 remaining visits; retained.test has 1 remaining visit.
  history::OriginCountAndLastVisitMap origin_map;
  origin_map[GURL("https://deleted.test")] = {0, base::Time::Now()};
  origin_map[GURL("https://retained.test")] = {1, base::Time::Now()};

  history::DeletionInfo deletion_info = history::DeletionInfo::ForUrls(
      /*deleted_rows=*/{}, /*favicon_urls=*/{});
  deletion_info.set_deleted_urls_origin_map(std::move(origin_map));

  service.OnHistoryDeletions(/*history_service=*/nullptr, deletion_info);

  // deleted.test allowlist should be revoked, retained.test and unrelated.test
  // should remain.
  EXPECT_FALSE(allowlist.IsSiteAllowedForHost("deleted.test"));
  EXPECT_TRUE(allowlist.IsSiteAllowedForHost("retained.test"));
  EXPECT_TRUE(allowlist.IsSiteAllowedForHost("unrelated.test"));
}

TEST_F(BrowsingDataHistoryObserverServiceTest,
       SuspiciousSiteWarningAllowlist_TimeRangeWithRemainingVisitsRetained) {
  std::unique_ptr<TestingProfile> profile = TestingProfile::Builder().Build();
  BrowsingDataHistoryObserverService service(profile.get());
  HostContentSettingsMap* hcsm =
      HostContentSettingsMapFactory::GetForProfile(profile.get());
  safe_browsing::SuspiciousSiteWarningAllowlist allowlist(hcsm);

  allowlist.AllowSiteForHost("testsafebrowsing.appspot.com");
  ASSERT_TRUE(allowlist.IsSiteAllowedForHost("testsafebrowsing.appspot.com"));

  // Simulate deleting 1 sub-URL where another visit to that domain remains
  // (count = 1).
  history::OriginCountAndLastVisitMap origin_map;
  origin_map[GURL("https://testsafebrowsing.appspot.com")] = {
      1, base::Time::Now()};

  base::Time now = base::Time::Now();
  history::DeletionInfo deletion_info(
      history::DeletionTimeRange(now - base::Days(1), now),
      /*is_from_expiration=*/false, /*deleted_rows=*/{},
      /*favicon_urls=*/{},
      /*restrict_urls=*/
      std::set<GURL>{GURL(
          "https://testsafebrowsing.appspot.com/s/rt_suspicious_warn.html")});
  deletion_info.set_deleted_urls_origin_map(std::move(origin_map));

  service.OnHistoryDeletions(/*history_service=*/nullptr, deletion_info);

  // Allowlist should STILL be allowed because 1 visit remains.
  EXPECT_TRUE(allowlist.IsSiteAllowedForHost("testsafebrowsing.appspot.com"));
}
