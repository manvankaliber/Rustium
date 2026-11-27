// Copyright 2022 The Abseil Authors.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
#include "absl/log/internal/test_helpers.h"

#include <csignal>

#include "gtest/gtest.h"
#include "absl/base/config.h"
#include "absl/base/log_severity.h"
#include "absl/log/globals.h"
#include "absl/log/initialize.h"
#include "absl/log/internal/globals.h"


#if defined(ABSL_HAVE_ALARM)
#include <signal.h>
#endif

namespace absl {
ABSL_NAMESPACE_BEGIN
namespace log_internal {

// Returns false if the specified severity level is disabled by
// `ABSL_MIN_LOG_LEVEL` or `absl::MinLogLevel()`.
bool LoggingEnabledAt(absl::LogSeverity severity) {
  return severity >= kAbslMinLogLevel && severity >= absl::MinLogLevel();
}

// -----------------------------------------------------------------------------
// Googletest Death Test Predicates
// -----------------------------------------------------------------------------

#if GTEST_HAS_DEATH_TEST

bool DiedOfFatal(int exit_status) {
#if defined(_WIN32)
  // Depending on NDEBUG and (configuration?) MSVC's abort either results
  // in error code 3 (SIGABRT) or error code 0x80000003 (breakpoint
  // triggered).
  return ::testing::ExitedWithCode(3)(exit_status & 0x7fffffff);
#else
  return ::testing::KilledBySignal(SIGABRT)(exit_status);
#endif
}

bool DiedOfQFatal(int exit_status) {
  return ::testing::ExitedWithCode(1)(exit_status);
}

#endif

// -----------------------------------------------------------------------------
// Helper for Log initialization in test
// -----------------------------------------------------------------------------

void LogTestEnvironment::SetUp() {
  if (!absl::log_internal::IsInitialized()) {
    absl::InitializeLog();
  }
}

}  // namespace log_internal
ABSL_NAMESPACE_END
}  // namespace absl
