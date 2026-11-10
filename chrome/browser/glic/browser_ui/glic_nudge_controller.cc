// Copyright 2025 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/glic/browser_ui/glic_nudge_controller.h"

#include "build/build_config.h"
#include "chrome/browser/glic/browser_ui/glic_nudge_controller_impl.h"
#include "chrome/browser/glic/browser_ui/glic_split_button_controller.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"


namespace glic {

DEFINE_USER_DATA(GlicNudgeController);

GlicNudgeController::~GlicNudgeController() = default;

// static
std::unique_ptr<GlicNudgeController> GlicNudgeController::CreateFor(
    BrowserWindowInterface* browser) {
  return std::make_unique<GlicNudgeControllerImpl>(
      browser, GlicSplitButtonController::From(browser));
}

// static
GlicNudgeController* GlicNudgeController::From(
    BrowserWindowInterface* browser) {
  if (!browser) {
    return nullptr;
  }
  auto* controller =
      GlicNudgeController::Get(browser->GetUnownedUserDataHost());
  return controller;
}

}  // namespace glic
