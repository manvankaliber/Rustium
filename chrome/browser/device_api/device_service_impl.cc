// Copyright 2020 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include "chrome/browser/device_api/device_service_impl.h"

#include <functional>
#include <optional>
#include <utility>

#include "base/check_deref.h"
#include "base/check_is_test.h"
#include "base/feature_list.h"
#include "base/functional/bind.h"
#include "base/memory/self_deleting.h"
#include "base/types/expected_macros.h"
#include "build/build_config.h"
#include "chrome/browser/app_mode/app_mode_utils.h"
#include "chrome/browser/device_api/device_attribute_api.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/web_applications/isolated_web_apps/isolated_web_app_features.h"
#include "chrome/browser/web_applications/policy/web_app_policy_constants.h"
#include "chrome/browser/web_applications/web_app_filter.h"
#include "chrome/browser/web_applications/web_app_provider.h"
#include "chrome/browser/web_applications/web_app_registrar.h"
#include "chrome/browser/web_applications/web_app_tab_helper.h"
#include "chrome/common/pref_names.h"
#include "components/permissions/features.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/render_frame_host.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "url/origin.h"


namespace {

Profile* GetProfile(content::RenderFrameHost& host) {
  return Profile::FromBrowserContext(host.GetBrowserContext());
}

std::optional<std::reference_wrapper<const web_app::WebAppRegistrar>>
GetRegistrar(content::RenderFrameHost& host) {
  const web_app::WebAppProvider* web_app_provider =
      web_app::WebAppProvider::GetForWebApps(GetProfile(host));
  if (!web_app_provider) {
    return std::nullopt;
  }
  return web_app_provider->registrar_unsafe();
}


bool IsTrustedContext(content::RenderFrameHost& host,
                      const url::Origin& origin) {
  // Do not create the service for incognito or enterprise isolated mode.
  if (GetProfile(host)->IsPrimaryOTRProfileWithRegularParent()) {
    return false;
  }


  ASSIGN_OR_RETURN(const web_app::WebAppRegistrar& registrar,
                   GetRegistrar(host), [] { return false; });
  return registrar
      .FindBestAppWithUrlInScope(
          origin.GetURL(),
          web_app::WebAppFilter::PolicyInstalledIsolatedWebApp() |
              web_app::WebAppFilter::IsDevModeIsolatedApp(),
          {.scope_score_options = {.exclude_scope_extensions = true}})
      .has_value();
}

bool IsAllowedByPermissionsPolicy(content::RenderFrameHost& host) {
  return host.IsFeatureEnabled(
      network::mojom::PermissionsPolicyFeature::kDeviceAttributes);
}
}  // namespace

DeviceServiceImpl::DeviceServiceImpl(
    content::RenderFrameHost& host,
    mojo::PendingReceiver<blink::mojom::DeviceAPIService> receiver,
    std::unique_ptr<DeviceAttributeApi> device_attribute_api,
    base::SelfDeletingPassKey key)
    : DocumentService(host, std::move(receiver), key),
      device_attribute_api_(std::move(device_attribute_api)) {
  Profile* const profile = GetProfile(host);
  pref_change_registrar_.Init(profile->GetPrefs());
  pref_change_registrar_.Add(
      ::prefs::kWebAppInstallForceList,
      base::BindRepeating(&DeviceServiceImpl::OnDisposingIfNeeded,
                          base::Unretained(this)));
  pref_change_registrar_.Add(
      ::prefs::kIsolatedWebAppInstallForceList,
      base::BindRepeating(&DeviceServiceImpl::OnDisposingIfNeeded,
                          base::Unretained(this)));
  auto& provider = CHECK_DEREF(web_app::WebAppProvider::GetForWebApps(profile));
  install_manager_observation_.Observe(&provider.install_manager());
}

DeviceServiceImpl::~DeviceServiceImpl() = default;

