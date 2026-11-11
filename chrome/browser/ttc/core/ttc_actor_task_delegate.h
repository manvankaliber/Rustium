// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_TTC_CORE_TTC_ACTOR_TASK_DELEGATE_H_
#define CHROME_BROWSER_TTC_CORE_TTC_ACTOR_TASK_DELEGATE_H_

#include <string>
#include <vector>

#include "base/containers/flat_map.h"
#include "base/memory/weak_ptr.h"
namespace ttc {

// The ActorTaskDelegate of a TTC session's actor task.
//
// Navigation confirmation requests are granted (see
// RequestToConfirmNavigation()), and user confirmation dialog requests are
// declined, since TTC has no UI for them. The other dialog requests are only
// made by tools that TTC doesn't expose, so they're NOTREACHED().


}  // namespace ttc

#endif  // CHROME_BROWSER_TTC_CORE_TTC_ACTOR_TASK_DELEGATE_H_
