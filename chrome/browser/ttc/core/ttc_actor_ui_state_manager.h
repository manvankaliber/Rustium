// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_TTC_CORE_TTC_ACTOR_UI_STATE_MANAGER_H_
#define CHROME_BROWSER_TTC_CORE_TTC_ACTOR_UI_STATE_MANAGER_H_

#include "build/build_config.h"
namespace ttc {

// TTC specific implementation of ActorUiStateManagerInterface. Actor tasks
// created by TTC use this instead of the profile-wide ActorUiStateManager so
// that TTC can drive its own actor UI.


}  // namespace ttc

#endif  // CHROME_BROWSER_TTC_CORE_TTC_ACTOR_UI_STATE_MANAGER_H_
