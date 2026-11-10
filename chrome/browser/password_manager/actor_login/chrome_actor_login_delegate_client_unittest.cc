// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/password_manager/actor_login/chrome_actor_login_delegate_client.h"

#include <memory>

#include "base/files/scoped_temp_dir.h"
#include "base/test/scoped_feature_list.h"
#include "chrome/browser/glic/glic_profile_manager.h"
#include "chrome/browser/glic/public/glic_enabling.h"
#include "chrome/browser/glic/public/glic_keyed_service.h"
#include "chrome/browser/glic/public/glic_keyed_service_factory.h"
#include "chrome/browser/glic/test_support/mock_glic_instance.h"
#include "chrome/browser/glic/test_support/mock_glic_keyed_service.h"
#include "chrome/browser/signin/identity_manager_factory.h"
#include "chrome/browser/tab_list/tab_list_interface.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
#include "chrome/browser/ui/browser_window/test/mock_browser_window_interface.h"
#include "chrome/test/base/chrome_render_view_host_test_harness.h"
#include "chrome/test/base/fake_profile_manager.h"
#include "chrome/test/base/testing_browser_process.h"
#include "components/password_manager/core/browser/features/password_features.h"
#include "components/tabs/public/mock_tab_interface.h"
#include "components/tabs/public/tab_interface.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/test_renderer_host.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/base/base_window.h"
#include "ui/base/mojom/window_show_state.mojom.h"
#include "ui/base/test/mock_base_window.h"
#include "ui/base/unowned_user_data/scoped_unowned_user_data.h"


namespace actor_login {
namespace {

using ::testing::NiceMock;
using ::testing::Return;
using ::testing::ReturnRef;


class ChromeActorLoginDelegateClientTest
    : public ChromeRenderViewHostTestHarness {
 public:
  ChromeActorLoginDelegateClientTest() = default;
  ~ChromeActorLoginDelegateClientTest() override = default;

  TestingProfile::TestingFactories GetTestingFactories() const override {
    return {
        TestingProfile::TestingFactory{
            glic::GlicKeyedServiceFactory::GetInstance(),
            base::BindRepeating(
                &ChromeActorLoginDelegateClientTest::CreateMockGlicService,
                base::Unretained(
                    const_cast<ChromeActorLoginDelegateClientTest*>(this)))},
    };
  }

  void SetUp() override {
    ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());
    TestingBrowserProcess::GetGlobal()->SetProfileManager(
        std::make_unique<FakeProfileManager>(temp_dir_.GetPath()));
    ChromeRenderViewHostTestHarness::SetUp();

    tabs::TabLookupFromWebContents::CreateForWebContents(web_contents(),
                                                         &mock_tab_);
    ON_CALL(mock_tab_, GetContents()).WillByDefault(Return(web_contents()));
    ON_CALL(mock_tab_, GetBrowserWindowInterface())
        .WillByDefault(Return(&mock_browser_window_));
    ON_CALL(mock_browser_window_, GetUnownedUserDataHost())
        .WillByDefault(ReturnRef(unowned_user_data_host_));


    ChromeActorLoginDelegateClient::CreateForWebContents(web_contents());
  }

  void TearDown() override {
    mock_glic_service_ = nullptr;
    ChromeRenderViewHostTestHarness::TearDown();
    TestingBrowserProcess::GetGlobal()->SetProfileManager(nullptr);
  }

  std::unique_ptr<KeyedService> CreateMockGlicService(
      content::BrowserContext* context) {
    Profile* p = Profile::FromBrowserContext(context);
    auto service = std::make_unique<NiceMock<glic::MockGlicKeyedService>>(
        context, IdentityManagerFactory::GetForProfile(p),
        TestingBrowserProcess::GetGlobal()->profile_manager(),
        &glic_profile_manager_,
        /*contextual_cueing_service=*/nullptr,
        /*actor_keyed_service=*/nullptr);
    mock_glic_service_ = service.get();
    return service;
  }

  ChromeActorLoginDelegateClient* client() {
    return ChromeActorLoginDelegateClient::FromWebContents(web_contents());
  }

 protected:
  base::ScopedTempDir temp_dir_;
  glic::GlicEnabling::ScopedBypassEnablementChecksForTesting
      scoped_glic_bypass_;
  glic::GlicProfileManager glic_profile_manager_;
  base::test::ScopedFeatureList feature_list_;
  tabs::MockTabInterface mock_tab_;
  NiceMock<MockBrowserWindowInterface> mock_browser_window_;
  ui::UnownedUserDataHost unowned_user_data_host_;
  raw_ptr<glic::MockGlicKeyedService> mock_glic_service_ = nullptr;
};


}  // namespace
}  // namespace actor_login
