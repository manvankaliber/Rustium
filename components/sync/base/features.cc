// Copyright 2022 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/sync/base/features.h"

#include "base/feature_list.h"

namespace syncer {

BASE_FEATURE(kDeferredSyncStartupCustomDelay,
             base::FEATURE_DISABLED_BY_DEFAULT);

BASE_FEATURE(kSyncSharedComment, base::FEATURE_DISABLED_BY_DEFAULT);

BASE_FEATURE(kSyncAIThread, base::FEATURE_DISABLED_BY_DEFAULT);

BASE_FEATURE(kSyncContextualTask, base::FEATURE_DISABLED_BY_DEFAULT);

BASE_FEATURE(kSyncEncryptedTabContext, base::FEATURE_DISABLED_BY_DEFAULT);

BASE_FEATURE(kSyncThemesIos, base::FEATURE_DISABLED_BY_DEFAULT);

BASE_FEATURE(kNewTabPageCustomizationThemeSync,
             base::FEATURE_DISABLED_BY_DEFAULT);

BASE_FEATURE(kSyncLoyaltyCardMetadata, base::FEATURE_DISABLED_BY_DEFAULT);

BASE_FEATURE(kSyncNotebook, base::FEATURE_DISABLED_BY_DEFAULT);

BASE_FEATURE(kSyncJourney, base::FEATURE_DISABLED_BY_DEFAULT);

BASE_FEATURE(kSyncAutofillEntitySuppression, base::FEATURE_DISABLED_BY_DEFAULT);

BASE_FEATURE(kUnoPhase2FollowUp, base::FEATURE_ENABLED_BY_DEFAULT);


bool IsContactInfoDataTypeForCustomPassphraseUsersEnabled() {
  return true;
}

BASE_FEATURE(kSyncEnableContactInfoDataTypeForDasherUsers,
             base::FEATURE_DISABLED_BY_DEFAULT);

bool IsReplaceSyncPromosWithSignInPromosEnabled() {
  return true;
}

// Like DECLARE_SYNC_AUTOFILL_AI_FEATURE but for the definition.
#define DEFINE_SYNC_AUTOFILL_AI_FEATURE(feature_name) \
  BASE_FEATURE(feature_name, base::FEATURE_ENABLED_BY_DEFAULT)

DEFINE_SYNC_AUTOFILL_AI_FEATURE(kSyncAccountSettings);


DEFINE_SYNC_AUTOFILL_AI_FEATURE(kSyncAutofillValuableMetadata);

DEFINE_SYNC_AUTOFILL_AI_FEATURE(kSyncWalletFlightReservations);

DEFINE_SYNC_AUTOFILL_AI_FEATURE(kSyncWalletVehicleRegistrations);

#undef DEFINE_SYNC_AUTOFILL_AI_FEATURE

BASE_FEATURE(kSpellcheckSeparateLocalAndAccountDictionaries,
             base::FEATURE_ENABLED_BY_DEFAULT);

BASE_FEATURE(kSyncIncreaseNudgeDelayForSingleClient,
             base::FEATURE_DISABLED_BY_DEFAULT);


BASE_FEATURE(kSyncUsePropagatedAccessToken, base::FEATURE_DISABLED_BY_DEFAULT);



BASE_FEATURE(kSyncDisambiguateDeviceNamesWithChannel,
             base::FEATURE_DISABLED_BY_DEFAULT);

BASE_FEATURE(kSyncNigoriAuthenticateIV, base::FEATURE_DISABLED_BY_DEFAULT);

BASE_FEATURE(kSyncMigrateFromImplicitPassphrase,
             base::FEATURE_ENABLED_BY_DEFAULT);

BASE_FEATURE(kSyncMigrateLoopbackServerBookmarksToClientTagHash,
             base::FEATURE_DISABLED_BY_DEFAULT);

BASE_FEATURE(kSyncDoNotSyncAppsAndAppSettings,
             base::FEATURE_ENABLED_BY_DEFAULT);

BASE_FEATURE(kSyncWipeAppsAndAppSettingsData,
             base::FEATURE_ENABLED_BY_DEFAULT);

BASE_FEATURE(kDeviceTabVisibilitySettings, base::FEATURE_DISABLED_BY_DEFAULT);

BASE_FEATURE(kCrossDeviceTabsInAppMenu, base::FEATURE_DISABLED_BY_DEFAULT);

BASE_FEATURE(kSyncRecordDeviceStatisticsMetrics,
             base::FEATURE_ENABLED_BY_DEFAULT);

}  // namespace syncer
