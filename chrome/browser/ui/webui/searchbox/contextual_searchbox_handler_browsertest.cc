// Copyright 2025 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/cr_components/searchbox/contextual_searchbox_handler.h"

#include <tuple>
#include <vector>

#include "base/base64.h"
#include "base/strings/strcat.h"
#include "base/test/bind.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/test_future.h"
#include "chrome/browser/contextual_search/contextual_search_service_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/search_engines/template_url_service_factory.h"
#include "chrome/browser/signin/identity_manager_factory.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
#include "chrome/browser/ui/contextual_search/tab_contextualization_controller.h"
#include "chrome/browser/ui/location_bar/location_bar.h"
#include "chrome/browser/ui/omnibox/omnibox_controller.h"
#include "chrome/browser/ui/omnibox/omnibox_popup_state_manager.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/webui/new_tab_page/composebox/variations/composebox_fieldtrial.h"
#include "chrome/browser/ui/webui/omnibox_popup/omnibox_popup_web_contents_helper.h"
#include "chrome/browser/ui/webui/searchbox/searchbox_test_utils.h"
#include "chrome/browser/ui/webui/searchbox/webui_omnibox_handler.h"
#include "chrome/browser/ui/webui/webui_embedding_context.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/search_test_utils.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/contextual_search/contextual_search_service.h"
#include "components/contextual_search/contextual_search_session_handle.h"
#include "components/contextual_search/pref_names.h"
#include "components/contextual_tasks/public/features.h"
#include "components/omnibox/common/omnibox_features.h"
#include "components/prefs/pref_service.h"
#include "components/signin/public/base/signin_switches.h"
#include "components/signin/public/identity_manager/identity_test_utils.h"
#include "components/tabs/public/tab_interface.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/web_contents_observer_test_utils.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "ui/base/mojom/window_open_disposition.mojom-shared.h"
#include "ui/base/page_transition_types.h"
#include "ui/display/display_switches.h"
#include "ui/gfx/codec/png_codec.h"
#include "ui/views/interaction/element_tracker_views.h"

#if BUILDFLAG(ENABLE_DICE_SUPPORT)
#include "chrome/browser/signin/chrome_signin_client_test_util.h"
#include "chrome/browser/ui/signin/promos/bubble_signin_promo_signin_button_view.h"
#include "chrome/common/url_constants.h"
#include "ui/base/interaction/element_tracker.h"
#include "ui/events/event.h"
#include "ui/events/types/event_type.h"
#include "ui/views/controls/button/button.h"
#include "ui/views/test/button_test_api.h"
#include "ui/views/view_utils.h"
#endif

class TestSearchboxHandler : public ContextualSearchboxHandler {
 public:
  TestSearchboxHandler(
      mojo::PendingReceiver<searchbox::mojom::PageHandler> pending_page_handler,
      mojo::PendingRemote<searchbox::mojom::Page> pending_page,
      Profile* profile,
      content::WebContents* web_contents,
      GetSessionHandleCallback get_session_callback)
      : ContextualSearchboxHandler(std::move(pending_page_handler),
                                   std::move(pending_page),
                                   profile,
                                   web_contents,
                                   std::make_unique<TestOmniboxClient>(),
                                   std::move(get_session_callback)) {}

  ~TestSearchboxHandler() override = default;

  void OnThumbnailRemoved() override {}
};

class ContextualSearchboxHandlerBrowserTest : public InProcessBrowserTest {
 public:
  ContextualSearchboxHandlerBrowserTest() {
    scoped_feature_list_.InitWithFeaturesAndParameters(
        {{contextual_tasks::kContextualTasksContext,
          {{"ContextualTasksContextSmartTabSharing", "true"}}},
         {contextual_tasks::kContextualTasksForceEntryPointEligibility, {}}},
        {});
  }

 protected:
  base::test::ScopedFeatureList scoped_feature_list_;
  testing::NiceMock<MockSearchboxPage> page_;
  std::unique_ptr<contextual_search::ContextualSearchSessionHandle>
      session_handle_;
  std::unique_ptr<TestSearchboxHandler> handler_;

  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();

