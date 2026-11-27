// Copyright 2025 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_OMNIBOX_BROWSER_AIM_ELIGIBILITY_SERVICE_H_
#define COMPONENTS_OMNIBOX_BROWSER_AIM_ELIGIBILITY_SERVICE_H_

#include <optional>
#include <string>

#include "base/callback_list.h"
#include "base/memory/scoped_refptr.h"
#include "components/contextual_tasks/public/host_override.h"
#include "components/keyed_service/core/keyed_service.h"
#include "components/omnibox/browser/aim_eligibility_service_features.h"
#include "third_party/omnibox_proto/aim_eligibility_response.pb.h"
#include "url/gurl.h"

class PrefRegistrySimple;
class PrefService;
class TemplateURLService;

namespace base {
struct Feature;
}

namespace signin {
struct AccessTokenInfo;
class IdentityManager;
class PrimaryAccountAccessTokenFetcher;
}  // namespace signin

namespace variations {
class VariationsService;
}

namespace network {
class SimpleURLLoader;
class SharedURLLoaderFactory;
struct ResourceRequest;
}  // namespace network

// Compatibility interface for shared search consumers. AI Mode has been
// removed: all availability checks fail, responses stay empty, and this class
// never observes accounts or networks or starts a request.
class AimEligibilityService : public KeyedService {
 public:
  static bool GenericKillSwitchFeatureCheck(
      const AimEligibilityService* aim_eligibility_service,
      const base::Feature& feature);
  static void RegisterProfilePrefs(PrefRegistrySimple* registry);
  static bool IsAimAllowedByPolicy(const PrefService* prefs);
  static bool IsAimAllowedByThirdPartyPolicy(const PrefService* prefs);

  enum class EligibilityResponseSource {
    kDefault = 0,
    kPrefs = 1,
    kServer = 2,
    kBrowserCache = 3,
    kUser = 4,
    kMaxValue = kUser,
  };

  static std::string EligibilityResponseSourceToString(
      EligibilityResponseSource source);

  enum class ServerEligibilityRequestMode {
    kLegacyGet = 0,
    kGetWithLocale = 1,
    kPostWithProto = 2,
  };

  struct Configuration {
    // Whether the profile is off-the-record.
    bool is_off_the_record = false;

    // The value for the `User-Agent` header when Co-Browse is enabled. The
    // enabled / disabled state refers to the feature flag for Co-Browse, not
    // whether this client is Co-Browse eligible.
    std::string user_agent_with_cobrowse_suffix;

    // The value for the `Sec-CH-UA-Full-Version-List` HTTP Header. The header
    // is skipped if it is empty.
    std::string full_version_list;

    // The value for the `Chrome-Search-Capabilities-Version` HTTP Header. The
    // header is skipped if it is empty.
    std::string search_capabilities_version;
  };

  static ServerEligibilityRequestMode GetServerEligibilityRequestMode();

  AimEligibilityService(
      PrefService& pref_service,
      TemplateURLService* template_url_service,
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
      signin::IdentityManager* identity_manager,
      const std::string& locale,
      Configuration configuration);
  ~AimEligibilityService() override;

  bool IsLanguage(const std::string& language) const;

  [[nodiscard]] virtual base::CallbackListSubscription
  RegisterEligibilityChangedCallback(base::RepeatingClosure callback);

  virtual bool IsServerEligibilityEnabled() const;

  virtual bool IsAimAllowedByDse() const;

  virtual bool IsAimAllowedByFeatureAndPolicy() const;

  virtual bool IsAimAllowedByThirdPartyPolicy() const;

  virtual bool IsAimLocallyEligible() const;

  virtual bool IsAimEligible() const;

  virtual bool IsPdfUploadEligible() const;

  virtual bool IsDeepSearchEligible() const;

  virtual bool IsCreateImagesEligible() const;

  virtual bool IsCanvasEligible() const;

  virtual bool IsCobrowseServerEligible() const;

  virtual bool IsCobrowseEligible() const;

  virtual bool IsFuseboxEligible() const;

  virtual bool IsCsbEligible() const;

  virtual bool IsAimUrl(
      const GURL& url,
      std::optional<contextual_tasks::HostOverride> host_override) const;

  virtual bool IsAimHost(
      const GURL& url,
      std::optional<contextual_tasks::HostOverride> host_override) const;

  virtual bool HasAimUrlParams(const GURL& url) const;

  virtual bool HasNoCobrowseParams(const GURL& url) const;

  virtual const omnibox::AimEligibilityResponse& GetMostRecentResponse() const;

  EligibilityResponseSource GetMostRecentResponseSource() const;

  enum class AuthenticationMethod {
    kNone = 0,
    kOauth = 1,
    kCookie = 2,
  };

  AuthenticationMethod GetMostRecentResponseAuthMethod() const;

  virtual const omnibox::SearchboxConfig* GetSearchboxConfig() const;

  virtual const omnibox::LhsRailConfig* GetLhsRailConfig() const;

  void StartServerEligibilityRequestForDebugging();
  bool SetEligibilityResponseForDebugging(
      const std::string& base64_encoded_response);

  enum class RequestSource {
    kStartup = 0,
    kCookieChange = 1,
    kPrimaryAccountChange = 2,
    kNetworkChange = 3,
    kUser = 4,
    kAimUrlNavigation = 5,
    kRefreshTokenUpdated = 6,
    kRefreshTokenRemoved = 7,
    kRefreshTokenError = 8,
    kOAuthFallbackCookieChange = 9,
    kMaxValue = kOAuthFallbackCookieChange,
  };

  virtual void FetchEligibility(RequestSource source);

 protected:
  std::string GetLocale() const;

  virtual std::string GetLocaleImpl() const = 0;

  virtual variations::VariationsService* GetVariationsService() const = 0;

 private:
  const omnibox::AimEligibilityResponse empty_response_;
};

#endif  // COMPONENTS_OMNIBOX_BROWSER_AIM_ELIGIBILITY_SERVICE_H_
