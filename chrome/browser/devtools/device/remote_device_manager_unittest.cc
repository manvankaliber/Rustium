// Copyright 2024 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/devtools/device/remote_device_manager.h"

#include "base/containers/span.h"
#include "base/functional/bind.h"
#include "base/memory/scoped_refptr.h"
#include "base/test/task_environment.h"
#include "base/test/test_future.h"
#include "content/public/test/browser_task_environment.h"
#include "net/base/address_list.h"
#include "net/base/host_port_pair.h"
#include "net/base/ip_address.h"
#include "net/base/net_errors.h"
#include "net/socket/socket_test_util.h"
#include "net/socket/stream_socket.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

class FakeDeviceProvider : public RemoteDeviceManager::DeviceProvider {
 public:
  void QueryDevices(SerialsCallback callback) override {}

  void QueryDeviceInfo(const std::string& serial,
                       DeviceInfoCallback callback) override {}

  void OpenSocket(const std::string& serial,
                  const std::string& socket_name,
                  SocketCallback callback) override {
    CreateMockAcceptSocket(std::move(callback));
  }

  void ReleaseDevice(const std::string& serial) override {}

 private:
  ~FakeDeviceProvider() override = default;

  void CreateMockAcceptSocket(SocketCallback callback) {
    net::StaticSocketDataProvider provider((base::span<net::MockRead>()),
                                           base::span<net::MockWrite>());
    provider.set_connect_data(net::MockConnect(net::SYNCHRONOUS, net::OK));
    auto mock_socket = std::make_unique<net::MockTCPClientSocket>(
        net::AddressList(), nullptr /*netlog*/, &provider);
    mock_socket->set_enable_read_if_ready(true);
    EXPECT_EQ(net::OK, mock_socket->Connect(base::DoNothing()));
    return std::move(callback).Run(net::OK, std::move(mock_socket));
  }
};

class EnumeratingDeviceProvider : public RemoteDeviceManager::DeviceProvider {
 public:
  void QueryDevices(SerialsCallback callback) override {
    std::move(callback).Run({
        {"localhost:9222", RemoteDeviceManager::DeviceInfo::kConnected},
        {"localhost:9229", RemoteDeviceManager::DeviceInfo::kConnected},
    });
  }
  void QueryDeviceInfo(const std::string& serial,
                       DeviceInfoCallback callback) override {
    RemoteDeviceManager::DeviceInfo info;
    info.model = "Network target";
    info.connected_state = RemoteDeviceManager::DeviceInfo::kConnected;
    std::move(callback).Run(info);
  }
  void OpenSocket(const std::string& serial,
                  const std::string& socket_name,
                  SocketCallback callback) override {
    std::move(callback).Run(net::ERR_NOT_IMPLEMENTED, nullptr);
  }
 private:
  ~EnumeratingDeviceProvider() override = default;
};

}  // namespace

TEST(RemoteDeviceManagerTest, SendJSONRequestTimesOutOnUnresponsiveSocket) {
  base::test::SingleThreadTaskEnvironment task_environment{
      base::test::TaskEnvironment::TimeSource::MOCK_TIME};

  int recorded_result;
  scoped_refptr<FakeDeviceProvider> fake_device_provider =
      base::MakeRefCounted<FakeDeviceProvider>();
  RemoteDeviceManager::CommandCallback callback =
      base::BindOnce([](int* out_result, int result,
                        const std::string& response) { *out_result = result; },
                     &recorded_result);

  fake_device_provider->SendJsonRequest("MockSerial", "MockSocket",
                                        "MockRequest", std::move(callback));
  task_environment.FastForwardBy(base::Seconds(2));

  EXPECT_EQ(net::ERR_TIMED_OUT, recorded_result);
}

TEST(RemoteDeviceManagerTest, CountsConfiguredNetworkEndpointsWithoutUsb) {
  content::BrowserTaskEnvironment task_environment;
  auto manager = RemoteDeviceManager::Create();
  RemoteDeviceManager::DeviceProviders providers;
  providers.push_back(base::MakeRefCounted<EnumeratingDeviceProvider>());
  manager->SetDeviceProviders(providers);
  base::test::TestFuture<int> result;
  manager->CountDevices(result.GetCallback());
  EXPECT_EQ(2, result.Get());
}
