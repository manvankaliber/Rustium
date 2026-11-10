// Copyright 2015 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/component_extensions_allowlist/allowlist.h"

#include <stddef.h>

#include "base/containers/fixed_flat_set.h"
#include "base/logging.h"
#include "base/notreached.h"
#include "build/branding_buildflags.h"
#include "build/build_config.h"
#include "build/chromeos_buildflags.h"
#include "chrome/common/buildflags.h"
#include "chrome/common/extensions/extension_constants.h"
#include "chrome/grit/aim_eligibility_extension_resources.h"
#include "chrome/grit/browser_resources.h"
#include "chrome/grit/component_extension_resources.h"
#include "chrome/grit/contextual_tasks_extension_resources.h"
#include "extensions/buildflags/buildflags.h"
#include "extensions/common/constants.h"
#include "printing/buildflags/buildflags.h"
#include "ui/webui/buildflags.h"

#if BUILDFLAG(ENABLE_WEBUI_NTP)
#include "chrome/grit/new_tab_page_extension_resources.h"
#endif  // BUILDFLAG(ENABLE_WEBUI_NTP)


static_assert(BUILDFLAG(ENABLE_EXTENSIONS_CORE));

namespace extensions {

bool IsComponentExtensionAllowlisted(const std::string& extension_id) {
  constexpr auto kAllowed = base::MakeFixedFlatSet<std::string_view>({
      extension_misc::kAimEligibilityExtensionId,
      extension_misc::kContextualTasksExtensionId,
      extension_misc::kGlicExtensionId,
      extension_misc::kInAppPaymentsSupportAppId,
#if BUILDFLAG(ENABLE_WEBUI_NTP)
      extension_misc::kNewTabPageExtensionId,
#endif  // BUILDFLAG(ENABLE_WEBUI_NTP)
      extension_misc::kPdfExtensionId,
#if BUILDFLAG(IS_WIN) || BUILDFLAG(IS_LINUX) || BUILDFLAG(IS_MAC)
      extension_misc::kTTSEngineExtensionId,
      extension_misc::kComponentUpdaterTTSEngineExtensionId,
#endif  // BUILDFLAG(IS_WIN) || BUILDFLAG(IS_LINUX) || BUILDFLAG(IS_MAC)
      extension_misc::kIndigoExtensionId,
      extension_misc::kDictationConnectorExtensionId,
  });

  if (kAllowed.contains(extension_id)) {
    return true;
  }

  LOG(ERROR) << "Component extension with id " << extension_id << " not in "
             << "allowlist and is not being loaded as a result.";
  NOTREACHED() << "Component extension with id " << extension_id << " not in "
               << "allowlist and is not being loaded as a result.";
}

bool IsComponentExtensionAllowlisted(int manifest_resource_id) {
  switch (manifest_resource_id) {
    // Please keep the list in alphabetical order.
#if BUILDFLAG(ENABLE_HANGOUT_SERVICES_EXTENSION)
    case IDR_HANGOUT_SERVICES_MANIFEST_V2:
    case IDR_HANGOUT_SERVICES_MANIFEST_V3:
#endif
    case IDR_AIM_ELIGIBILITY_EXTENSION_MANIFEST_JSON:
    case IDR_CONTEXTUAL_TASKS_EXTENSION_MANIFEST_JSON:
    case IDR_GLIC_EXTENSION_MANIFEST:
#if BUILDFLAG(ENABLE_WEBUI_NTP)
    case IDR_NEW_TAB_PAGE_EXTENSION_MANIFEST_JSON:
#endif  // BUILDFLAG(ENABLE_WEBUI_NTP)
    case IDR_NETWORK_SPEECH_SYNTHESIS_MANIFEST:
    case IDR_NETWORK_SPEECH_SYNTHESIS_MANIFEST_MV3:
    case IDR_WEBSTORE_MANIFEST:

      return true;
  }

  LOG(ERROR) << "Component extension with manifest resource id "
             << manifest_resource_id << " not in allowlist and is not being "
             << "loaded as a result.";
  NOTREACHED() << "Component extension with manifest resource id "
               << manifest_resource_id << " not in allowlist and is not being "
               << "loaded as a result.";
}


}  // namespace extensions
