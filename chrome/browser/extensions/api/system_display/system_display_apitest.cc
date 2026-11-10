// Copyright 2013 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <memory>
#include <optional>
#include <string>

#include "base/functional/bind.h"
#include "base/test/gtest_tags.h"
#include "build/build_config.h"
#include "build/chromeos_buildflags.h"
#include "chrome/browser/extensions/extension_apitest.h"
#include "chrome/browser/profiles/profile.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "extensions/browser/api/system_display/display_info_provider.h"
#include "extensions/browser/api/system_display/system_display_api.h"
#include "extensions/browser/api_test_utils.h"
#include "extensions/browser/extension_host.h"
#include "extensions/browser/mock_display_info_provider.h"
#include "extensions/browser/process_manager.h"
#include "extensions/common/api/system_display.h"
#include "extensions/common/extension_builder.h"
#include "extensions/test/result_catcher.h"

namespace extensions {

class SystemDisplayApiTest : public ExtensionApiTest {
 public:
  SystemDisplayApiTest()
      : provider_(std::make_unique<MockDisplayInfoProvider>()) {}

  SystemDisplayApiTest(const SystemDisplayApiTest&) = delete;
  SystemDisplayApiTest& operator=(const SystemDisplayApiTest&) = delete;

  ~SystemDisplayApiTest() override = default;

  void SetUpOnMainThread() override {
    ExtensionApiTest::SetUpOnMainThread();
    DisplayInfoProvider::InitializeForTesting(provider_.get());
  }

 protected:
  void SetInfo(const std::string& display_id,
               const api::system_display::DisplayProperties& properties) {
    provider_->SetDisplayProperties(
        display_id, properties,
        base::BindOnce([](std::optional<std::string>) {}));
  }
  std::unique_ptr<MockDisplayInfoProvider> provider_;
};


#if BUILDFLAG(IS_WIN)
using SystemDisplayGetInfoTest = ExtensionApiTest;

IN_PROC_BROWSER_TEST_F(SystemDisplayGetInfoTest, GetInfo) {
  DisplayInfoProvider::ResetForTesting();
  scoped_refptr<const Extension> test_extension =
      ExtensionBuilder("Test", ExtensionBuilder::Type::PLATFORM_APP).Build();

  scoped_refptr<SystemDisplayGetInfoFunction> get_info_function(
      new SystemDisplayGetInfoFunction());

  get_info_function->set_has_callback(true);
  get_info_function->set_extension(test_extension.get());

  // Verify that the GetInfo function runs successfully. This uses the real
  // DisplayInfoProvider to verify that DisplayInfoProvider::GetAllDisplaysInfo
  // calls its callback.
  ASSERT_TRUE(
      api_test_utils::RunFunction(get_info_function.get(), "[]", profile()));
}
#endif  // BUILDFLAG(IS_WIN)

}  // namespace extensions
