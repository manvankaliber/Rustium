// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_OMNIBOX_OMNIBOX_POPUP_WEBUI_CREATION_H_
#define CHROME_BROWSER_UI_OMNIBOX_OMNIBOX_POPUP_WEBUI_CREATION_H_

#include "base/auto_reset.h"

namespace omnibox {

// Whether WebUI omnibox popups create their content only when first needed
// (on show, or when the classic popup's context menu opens). On-demand creation
// is the default to avoid keeping hidden documents alive in idle windows.
// The first show loads the page, so the popup may show before it has painted.
// TODO(crbug.com/542347163): Make the full WebUI omnibox popup honor this too.
bool ShouldCreatePopupWebUIOnDemand();

// Sets the value while the returned object is alive.
[[nodiscard]] base::AutoReset<bool> SetCreatePopupWebUIOnDemandForTesting(
    bool on_demand);

}  // namespace omnibox

#endif  // CHROME_BROWSER_UI_OMNIBOX_OMNIBOX_POPUP_WEBUI_CREATION_H_
