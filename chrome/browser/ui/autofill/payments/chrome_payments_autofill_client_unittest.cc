// Copyright 2024 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/autofill/payments/chrome_payments_autofill_client.h"

#include <optional>
#include <vector>

#include "base/test/mock_callback.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/values_test_util.h"
#include "base/values.h"
#include "build/branding_buildflags.h"
#include "chrome/browser/ui/autofill/chrome_autofill_client.h"
#include "chrome/browser/ui/autofill/payments/virtual_card_enroll_bubble_controller_impl.h"
#include "chrome/browser/ui/autofill/payments/virtual_card_enroll_bubble_controller_impl_test_api.h"
#include "chrome/test/base/chrome_render_view_host_test_harness.h"
#include "chrome/test/base/scoped_browser_locale.h"
#include "components/autofill/core/browser/data_manager/payments/payments_data_manager.h"
#include "components/autofill/core/browser/data_manager/payments/payments_data_manager_test_api.h"
#include "components/autofill/core/browser/data_manager/personal_data_manager.h"
#include "components/autofill/core/browser/data_model/payments/autofill_offer_data.h"
#include "components/autofill/core/browser/data_model/payments/credit_card.h"
#include "components/autofill/core/browser/payments/autofill_error_dialog_context.h"
#include "components/autofill/core/browser/payments/card_unmask_challenge_option.h"
#include "components/autofill/core/browser/payments/payments_autofill_client.h"
#include "components/autofill/core/browser/test_utils/autofill_test_util.h"
#include "components/autofill/core/browser/test_utils/valuables_data_test_util.h"
#include "components/autofill/core/browser/ui/payments/autofill_progress_ui_type.h"
#include "components/autofill/core/browser/ui/payments/bnpl_ui_delegate.h"
#include "components/autofill/core/browser/ui/payments/bubble_show_options.h"
#include "components/autofill/core/browser/ui/payments/card_unmask_prompt_options.h"
#include "components/autofill/core/common/autofill_features.h"
#include "components/autofill/core/common/autofill_payments_features.h"
#include "components/autofill/core/common/autofill_prefs.h"
#include "components/signin/public/base/consent_level.h"
#include "components/signin/public/identity_manager/account_info.h"
#include "components/signin/public/identity_manager/identity_test_utils.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/web_contents_tester.h"
#include "testing/gmock/include/gmock/gmock.h"

#include "chrome/browser/ui/actions/chrome_action_id.h"
#include "chrome/browser/ui/autofill/payments/offer_notification_bubble_controller_impl.h"
#include "chrome/browser/ui/autofill/payments/omnibox_autofill_page_action_controller.h"
#include "chrome/browser/ui/autofill/payments/save_card_bubble_controller_impl.h"
#include "chrome/browser/ui/page_action/test_support/mock_page_action_controller.h"
#include "chrome/browser/ui/tabs/public/tab_features.h"
#include "chrome/browser/ui/ui_features.h"  // nogncheck
#include "components/autofill/core/browser/payments/desktop_bnpl_strategy.h"
#include "components/tabs/public/mock_tab_interface.h"
#include "ui/base/unowned_user_data/unowned_user_data_host.h"

using ::autofill::test::CreateLoyaltyCard;
using ::testing::_;
using ::testing::AllOf;
using ::testing::ElementsAre;
using ::testing::ElementsAreArray;
using ::testing::Eq;
using ::testing::Field;
using ::testing::Matcher;
using ::testing::Ne;
using ::testing::NotNull;
using ::testing::Property;
using ::testing::Ref;
using ::testing::Return;

namespace autofill {

class MockSaveCardBubbleController : public SaveCardBubbleControllerImpl {
 public:
  explicit MockSaveCardBubbleController(content::WebContents* web_contents)
      : SaveCardBubbleControllerImpl(web_contents) {}
  ~MockSaveCardBubbleController() override = default;

