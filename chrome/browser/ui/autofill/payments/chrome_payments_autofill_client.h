// Copyright 2023 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_AUTOFILL_PAYMENTS_CHROME_PAYMENTS_AUTOFILL_CLIENT_H_
#define CHROME_BROWSER_UI_AUTOFILL_PAYMENTS_CHROME_PAYMENTS_AUTOFILL_CLIENT_H_

#include <memory>
#include <optional>
#include <vector>

#include "base/containers/span.h"
#include "base/functional/callback.h"
#include "base/memory/raw_ref.h"
#include "base/scoped_observation.h"
#include "components/autofill/core/browser/data_manager/payments/payments_data_manager.h"
#include "components/autofill/core/browser/data_model/payments/credit_card.h"
#include "components/autofill/core/browser/payments/autofill_error_dialog_context.h"
#include "components/autofill/core/browser/payments/iban_manager.h"
#include "components/autofill/core/browser/payments/payments_autofill_client.h"
#include "components/autofill/core/browser/suggestions/suggestion.h"
#include "components/autofill/core/browser/ui/payments/autofill_error_dialog_controller_impl.h"
#include "components/autofill/core/browser/ui/payments/autofill_progress_dialog_controller_impl.h"
#include "components/autofill/core/browser/ui/payments/card_unmask_prompt_controller_impl.h"
#include "components/autofill/core/browser/ui/payments/save_and_fill_dialog_controller_impl.h"
#include "content/public/browser/web_contents_observer.h"


namespace webauthn {
class InternalAuthenticator;
}