    auto* service =
        ContextualSearchServiceFactory::GetForProfile(browser()->GetProfile());
    session_handle_ = service->CreateSession(
        ntp_composebox::CreateQueryControllerConfigParams(),
        contextual_search::ContextualSearchSource::kUnknown,
        /*invocation_source=*/std::nullopt);
    // Check the search content sharing settings to notify the session handle
    // that the client is properly checking the pref value.
    session_handle_->CheckSearchContentSharingSettings(
        browser()->GetProfile()->GetPrefs());

    handler_ = std::make_unique<TestSearchboxHandler>(
        mojo::PendingReceiver<searchbox::mojom::PageHandler>(),
        page_.BindAndGetRemote(), browser()->GetProfile(),
        /*web_contents=*/browser()->GetTabStripModel()->GetActiveWebContents(),
        base::BindLambdaForTesting([&]() { return session_handle_.get(); }));
  }

  void SetUpCommandLine(base::CommandLine* command_line) override {
    InProcessBrowserTest::SetUpCommandLine(command_line);
    command_line->RemoveSwitch(switches::kForceDeviceScaleFactor);
    command_line->AppendSwitchASCII(switches::kForceDeviceScaleFactor, "1");
  }

  void TearDownOnMainThread() override { handler_.reset(); }

  // Creates a popup WebContents with OmniboxPopupWebContentsHelper attached,
  // simulating the AIM / Full WebUI Omnibox popup.
  std::unique_ptr<content::WebContents> CreateAimPopupWebContents() {
    std::unique_ptr<content::WebContents> popup_web_contents =
        content::WebContents::Create(
            content::WebContents::CreateParams(browser()->GetProfile()));
    webui::SetBrowserWindowInterface(popup_web_contents.get(), browser());
    OmniboxPopupWebContentsHelper::CreateForWebContents(
        popup_web_contents.get());
    OmniboxController* omnibox_controller =
        BrowserWindow::FromBrowser(browser())
            ->GetLocationBar()
            ->GetOmniboxController();
    OmniboxPopupWebContentsHelper::FromWebContents(popup_web_contents.get())
        ->set_omnibox_controller(omnibox_controller);
    omnibox_controller->popup_state_manager()->SetPopupState(
        OmniboxPopupState::kAim);
    return popup_web_contents;
  }
};

IN_PROC_BROWSER_TEST_F(ContextualSearchboxHandlerBrowserTest,
                       CreateTabPreviewEncodingOptions_NotScaled) {
  // When no device scale factor is applied, physical pixels should translate to
  // CSS pixels at a 1:1 ratio.
  int expected_width = 125 * 1;
  int expected_height = 200 * 1;

  content::WebContents* web_contents =
      browser()->GetTabStripModel()->GetActiveWebContents();
  auto options = handler_->CreateTabPreviewEncodingOptions(web_contents);

  ASSERT_TRUE(options.has_value());
  EXPECT_EQ(options->max_width, expected_width);
  EXPECT_EQ(options->max_height, expected_height);
}

class ContextualSearchboxHandlerBrowserTestDSF2
    : public ContextualSearchboxHandlerBrowserTest {
 protected:
  void SetUpCommandLine(base::CommandLine* command_line) override {
    ContextualSearchboxHandlerBrowserTest::SetUpCommandLine(command_line);
    command_line->RemoveSwitch(switches::kForceDeviceScaleFactor);
    command_line->AppendSwitchASCII(switches::kForceDeviceScaleFactor, "2");
  }
};

IN_PROC_BROWSER_TEST_F(ContextualSearchboxHandlerBrowserTestDSF2,
                       CreateTabPreviewEncodingOptions_Scaled) {
  // 60 physical pixels translates to 30 CSS pixels when the device scale factor
  // = 2 (2 physical pixels : 1 CSS pixel);
  int expected_width = 125 * 2;
  int expected_height = 200 * 2;

  content::WebContents* web_contents =
      browser()->GetTabStripModel()->GetActiveWebContents();
  auto options = handler_->CreateTabPreviewEncodingOptions(web_contents);

  ASSERT_TRUE(options.has_value());
  EXPECT_EQ(options->max_width, expected_width);
  EXPECT_EQ(options->max_height, expected_height);
}