  MOCK_METHOD(void,
              OfferLocalSave,
              (const CreditCard&,
               payments::PaymentsAutofillClient::SaveCreditCardOptions,
               payments::PaymentsAutofillClient::LocalSaveCardPromptCallback),
              (override));
  MOCK_METHOD(
      void,
      ShowConfirmationBubbleView,
      (bool,
       bool,
       std::optional<
           payments::PaymentsAutofillClient::OnConfirmationClosedCallback>),
      (override));
  MOCK_METHOD(void, HideSaveCardBubble, (), (override));
};

class MockVirtualCardEnrollBubbleController
    : public VirtualCardEnrollBubbleControllerImpl {
 public:
  explicit MockVirtualCardEnrollBubbleController(
      content::WebContents* web_contents)
      : VirtualCardEnrollBubbleControllerImpl(web_contents) {}
  ~MockVirtualCardEnrollBubbleController() override = default;

  MOCK_METHOD(void,
              ShowConfirmationBubbleView,
              (payments::PaymentsAutofillClient::PaymentsRpcResult),
              (override));
};

class ChromePaymentsAutofillClientTest
    : public ChromeRenderViewHostTestHarness {
 public:

  void SetUp() override {
    ChromeRenderViewHostTestHarness::SetUp();

    ChromeAutofillClient::CreateForWebContents(web_contents());
    auto mock_virtual_card_bubble_controller =
        std::make_unique<MockVirtualCardEnrollBubbleController>(web_contents());
    const auto* user_data_key =
        mock_virtual_card_bubble_controller->UserDataKey();
    web_contents()->SetUserData(user_data_key,
                                std::move(mock_virtual_card_bubble_controller));
    auto mock_save_card_bubble_controller =
        std::make_unique<MockSaveCardBubbleController>(web_contents());
    user_data_key = mock_save_card_bubble_controller->UserDataKey();
    web_contents()->SetUserData(user_data_key,
                                std::move(mock_save_card_bubble_controller));
  }

  ChromeAutofillClient* client() {
    return ChromeAutofillClient::FromWebContentsForTesting(web_contents());
  }

  payments::ChromePaymentsAutofillClient* chrome_payments_client() {
    return static_cast<payments::ChromePaymentsAutofillClient*>(
        client()->GetPaymentsAutofillClient());
  }

  MockVirtualCardEnrollBubbleController& virtual_card_bubble_controller() {
    return static_cast<MockVirtualCardEnrollBubbleController&>(
        *VirtualCardEnrollBubbleController::GetOrCreate(web_contents()));
  }
  MockSaveCardBubbleController& save_card_bubble_controller() {
    return static_cast<MockSaveCardBubbleController&>(
        *SaveCardBubbleController::GetOrCreate(web_contents()));
  }

 private:
};

// TODO(crbug.com/410047802): Disable test on Linux TSan due to flakiness/issue.
#if BUILDFLAG(IS_LINUX) && defined(THREAD_SANITIZER)
#define MAYBE_ShowSaveCreditCardLocally_CallsOfferLocalSave \
  DISABLED_ShowSaveCreditCardLocally_CallsOfferLocalSave
#else
#define MAYBE_ShowSaveCreditCardLocally_CallsOfferLocalSave \
  ShowSaveCreditCardLocally_CallsOfferLocalSave
#endif  // BUILDFLAG(IS_LINUX) && defined(THREAD_SANITIZER)
TEST_F(ChromePaymentsAutofillClientTest,
       MAYBE_ShowSaveCreditCardLocally_CallsOfferLocalSave) {
  EXPECT_CALL(save_card_bubble_controller(), OfferLocalSave);

  chrome_payments_client()->ShowSaveCreditCardLocally(
      CreditCard(),
      payments::ChromePaymentsAutofillClient::SaveCreditCardOptions(),
      base::DoNothing());
}

// Test that calling `CreditCardUploadCompleted` calls
// SaveCardBubbleControllerImpl::ShowConfirmationBubbleView on card upload
// success.
TEST_F(ChromePaymentsAutofillClientTest,
       CreditCardUploadCompletedSuccess_CallsShowConfirmationBubbleView) {
  EXPECT_CALL(save_card_bubble_controller(),
              ShowConfirmationBubbleView(/*card_saved=*/true,
                                         /*is_for_save_and_fill=*/true, _));
  chrome_payments_client()->CreditCardUploadCompleted(
      payments::PaymentsAutofillClient::PaymentsRpcResult::kSuccess,
      std::nullopt);
}

// Test that calling `CreditCardUploadCompleted` calls
// SaveCardBubbleControllerImpl::ShowConfirmationBubbleView on card upload
// failure.
TEST_F(ChromePaymentsAutofillClientTest,
       CreditCardUploadCompletedFailure_CallsShowConfirmationBubbleView) {
  EXPECT_CALL(save_card_bubble_controller(),
              ShowConfirmationBubbleView(/*card_saved=*/false,
                                         /*is_for_save_and_fill=*/false, _));
  chrome_payments_client()->CreditCardUploadCompleted(
      payments::PaymentsAutofillClient::PaymentsRpcResult::kPermanentFailure,
      std::nullopt);
}

// Test that calling `CreditCardUploadCompleted` does not show failure
// confirmation on card upload client-side timeout.
TEST_F(
    ChromePaymentsAutofillClientTest,
    CreditCardUploadCompletedClientSideTimeout_CallsShowConfirmationBubbleView) {
  EXPECT_CALL(save_card_bubble_controller(), HideSaveCardBubble());
  EXPECT_CALL(save_card_bubble_controller(),
              ShowConfirmationBubbleView(/*card_saved=*/false,
                                         /*is_for_save_and_fill=*/false, _))
      .Times(0);
  chrome_payments_client()->CreditCardUploadCompleted(
      payments::PaymentsAutofillClient::PaymentsRpcResult::kClientSideTimeout,
      std::nullopt);
}

// Verify that the confirmation bubble view is shown after virtual card
// enrollment is completed.
TEST_F(ChromePaymentsAutofillClientTest,
       VirtualCardEnrollCompleted_ShowsConfirmation) {
  EXPECT_CALL(
      virtual_card_bubble_controller(),
      ShowConfirmationBubbleView(
          payments::PaymentsAutofillClient::PaymentsRpcResult::kSuccess));
  chrome_payments_client()->VirtualCardEnrollCompleted(
      payments::PaymentsAutofillClient::PaymentsRpcResult::kSuccess);
}

// Test that there is always an PaymentsWindowManager present if attempted
// to be retrieved.
TEST_F(ChromePaymentsAutofillClientTest, GetPaymentsWindowManager) {
    EXPECT_NE(chrome_payments_client()->GetPaymentsWindowManager(), nullptr);
}

// Test that BNPL strategy is created and returned correctly.
TEST_F(ChromePaymentsAutofillClientTest, GetBnplStrategy) {
  payments::BnplStrategy* strategy =
      chrome_payments_client()->GetBnplStrategy();
  ASSERT_NE(strategy, nullptr);

  // Test that the same instance is returned on subsequent calls.
  EXPECT_EQ(strategy, chrome_payments_client()->GetBnplStrategy());
}

// Test that BNPL UI delegate is created and returned correctly.
TEST_F(ChromePaymentsAutofillClientTest, GetBnplUiDelegate) {
  payments::BnplUiDelegate* ui_delegate =
      chrome_payments_client()->GetBnplUiDelegate();
  ASSERT_NE(ui_delegate, nullptr);

  // Test that the same instance is returned on subsequent calls.
  EXPECT_EQ(ui_delegate, chrome_payments_client()->GetBnplUiDelegate());
}

// Test that Wallet Reminder Notice UI delegate is created and returned
// correctly.
TEST_F(ChromePaymentsAutofillClientTest, GetWalletReminderNoticeUiDelegate) {
  payments::WalletReminderNoticeUiDelegate* ui_delegate =
      chrome_payments_client()->GetWalletReminderNoticeUiDelegate();
  ASSERT_NE(ui_delegate, nullptr);

  // Test that the same instance is returned on subsequent calls.
  EXPECT_EQ(ui_delegate,
            chrome_payments_client()->GetWalletReminderNoticeUiDelegate());
}

// Test that Wallet Reminder Notice manager is created and returned correctly.
TEST_F(ChromePaymentsAutofillClientTest, GetWalletReminderNoticeManager) {
  payments::WalletReminderNoticeManager* manager =
      chrome_payments_client()->GetWalletReminderNoticeManager();
  ASSERT_NE(manager, nullptr);

  // Test that the same instance is returned on subsequent calls.
  EXPECT_EQ(manager,
            chrome_payments_client()->GetWalletReminderNoticeManager());
}

// Test that Payments Churned Users UI delegate is created and returned
// correctly.
TEST_F(ChromePaymentsAutofillClientTest, GetPaymentsChurnedUsersUiDelegate) {
  payments::PaymentsChurnedUsersUiDelegate* ui_delegate =
      chrome_payments_client()->GetPaymentsChurnedUsersUiDelegate();
  ASSERT_NE(ui_delegate, nullptr);

  // Test that the same instance is returned on subsequent calls.
  EXPECT_EQ(ui_delegate,
            chrome_payments_client()->GetPaymentsChurnedUsersUiDelegate());
}

// Test that `DisablePaymentsAutofill` correctly disables the client's support
// for autofill payment methods.
TEST_F(ChromePaymentsAutofillClientTest, DisablePaymentsAutofill) {
  EXPECT_TRUE(chrome_payments_client()->IsAutofillPaymentMethodsEnabled());

  chrome_payments_client()->DisablePaymentsAutofill();

  EXPECT_FALSE(chrome_payments_client()->IsAutofillPaymentMethodsEnabled());
}

TEST_F(ChromePaymentsAutofillClientTest,
       IsAutofillPaymentMethodsEnabled_BlockedByPolicy) {
  base::test::ScopedFeatureList feature_list(
      features::kAutofillEnableAutofillSettingsEnterprisePolicy);
  NavigateAndCommit(GURL("https://example.com"));

  EXPECT_TRUE(chrome_payments_client()->IsAutofillPaymentMethodsEnabled());

  profile()->GetPrefs()->Set(
      prefs::kAutofillTypesBlocked,
      base::test::ParseJson(
          R"([{"url_pattern": "https://example.com", "blocked_types": ["payments"]}])"));

  EXPECT_FALSE(chrome_payments_client()->IsAutofillPaymentMethodsEnabled());
}

class TestOfferNotificationBubbleController
    : public OfferNotificationBubbleControllerImpl {
 public:
  explicit TestOfferNotificationBubbleController(
      content::WebContents* web_contents)
      : OfferNotificationBubbleControllerImpl(web_contents) {}

 private:
  // OfferNotificationBubbleControllerImpl:
  void UpdatePageActionIcon() override {}
  bool IsWebContentsActive() override { return false; }
};

// Enables the offer notification before the client is created, since the
// client only observes the payments data if it is enabled.
class ChromePaymentsAutofillClientOfferNotificationTest
    : public ChromePaymentsAutofillClientTest {
 public:
  ChromePaymentsAutofillClientOfferNotificationTest() {
    feature_list_.InitWithFeatures(
        {features::kAutofillEnableWalletDirectOffers,
         features::kAutofillEnableWalletDirectOffersNotificationBubble},
        {});
  }

 private:
  base::test::ScopedFeatureList feature_list_;
  // Wallet direct offers are only shown for supported locales.
  ScopedBrowserLocale scoped_browser_locale_{"en-US"};
};

// Tests that offers arriving after the navigation update the offer
// notification for the current page.
TEST_F(ChromePaymentsAutofillClientOfferNotificationTest,
       PaymentsDataChanged_UpdatesOfferNotification) {
  tabs::MockTabInterface mock_tab_interface;
  tabs::TabLookupFromWebContents::CreateForWebContents(web_contents(),
                                                       &mock_tab_interface);
  web_contents()->SetUserData(
      OfferNotificationBubbleControllerImpl::UserDataKey(),
      std::make_unique<TestOfferNotificationBubbleController>(web_contents()));
  const OfferNotificationBubbleControllerImpl* const controller =
      OfferNotificationBubbleControllerImpl::FromWebContents(web_contents());
  PaymentsDataManager& payments_data_manager =
      client()->GetPersonalDataManager().payments_data_manager();
  payments_data_manager.SetSyncingForTest(true);
  NavigateAndCommit(GURL("https://www.example.com/checkout"));
  ASSERT_FALSE(controller->IsIconVisible());

  const AutofillOfferData offer =
      test::GetPromoCodeOfferData(GURL("https://www.example.com"));
  test_api(payments_data_manager)
      .AddOfferData(std::make_unique<AutofillOfferData>(offer));
  test_api(payments_data_manager).NotifyObservers();

  EXPECT_TRUE(controller->IsIconVisible());
  EXPECT_EQ(*controller->GetOffer(), offer);
}


class ChromePaymentsAutofillIOSPromoClientTest
    : public ChromePaymentsAutofillClientTest {};

// Test that calling `CreditCardUploadCompleted` still calls
// SaveCardBubbleControllerImpl::ShowConfirmationBubbleView on card upload
// success as callback, after failing to show the iOS promo.
TEST_F(ChromePaymentsAutofillIOSPromoClientTest,
       IOSPaymentPromoFailedToShow_CallsShowConfirmationBubbleView) {
  EXPECT_CALL(save_card_bubble_controller(),
              ShowConfirmationBubbleView(/*card_saved=*/true,
                                         /*is_for_save_and_fill=*/true, _));
  chrome_payments_client()->CreditCardUploadCompleted(
      payments::PaymentsAutofillClient::PaymentsRpcResult::kSuccess,
      std::nullopt);
}


class ChromePaymentsAutofillClientOmniboxTest
    : public ChromePaymentsAutofillClientTest {
 public:
  ChromePaymentsAutofillClientOmniboxTest() {
    feature_list_.InitAndEnableFeature(
        features::kAutofillEnableOmniboxAutofill);
  }

 private:
  base::test::ScopedFeatureList feature_list_;
};

// Test that Omnibox Autofill delegate is created and returned correctly.
TEST_F(ChromePaymentsAutofillClientOmniboxTest, GetOmniboxAutofillDelegate) {
  OmniboxAutofillDelegate* omnibox_autofill_delegate =
      chrome_payments_client()->GetOmniboxAutofillDelegate();
  ASSERT_NE(omnibox_autofill_delegate, nullptr);

  // Test that the same instance is returned on subsequent calls.
  EXPECT_EQ(omnibox_autofill_delegate,
            chrome_payments_client()->GetOmniboxAutofillDelegate());
}

TEST_F(ChromePaymentsAutofillClientOmniboxTest,
       ShowExpandedOmniboxAutofillChip) {
  tabs::MockTabInterface mock_tab_interface;
  ui::UnownedUserDataHost user_data_host;
  ON_CALL(mock_tab_interface, GetUnownedUserDataHost())
      .WillByDefault(testing::ReturnRef(user_data_host));

  page_actions::MockPageActionController mock_page_action_controller;
  OmniboxAutofillPageActionController omnibox_controller(
      mock_tab_interface, mock_page_action_controller);

  tabs::TabLookupFromWebContents::CreateForWebContents(web_contents(),
                                                       &mock_tab_interface);

  EXPECT_CALL(mock_page_action_controller, Show(kActionAutofillPayment))
      .Times(1);
  EXPECT_CALL(mock_page_action_controller,
              ShowSuggestionChip(kActionAutofillPayment, _))
      .Times(1);

  chrome_payments_client()->ShowExpandedOmniboxAutofillChip(
      /*suggestions=*/{},
      /*on_chip_shown=*/base::DoNothing(),
      /*on_suggestions_shown=*/base::DoNothing(),
      /*on_suggestions_hidden=*/base::DoNothing(),
      /*did_select_suggestion=*/base::DoNothing(),
      /*did_deselect_suggestion=*/base::DoNothing(),
      /*did_accept_suggestion=*/base::DoNothing());
}

TEST_F(ChromePaymentsAutofillClientOmniboxTest, HideOmniboxAutofillChip) {
  tabs::MockTabInterface mock_tab_interface;
  ui::UnownedUserDataHost user_data_host;
  ON_CALL(mock_tab_interface, GetUnownedUserDataHost())
      .WillByDefault(testing::ReturnRef(user_data_host));

  page_actions::MockPageActionController mock_page_action_controller;
  OmniboxAutofillPageActionController omnibox_controller(
      mock_tab_interface, mock_page_action_controller);

  tabs::TabLookupFromWebContents::CreateForWebContents(web_contents(),
                                                       &mock_tab_interface);

  EXPECT_CALL(mock_page_action_controller,
              HideSuggestionChip(kActionAutofillPayment))
      .Times(1);
  EXPECT_CALL(mock_page_action_controller, Hide(kActionAutofillPayment))
      .Times(1);

  chrome_payments_client()->HideOmniboxAutofillChip();
}


class TestPaymentsAutofillClientForWebContentsDestruction
    : public payments::ChromePaymentsAutofillClient {
 public:
  explicit TestPaymentsAutofillClientForWebContentsDestruction(
      ContentAutofillClient* client)
      : payments::ChromePaymentsAutofillClient(client) {}

  void ClearWebContentsForTesting() { Observe(nullptr); }
};

TEST_F(ChromePaymentsAutofillClientTest,
       DialogsDoNotShowWhenWebContentsNullOrDestroyed) {
  TestPaymentsAutofillClientForWebContentsDestruction payments_client(client());
  // Simulate WebContents being destroyed/detached.
  payments_client.ClearWebContentsForTesting();

  // None of the following calls should crash or dereference null/dangling
  // pointers.
  payments_client.ShowUnmaskPrompt(
      test::GetCreditCard(), CardUnmaskPromptOptions(),
      /*delegate=*/base::WeakPtr<CardUnmaskDelegate>());

  payments_client.ShowAutofillProgressDialog(
      AutofillProgressUiType::kServerCardUnmaskProgressUi,
      /*cancel_callback=*/base::DoNothing());

  payments_client.ShowCardUnmaskOtpInputDialog(
      CreditCard::RecordType::kVirtualCard, CardUnmaskChallengeOption(),
      /*delegate=*/base::WeakPtr<OtpUnmaskDelegate>());

  payments_client.ShowUnmaskAuthenticatorSelectionDialog(
      /*challenge_options=*/{},
      /*confirm_unmask_challenge_option_callback=*/base::DoNothing(),
      /*cancel_unmasking_closure=*/base::DoNothing());

  payments_client.ShowAutofillErrorDialog(AutofillErrorDialogContext());

  payments_client.ShowCreditCardLocalSaveAndFillDialog(
      /*callback=*/base::DoNothing());

  payments_client.ShowCreditCardSaveAndFillPendingDialog(
      /*callback=*/base::DoNothing());
}

}  // namespace autofill
