// Copyright 2024 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_FACILITATED_PAYMENTS_CORE_BROWSER_FACILITATED_PAYMENTS_CLIENT_H_
#define COMPONENTS_FACILITATED_PAYMENTS_CORE_BROWSER_FACILITATED_PAYMENTS_CLIENT_H_

#include <cstdint>
#include <memory>
#include <optional>
#include <string_view>

#include "base/containers/span.h"
#include "base/functional/callback_forward.h"
#include "build/build_config.h"
#include "components/autofill/core/browser/data_model/payments/ewallet.h"
#include "components/autofill/core/browser/payments/risk_data_loader.h"
#include "components/facilitated_payments/core/browser/account_linking_params.h"
#include "components/facilitated_payments/core/browser/device_delegate.h"
#include "components/facilitated_payments/core/browser/facilitated_payments_app_info_list.h"
#include "components/facilitated_payments/core/utils/facilitated_payments_ui_utils.h"
#include "components/signin/public/identity_manager/account_info.h"


namespace url {
class Origin;
}  // namespace url

namespace optimization_guide {
class OptimizationGuideDecider;
}  // namespace optimization_guide

namespace autofill {
class BankAccount;
class PaymentsDataManager;
}  // namespace autofill

namespace strike_database {
class StrikeDatabase;
}  // namespace strike_database

namespace payments::facilitated {

class PixAccountLinkingManager;
class FacilitatedPaymentsNetworkInterface;

// A cross-platform client interface for showing UI for non-form based FOPs.
class FacilitatedPaymentsClient : public autofill::RiskDataLoader {
 public:
  FacilitatedPaymentsClient();
  ~FacilitatedPaymentsClient() override;

  // Gets the URL of the last committed page.
  virtual const url::Origin& GetLastCommittedOrigin() const = 0;

  // Gets the `PaymentsDataManager` instance associated with the Chrome profile.
  // It is used to get user's account info.
  virtual autofill::PaymentsDataManager* GetPaymentsDataManager() = 0;

  // Gets the `FacilitatedPaymentsNetworkInterface` instance owned by the client
  // used for making payment requests. It can be null if the browser context
  // associated with the WebContents is null. Support multiple active
  // requests at a time.
  virtual FacilitatedPaymentsNetworkInterface*
  GetFacilitatedPaymentsNetworkInterface() = 0;

  // Provides access to the core information of the user's primary account.
  virtual std::optional<CoreAccountInfo> GetCoreAccountInfo() = 0;

  // Returns true if the device is being used in the landscape mode.
  virtual bool IsInLandscapeMode() = 0;

  // Returns true if the device is a foldable device.
  virtual bool IsFoldable() = 0;

  // Returns true if the current tab is opened as a CCT in another app instead
  // of the Chrome browser app.
  virtual bool IsInChromeCustomTabMode() = 0;

  // Returns an instance of the OptimizationGuideDecider associated with the
  // Chrome profile. It is used to determine whether a render frame host URL is
  // part of a feature allowlist.
  virtual optimization_guide::OptimizationGuideDecider*
  GetOptimizationGuideDecider() = 0;

  // Returns the `DeviceDelegate` instance owned by the implementation class.
  virtual DeviceDelegate* GetDeviceDelegate() = 0;

  // Returns true if the WebContents associated with this instance is either
  // visible or occluded, but not hidden. When a tab is occluded, it is still
  // open, but not visible either because it is covered by other windows or
  // because it's outside the screen bounds.
  virtual bool IsWebContentsVisibleOrOccluded() = 0;

  // Gets the StrikeDatabase associated with the client. Note: Nullptr may be
  // returned so check before use.
  virtual strike_database::StrikeDatabase* GetStrikeDatabase() = 0;

};

}  // namespace payments::facilitated

#endif  // COMPONENTS_FACILITATED_PAYMENTS_CORE_BROWSER_FACILITATED_PAYMENTS_CLIENT_H_
