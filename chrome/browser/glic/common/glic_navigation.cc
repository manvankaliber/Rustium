// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/glic/common/glic_navigation.h"

#include <memory>
#include <utility>

#include "base/functional/bind.h"
#include "base/functional/callback.h"
#include "base/memory/weak_ptr.h"
#include "build/build_config.h"
#include "chrome/browser/ui/navigator/browser_navigator.h"
#include "chrome/browser/ui/navigator/browser_navigator_params.h"
#include "content/public/browser/navigation_handle.h"


namespace glic {

namespace {

void EnsureBrowserForNavigateParams(NavigateParams* params) {
}

}  // namespace

base::WeakPtr<content::NavigationHandle> Navigate(
    std::unique_ptr<NavigateParams> params) {
  EnsureBrowserForNavigateParams(params.get());
  return ::Navigate(params.get());
}

void NavigateAsync(
    std::unique_ptr<NavigateParams> params,
    base::OnceCallback<void(base::WeakPtr<content::NavigationHandle>)>
        callback) {
  EnsureBrowserForNavigateParams(params.get());
  NavigateParams* params_ptr = params.get();
  // We use BindOnce with a lambda that takes the unique_ptr by value to bind
  // it to the internal callback and ensure it lives until ::Navigate completes.
  ::Navigate(
      params_ptr,
      base::BindOnce(
          [](std::unique_ptr<NavigateParams> kept_alive_params,
             base::OnceCallback<void(base::WeakPtr<content::NavigationHandle>)>
                 original_callback,
             base::WeakPtr<content::NavigationHandle> handle) {
            if (original_callback) {
              std::move(original_callback).Run(handle);
            }
          },
          std::move(params), std::move(callback)));
}

}  // namespace glic
