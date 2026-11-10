// Copyright 2022 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/policy/status_provider/status_provider_util.h"

#include "base/values.h"
#include "chrome/browser/enterprise/identifiers/profile_id_service_factory.h"
#include "components/enterprise/browser/identifiers/profile_id_service.h"
#include "components/policy/core/browser/webui/policy_status_provider.h"
#include "components/policy/resources/webui/mojom/policy.mojom.h"
#include "google_apis/gaia/gaia_auth_util.h"

#include "chrome/browser/enterprise/util/affiliation.h"
#include "chrome/browser/policy/dm_token_utils.h"

const char kDevicePolicyStatusDescription[] = "statusDevice";
const char kUserPolicyStatusDescription[] = "statusUser";

void SetDomainExtractedFromUsername(base::DictValue& dict) {

  const std::string* username = dict.FindString(policy::kUsernameKey);
  if (username && !username->empty())
    dict.Set(policy::kDomainKey, gaia::ExtractDomainName(*username));
}

void SetDomainExtractedFromUsername(policy::mojom::StatusPtr& status) {

  const auto& username = status->username;
  if (username.has_value() && !username->empty()) {
    status->domain = gaia::ExtractDomainName(username.value());
  }
}

std::optional<bool> GetUserAffiliationStatus(Profile* profile) {
  CHECK(profile);

  // Don't show affiliation status if the browser isn't enrolled in CBCM.
  if (!policy::GetDMToken(profile).is_valid()) {
    return std::nullopt;
  }
  return enterprise_util::IsProfileAffiliated(profile);
}

std::optional<std::string> GetProfileId(Profile* profile) {
  CHECK(profile);
  auto* profile_id_service =
      enterprise::ProfileIdServiceFactory::GetForProfile(profile);
  if (!profile_id_service) {
    return std::nullopt;
  }

  return profile_id_service->GetProfileId();
}
