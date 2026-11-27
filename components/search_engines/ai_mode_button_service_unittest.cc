// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/search_engines/ai_mode_button_service.h"

#include "components/search_engines/search_terms_data.h"
#include "components/search_engines/template_url.h"
#include "components/search_engines/template_url_data.h"
#include "testing/gtest/include/gtest/gtest.h"

TEST(AiModeButtonServiceTest, NoButtonForAnySearchEngine) {
  SearchTermsData terms;
  for (const char* url : {"https://www.google.com/search?q={searchTerms}",
                          "https://example.com/search?q={searchTerms}"}) {
    TemplateURLData data;
    data.SetURL(url);
    TemplateURL engine(data);
    EXPECT_FALSE(AiModeButtonService::GetUiConfigForTemplateUrl(engine, terms));
  }
  AiModeButtonService service(nullptr);
  EXPECT_EQ(nullptr, service.GetCurrentConfig());
}
