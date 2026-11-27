// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webid/identity_dialog_controller.h"

#include <memory>
#include <utility>

#include "base/functional/callback_helpers.h"
#include "base/test/bind.h"
#include "base/test/scoped_feature_list.h"
#include "build/build_config.h"
#include "build/buildflag.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/webid/account_selection_view.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "content/public/browser/webid/identity_request_account.h"
#include "content/public/browser/webid/identity_request_dialog_controller.h"
#include "content/public/test/browser_test.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/public/mojom/webid/federated_request.mojom.h"
#include "url/gurl.h"
#include "url/origin.h"

using actor::TaskId;
using testing::_;
using testing::SaveArg;

namespace {

std::vector<content::IdentityRequestDialogDisclosureField>
GetDefaultPermissions() {
  return {content::IdentityRequestDialogDisclosureField::kName,
          content::IdentityRequestDialogDisclosureField::kEmail,
          content::IdentityRequestDialogDisclosureField::kPicture};
}

}  // namespace

constexpr char kAccountId[] = "account_id1";
constexpr char16_t kTopFrameEtldPlusOne[] = u"top-frame-example.com";
constexpr char kIdpEtldPlusOne[] = "idp-example.com";

// Mock version of AccountSelectionView for injection during tests.
class MockAccountSelectionView : public AccountSelectionView {
 public:
  MockAccountSelectionView() : AccountSelectionView(/*delegate=*/nullptr) {}
  ~MockAccountSelectionView() override = default;

  MockAccountSelectionView(const MockAccountSelectionView&) = delete;
  MockAccountSelectionView& operator=(const MockAccountSelectionView&) = delete;

  MOCK_METHOD(void, OnPageActionClicked, (), (override));

  MOCK_METHOD(
      bool,
      Show,
      (const content::RelyingPartyData& rp_data,
       const std::vector<IdentityProviderDataPtr>& identity_provider_data,
       const std::vector<IdentityRequestAccountPtr>& accounts,
       blink::mojom::RpMode rp_mode,
       const std::vector<IdentityRequestAccountPtr>& new_accounts),
      (override));

  MOCK_METHOD(bool,
              ShowFailureDialog,
              (const content::RelyingPartyData& rp_data,
               const std::string& idp_for_display,
               blink::mojom::RpContext rp_context,
               blink::mojom::RpMode rp_mode,
               const content::IdentityProviderMetadata& idp_metadata),
              (override));

  MOCK_METHOD(bool,
              ShowErrorDialog,
              (const content::RelyingPartyData& rp_data,
               const std::string& idp_for_display,
               blink::mojom::RpContext rp_context,
               blink::mojom::RpMode rp_mode,
               const content::IdentityProviderMetadata& idp_metadata,
               const std::optional<TokenError>& error),
              (override));

  MOCK_METHOD(bool,
              ShowLoadingDialog,
              (const content::RelyingPartyData& rp_data,
               const std::string& idp_for_display,
               blink::mojom::RpContext rp_context,
               blink::mojom::RpMode rp_mode),
              (override));

  MOCK_METHOD(bool,
              ShowVerifyingDialog,
              (const content::RelyingPartyData&,
               const IdentityProviderDataPtr&,
               const IdentityRequestAccountPtr&,
               Account::SignInMode sign_in_mode,
               blink::mojom::RpMode),
              (override));

  MOCK_METHOD(bool,
              ShowNativeAppUi,
              (const content::NativeAppRequestOptions&),
              (override));

  MOCK_METHOD(void, SetCanShowUi, (bool can_show_ui), (override));

  MOCK_METHOD(std::string, GetTitle, (), (const, override));

  MOCK_METHOD(std::optional<std::string>, GetSubtitle, (), (const, override));

  MOCK_METHOD(void, ShowUrl, (LinkType type, const GURL& url), (override));

  MOCK_METHOD(content::WebContents*,
              ShowModalDialog,
              (const GURL& url,
               blink::mojom::RpMode rp_mode,
               content::IdentityRequestDialogController::ShownModalAsyncCallback
                   on_shown_async),
              (override));

  MOCK_METHOD(void, CloseModalDialog, (), (override));

  MOCK_METHOD(content::WebContents*, GetRpWebContents, (), (override));
};

class IdentityDialogControllerBrowserTest : public InProcessBrowserTest {
 public:
  IdentityDialogControllerBrowserTest() = default;
  ~IdentityDialogControllerBrowserTest() override = default;
  IdentityDialogControllerBrowserTest(IdentityDialogControllerBrowserTest&) =
      delete;
  IdentityDialogControllerBrowserTest& operator=(
      IdentityDialogControllerBrowserTest&) = delete;

  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();
    web_contents_ = browser()->tab_strip_model()->GetActiveWebContents();
  }

  void TearDownOnMainThread() override {
    web_contents_ = nullptr;
    InProcessBrowserTest::TearDownOnMainThread();
  }

  std::vector<IdentityRequestAccountPtr> CreateAccount() {
    return {base::MakeRefCounted<Account>(
        kAccountId, "", "", "", "", "", GURL(), "", "",
        /*potentially_approved_origin_hashes=*/std::vector<std::string>(),
        /*login_hints=*/std::vector<std::string>(),
        /*domain_hints=*/std::vector<std::string>(),
        /*labels=*/std::vector<std::string>(),
        /*idp_claimed_login_state=*/
        content::IdentityRequestAccount::LoginState::kSignUp,
        /*browser_trusted_login_state=*/
        content::IdentityRequestAccount::LoginState::kSignUp)};
  }

  IdentityProviderDataPtr CreateIdentityProviderData(
      std::vector<IdentityRequestAccountPtr>& accounts) {
    IdentityProviderDataPtr idp_data =
        base::MakeRefCounted<content::IdentityProviderData>(
            kIdpEtldPlusOne, content::IdentityProviderMetadata(),
            content::ClientMetadata(GURL(), GURL(), GURL(), gfx::Image()),
            blink::mojom::RpContext::kSignIn, /*format=*/std::nullopt,
            GetDefaultPermissions(),
            /*has_login_status_mismatch=*/false);
    for (auto& account : accounts) {
      account->identity_provider = idp_data;
    }
    return idp_data;
  }

 protected:
  raw_ptr<content::WebContents> web_contents_;


  bool HasActingTaskId(IdentityDialogController* controller) {
    return !controller->acting_task_id_.is_null();
  }

};