IN_PROC_BROWSER_TEST_F(ContextualSearchboxHandlerBrowserTest,
                       ResetInputStateModel) {
  // Access private member via friend class.
  ContextualSearchboxHandler* base_handler = handler_.get();
  ASSERT_TRUE(base_handler->input_state_model_);

  handler_->ResetInputStateModel();

  EXPECT_FALSE(base_handler->input_state_model_);
}

IN_PROC_BROWSER_TEST_F(ContextualSearchboxHandlerBrowserTest,
                       WaitForTabFaviconLoad_Async) {
  ASSERT_TRUE(embedded_test_server()->Start());
  GURL url = embedded_test_server()->GetURL("/favicon/page_with_favicon.html");

  ui_test_utils::NavigateToURLWithDisposition(
      browser(), url, ui::mojom::WindowOpenDisposition::kCurrentTab,
      ui_test_utils::BROWSER_TEST_NO_WAIT);

  tabs::TabInterface* tab = browser()->GetTabStripModel()->GetActiveTab();
  ASSERT_TRUE(tab);
  int32_t tab_id = tab->GetHandle().raw_value();

  base::test::TestFuture<const std::optional<GURL>&> future;
  handler_->WaitForTabFaviconLoad(tab_id, future.GetCallback());
  std::optional<GURL> data_url = future.Get();
  ASSERT_TRUE(data_url.has_value());
  EXPECT_TRUE(data_url->spec().starts_with("data:image/png;base64,"));
}

IN_PROC_BROWSER_TEST_F(ContextualSearchboxHandlerBrowserTest,
                       WaitForTabFaviconLoad_Cached) {
  ASSERT_TRUE(embedded_test_server()->Start());
  GURL url = embedded_test_server()->GetURL("/favicon/page_with_favicon.html");
  ASSERT_TRUE(ui_test_utils::NavigateToURL(browser(), url));

  tabs::TabInterface* tab = browser()->GetTabStripModel()->GetActiveTab();
  ASSERT_TRUE(tab);
  int32_t tab_id = tab->GetHandle().raw_value();

  // Ensure the favicon is fully processed and valid in the browser.
  {
    base::test::TestFuture<const std::optional<GURL>&> future;
    handler_->WaitForTabFaviconLoad(tab_id, future.GetCallback());
    ASSERT_TRUE(future.Get().has_value());
  }

  // The cached favicon data URL should be returned immediately at 2x (32x32)
  // resolution for crisp rendering on HiDPI displays.
  {
    base::test::TestFuture<const std::optional<GURL>&> future;
    handler_->WaitForTabFaviconLoad(tab_id, future.GetCallback());
    std::optional<GURL> data_url = future.Get();
    ASSERT_TRUE(data_url.has_value());
    constexpr std::string_view kPrefix = "data:image/png;base64,";
    ASSERT_TRUE(data_url->spec().starts_with(kPrefix));
    std::optional<std::vector<uint8_t>> png_bytes = base::Base64Decode(
        std::string_view(data_url->spec()).substr(kPrefix.size()));
    ASSERT_TRUE(png_bytes.has_value());
    SkBitmap bitmap = gfx::PNGCodec::Decode(*png_bytes);
    EXPECT_EQ(bitmap.width(), 32);
    EXPECT_EQ(bitmap.height(), 32);
  }
}

IN_PROC_BROWSER_TEST_F(ContextualSearchboxHandlerBrowserTest,
                       WaitForTabFaviconLoad_WebContentsDestroyed) {
  ASSERT_TRUE(embedded_test_server()->Start());
  GURL url = embedded_test_server()->GetURL("/empty.html");

  ui_test_utils::NavigateToURLWithDisposition(
      browser(), url, ui::mojom::WindowOpenDisposition::kNewForegroundTab,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_LOAD_STOP);

  tabs::TabInterface* tab = browser()->GetTabStripModel()->GetActiveTab();
  ASSERT_TRUE(tab);
  int32_t tab_id = tab->GetHandle().raw_value();

  base::test::TestFuture<const std::optional<GURL>&> future;
  handler_->WaitForTabFaviconLoad(tab_id, future.GetCallback());

  // Destroy the WebContents by closing the tab.
  browser()->GetTabStripModel()->CloseSelectedTabs();

  std::optional<GURL> data_url = future.Get();
  EXPECT_FALSE(data_url.has_value());
}