namespace autofill {

class AutofillDriver;
class AutofillErrorDialogControllerImpl;
class AutofillOfferData;
class AutofillOfferManager;
enum class AutofillProgressUiType;
class AutofillSaveCardBottomSheetBridge;
class AutofillSaveIbanBottomSheetBridge;
class BnplIssuer;
class CardUnmaskAuthenticationSelectionDialogControllerImpl;
struct CardUnmaskChallengeOption;
class CardUnmaskOtpInputDialogControllerImpl;
class CreditCardCvcAuthenticator;
class CreditCardOtpAuthenticator;
class ContentAutofillClient;
class CreditCardRiskBasedAuthenticator;
struct FilledCardInformationBubbleOptions;
class IbanAccessManager;
class MerchantPromoCodeManager;
class OfferNotificationHandler;
struct OfferNotificationOptions;
class OtpUnmaskDelegate;
enum class OtpUnmaskResult;
class SaveAndFillDialogControllerImpl;
class SaveAndFillManager;
class TouchToFillPaymentMethodDelegate;
struct VirtualCardEnrollmentFields;
class VirtualCardEnrollmentManager;

namespace payments {

struct BnplIssuerContext;
class BnplStrategy;
struct BnplTosModel;
class BnplUiDelegate;
class MandatoryReauthManager;
class MultipleRequestPaymentsNetworkInterface;
class PaymentsChurnedUsersManager;
class PaymentsChurnedUsersUiDelegate;
class PaymentsWindowManager;
class WalletReminderNoticeManager;
class WalletReminderNoticeUiDelegate;

// Chrome implementation of PaymentsAutofillClient. Base class shared by
// ChromePaymentsAutofillClientDesktop and ChromePaymentsAutofillClientAndroid.
// Owned by ChromeAutofillClient via `Create()`, and it observes the same
// WebContents as its owning ChromeAutofillClient.
class ChromePaymentsAutofillClient : public PaymentsAutofillClient,
                                     public content::WebContentsObserver,
                                     public PaymentsDataManager::Observer {
 public:
  // Creates a platform-specific ChromePaymentsAutofillClient instance for
  // `client`. Defined in the respective Desktop and Android implementations
  // (`chrome_payments_autofill_client_desktop.cc` and
  // `chrome_payments_autofill_client_android.cc`).
  static std::unique_ptr<ChromePaymentsAutofillClient> Create(
      ContentAutofillClient* client);

  ChromePaymentsAutofillClient(const ChromePaymentsAutofillClient&) = delete;
  ChromePaymentsAutofillClient& operator=(const ChromePaymentsAutofillClient&) =
      delete;
  ~ChromePaymentsAutofillClient() override;

  static constexpr base::TimeDelta kSaveCardConfirmationSnackbarDuration =
      base::Seconds(3);

  // RiskDataLoader:
  void LoadRiskData(
      base::OnceCallback<void(const std::string&)> callback) override;

  // PaymentsAutofillClient:
  void ShowWebauthnOfferDialog(
      WebauthnDialogCallback offer_dialog_callback) override;
  void ShowWebauthnVerifyPendingDialog(
      WebauthnDialogCallback verify_pending_dialog_callback) override;
  void UpdateWebauthnOfferDialogWithError() override;
  bool CloseWebauthnDialog() override;
  void HideVirtualCardEnrollBubbleAndIconIfVisible() override;
  bool HasCreditCardScanFeature() const override;
  void ScanCreditCard(CreditCardScanCallback callback) override;
  bool LocalCardSaveIsSupported() override;
  void ShowSaveCreditCardLocally(const CreditCard& card,
                                 SaveCreditCardOptions options,
                                 LocalSaveCardPromptCallback callback) override;
  void ShowSaveCreditCardToCloud(
      const CreditCard& card,
      const LegalMessageLines& legal_message_lines,
      SaveCreditCardOptions options,
      UploadSaveCardPromptCallback callback) override;
  void CreditCardUploadCompleted(PaymentsRpcResult result,
                                 std::optional<OnConfirmationClosedCallback>
                                     on_confirmation_closed_callback) override;
  void HideSaveCardPrompt() override;
  void ShowVirtualCardEnrollDialog(
      const VirtualCardEnrollmentFields& virtual_card_enrollment_fields,
      base::OnceClosure accept_virtual_card_callback,
      base::OnceClosure decline_virtual_card_callback) override;
  void VirtualCardEnrollCompleted(PaymentsRpcResult result) override;
  void OnCardDataAvailable(const FilledCardInformationBubbleOptions& options,
                           const url::Origin& origin) override;
  void ConfirmSaveIbanLocally(const Iban& iban,
                              bool should_show_prompt,
                              SaveIbanPromptCallback callback) override;
  void ConfirmUploadIbanToCloud(const Iban& iban,
                                LegalMessageLines legal_message_lines,
                                bool should_show_prompt,
                                SaveIbanPromptCallback callback) override;
  void IbanUploadCompleted(bool iban_saved, bool hit_max_strikes) override;
  void ShowAutofillProgressDialog(
      AutofillProgressUiType autofill_progress_dialog_type,
      base::OnceClosure cancel_callback) override;
  void CloseAutofillProgressDialog(
      bool show_confirmation_before_closing,
      base::OnceClosure no_interactive_authentication_callback) override;
  void ShowCardUnmaskOtpInputDialog(
      CreditCard::RecordType card_type,
      const CardUnmaskChallengeOption& challenge_option,
      base::WeakPtr<OtpUnmaskDelegate> delegate) override;
  void OnUnmaskOtpVerificationResult(OtpUnmaskResult unmask_result) override;
  void ShowUnmaskAuthenticatorSelectionDialog(
      const std::vector<CardUnmaskChallengeOption>& challenge_options,
      base::OnceCallback<void(const std::string&)>
          confirm_unmask_challenge_option_callback,
      base::OnceClosure cancel_unmasking_closure) override;
  void DismissUnmaskAuthenticatorSelectionDialog(bool server_success) override;
  PaymentsNetworkInterface* GetPaymentsNetworkInterface() override;
  MultipleRequestPaymentsNetworkInterface*
  GetMultipleRequestPaymentsNetworkInterface() override;
  void ShowAutofillErrorDialog(AutofillErrorDialogContext context) override;
  PaymentsWindowManager* GetPaymentsWindowManager() override;
  void ShowUnmaskPrompt(
      const CreditCard& card,
      const CardUnmaskPromptOptions& card_unmask_prompt_options,
      base::WeakPtr<CardUnmaskDelegate> delegate) override;
  void OnUnmaskVerificationResult(PaymentsRpcResult result) override;
  VirtualCardEnrollmentManager* GetVirtualCardEnrollmentManager() override;
  CreditCardCvcAuthenticator& GetCvcAuthenticator() override;
  CreditCardOtpAuthenticator* GetOtpAuthenticator() override;
  CreditCardRiskBasedAuthenticator* GetRiskBasedAuthenticator() override;
  bool IsMandatoryReauthEnabled() override;
  void ShowMandatoryReauthOptInPrompt(
      base::OnceClosure accept_mandatory_reauth_callback,
      base::OnceClosure cancel_mandatory_reauth_callback,
      base::RepeatingClosure close_mandatory_reauth_callback) override;
  void ShowMandatoryReauthOptInConfirmation() override;
  bool IsAutofillPaymentMethodsEnabled() const final;
  void DisablePaymentsAutofill() final;
  IbanManager* GetIbanManager() override;
  IbanAccessManager* GetIbanAccessManager() override;
  MerchantPromoCodeManager* GetMerchantPromoCodeManager() override;
  AutofillOfferManager* GetAutofillOfferManager() override;
  void UpdateOfferNotification(
      const AutofillOfferData& offer,
      const OfferNotificationOptions& options) override;
  void DismissOfferNotification() override;
  bool IsTabVisibleForOfferNotification() const override;
  bool ShowTouchToFillCreditCard(
      base::WeakPtr<TouchToFillPaymentMethodDelegate> delegate,
      base::span<const Suggestion> suggestions) override;
  bool ShowTouchToFillIban(
      base::WeakPtr<TouchToFillPaymentMethodDelegate> delegate,
      base::span<const Iban> ibans_to_suggest) override;
  bool ShowTouchToFillAffiliatedLoyaltyCard(
      base::WeakPtr<TouchToFillPaymentMethodDelegate> delegate,
      std::vector<LoyaltyCard> loyalty_cards_to_suggest) override;
  bool ShowTouchToFillForAllLoyaltyCards(
      base::WeakPtr<TouchToFillPaymentMethodDelegate> delegate,
      std::vector<LoyaltyCard> loyalty_cards_to_suggest) override;
  bool OnPurchaseAmountExtracted(
      base::span<const payments::BnplIssuerContext> bnpl_issuer_contexts,
      std::optional<int64_t> extracted_amount,
      bool is_amount_supported_by_any_issuer,
      const std::optional<std::string>& app_locale,
      base::OnceCallback<void(BnplIssuer)> selected_issuer_callback,
      base::OnceClosure cancel_callback) override;
  bool ShowTouchToFillProgress(base::OnceClosure cancel_callback) override;
  bool ShowTouchToFillBnplIssuers(
      base::span<const payments::BnplIssuerContext> bnpl_issuer_contexts,
      const std::string& app_locale,
      base::OnceCallback<void(BnplIssuer)> selected_issuer_callback,
      base::OnceClosure cancel_callback) override;
  bool ShowTouchToFillError(const AutofillErrorDialogContext& context) override;
  bool ShowTouchToFillBnplTos(BnplTosModel bnpl_tos_model,
                              base::OnceClosure accept_callback,
                              base::OnceClosure cancel_callback) override;
  void HideTouchToFillPaymentMethod() override;
  void SetTouchToFillVisible(bool visible) override;
  PaymentsDataManager& GetPaymentsDataManager() final;
  std::unique_ptr<webauthn::InternalAuthenticator>
  CreateCreditCardInternalAuthenticator(AutofillDriver* driver) override;
  payments::MandatoryReauthManager* GetOrCreatePaymentsMandatoryReauthManager()
      override;
  payments::SaveAndFillManager* GetSaveAndFillManager() override;
  void ShowCreditCardLocalSaveAndFillDialog(
      CardSaveAndFillDialogCallback callback) override;
  void ShowCreditCardUploadSaveAndFillDialog(
      const LegalMessageLines& legal_message_lines,
      CardSaveAndFillDialogCallback callback) override;
  void ShowCreditCardSaveAndFillPendingDialog(
      CardSaveAndFillDialogCallback callback) override;
  void HideCreditCardSaveAndFillDialog() override;
  bool IsTabModalPopup() const override;
  BnplStrategy* GetBnplStrategy() override;
  BnplUiDelegate* GetBnplUiDelegate() override;
  WalletReminderNoticeUiDelegate* GetWalletReminderNoticeUiDelegate() override;
  WalletReminderNoticeManager* GetWalletReminderNoticeManager() override;
  PaymentsChurnedUsersUiDelegate* GetPaymentsChurnedUsersUiDelegate() override;
  OmniboxAutofillDelegate* GetOmniboxAutofillDelegate() override;
  void ShowExpandedOmniboxAutofillChip(
      std::vector<Suggestion> suggestions,
      base::OnceClosure on_chip_shown,
      base::RepeatingCallback<void(base::span<const Suggestion>)>
          on_suggestions_shown,
      base::RepeatingCallback<void(SuggestionHidingReason)>
          on_suggestions_hidden,
      base::RepeatingCallback<void(const Suggestion&)> did_select_suggestion,
      base::RepeatingClosure did_deselect_suggestion,
      base::RepeatingCallback<
          void(const Suggestion&,
               const AutofillSuggestionDelegate::SuggestionMetadata&)>
          did_accept_suggestion) override;
  void HideOmniboxAutofillChip() override;

  // Begin ChromePaymentsAutofillClient-specific section.


  AutofillProgressDialogControllerImpl*
  AutofillProgressDialogControllerForTesting();

  std::unique_ptr<CardUnmaskPromptControllerImpl>
  ExtractCardUnmaskControllerForTesting();

  void SetCardUnmaskControllerForTesting(
      std::unique_ptr<CardUnmaskPromptControllerImpl> test_controller);


 protected:
  explicit ChromePaymentsAutofillClient(ContentAutofillClient* client);

  ContentAutofillClient* client() const { return &client_.get(); }

 private:
  // content::WebContentsObserver:
  void DidFinishNavigation(
      content::NavigationHandle* navigation_handle) override;
  void OnVisibilityChanged(content::Visibility visibility) override;

  // PaymentsDataManager::Observer:
  void OnPaymentsDataChanged() override;

  std::u16string GetAccountHolderName() const;

  const raw_ref<ContentAutofillClient> client_;


  std::unique_ptr<PaymentsNetworkInterface> payments_network_interface_;

  std::unique_ptr<MultipleRequestPaymentsNetworkInterface>
      multiple_request_payments_network_interface_;

  std::unique_ptr<AutofillProgressDialogControllerImpl>
      autofill_progress_dialog_controller_;

  std::unique_ptr<AutofillErrorDialogControllerImpl>
      autofill_error_dialog_controller_;

  std::unique_ptr<CardUnmaskOtpInputDialogControllerImpl>
      card_unmask_otp_input_dialog_controller_;

  std::unique_ptr<PaymentsWindowManager> payments_window_manager_;

  std::unique_ptr<CardUnmaskPromptControllerImpl> unmask_controller_;

  // `virtual_card_enrollment_manager_` must be destroyed before
  // `payments_network_interface_` because the former keeps a reference to the
  // latter.
  // TODO(crbug.com/41489024): Remove the reference to
  // `payments_network_interface_` in `virtual_card_enrollment_manager_`.
  std::unique_ptr<VirtualCardEnrollmentManager>
      virtual_card_enrollment_manager_;

  std::unique_ptr<CreditCardCvcAuthenticator> cvc_authenticator_;

  std::unique_ptr<CreditCardOtpAuthenticator> otp_authenticator_;

  std::unique_ptr<CreditCardRiskBasedAuthenticator> risk_based_authenticator_;

  std::unique_ptr<CardUnmaskAuthenticationSelectionDialogControllerImpl>
      card_unmask_authentication_selection_controller_;

  std::unique_ptr<IbanManager> iban_manager_;
  std::unique_ptr<IbanAccessManager> iban_access_manager_;
  std::unique_ptr<MerchantPromoCodeManager> merchant_promo_code_manager_;

  std::unique_ptr<payments::MandatoryReauthManager>
      payments_mandatory_reauth_manager_;

  std::unique_ptr<SaveAndFillDialogControllerImpl>
      save_and_fill_dialog_controller_;

  std::unique_ptr<SaveAndFillManager> save_and_fill_manager_;

  // The PaymentsChurnedUsersUiDelegate used to handle the resurrection UI in
  // the Payments Churned Users flow. Lazily initialized: access only through
  // `GetPaymentsChurnedUsersUiDelegate()`.
  std::unique_ptr<PaymentsChurnedUsersUiDelegate>
      payments_churned_users_ui_delegate_;

  // Manages the flows related to getting users that have payments autofill
  // turned off back. Initiated upon construction of `this`.
  std::unique_ptr<payments::PaymentsChurnedUsersManager>
      payments_churned_users_manager_;

  // The BnplStrategy used to determine the next step in a BNPL flow depending
  // on the platform.
  // Lazily initialized: access only through `GetBnplStrategy()`.
  std::unique_ptr<BnplStrategy> bnpl_strategy_;

  // The BnplUiDelegate used to handle the UI in the BNPL flow depending on the
  // platform.
  // Lazily initialized: access only through `GetBnplUiDelegate()`.
  std::unique_ptr<BnplUiDelegate> bnpl_ui_delegate_;

  // The WalletReminderNoticeUiDelegate used to handle the UI in the Wallet
  // Reminder Notice flow. Lazily initialized: access only through
  // `GetWalletReminderNoticeUiDelegate()`.
  std::unique_ptr<WalletReminderNoticeUiDelegate>
      wallet_reminder_notice_ui_delegate_;

  // The WalletReminderNoticeManager used to orchestrate the Wallet Reminder
  // Notice flow. Lazily initialized: access only through
  // `GetWalletReminderNoticeManager()`.
  std::unique_ptr<WalletReminderNoticeManager> wallet_reminder_notice_manager_;

  // The OmniboxAutofillDelegate used to handle the logic flow and user
  // interactions when the user triggers Autofill from the Omnibox.
  std::unique_ptr<OmniboxAutofillDelegate> omnibox_autofill_delegate_;

  // Shows the offer notification on this tab. Nullptr if there is no
  // `AutofillOfferManager`, e.g. in off-the-record profiles.
  std::unique_ptr<OfferNotificationHandler> offer_notification_handler_;

  // Whether autofill payment methods are supported for this client. Is true by
  // default, and is flipped manually when `DisablePaymentsAutofill` is called.
  // Intended to be turned off in situations where payments autofill (both
  // uploading and filling) should be disabled for the given WebContents `this`
  // is owned by.
  bool autofill_payment_methods_supported_ = true;

  base::ScopedObservation<PaymentsDataManager, PaymentsDataManager::Observer>
      payments_data_manager_observation_{this};
};

}  // namespace payments

}  // namespace autofill

#endif  // CHROME_BROWSER_UI_AUTOFILL_PAYMENTS_CHROME_PAYMENTS_AUTOFILL_CLIENT_H_
