// Copyright 2024 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/facilitated_payments/core/browser/facilitated_payments_driver.h"

#include <optional>
#include <string>
#include <utility>

#include "base/check_deref.h"
#include "build/build_config.h"
#include "components/facilitated_payments/core/browser/facilitated_payments_client.h"


namespace payments::facilitated {


FacilitatedPaymentsDriver::FacilitatedPaymentsDriver(
    FacilitatedPaymentsClient* client,
    FacilitatedPaymentsApiClientCreator api_client_creator)
    : facilitated_payments_client_(CHECK_DEREF(client)),
      api_client_creator_(std::move(api_client_creator)) {}

FacilitatedPaymentsDriver::~FacilitatedPaymentsDriver() = default;


}  // namespace payments::facilitated
