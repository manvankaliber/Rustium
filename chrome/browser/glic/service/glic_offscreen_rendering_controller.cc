// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/glic/service/glic_offscreen_rendering_controller.h"

#include "base/check.h"
#include "base/feature_list.h"
#include "build/build_config.h"
#include "chrome/browser/glic/public/features.h"
#include "content/public/browser/web_contents.h"


namespace glic {

GlicOffscreenRenderingController::GlicOffscreenRenderingController(
    Delegate& delegate)
    : delegate_(delegate) {}

GlicOffscreenRenderingController::~GlicOffscreenRenderingController() = default;

void GlicOffscreenRenderingController::SetHiddenInitializing(
    bool initializing) {
  if (is_hidden_initializing_ == initializing) {
    return;
  }
  is_hidden_initializing_ = initializing;
  UpdateState();
}

void GlicOffscreenRenderingController::UpdateState() {
}

void GlicOffscreenRenderingController::Stop() {
}

}  // namespace glic
