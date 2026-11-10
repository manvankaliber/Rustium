// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/history/history_ui.h"

#include <memory>
#include <vector>

#include "base/functional/callback_helpers.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/test_future.h"
#include "build/build_config.h"
#include "chrome/browser/ui/webui/history/browsing_history_handler.h"
#include "chrome/browser/ui/webui/theme_source.h"
#include "chrome/common/url_constants.h"
#include "chrome/test/base/testing_profile.h"
#include "components/user_education/webui/user_education.mojom.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_task_environment.h"
#include "content/public/test/test_renderer_host.h"
#include "content/public/test/test_web_ui.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/base/mojom/window_open_disposition.mojom.h"
#include "ui/webui/resources/cr_components/history/foreign_sessions.mojom.h"
#include "ui/webui/resources/cr_components/history/history.mojom.h"
#include "ui/webui/resources/cr_components/history/history_cross_device_signin_promo.mojom.h"

namespace {

class HistoryUITest : public testing::Test {
 public:
  HistoryUITest() = default;
  ~HistoryUITest() override = default;

  content::TestWebUI* web_ui() { return &web_ui_; }
  Profile* profile() { return &profile_; }

 protected:
  void SetUp() override {
    web_contents_ = content::WebContents::Create(
        content::WebContents::CreateParams(&profile_));
    web_ui_.set_web_contents(web_contents_.get());
  }

  void TearDown() override {
    web_ui_.set_web_contents(nullptr);
    web_contents_.reset();
  }

  content::BrowserTaskEnvironment task_environment_;
  content::RenderViewHostTestEnabler rvh_enabler_;
  TestingProfile profile_;
  std::unique_ptr<content::WebContents> web_contents_;
  content::TestWebUI web_ui_;
};

TEST_F(HistoryUITest, InstantiationAndBindPageHandler) {
  auto history_ui = std::make_unique<HistoryUI>(web_ui());
  mojo::Remote<history::mojom::PageHandler> handler_remote;
  history_ui->BindInterface(handler_remote.BindNewPipeAndPassReceiver());
  EXPECT_NE(history_ui->GetBrowsingHistoryHandlerForTesting(), nullptr);
}

TEST_F(HistoryUITest, ThemeSourceProperties) {
  ThemeSource theme_source(profile());
  EXPECT_EQ(theme_source.GetSource(), chrome::kChromeUIThemeHost);
}

TEST_F(HistoryUITest, WebUIConfigEnabledByDefaultOnDesktop) {
  HistoryUIConfig config;
  EXPECT_TRUE(config.IsWebUIEnabled(profile()));
}

}  // namespace
