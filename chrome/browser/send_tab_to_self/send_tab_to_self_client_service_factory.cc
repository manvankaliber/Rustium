// Copyright 2019 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/send_tab_to_self/send_tab_to_self_client_service_factory.h"

#include <string>

#include "base/functional/bind.h"
#include "base/no_destructor.h"
#include "build/build_config.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/send_tab_to_self/send_tab_to_self_client_service.h"
#include "chrome/browser/sync/send_tab_to_self_sync_service_factory.h"
#include "chrome/common/chrome_features.h"
#include "components/send_tab_to_self/send_tab_to_self_model.h"
#include "components/send_tab_to_self/send_tab_to_self_sync_service.h"


#include "chrome/browser/ui/send_tab_to_self/send_tab_to_self_receiving_ui_handler.h"

namespace send_tab_to_self {
// static
SendTabToSelfClientService* SendTabToSelfClientServiceFactory::GetForProfile(
    Profile* profile) {
  return static_cast<send_tab_to_self::SendTabToSelfClientService*>(
      GetInstance()->GetServiceForBrowserContext(profile, true));
}

// static
SendTabToSelfClientServiceFactory*
SendTabToSelfClientServiceFactory::GetInstance() {
  static base::NoDestructor<SendTabToSelfClientServiceFactory> instance;
  return instance.get();
}

SendTabToSelfClientServiceFactory::SendTabToSelfClientServiceFactory()
    : ProfileKeyedServiceFactory(
          "SendTabToSelfClientService",
          ProfileSelections::Builder()
              .WithRegular(ProfileSelection::kOriginalOnly)
              .WithGuest(ProfileSelection::kNone)
              .WithAshInternals(ProfileSelection::kNone)
              .Build()) {
  DependsOn(SendTabToSelfSyncServiceFactory::GetInstance());
}

SendTabToSelfClientServiceFactory::~SendTabToSelfClientServiceFactory() =
    default;

// BrowserContextKeyedServiceFactory implementation.
std::unique_ptr<KeyedService>
SendTabToSelfClientServiceFactory::BuildServiceInstanceForBrowserContext(
    content::BrowserContext* context) const {
  Profile* profile = Profile::FromBrowserContext(context);
  SendTabToSelfSyncService* sync_service =
      SendTabToSelfSyncServiceFactory::GetForProfile(profile);


  SendTabToSelfModel* model = sync_service->GetSendTabToSelfModel();
  return std::make_unique<SendTabToSelfClientService>(
      std::make_unique<SendTabToSelfReceivingUiHandler>(profile)
          ,
      model);
}

bool SendTabToSelfClientServiceFactory::ServiceIsCreatedWithBrowserContext()
    const {
  return !::features::IsLazyAfterStartupKeyedServicesEnabled();
}

bool SendTabToSelfClientServiceFactory::ServiceIsCreatedAfterStartup() const {
  return ::features::IsLazyAfterStartupKeyedServicesEnabled();
}

bool SendTabToSelfClientServiceFactory::ServiceIsNULLWhileTesting() const {
  return true;
}

}  // namespace send_tab_to_self
