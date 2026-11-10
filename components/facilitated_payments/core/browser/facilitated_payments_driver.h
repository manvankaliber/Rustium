// Copyright 2024 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_FACILITATED_PAYMENTS_CORE_BROWSER_FACILITATED_PAYMENTS_DRIVER_H_
#define COMPONENTS_FACILITATED_PAYMENTS_CORE_BROWSER_FACILITATED_PAYMENTS_DRIVER_H_

#include <memory>

#include "base/functional/callback_forward.h"
#include "build/build_config.h"
#include "components/facilitated_payments/core/browser/facilitated_payments_api_client.h"
#include "services/metrics/public/cpp/ukm_source_id.h"

class GURL;

namespace url {
class Origin;
}  // namespace url

namespace payments::facilitated {

class PaymentLinkManager;
class FacilitatedPaymentsClient;
class PixManager;

// A cross-platform interface which is a gateway for all Facilitated Payments
// related communication between the browser and the DOM. There can be one
// instance for each outermost main frame. It is only created if the main frame
// is active at the time of load.
//
// TODO(crbug.com/371059457): `FacilitatedPaymentsDriver` is currently an
// abstract class. Considering migrating it to a pure interface and use delegate
// to handle common logics shared by cross-platform.
class FacilitatedPaymentsDriver {
 public:
  FacilitatedPaymentsDriver(
      FacilitatedPaymentsClient* client,
      FacilitatedPaymentsApiClientCreator api_client_creator);
  FacilitatedPaymentsDriver(const FacilitatedPaymentsDriver&) = delete;
  FacilitatedPaymentsDriver& operator=(const FacilitatedPaymentsDriver&) =
      delete;
  virtual ~FacilitatedPaymentsDriver();


 protected:
  // Returns true if the current page context is secure for payments.
  virtual bool IsSecureForPaymentHandling() const = 0;

 private:
  const raw_ref<FacilitatedPaymentsClient> facilitated_payments_client_;
  FacilitatedPaymentsApiClientCreator api_client_creator_;
};

}  // namespace payments::facilitated

#endif  // COMPONENTS_FACILITATED_PAYMENTS_CORE_BROWSER_FACILITATED_PAYMENTS_DRIVER_H_
