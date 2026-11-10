// Copyright 2013 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/signin/chrome_signin_helper.h"

#include <algorithm>
#include <memory>
#include <utility>

#include "base/functional/bind.h"
#include "base/functional/callback_helpers.h"
#include "base/logging.h"
#include "base/metrics/histogram_functions.h"
#include "base/strings/string_util.h"
#include "base/supports_user_data.h"
#include "build/build_config.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/signin/account_consistency_mode_manager.h"
#include "chrome/browser/signin/account_reconcilor_factory.h"
#include "chrome/browser/signin/header_modification_delegate_impl.h"
#include "chrome/browser/signin/identity_manager_factory.h"
#include "chrome/browser/signin/signin_ui_util.h"
#include "chrome/browser/ui/webui/signin/signin_url_utils.h"
#include "components/google/core/common/google_util.h"
#include "components/policy/core/common/policy_pref_names.h"
#include "components/signin/core/browser/account_reconcilor.h"
#include "components/signin/core/browser/dice_header_helper.h"
#include "components/signin/public/base/account_consistency_method.h"
#include "components/signin/public/base/consent_level.h"
#include "components/signin/public/base/signin_buildflags.h"
#include "components/signin/public/base/signin_metrics.h"
#include "components/signin/public/base/signin_switches.h"
#include "components/signin/public/identity_manager/account_info.h"
#include "components/signin/public/identity_manager/accounts_cookie_mutator.h"
#include "components/signin/public/identity_manager/accounts_in_cookie_jar_info.h"
#include "components/signin/public/identity_manager/identity_manager.h"
#include "components/signin/public/identity_manager/tribool.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"
#include "google_apis/gaia/gaia_auth_util.h"
#include "google_apis/gaia/gaia_id.h"
#include "net/http/http_response_headers.h"
#include "ui/base/mojom/window_open_disposition.mojom.h"
#include "ui/base/page_transition_types.h"

#include "chrome/browser/ui/browser_commands.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
#include "chrome/browser/ui/browser_window/public/global_browser_collection.h"


#if BUILDFLAG(ENABLE_DICE_SUPPORT)
#include "chrome/browser/signin/dice_response_handler.h"
#include "chrome/browser/signin/dice_response_handler_factory.h"
#include "chrome/browser/signin/process_dice_header_delegate_impl.h"
#endif

namespace signin {

const void* const kManageAccountsHeaderReceivedUserDataKey =
    &kManageAccountsHeaderReceivedUserDataKey;

const char kChromeMirrorHeaderSource[] = "Chrome";

namespace {

// Key for RequestDestructionObserverUserData.
const void* const kRequestDestructionObserverUserDataKey =
    &kRequestDestructionObserverUserDataKey;

const char kGoogleRemoveLocalAccountResponseHeader[] =
    "Google-Accounts-RemoveLocalAccount";

const char kRemoveLocalAccountObfuscatedIDAttrName[] = "obfuscatedid";

// TODO(droger): Remove this delay when the Dice implementation is finished on
// the server side.
int g_dice_account_reconcilor_blocked_delay_ms = 1000;


#if BUILDFLAG(ENABLE_DICE_SUPPORT)

// Refcounted wrapper that facilitates creating and deleting a
// AccountReconcilor::Lock.
class AccountReconcilorLockWrapper
    : public base::RefCountedThreadSafe<AccountReconcilorLockWrapper> {
 public:
  explicit AccountReconcilorLockWrapper(
      const content::WebContents::Getter& web_contents_getter) {
    CHECK_CURRENTLY_ON(content::BrowserThread::UI, base::NotFatalUntil::M162);
    content::WebContents* web_contents = web_contents_getter.Run();
    if (!web_contents)
      return;
    Profile* profile =
        Profile::FromBrowserContext(web_contents->GetBrowserContext());
    AccountReconcilor* account_reconcilor =
        AccountReconcilorFactory::GetForProfile(profile);
    account_reconcilor_lock_ =
        std::make_unique<AccountReconcilor::Lock>(account_reconcilor);
  }

  AccountReconcilorLockWrapper(const AccountReconcilorLockWrapper&) = delete;
  AccountReconcilorLockWrapper& operator=(const AccountReconcilorLockWrapper&) =
      delete;

  void DestroyAfterDelay() {
    // TODO(dcheng): Should ReleaseSoon() support this use case?
    content::GetUIThreadTaskRunner({})->PostDelayedTask(
        FROM_HERE, base::DoNothingWithBoundArgs(base::RetainedRef(this)),
        base::Milliseconds(g_dice_account_reconcilor_blocked_delay_ms));
  }

 private:
  friend class base::RefCountedThreadSafe<AccountReconcilorLockWrapper>;
  ~AccountReconcilorLockWrapper() {
    CHECK_CURRENTLY_ON(content::BrowserThread::UI, base::NotFatalUntil::M162);
  }

  std::unique_ptr<AccountReconcilor::Lock> account_reconcilor_lock_;
};

// Returns true if the account reconcilor needs be be blocked while a Gaia
// sign-in request is in progress.
//
// The account reconcilor must be blocked on all request that may change the
// Gaia authentication cookies. This includes:
// * Main frame  requests.
// * XHR requests having Gaia URL as referrer.
bool ShouldBlockReconcilorForRequest(ChromeRequestAdapter* request) {
  if (request->IsOutermostMainFrame() &&
      request->GetRequestDestination() ==
          network::mojom::RequestDestination::kDocument) {
    return true;
  }

  return request->IsFetchLikeAPI() &&
         gaia::HasGaiaSchemeHostPort(request->GetReferrer());
}

#endif  // BUILDFLAG(ENABLE_DICE_SUPPORT)

class RequestDestructionObserverUserData : public base::SupportsUserData::Data {
 public:
  explicit RequestDestructionObserverUserData(base::OnceClosure closure)
      : closure_(std::move(closure)) {}

