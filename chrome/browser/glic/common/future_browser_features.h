// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_GLIC_COMMON_FUTURE_BROWSER_FEATURES_H_
#define CHROME_BROWSER_GLIC_COMMON_FUTURE_BROWSER_FEATURES_H_

#include "base/callback_list.h"
#include "base/functional/callback.h"
#include "build/build_config.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"

// The intent is to remove functions from this file as things become available
// on Android.
namespace glic {

inline base::CallbackListSubscription RegisterBrowserDidClose(
    BrowserWindowInterface* browser_window,
    base::RepeatingCallback<void(BrowserWindowInterface*)> callback) {
  return browser_window->RegisterBrowserDidClose(std::move(callback));
}

inline base::CallbackListSubscription RegisterActiveTabDidChange(
    BrowserWindowInterface* browser_window,
    base::RepeatingCallback<void(BrowserWindowInterface*)> callback) {
  return browser_window->RegisterActiveTabDidChange(std::move(callback));
}

inline bool IsActive(BrowserWindowInterface* browser_window) {
  return browser_window->IsActive();
}

inline base::WeakPtr<BrowserWindowInterface> GetBrowserWindowInterfaceWeakPtr(
    BrowserWindowInterface* browser_window) {
  return browser_window ? browser_window->GetWeakPtr() : nullptr;
}

}  // namespace glic

#endif  // CHROME_BROWSER_GLIC_COMMON_FUTURE_BROWSER_FEATURES_H_
