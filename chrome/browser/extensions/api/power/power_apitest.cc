// Copyright 2024 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/test/test_future.h"
#include "chrome/browser/extensions/extension_apitest.h"
#include "chrome/browser/profiles/profile.h"
#include "content/public/browser/device_service.h"
#include "content/public/test/browser_test.h"
#include "extensions/browser/api/power/power_api.h"
#include "extensions/browser/background_script_executor.h"
#include "extensions/buildflags/buildflags.h"
#include "extensions/common/extension.h"
#include "extensions/test/test_extension_dir.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "services/device/public/mojom/wake_lock.mojom.h"
#include "services/device/public/mojom/wake_lock_provider.mojom.h"


static_assert(BUILDFLAG(ENABLE_EXTENSIONS_CORE));

namespace extensions {
namespace {

class PowerApiTest : public ExtensionApiTest {
 protected:
  int32_t GetActiveWakeLocks(device::mojom::WakeLockType type) {
    PowerAPI::Get(profile())->FlushWakeLockForTesting();
    mojo::Remote<device::mojom::WakeLockProvider> wake_lock_provider;
    content::GetDeviceService().BindWakeLockProvider(
        wake_lock_provider.BindNewPipeAndPassReceiver());
    base::test::TestFuture<int32_t> future;
    wake_lock_provider->GetActiveWakeLocksForTests(type, future.GetCallback());
    return future.Get();
  }
};

IN_PROC_BROWSER_TEST_F(PowerApiTest, Basics) {
  ASSERT_TRUE(RunExtensionTest("power/basics")) << message_;

  // The test should leave no wake locks (no "level" for any extension).
  EXPECT_TRUE(PowerAPI::Get(profile())->extension_levels().empty());
  EXPECT_EQ(0, GetActiveWakeLocks(
                   device::mojom::WakeLockType::kPreventAppSuspension));
  EXPECT_EQ(
      0, GetActiveWakeLocks(device::mojom::WakeLockType::kPreventDisplaySleep));
}

IN_PROC_BROWSER_TEST_F(PowerApiTest, RequestAndChangeWakeLockTypes) {
  TestExtensionDir test_dir;
  test_dir.WriteManifest(R"({
    "name": "Power API WakeLock Test",
    "version": "1.0",
    "manifest_version": 3,
    "permissions": ["power"],
    "background": {"service_worker": "background.js"}
  })");
  test_dir.WriteFile(FILE_PATH_LITERAL("background.js"), "");

  const Extension* extension = LoadExtension(test_dir.UnpackedPath());
  ASSERT_TRUE(extension);

  auto run_script = [&](const char* script) {
    BackgroundScriptExecutor::ExecuteScript(
        profile(), extension->id(), script,
        BackgroundScriptExecutor::ResultCapture::kSendScriptResult);
  };

  run_script(
      "chrome.power.requestKeepAwake('display'); "
      "chrome.test.sendScriptResult('done');");
  EXPECT_EQ(0, GetActiveWakeLocks(
                   device::mojom::WakeLockType::kPreventAppSuspension));
  EXPECT_EQ(
      1, GetActiveWakeLocks(device::mojom::WakeLockType::kPreventDisplaySleep));

  run_script(
      "chrome.power.requestKeepAwake('system'); "
      "chrome.test.sendScriptResult('done');");
  EXPECT_EQ(1, GetActiveWakeLocks(
                   device::mojom::WakeLockType::kPreventAppSuspension));
  EXPECT_EQ(
      0, GetActiveWakeLocks(device::mojom::WakeLockType::kPreventDisplaySleep));

  run_script(
      "chrome.power.requestKeepAwake('display'); "
      "chrome.test.sendScriptResult('done');");
  EXPECT_EQ(0, GetActiveWakeLocks(
                   device::mojom::WakeLockType::kPreventAppSuspension));
  EXPECT_EQ(
      1, GetActiveWakeLocks(device::mojom::WakeLockType::kPreventDisplaySleep));

  run_script(
      "chrome.power.releaseKeepAwake(); "
      "chrome.test.sendScriptResult('done');");
  EXPECT_EQ(0, GetActiveWakeLocks(
                   device::mojom::WakeLockType::kPreventAppSuspension));
  EXPECT_EQ(
      0, GetActiveWakeLocks(device::mojom::WakeLockType::kPreventDisplaySleep));

  // Request 'system' (differing from the initial 'display' creation type) and
  // leave it active so extension unload / profile teardown verifies that
  // WakeLockProvider cleans up a type-changed WakeLock without crashing.
  run_script(
      "chrome.power.requestKeepAwake('system'); "
      "chrome.test.sendScriptResult('done');");
  EXPECT_EQ(1, GetActiveWakeLocks(
                   device::mojom::WakeLockType::kPreventAppSuspension));
}


}  // namespace
}  // namespace extensions
