// Copyright 2024 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/facilitated_payments/ui/chrome_facilitated_payments_client.h"

#include <memory>
#include <string>
#include <string_view>

#include "base/check_deref.h"
#include "base/functional/callback_helpers.h"
#include "build/build_config.h"
#include "chrome/browser/autofill/personal_data_manager_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/signin/identity_manager_factory.h"
#include "chrome/browser/strike_database/strike_database_factory.h"
#include "chrome/browser/ui/autofill/risk_util.h"
#include "components/autofill/core/browser/data_manager/payments/payments_data_manager.h"
#include "components/autofill/core/browser/data_manager/personal_data_manager.h"
#include "components/autofill/core/browser/data_model/payments/bank_account.h"
#include "components/autofill/core/browser/data_model/payments/ewallet.h"
#include "components/facilitated_payments/content/browser/facilitated_payments_api_client_factory.h"
#include "components/facilitated_payments/core/browser/facilitated_payments_app_info_list.h"
#include "components/facilitated_payments/core/browser/network_api/facilitated_payments_network_interface.h"
#include "components/facilitated_payments/core/features/features.h"
#include "components/facilitated_payments/core/utils/facilitated_payments_ui_utils.h"
#include "components/optimization_guide/core/hints/optimization_guide_decider.h"
#include "components/optimization_guide/proto/hints.pb.h"
#include "components/signin/public/identity_manager/identity_manager.h"
#include "components/tabs/public/tab_interface.h"
#include "content/public/browser/visibility.h"
#include "content/public/browser/web_contents.h"
#include "url/origin.h"


DEFINE_USER_DATA(ChromeFacilitatedPaymentsClient);

ChromeFacilitatedPaymentsClient::ChromeFacilitatedPaymentsClient(
    tabs::TabInterface& tab,
    content::WebContents* web_contents,
    optimization_guide::OptimizationGuideDecider* optimization_guide_decider,
    base::RepeatingCallback<bool(content::WebContents*)> is_cct_callback)
    : web_contents_(CHECK_DEREF(web_contents)),
      optimization_guide_decider_(optimization_guide_decider),
      is_cct_callback_(std::move(is_cct_callback)),
      driver_factory_(web_contents,
                      /* client= */ this),
      scoped_unowned_user_data_(tab.GetUnownedUserDataHost(), *this) {
  RegisterAllowlists();
}

ChromeFacilitatedPaymentsClient::~ChromeFacilitatedPaymentsClient() {
}

// static
ChromeFacilitatedPaymentsClient* ChromeFacilitatedPaymentsClient::From(
    tabs::TabInterface* tab) {
  return tab ? Get(tab->GetUnownedUserDataHost()) : nullptr;
}

void ChromeFacilitatedPaymentsClient::LoadRiskData(
    base::OnceCallback<void(const std::string&)> on_risk_data_loaded_callback) {
  autofill::risk_util::LoadRiskData(/*obfuscated_gaia_id=*/0, &GetWebContents(),
                                    std::move(on_risk_data_loaded_callback));
}

payments::facilitated::ContentFacilitatedPaymentsDriver*
ChromeFacilitatedPaymentsClient::GetFacilitatedPaymentsDriverForFrame(
    content::RenderFrameHost* render_frame_host) {
  return &driver_factory_.GetOrCreateForFrame(render_frame_host);
}

const url::Origin& ChromeFacilitatedPaymentsClient::GetLastCommittedOrigin()
    const {
  return GetWebContents().GetPrimaryMainFrame()->GetLastCommittedOrigin();
}

autofill::PaymentsDataManager*
ChromeFacilitatedPaymentsClient::GetPaymentsDataManager() {
  content::BrowserContext* context = GetWebContents().GetBrowserContext();
  if (!context) {
    return nullptr;
  }
  autofill::PersonalDataManager* pdm =
      autofill::PersonalDataManagerFactory::GetForBrowserContext(context);
  return pdm ? &pdm->payments_data_manager() : nullptr;
}

payments::facilitated::FacilitatedPaymentsNetworkInterface*
ChromeFacilitatedPaymentsClient::GetFacilitatedPaymentsNetworkInterface() {
  if (!facilitated_payments_network_interface_) {
    Profile* profile =
        Profile::FromBrowserContext(GetWebContents().GetBrowserContext());
    if (!profile) {
      return nullptr;
    }
    facilitated_payments_network_interface_ = std::make_unique<
        payments::facilitated::FacilitatedPaymentsNetworkInterface>(
        profile->GetURLLoaderFactory(),
        *IdentityManagerFactory::GetForProfile(profile->GetOriginalProfile()),
        *GetPaymentsDataManager(), profile->IsOffTheRecord());
  }
  return facilitated_payments_network_interface_.get();
}

std::optional<CoreAccountInfo>
ChromeFacilitatedPaymentsClient::GetCoreAccountInfo() {
  Profile* profile =
      Profile::FromBrowserContext(GetWebContents().GetBrowserContext());
  if (!profile) {
    return std::nullopt;
  }
  auto* identity_manager =
      IdentityManagerFactory::GetForProfile(profile->GetOriginalProfile());
  return identity_manager->GetPrimaryAccountInfo(signin::ConsentLevel::kSignin);
}

bool ChromeFacilitatedPaymentsClient::IsInLandscapeMode() {
  return false;
}

bool ChromeFacilitatedPaymentsClient::IsFoldable() {
  return false;
}

bool ChromeFacilitatedPaymentsClient::IsInChromeCustomTabMode() {
  return is_cct_callback_ && is_cct_callback_.Run(&GetWebContents());
}

optimization_guide::OptimizationGuideDecider*
ChromeFacilitatedPaymentsClient::GetOptimizationGuideDecider() {
  return optimization_guide_decider_;
}

payments::facilitated::DeviceDelegate*
ChromeFacilitatedPaymentsClient::GetDeviceDelegate() {
  // Only the Android payment flows use the device delegate.
  return nullptr;
}

bool ChromeFacilitatedPaymentsClient::IsWebContentsVisibleOrOccluded() {
  return GetWebContents().GetVisibility() != content::Visibility::HIDDEN;
}

strike_database::StrikeDatabase*
ChromeFacilitatedPaymentsClient::GetStrikeDatabase() {
  content::BrowserContext* context = GetWebContents().GetBrowserContext();

  Profile* profile = Profile::FromBrowserContext(context);
  if (!profile) {
    return nullptr;
  }

  return StrikeDatabaseFactory::GetForProfile(profile);
}


void ChromeFacilitatedPaymentsClient::RegisterAllowlists() {
  if (optimization_guide_decider_) {
    if (base::FeatureList::IsEnabled(
            payments::facilitated::kEnableDesktopQrCodeDetection)) {
      optimization_guide_decider_->RegisterOptimizationTypes(
          {optimization_guide::proto::
               PAYMENT_QR_CODE_MERCHANT_URL_REGEX_ALLOWLIST});
    }
  }
}
