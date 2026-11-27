// Copyright 2024 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/public/browser/back_forward_transition_animation_manager.h"

#include "content/public/browser/content_browser_client.h"
#include "content/public/common/content_client.h"
#include "content/public/common/content_features.h"
#include "third_party/blink/public/common/features_generated.h"
#include "ui/gfx/animation/animation.h"


namespace content {

// static
bool BackForwardTransitionAnimationManager::
    ShouldAnimateBackForwardTransitions() {
  return content::GetContentClient()
      ->browser()
      ->ShouldAnimateBackForwardTransitions();
}

// static
bool BackForwardTransitionAnimationManager::ShouldAnimateNavigationTransition(
    NavigationDirection navigation_direction,
    ui::BackGestureEventSwipeEdge edge) {
  return false;
}
}  // namespace content
