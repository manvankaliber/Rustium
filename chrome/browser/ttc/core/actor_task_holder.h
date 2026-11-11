// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_TTC_CORE_ACTOR_TASK_HOLDER_H_
#define CHROME_BROWSER_TTC_CORE_ACTOR_TASK_HOLDER_H_

#include "base/memory/raw_ref.h"
#include "chrome/browser/ttc/core/ttc_actor_task_delegate.h"
#include "components/actor/core/task_id.h"

class Profile;

namespace ttc {

class SessionControllerImpl;

// Creates and owns the actor task for a TTC session. The task is started on
// construction and stopped on destruction.


}  // namespace ttc

#endif  // CHROME_BROWSER_TTC_CORE_ACTOR_TASK_HOLDER_H_
