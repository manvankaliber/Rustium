// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/glic/common/glic_tab_observer.h"

#include "build/build_config.h"

#include "chrome/browser/glic/common/glic_tab_observer_impl.h"

// static
std::unique_ptr<GlicTabObserver> GlicTabObserver::Create(
    Profile* profile,
    EventCallback callback) {
  return std::make_unique<GlicTabObserverImpl>(profile, std::move(callback));
}
