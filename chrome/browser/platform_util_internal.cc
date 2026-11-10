// Copyright 2025 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/platform_util_internal.h"

#include <utility>

#include "base/functional/callback.h"
#include "base/no_destructor.h"
#include "build/build_config.h"
#include "build/buildflag.h"


namespace platform_util::internal {
namespace {

bool g_shell_operations_allowed = true;
base::RepeatingClosure& GetOpenItemThreadObserverForTesting() {
  static base::NoDestructor<base::RepeatingClosure> observer;
  return *observer;
}

}  // namespace

void DisableShellOperationsForTesting() {
  g_shell_operations_allowed = false;

}

bool AreShellOperationsAllowed() {
  return g_shell_operations_allowed;
}

void SetOpenItemThreadObserverForTesting(base::RepeatingClosure observer) {
  GetOpenItemThreadObserverForTesting() = std::move(observer);
}

void RunOpenItemThreadObserverForTesting() {
  const base::RepeatingClosure& observer =
      GetOpenItemThreadObserverForTesting();
  if (observer) {
    observer.Run();
  }
}

}  // namespace platform_util::internal
