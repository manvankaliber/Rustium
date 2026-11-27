// Copyright 2012 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stddef.h>

#include <algorithm>
#include <cstdlib>
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "base/base_paths.h"
#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/no_destructor.h"
#include "base/path_service.h"
#include "base/run_loop.h"
#include "base/threading/thread_restrictions.h"
#include "build/build_config.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/common/chrome_paths.h"
#include "chrome/test/base/chrome_test_utils.h"
#include "chrome/test/base/platform_browser_test.h"
#include "components/policy/core/browser/browser_policy_connector.h"
#include "components/policy/core/browser/policy_pref_mapping_test.h"
#include "components/policy/core/common/mock_configuration_policy_provider.h"
#include "components/policy/core/common/policy_map.h"
#include "components/prefs/pref_service.h"
#include "components/safe_browsing/buildflags.h"
#include "components/variations/variations_test_utils.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

#include "components/enterprise/browser/controller/fake_browser_dm_token_storage.h"

#if BUILDFLAG(IS_WIN)
#include "base/test/test_reg_util_win.h"
#endif  // BUILDFLAG(IS_WIN)

namespace policy {

const size_t kNumChunks = 32;

namespace {

constexpr PolicyPrefMappingBuildFlag kPolicyPrefMappingBuildFlags[] = {
    {"SAFE_BROWSING_AVAILABLE", BUILDFLAG(SAFE_BROWSING_AVAILABLE)},
};

base::FilePath GetTestCaseDir() {
  base::ScopedAllowBlockingForTesting allow_blocking;
  base::FilePath path;
  base::PathService::Get(base::DIR_SRC_TEST_DATA_ROOT, &path);
  return path.Append(FILE_PATH_LITERAL("components"))
      .Append(FILE_PATH_LITERAL("policy"))
      .Append(FILE_PATH_LITERAL("test"))
      .Append(FILE_PATH_LITERAL("data"))
      .Append(FILE_PATH_LITERAL("pref_mapping"));
}

size_t GetNumChunks() {
  if (base::CommandLine::ForCurrentProcess()->HasSwitch(
          kPolicyToPrefMappingsFilterSwitch)) {
    // Run as one chunk when test filter specified.
    return 1;
  }
  return kNumChunks;
}

}  // namespace

typedef PlatformBrowserTest PolicyPrefsTestCoverageTest;

IN_PROC_BROWSER_TEST_F(PolicyPrefsTestCoverageTest, AllPoliciesHaveATestCase) {
  VerifyAllPoliciesHaveATestCase(GetTestCaseDir());
}

// Base class for tests that change policy.
class PolicyPrefsTest : public PlatformBrowserTest {
 public:
  PolicyPrefsTest() = default;
  PolicyPrefsTest(const PolicyPrefsTest&) = delete;
  PolicyPrefsTest& operator=(const PolicyPrefsTest&) = delete;
  ~PolicyPrefsTest() override = default;

 protected:
  void SetUpInProcessBrowserTestFixture() override {
#if BUILDFLAG(IS_WIN)
    // Override HKCU to isolate from and prevent writing to real registry keys.
    // On Windows, the metrics reporting consent is stored in the registry and
    // is used to determine the default value for
    // `metrics::prefs::kMetricsReportingEnabled` on startup. Without this,
    // tests may flake if the registry already has metrics reporting enabled or
    // if policy changes during the test write to the real registry.
    ASSERT_NO_FATAL_FAILURE(
        registry_override_manager_.OverrideRegistry(HKEY_CURRENT_USER));
#endif  // BUILDFLAG(IS_WIN)

    // Some policies default value might depend on features, enforce use of
    // field trial testing config to avoid having unexpected results based on
    // new feature flags coming from the server (e.g. on Chrome-branded CI
    // bots).
    variations::EnableTestingConfig();

    GetMockPolicyProvider()->SetDefaultReturns(
        true /* is_initialization_complete_return */,
        true /* is_first_policy_load_complete_return */);
    BrowserPolicyConnector::SetPolicyProviderForTesting(
        GetMockPolicyProvider());

  }

  void TearDownOnMainThread() override { ClearProviderPolicy(); }

  void ClearProviderPolicy() {
    GetMockPolicyProvider()->UpdateChromePolicy(PolicyMap());
    base::RunLoop().RunUntilIdle();
  }

  MockConfigurationPolicyProvider* GetMockPolicyProvider() {
    // On non-Android platforms, the mock provider cleanup will be triggered
    // by ChromeBrowserPolicyConnector and ProfileImpl destructors. Thus it's
    // safe to define a provider object that is deleted on scope destruction.
    return &provider_;
  }

  testing::NiceMock<MockConfigurationPolicyProvider> provider_;
#if BUILDFLAG(IS_WIN)
  registry_util::RegistryOverrideManager registry_override_manager_;
#endif  // BUILDFLAG(IS_WIN)
};

// Splits the test cases into `kNumChunks` and the testing parameter determines
// the index of the current chunk. This prevents the test from timing out when
// testing all test cases in a single browser test.
class ChunkedPolicyPrefsTest : public PolicyPrefsTest,
                               public ::testing::WithParamInterface<size_t> {
 public:
  ChunkedPolicyPrefsTest() {
  }
  ChunkedPolicyPrefsTest(const ChunkedPolicyPrefsTest&) = delete;
  ChunkedPolicyPrefsTest& operator=(const ChunkedPolicyPrefsTest&) = delete;
  ~ChunkedPolicyPrefsTest() override = default;

 protected:
  PrefMappingChunkInfo chunk_info_{GetParam(), GetNumChunks()};
  policy::FakeBrowserDMTokenStorage storage_;
};

// Verifies that policies make their corresponding preferences become managed,
// and that the user can't override that setting.
// README SHERIFFs: This test encapsulates a whole suite of individual browser
// tests for performance reasons and therefore has an increased chance of
// failure/flakiness.
// IMPORTANT: Please add hendrich@chromium.org on any related bugs when
// disabling this test.
IN_PROC_BROWSER_TEST_P(ChunkedPolicyPrefsTest, PolicyToPrefsMapping) {
  base::ScopedAllowBlockingForTesting allow_blocking;

  policy::BrowserDMTokenStorage::SetForTesting(&storage_);

  PrefService* local_state = g_browser_process->local_state();
  PrefService* user_prefs = ProfileManager::GetLastUsedProfileIfLoaded()
                                ->GetOriginalProfile()
                                ->GetPrefs();

  VerifyPolicyToPrefMappings(GetTestCaseDir(), local_state, user_prefs,
                             /* signin_profile_prefs= */ nullptr,
                             GetMockPolicyProvider(), &chunk_info_,
                             kPolicyPrefMappingBuildFlags);
}

INSTANTIATE_TEST_SUITE_P(Chunked,
                         ChunkedPolicyPrefsTest,
                         ::testing::Range(/* start= */ static_cast<size_t>(0),
                                          /* end= */ GetNumChunks()));


// For WebUI integration tests, see cr_policy_indicator_tests.js and
// cr_policy_pref_indicator_tests.js.

}  // namespace policy
