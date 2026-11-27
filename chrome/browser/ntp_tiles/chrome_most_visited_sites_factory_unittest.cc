// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ntp_tiles/chrome_most_visited_sites_factory.h"

#include <memory>
#include <string>
#include <utility>

#include "build/build_config.h"
#include "chrome/browser/search_engines/template_url_service_factory.h"
#include "chrome/browser/search_engines/template_url_service_test_util.h"
#include "components/ntp_tiles/most_visited_sites.h"
#include "components/ntp_tiles/tile_source.h"
#include "components/search_engines/template_url.h"
#include "components/search_engines/template_url_service.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"


namespace {

class ChromeMostVisitedSitesFactoryTest : public testing::Test {
 protected:
  void SetUp() override {
    test_util_ = std::make_unique<TemplateURLServiceTestUtil>();
    TemplateURLServiceFactory::GetInstance()->SetTestingFactory(
        profile(),
        TemplateURLServiceTestUtil::GetTemplateURLServiceTestingFactory());
  }

  void TearDown() override {
    test_util_.reset();
  }

  // Helper function to set the default search provider.
  void SetDefaultSearchProvider(const std::u16string& keyword,
                                const std::string& search_url) {
    std::unique_ptr<TemplateURL> turl =
        CreateTestTemplateURL(keyword, search_url);
    TemplateURL* added_turl = model()->Add(std::move(turl));
    model()->SetUserSelectedDefaultSearchProvider(added_turl);
    test_util_->ChangeModelToLoadState();
  }

  // Helper function to check if the popular sites source exists.
  bool DoesPopularSitesSourceExist() {
    std::unique_ptr<ntp_tiles::MostVisitedSites> most_visited_sites =
        ChromeMostVisitedSitesFactory::NewForProfile(profile());
    EXPECT_NE(nullptr, most_visited_sites);
    return most_visited_sites &&
           most_visited_sites->DoesSourceExist(ntp_tiles::TileSource::POPULAR);
  }

  TemplateURLService* model() { return test_util_->model(); }
  TestingProfile* profile() { return test_util_->profile(); }

  content::BrowserTaskEnvironment task_environment_;
  std::unique_ptr<TemplateURLServiceTestUtil> test_util_;
};

TEST_F(ChromeMostVisitedSitesFactoryTest, OffTheRecordReturnsNull) {
  Profile* otr_profile =
      profile()->GetPrimaryOTRProfile(/*create_if_needed=*/true);
  EXPECT_EQ(nullptr, ChromeMostVisitedSitesFactory::NewForProfile(otr_profile));
}

TEST_F(ChromeMostVisitedSitesFactoryTest, DesktopDoesNotCreatePopularSites) {
  // Arrange
  SetDefaultSearchProvider(u"google",
                           "https://www.google.com/search?q={searchTerms}");

  // Act & Assert
  EXPECT_FALSE(DoesPopularSitesSourceExist());
}
}  // namespace
