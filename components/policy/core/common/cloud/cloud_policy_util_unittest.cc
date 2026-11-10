// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/policy/core/common/cloud/cloud_policy_util.h"

#include <memory>
#include <string>

#include "build/build_config.h"
#include "components/policy/proto/device_management_backend.pb.h"
#include "testing/gtest/include/gtest/gtest.h"


namespace policy {

class CloudPolicyUtilTest : public testing::Test {
 protected:
  void TearDown() override {
  }
};

TEST_F(CloudPolicyUtilTest, GetMachineNameAndDeviceIdentifier) {
  const std::string machine_name = GetMachineName();
  EXPECT_FALSE(machine_name.empty());
  EXPECT_EQ(machine_name, GetDeviceName());

  std::unique_ptr<enterprise_management::BrowserDeviceIdentifier>
      device_identifier = GetBrowserDeviceIdentifier();
  ASSERT_TRUE(device_identifier);
  EXPECT_EQ(machine_name, device_identifier->computer_name());
}


}  // namespace policy
