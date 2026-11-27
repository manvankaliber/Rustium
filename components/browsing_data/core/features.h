// Copyright 2024 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_BROWSING_DATA_CORE_FEATURES_H_
#define COMPONENTS_BROWSING_DATA_CORE_FEATURES_H_

#include "base/feature_list.h"

namespace browsing_data::features {

// Pipes down the BrowsingDataModel to power site settings on Android.


// When enabled, calls to browsingData.removePasswords extension API would
// result in an error response instead of a console warning.
BASE_DECLARE_FEATURE(kPasswordRemovalExtensionErrorKillSwitch);


// When enabled, Shared Workers are tracked in BrowsingDataModel using their
// blink::StorageKey rather than individual SharedWorkerInfo entries, avoiding
// unbounded URL/name string retention in memory.
BASE_DECLARE_FEATURE(kBrowsingDataModelSharedWorkerKeyWithoutURL);

}  // namespace browsing_data::features

#endif  // COMPONENTS_BROWSING_DATA_CORE_FEATURES_H_
