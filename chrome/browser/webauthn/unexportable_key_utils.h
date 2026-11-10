// Copyright 2024 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_WEBAUTHN_UNEXPORTABLE_KEY_UTILS_H_
#define CHROME_BROWSER_WEBAUTHN_UNEXPORTABLE_KEY_UTILS_H_

#include <memory>

#include "base/functional/callback_forward.h"
#include "base/memory/raw_ptr.h"
#include "build/build_config.h"
#include "crypto/user_verifying_key.h"



namespace crypto {
class UnexportableKeyProvider;
}  // namespace crypto

inline constexpr char kUnexportableKeyProviderTypeHistogram[] =
    "WebAuthentication.UnexportableKeyProviderType";

// These values are persisted to logs. Entries should not be renumbered and
// numeric values should never be reused.
//
// LINT.IfChange(WebAuthenticationUnexportableKeyProviderType)
enum class UnexportableKeyProviderType {
  kNone = 0,
  kHardware = 1,
  kMicrosoftSoftware = 2,
  kSoftwareFallback = 3,
  kMaxValue = kSoftwareFallback,
};
// LINT.ThenChange(//tools/metrics/histograms/metadata/webauthn/enums.xml:WebAuthenticationUnexportableKeyProviderType)

std::unique_ptr<crypto::UnexportableKeyProvider>
GetWebAuthnUnexportableKeyProvider();

std::unique_ptr<crypto::UserVerifyingKeyProvider>
GetWebAuthnUserVerifyingKeyProvider(
    crypto::UserVerifyingKeyProvider::Config config);


void SetWebAuthnUnexportableKeyProviderForTesting(
    std::unique_ptr<crypto::UnexportableKeyProvider> (*func)());

// Allows `kUnexportableKeyProviderTypeHistogram`, which is normally recorded at
// most once per process, to be recorded again.
void ResetUnexportableKeyProviderTypeMetricForTesting();

#endif  // CHROME_BROWSER_WEBAUTHN_UNEXPORTABLE_KEY_UTILS_H_