IN_PROC_BROWSER_TEST_F(ContextualSearchboxHandlerBrowserTest,
                       SmartTabSharingActive) {
  base::test::TestFuture<bool> get_future;
  handler_->GetSmartTabSharingActive(get_future.GetCallback());
  EXPECT_FALSE(get_future.Get());

  handler_->SetSmartTabSharingActive(true);

  base::test::TestFuture<bool> get_future2;
  handler_->GetSmartTabSharingActive(get_future2.GetCallback());
  EXPECT_TRUE(get_future2.Get());
}

class WebuiOmniboxHandlerBrowserTest
    : public ContextualSearchboxHandlerBrowserTest {
 protected:
  void SetUpOnMainThread() override {
    ContextualSearchboxHandlerBrowserTest::SetUpOnMainThread();

    omnibox_controller_ = std::make_unique<OmniboxController>(
        std::make_unique<TestOmniboxClient>());
    omnibox_controller_->SetEditModelForTesting(
        std::make_unique<testing::NiceMock<MockOmniboxEditModel>>(
            omnibox_controller_.get()));

    omnibox_handler_ = std::make_unique<WebuiOmniboxHandler>(
        mojo::PendingReceiver<searchbox::mojom::PageHandler>(),
        omnibox_page_.BindAndGetRemote(),
        /*metrics_reporter=*/nullptr, omnibox_controller_.get(),
        browser()->GetProfile(),
        browser()->GetTabStripModel()->GetActiveWebContents(),
        base::BindLambdaForTesting([&]() { return session_handle_.get(); }));
  }

  void TearDownOnMainThread() override {
    omnibox_handler_.reset();
    omnibox_controller_.reset();
    ContextualSearchboxHandlerBrowserTest::TearDownOnMainThread();
  }

  testing::NiceMock<MockSearchboxPage> omnibox_page_;
  std::unique_ptr<OmniboxController> omnibox_controller_;
  std::unique_ptr<WebuiOmniboxHandler> omnibox_handler_;
};

IN_PROC_BROWSER_TEST_F(WebuiOmniboxHandlerBrowserTest,
                       AddTabContextRejectsTabFromDifferentProfile) {
  BrowserWindowInterface* incognito_browser = CreateIncognitoBrowser();
  tabs::TabInterface* incognito_tab =
      incognito_browser->GetTabStripModel()->GetActiveTab();
  ASSERT_TRUE(incognito_tab);
  ASSERT_NE(incognito_tab->GetProfile(), browser()->GetProfile());

  base::test::TestFuture<base::expected<
      base::UnguessableToken, contextual_search::ContextUploadErrorType>>
      future;
  omnibox_handler_->AddTabContext(incognito_tab->GetHandle().raw_value(),
                                  /*delay_upload=*/false,
                                  searchbox::mojom::TabAttachmentSource::kOther,
                                  future.GetCallback());

  auto result = future.Take();
  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error(),
            contextual_search::ContextUploadErrorType::kBrowserProcessingError);
}

IN_PROC_BROWSER_TEST_F(WebuiOmniboxHandlerBrowserTest,
                       AddTabContextRejectsWhenContentSharingDisabled) {
  browser()->GetProfile()->GetPrefs()->SetInteger(
      contextual_search::kSearchContentSharingSettings,
      static_cast<int>(
          contextual_search::SearchContentSharingSettingsValue::kDisabled));

  tabs::TabInterface* tab = browser()->GetTabStripModel()->GetActiveTab();
  ASSERT_TRUE(tab);

  base::test::TestFuture<base::expected<
      base::UnguessableToken, contextual_search::ContextUploadErrorType>>
      future;
  omnibox_handler_->AddTabContext(tab->GetHandle().raw_value(),
                                  /*delay_upload=*/false,
                                  searchbox::mojom::TabAttachmentSource::kOther,
                                  future.GetCallback());

  auto result = future.Take();
  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error(),
            contextual_search::ContextUploadErrorType::kBrowserProcessingError);
}

