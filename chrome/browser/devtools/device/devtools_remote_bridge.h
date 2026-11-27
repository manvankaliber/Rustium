// Copyright 2014 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_DEVTOOLS_DEVICE_DEVTOOLS_REMOTE_BRIDGE_H_
#define CHROME_BROWSER_DEVTOOLS_DEVICE_DEVTOOLS_REMOTE_BRIDGE_H_

#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/cancelable_callback.h"
#include "base/functional/callback.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/scoped_refptr.h"
#include "base/memory/weak_ptr.h"
#include "base/no_destructor.h"
#include "chrome/browser/devtools/device/remote_device_manager.h"
#include "chrome/browser/devtools/device/devtools_device_discovery.h"
#include "chrome/browser/profiles/profile_keyed_service_factory.h"
#include "components/keyed_service/core/keyed_service.h"
#include "components/prefs/pref_change_registrar.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/devtools_agent_host.h"


namespace content {
class BrowserContext;
}

class Profile;
class TCPDeviceProvider;

class DevToolsRemoteBridge : public KeyedService {
 public:
  class Factory : public ProfileKeyedServiceFactory {
   public:
    // Returns singleton instance of DevToolsRemoteBridge.
    static Factory* GetInstance();

    // Returns DevToolsRemoteBridge associated with |profile|.
    static DevToolsRemoteBridge* GetForProfile(Profile* profile);

    Factory(const Factory&) = delete;
    Factory& operator=(const Factory&) = delete;

   private:
    friend base::NoDestructor<Factory>;

    Factory();
    ~Factory() override;

    // BrowserContextKeyedServiceFactory overrides:
    std::unique_ptr<KeyedService> BuildServiceInstanceForBrowserContext(
        content::BrowserContext* context) const override;
  };

  using RemotePage = DevToolsDeviceDiscovery::RemotePage;
  using RemotePages = DevToolsDeviceDiscovery::RemotePages;
  using RemoteBrowser = DevToolsDeviceDiscovery::RemoteBrowser;
  using RemoteBrowsers = DevToolsDeviceDiscovery::RemoteBrowsers;
  using RemoteDevice = DevToolsDeviceDiscovery::RemoteDevice;
  using RemoteDevices = DevToolsDeviceDiscovery::RemoteDevices;
  using CompleteDevice = DevToolsDeviceDiscovery::CompleteDevice;
  using CompleteDevices = DevToolsDeviceDiscovery::CompleteDevices;

  using JsonRequestCallback = base::OnceCallback<void(int, const std::string&)>;

  class DeviceListListener {
   public:
    virtual void DeviceListChanged(const RemoteDevices& devices) = 0;
   protected:
    virtual ~DeviceListListener() = default;
  };

  explicit DevToolsRemoteBridge(Profile* profile);
  ~DevToolsRemoteBridge() override;
  DevToolsRemoteBridge(const DevToolsRemoteBridge&) = delete;
  DevToolsRemoteBridge& operator=(const DevToolsRemoteBridge&) = delete;

  void AddDeviceListListener(DeviceListListener* listener);
  void RemoveDeviceListListener(DeviceListListener* listener);

  class DeviceCountListener {
   public:
    virtual void DeviceCountChanged(int count) = 0;
   protected:
    virtual ~DeviceCountListener() = default;
  };

  void AddDeviceCountListener(DeviceCountListener* listener);
  void RemoveDeviceCountListener(DeviceCountListener* listener);

  void set_device_providers_for_test(
      const RemoteDeviceManager::DeviceProviders& device_providers) {
    device_manager_->SetDeviceProviders(device_providers);
  }

  void set_task_scheduler_for_test(
      base::RepeatingCallback<void(base::OnceClosure)> scheduler) {
    task_scheduler_ = scheduler;
  }

  void OpenRemotePage(scoped_refptr<RemoteBrowser> browser,
                      const std::string& url);

  scoped_refptr<content::DevToolsAgentHost> GetBrowserAgentHost(
      scoped_refptr<RemoteBrowser> browser);

  void SendJsonRequest(const std::string& browser_id_str,
                       const std::string& url,
                       JsonRequestCallback callback);

  using TCPProviderCallback =
      base::RepeatingCallback<void(scoped_refptr<TCPDeviceProvider>)>;
  void set_tcp_provider_callback_for_test(TCPProviderCallback callback);

  void Shutdown() override;

 private:
  friend struct content::BrowserThread::DeleteOnThread<
      content::BrowserThread::UI>;
  friend class base::DeleteHelper<DevToolsRemoteBridge>;


  void StartDeviceListPolling();
  void StopDeviceListPolling();
  bool NeedsDeviceListPolling();

  void ReceivedDeviceList(const CompleteDevices& complete_devices);

  void StartDeviceCountPolling();
  void StopDeviceCountPolling();
  void RequestDeviceCount(base::RepeatingCallback<void(int)> callback);
  void ReceivedDeviceCount(int count);

  void CreateDeviceProviders();

  base::WeakPtr<DevToolsRemoteBridge> AsWeakPtr() {
      return weak_factory_.GetWeakPtr();
  }

  const raw_ptr<Profile> profile_;
  std::unique_ptr<RemoteDeviceManager> device_manager_;

  using DeviceMap =
      std::map<std::string, scoped_refptr<RemoteDeviceManager::Device> >;
  DeviceMap device_map_;

  using DeviceListListeners =
      std::vector<raw_ptr<DeviceListListener, VectorExperimental>>;
  DeviceListListeners device_list_listeners_;

  using DeviceCountListeners =
      std::vector<raw_ptr<DeviceCountListener, VectorExperimental>>;
  DeviceCountListeners device_count_listeners_;
  base::CancelableRepeatingCallback<void(int)> device_count_callback_;
  base::RepeatingCallback<void(base::OnceClosure)> task_scheduler_;

  PrefChangeRegistrar pref_change_registrar_;

  TCPProviderCallback tcp_provider_callback_;

  std::unique_ptr<DevToolsDeviceDiscovery> device_discovery_;

  base::WeakPtrFactory<DevToolsRemoteBridge> weak_factory_{this};
};

#endif  // CHROME_BROWSER_DEVTOOLS_DEVICE_DEVTOOLS_REMOTE_BRIDGE_H_