// static
void DeviceServiceImpl::Create(
    content::RenderFrameHost* host,
    mojo::PendingReceiver<blink::mojom::DeviceAPIService> receiver,
    std::unique_ptr<DeviceAttributeApi> device_attribute_api) {
  CHECK(host);
  CHECK_CURRENTLY_ON(content::BrowserThread::UI, base::NotFatalUntil::M161);

  if (host->GetParentOrOuterDocumentOrEmbedder()) {
    mojo::ReportBadMessage(
        "Device Attributes are allowed only in top level frames.");
    return;
  }
  if (!IsTrustedContext(*host, host->GetLastCommittedOrigin())) {
    // Not sending bad message here since the API is always exposed to the end
    // user.
    return;
  }
  if (!IsAllowedByPermissionsPolicy(*host)) {
    mojo::ReportBadMessage(
        "Permissions policy blocks access to Device Attributes.");
    return;
  }
  // The object is bound to the lifetime of |host| and the mojo
  // connection. See DocumentService for details.
  base::MakeSelfDeleting<DeviceServiceImpl>(*host, std::move(receiver),
                                            std::move(device_attribute_api));
}

// static
void DeviceServiceImpl::Create(
    content::RenderFrameHost* host,
    mojo::PendingReceiver<blink::mojom::DeviceAPIService> receiver) {
  Create(host, std::move(receiver), std::make_unique<DeviceAttributeApiImpl>());
}

// static
void DeviceServiceImpl::CreateForTest(
    content::RenderFrameHost* host,
    mojo::PendingReceiver<blink::mojom::DeviceAPIService> receiver,
    std::unique_ptr<DeviceAttributeApi> device_attribute_api) {
  CHECK_IS_TEST();
  Create(host, std::move(receiver), std::move(device_attribute_api));
}

void DeviceServiceImpl::OnWebAppSourceRemoved(const webapps::AppId& app_id) {
  OnDisposingIfNeeded();
}

void DeviceServiceImpl::OnWebAppUninstalled(
    const webapps::AppId& app_id,
    webapps::WebappUninstallSource uninstall_source) {
  OnDisposingIfNeeded();
}

void DeviceServiceImpl::OnWebAppInstallManagerDestroyed() {
  install_manager_observation_.Reset();
}


void DeviceServiceImpl::OnDisposingIfNeeded() {
  // DeviceServiceImpl is allocated on the heap, thus it is safe to remove it
  // like this.
  if (!IsTrustedContext(render_frame_host(), origin())) {
    ResetAndDeleteThis();
  }
}

void DeviceServiceImpl::GetDirectoryId(GetDirectoryIdCallback callback) {
  GetDeviceAttribute(&DeviceAttributeApi::GetDirectoryId, std::move(callback));
}

void DeviceServiceImpl::GetHostname(GetHostnameCallback callback) {
  GetDeviceAttribute(&DeviceAttributeApi::GetHostname, std::move(callback));
}

void DeviceServiceImpl::GetSerialNumber(GetSerialNumberCallback callback) {
  GetDeviceAttribute(&DeviceAttributeApi::GetSerialNumber, std::move(callback));
}

void DeviceServiceImpl::GetAnnotatedAssetId(
    GetAnnotatedAssetIdCallback callback) {
  GetDeviceAttribute(&DeviceAttributeApi::GetAnnotatedAssetId,
                     std::move(callback));
}

void DeviceServiceImpl::GetAnnotatedLocation(
    GetAnnotatedLocationCallback callback) {
  GetDeviceAttribute(&DeviceAttributeApi::GetAnnotatedLocation,
                     std::move(callback));
}

void DeviceServiceImpl::GetDeviceAttribute(
    [[maybe_unused]] void (DeviceAttributeApi::*method)(
        DeviceAttributeCallback callback),
    DeviceAttributeCallback callback) {
  device_attribute_api_->ReportNotSupportedPlatformError(std::move(callback));
}
