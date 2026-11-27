// Copyright 2013 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/search/instant_service_factory.h"

#include "base/trace_event/trace_event.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/search/instant_service.h"
#include "components/search/search.h"

#include "chrome/browser/themes/theme_service_factory.h"

// static
InstantService* InstantServiceFactory::GetForProfile(Profile* profile) {
  CHECK(search::IsInstantExtendedAPIEnabled(), base::NotFatalUntil::M162);
  TRACE_EVENT(TRACE_DISABLED_BY_DEFAULT("loading"),
              "InstantServiceFactory::GetForProfile");
  return static_cast<InstantService*>(
      GetInstance()->GetServiceForBrowserContext(profile, true));
}

// static
InstantServiceFactory* InstantServiceFactory::GetInstance() {
  static base::NoDestructor<InstantServiceFactory> instance;
  return instance.get();
}

InstantServiceFactory::InstantServiceFactory()
    : ProfileKeyedServiceFactory(
          "InstantService",
          ProfileSelections::Builder()
              .WithRegular(ProfileSelection::kOwnInstance)
              // TODO(crbug.com/40257657): Check if this service is needed in
              // Guest mode.
              .WithGuest(ProfileSelection::kOwnInstance)
              // TODO(crbug.com/41488885): Check if this service is needed for
              // Ash Internals.
              .WithAshInternals(ProfileSelection::kOwnInstance)
              .Build()) {
  // TODO(b/562623656): Support ThemeService for on Desktop Android.
  DependsOn(ThemeServiceFactory::GetInstance());
}

InstantServiceFactory::~InstantServiceFactory() = default;

std::unique_ptr<KeyedService>
InstantServiceFactory::BuildServiceInstanceForBrowserContext(
    content::BrowserContext* context) const {
  CHECK(search::IsInstantExtendedAPIEnabled(), base::NotFatalUntil::M162);
  return std::make_unique<InstantService>(Profile::FromBrowserContext(context));
}

void InstantServiceFactory::BrowserContextDestroyed(
    content::BrowserContext* browser_context) {
  Profile::FromBrowserContext(browser_context)->set_instant_service(nullptr);
  BrowserContextKeyedServiceFactory::BrowserContextDestroyed(browser_context);
}