  RequestDestructionObserverUserData(
      const RequestDestructionObserverUserData&) = delete;
  RequestDestructionObserverUserData& operator=(
      const RequestDestructionObserverUserData&) = delete;

  ~RequestDestructionObserverUserData() override { std::move(closure_).Run(); }

 private:
  base::OnceClosure closure_;
};

// This user data is used as a marker that a Mirror header was found on the
// redirect chain. It does not contain any data, its presence is enough to
// indicate that a header has already be found on the request.
class ManageAccountsHeaderReceivedUserData
    : public base::SupportsUserData::Data {};


#if BUILDFLAG(ENABLE_DICE_SUPPORT)

void ProcessDiceHeader(
    DiceResponseParams dice_params,
    const content::WebContents::Getter& web_contents_getter) {
  CHECK_CURRENTLY_ON(content::BrowserThread::UI, base::NotFatalUntil::M162);

  content::WebContents* web_contents = web_contents_getter.Run();
  if (!web_contents)
    return;

  Profile* profile =
      Profile::FromBrowserContext(web_contents->GetBrowserContext());
  CHECK(!profile->IsOffTheRecord(), base::NotFatalUntil::M162);

  // Ignore Dice response headers if Dice is not enabled.
  if (!AccountConsistencyModeManager::IsDiceEnabledForProfile(profile))
    return;

  DiceResponseHandler* dice_response_handler =
      DiceResponseHandlerFactory::GetForProfile(profile);
  dice_response_handler->ProcessDiceHeader(
      std::move(dice_params),
      ProcessDiceHeaderDelegateImpl::Create(web_contents));
}
#endif  // BUILDFLAG(ENABLE_DICE_SUPPORT)


#if BUILDFLAG(ENABLE_DICE_SUPPORT)
void ProcessDiceResponseHeaderIfExists(ResponseAdapter* response,
                                       bool is_off_the_record) {
  CHECK(gaia::HasGaiaSchemeHostPort(response->GetUrl()));

  if (is_off_the_record)
    return;

  const net::HttpResponseHeaders* response_headers = response->GetHeaders();
  if (!response_headers)
    return;

  DiceResponseParams params =
      DiceHeaderHelper::CreateDiceResponseParams(response_headers);

  if (response_headers->HasHeader(kDiceResponseHeader)) {
    response->RemoveHeader(kDiceResponseHeader);
  }

  if (!params.IsValid()) {
    return;
  }

  // Post a task even if we are already on the UI thread to avoid making any
  // requests while processing a throttle event.
  content::GetUIThreadTaskRunner({})->PostTask(
      FROM_HERE, base::BindOnce(ProcessDiceHeader, std::move(params),
                                response->GetWebContentsGetter()));
}
#endif  // BUILDFLAG(ENABLE_DICE_SUPPORT)

GaiaId ParseGaiaIdFromRemoveLocalAccountResponseHeader(
    const net::HttpResponseHeaders* response_headers) {
  if (!response_headers) {
    return GaiaId();
  }

  std::optional<std::string> header_value =
      response_headers->GetNormalizedHeader(
          kGoogleRemoveLocalAccountResponseHeader);
  if (!header_value) {
    return GaiaId();
  }

  const SigninHeaderHelper::ResponseHeaderDictionary header_dictionary =
      SigninHeaderHelper::ParseAccountConsistencyResponseHeader(*header_value);

  const auto it =
      header_dictionary.find(kRemoveLocalAccountObfuscatedIDAttrName);
  if (it == header_dictionary.end()) {
    return GaiaId();
  }

  // The Gaia ID is wrapped in quotes.
  std::string gaia_id_str;
  base::TrimString(it->second, "\"", &gaia_id_str);
  return GaiaId(gaia_id_str);
}

void ProcessRemoveLocalAccountResponseHeaderIfExists(ResponseAdapter* response,
                                                     bool is_off_the_record) {
  CHECK(gaia::HasGaiaSchemeHostPort(response->GetUrl()));

  if (is_off_the_record)
    return;

  const GaiaId gaia_id =
      ParseGaiaIdFromRemoveLocalAccountResponseHeader(response->GetHeaders());

  if (gaia_id.empty())
    return;

  content::WebContents* web_contents = response->GetWebContentsGetter().Run();
  // The tab could have just closed. Technically, it would be possible to
  // refactor the code to pass around the profile by other means, but this
  // should be rare enough to be worth supporting.
  if (!web_contents)
    return;

  Profile* profile =
      Profile::FromBrowserContext(web_contents->GetBrowserContext());
  CHECK(!profile->IsOffTheRecord(), base::NotFatalUntil::M162);

  IdentityManagerFactory::GetForProfile(profile)
      ->GetAccountsCookieMutator()
      ->RemoveLoggedOutAccountByGaiaId(gaia_id);
}

}  // namespace

ChromeRequestAdapter::ChromeRequestAdapter(
    const GURL& url,
    const net::HttpRequestHeaders& original_headers,
    net::HttpRequestHeaders* modified_headers,
    std::vector<std::string>* headers_to_remove)
    : RequestAdapter(url,
                     original_headers,
                     modified_headers,
                     headers_to_remove) {}

ChromeRequestAdapter::~ChromeRequestAdapter() = default;

ResponseAdapter::ResponseAdapter() = default;

ResponseAdapter::~ResponseAdapter() = default;

void SetDiceAccountReconcilorBlockDelayForTesting(int delay_ms) {
  g_dice_account_reconcilor_blocked_delay_ms = delay_ms;
}

void FixAccountConsistencyRequestHeader(
    ChromeRequestAdapter* request,
    const GURL& redirect_url,
    bool is_off_the_record,
    int incognito_availability,
    AccountConsistencyMethod account_consistency,
    const GaiaId& primary_account_gaia_id,
    ConsentLevel primary_account_consent_level,
    signin::Tribool is_child_account,
    bool is_sync_feature_enabled,
#if BUILDFLAG(ENABLE_DICE_SUPPORT)
    const std::string& signin_scoped_device_id,
#endif
    content_settings::CookieSettings* cookie_settings) {
  if (is_off_the_record)
    return;  // Account consistency is disabled in incognito.

  // If new url is eligible to have the header, add it, otherwise remove it.

  // Mirror header:
  // The Mirror header may be added on desktop platforms, for integration with
  // Google Drive.
  int profile_mode_mask = PROFILE_MODE_DEFAULT;
  if (incognito_availability ==
      static_cast<int>(policy::IncognitoModeAvailability::kDisabled)) {
    profile_mode_mask |= PROFILE_MODE_INCOGNITO_DISABLED;
  }


  GaiaId primary_account_gaia_id_to_use =
      (primary_account_consent_level == ConsentLevel::kSignin ||
       is_sync_feature_enabled)
          ? primary_account_gaia_id
          : GaiaId();

  AppendOrRemoveMirrorRequestHeader(
      request, redirect_url, primary_account_gaia_id_to_use, is_child_account,
      account_consistency, cookie_settings, profile_mode_mask,
      kChromeMirrorHeaderSource,
      /*force_account_consistency=*/false);

// Dice header:
#if BUILDFLAG(ENABLE_DICE_SUPPORT)
  bool dice_header_added = DiceHeaderHelper::AppendOrRemoveDiceRequestHeader(
      request, redirect_url, primary_account_gaia_id, is_sync_feature_enabled,
      account_consistency, signin_scoped_device_id);

  // Block the AccountReconcilor while the Dice requests are in flight. This
  // allows the DiceReponseHandler to process the response before the reconcilor
  // starts.
  if (dice_header_added && ShouldBlockReconcilorForRequest(request)) {
    auto lock_wrapper = base::MakeRefCounted<AccountReconcilorLockWrapper>(
        request->GetWebContentsGetter());
    // On destruction of the request |lock_wrapper| will be released.
    request->SetDestructionCallback(base::BindOnce(
        &AccountReconcilorLockWrapper::DestroyAfterDelay, lock_wrapper));
  }
#endif
}

void ProcessAccountConsistencyResponseHeaders(ResponseAdapter* response,
                                              const GURL& redirect_url,
                                              bool is_off_the_record) {
  if (!gaia::HasGaiaSchemeHostPort(response->GetUrl())) {
    return;
  }


#if BUILDFLAG(ENABLE_DICE_SUPPORT)
  // Process the Dice header: on sign-in, exchange the authorization code for a
  // refresh token, on sign-out just follow the sign-out URL.
  ProcessDiceResponseHeaderIfExists(response, is_off_the_record);
#endif  // BUILDFLAG(ENABLE_DICE_SUPPORT)

  ProcessRemoveLocalAccountResponseHeaderIfExists(response, is_off_the_record);
}

GaiaId ParseGaiaIdFromRemoveLocalAccountResponseHeaderForTesting(
    const net::HttpResponseHeaders* response_headers) {
  return ParseGaiaIdFromRemoveLocalAccountResponseHeader(response_headers);
}

}  // namespace signin
