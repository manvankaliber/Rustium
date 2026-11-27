// Copyright 2024 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_FACILITATED_PAYMENTS_UI_CHROME_FACILITATED_PAYMENTS_CLIENT_H_
#define CHROME_BROWSER_FACILITATED_PAYMENTS_UI_CHROME_FACILITATED_PAYMENTS_CLIENT_H_

#include <memory>
#include <string>
#include <string_view>

#include "base/containers/span.h"
#include "base/functional/callback.h"
#include "base/memory/raw_ref.h"
#include "build/build_config.h"
#include "components/facilitated_payments/content/browser/content_facilitated_payments_driver_factory.h"
#include "components/facilitated_payments/core/browser/facilitated_payments_app_info_list.h"
#include "components/facilitated_payments/core/browser/facilitated_payments_client.h"
#include "components/facilitated_payments/core/browser/network_api/facilitated_payments_network_interface.h"
#include "components/facilitated_payments/core/utils/facilitated_payments_ui_utils.h"
#include "ui/base/unowned_user_data/scoped_unowned_user_data.h"


namespace url {
class Origin;
}  // namespace url

namespace autofill {
class BankAccount;
class Ewallet;
}  // namespace autofill

namespace content {
class WebContents;
class RenderFrameHost;
}  // namespace content

namespace optimization_guide {
class OptimizationGuideDecider;
}  // namespace optimization_guide

namespace strike_database {
class StrikeDatabase;
}  // namespace strike_database

namespace tabs {
class TabInterface;
}  // namespace tabs

// Chrome implementation of `FacilitatedPaymentsClient`. `TabFeatures` owns 1
// instance of this class. Creates and owns
// `ContentFacilitatedPaymentsDriverFactory`.
class ChromeFacilitatedPaymentsClient
    : public payments::facilitated::FacilitatedPaymentsClient {
 public:
  DECLARE_USER_DATA(ChromeFacilitatedPaymentsClient);

  // Note: `web_contents` is passed explicitly because `tab.GetContents()` still
  // returns the old contents during `TabFeatures::WillDiscardContents`.
  ChromeFacilitatedPaymentsClient(
      tabs::TabInterface& tab,
      content::WebContents* web_contents,
      optimization_guide::OptimizationGuideDecider* optimization_guide_decider,
      base::RepeatingCallback<bool(content::WebContents*)> is_cct_callback =
          {});
  ChromeFacilitatedPaymentsClient(const ChromeFacilitatedPaymentsClient&) =
      delete;
  ChromeFacilitatedPaymentsClient& operator=(
      const ChromeFacilitatedPaymentsClient&) = delete;
  ~ChromeFacilitatedPaymentsClient() override;

  static ChromeFacilitatedPaymentsClient* From(tabs::TabInterface* tab);

  // RiskDataLoader:
  void LoadRiskData(base::OnceCallback<void(const std::string&)>
                        on_risk_data_loaded_callback) final;

  payments::facilitated::ContentFacilitatedPaymentsDriver*
  GetFacilitatedPaymentsDriverForFrame(
      content::RenderFrameHost* render_frame_host);


 private:
  content::WebContents& GetWebContents() const { return *web_contents_; }

  // FacilitatedPaymentsClient:
  const url::Origin& GetLastCommittedOrigin() const final;
  // This returns nullptr if the `Profile` associated is null.
  autofill::PaymentsDataManager* GetPaymentsDataManager() final;
  // This returns nullptr if the `Profile` associated is null.
  payments::facilitated::FacilitatedPaymentsNetworkInterface*
  GetFacilitatedPaymentsNetworkInterface() final;
  // This returns std::nullopt if the `Profile` associated is null.
  std::optional<CoreAccountInfo> GetCoreAccountInfo() final;
  bool IsInLandscapeMode() final;
  bool IsFoldable() final;
  bool IsInChromeCustomTabMode() final;
  optimization_guide::OptimizationGuideDecider* GetOptimizationGuideDecider()
      final;
  payments::facilitated::DeviceDelegate* GetDeviceDelegate() final;
  bool IsWebContentsVisibleOrOccluded() final;
  strike_database::StrikeDatabase* GetStrikeDatabase() final;

  // Register any allowlists with the OptimizationGuide framework, so that
  // individual features can later request to check whether the current main
  // frame URL is eligible for that feature.
  void RegisterAllowlists();

  const raw_ref<content::WebContents> web_contents_;

  std::unique_ptr<payments::facilitated::FacilitatedPaymentsNetworkInterface>
      facilitated_payments_network_interface_;


  // The optimization guide decider to help determine whether the current main
  // frame URL is eligible for facilitated payments.
  raw_ptr<optimization_guide::OptimizationGuideDecider>
      optimization_guide_decider_ = nullptr;

  base::RepeatingCallback<bool(content::WebContents*)> is_cct_callback_;

  // Declared after `facilitated_payments_controller_` and other client
  // dependencies so that drivers (and their owned `PixManager` and
  // `PaymentLinkManager` instances, which call `client_->DismissPrompt()` in
  // their destructors) are destroyed before `facilitated_payments_controller_`.
  payments::facilitated::ContentFacilitatedPaymentsDriverFactory
      driver_factory_;

  ui::ScopedUnownedUserData<ChromeFacilitatedPaymentsClient>
      scoped_unowned_user_data_;
};

#endif  // CHROME_BROWSER_FACILITATED_PAYMENTS_UI_CHROME_FACILITATED_PAYMENTS_CLIENT_H_