// Submits a query from a simulated omnibox AI Mode popup. Parameters: voice
// input, ContextualTasks enabled.
class ContextualSearchboxHandlerOmniboxPopupSubmitTest
    : public ContextualSearchboxHandlerBrowserTest,
      public testing::WithParamInterface<std::tuple<bool, bool>> {
 public:
  ContextualSearchboxHandlerOmniboxPopupSubmitTest() {
    std::vector<base::test::FeatureRefAndParams> enabled_features = {
        {contextual_tasks::kContextualTasksContext,
         {{"ContextualTasksContextSmartTabSharing", "true"}}},
        {contextual_tasks::kContextualTasksForceEntryPointEligibility, {}}};
    std::vector<base::test::FeatureRef> disabled_features;
    if (contextual_tasks_enabled()) {
      enabled_features.push_back({contextual_tasks::kContextualTasks, {}});
    } else {
      disabled_features = {contextual_tasks::kContextualTasks,
                           contextual_tasks::kContextualTasksSidePanel};
    }
    scoped_feature_list_.Reset();
    scoped_feature_list_.InitWithFeaturesAndParameters(enabled_features,
                                                       disabled_features);
  }

 protected:
  bool is_voice_search() const { return std::get<0>(GetParam()); }
  bool contextual_tasks_enabled() const { return std::get<1>(GetParam()); }
};

IN_PROC_BROWSER_TEST_P(ContextualSearchboxHandlerOmniboxPopupSubmitTest,
                       NavigatesActiveTabWithGeneratedTransition) {
  search_test_utils::WaitForTemplateURLServiceToLoad(
      TemplateURLServiceFactory::GetForProfile(browser()->GetProfile()));

  std::unique_ptr<content::WebContents> popup_web_contents =
      CreateAimPopupWebContents();

  testing::NiceMock<MockSearchboxPage> popup_page;
  auto popup_handler = std::make_unique<TestSearchboxHandler>(
      mojo::PendingReceiver<searchbox::mojom::PageHandler>(),
      popup_page.BindAndGetRemote(), browser()->GetProfile(),
      popup_web_contents.get(),
      base::BindLambdaForTesting([&]() { return session_handle_.get(); }));

  // Read the transition at start, before throttles can cancel or reissue the
  // navigation.
  base::test::TestFuture<ui::PageTransition> transition_future;
  content::NavigationStartObserver observer(
      browser()->GetTabStripModel()->GetActiveWebContents(),
      base::BindLambdaForTesting([&](content::NavigationHandle* handle) {
        if (handle->IsInPrimaryMainFrame() && !transition_future.IsReady()) {
          transition_future.SetValue(handle->GetPageTransition());
        }
      }));
  popup_handler->SubmitQuery("test query", /*mouse_button=*/0,
                             /*alt_key=*/false, /*ctrl_key=*/false,
                             /*meta_key=*/false, /*shift_key=*/false,
                             is_voice_search());

  // Omnibox searches use GENERATED. The actor leave confirmation does not
  // cover LINK navigations.
  const ui::PageTransition transition = transition_future.Get();
  EXPECT_TRUE(
      ui::PageTransitionCoreTypeIs(transition, ui::PAGE_TRANSITION_GENERATED))
      << ui::PageTransitionGetCoreTransitionString(transition);
}

INSTANTIATE_TEST_SUITE_P(
    All,
    ContextualSearchboxHandlerOmniboxPopupSubmitTest,
    testing::Combine(testing::Bool(), testing::Bool()),
    [](const testing::TestParamInfo<std::tuple<bool, bool>>& info) {
      return base::StrCat({std::get<0>(info.param) ? "Voice" : "Typed",
                           std::get<1>(info.param) ? "_ContextualTasksOn"
                                                   : "_ContextualTasksOff"});
    });
