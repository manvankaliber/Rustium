// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ttc/core/page_context_util.h"

#include <utility>

#include "base/functional/bind.h"
#include "base/functional/callback.h"
#include "chrome/browser/search/search.h"
#include "components/actor/core/safety_list_manager.h"
#include "url/gurl.h"

namespace ttc {

namespace {

// Returns whether `url`'s scheme and host are supported for page context.
// Similar to, but stricter than, Glic's IsTabValidForSharing() (see
// chrome/browser/glic/host/context/glic_sharing_utils.cc), which also allows
// file: URLs and a few other WebUI pages.
//
// The WebUI New Tab Page is allowed on every platform that serves one, which
// includes desktop Android. The native New Tab Page used on mobile Android has
// no DOM to extract and is not supported.
//
// TODO(crbug.com/555804152): Decide what page context, if any, the mobile
// Android native New Tab Page should produce.
bool IsSchemeAndHostSupported(const GURL& url) {
  if (search::IsWebUINewTabPageOrigin(url)) {
    return true;
  }
  // Excludes all other chrome:// URLs, as well as file:, javascript:, about:,
  // data:, blob:, chrome-extension:, etc.
  return url.SchemeIsHTTPOrHTTPS();
}

}  // namespace


}  // namespace ttc
