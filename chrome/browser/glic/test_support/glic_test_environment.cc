// Copyright 2025 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/glic/test_support/glic_test_environment.h"

#include "base/command_line.h"
#include "base/no_destructor.h"
#include "base/path_service.h"
#include "chrome/browser/glic/host/auth_controller.h"
#include "chrome/browser/glic/host/glic_cookie_synchronizer.h"
#include "chrome/browser/glic/public/features.h"
#include "chrome/browser/glic/public/glic_enabling.h"
#include "chrome/browser/glic/public/glic_keyed_service.h"
#include "chrome/browser/glic/public/glic_keyed_service_factory.h"
#include "chrome/browser/glic/test_support/glic_test_util.h"
#include "chrome/browser/signin/identity_manager_factory.h"
#include "chrome/browser/signin/identity_test_environment_profile_adaptor.h"
#include "chrome/common/chrome_features.h"
#include "chrome/common/chrome_switches.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "components/signin/public/identity_manager/account_info.h"
#include "components/signin/public/identity_manager/identity_test_utils.h"
#include "components/skills/features.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "url/url_util.h"



namespace {

void ServeGlicFiles(net::test_server::EmbeddedTestServer* server,
                    std::string_view server_name) {
  if (!server) {
    return;
  }
  if (server->StartedAcceptingConnection()) {
    LOG(WARNING) << server_name
                 << " was already started; unable to serve Glic test files.";
    return;
  }
  // On Desktop, tests serve test files directly from the repository source
  // directory. However, on Android automated testing bots and devices, the
  // host repository checkout path does not exist on the device filesystem;
  // test files are packaged under DIR_ASSETS/chrome/test/data/webui/glic/.
  // Both gen/ and source asset paths must be served to prevent 404 errors
  // when loading scripts and initializing Mojo connection in test
  // environments.
  server->ServeFilesFromDirectory(
      base::PathService::CheckedGet(base::DIR_ASSETS)
          .AppendASCII("gen/chrome/test/data/webui/glic/"));
  server->ServeFilesFromSourceDirectory("chrome/test/data/webui/glic/");
}

}  // namespace

namespace glic {

namespace internal {
GlicTestEnvironmentConfig* g_current_config = nullptr;

// A fake GlicCookieSynchronizer.
class TestCookieSynchronizer : public glic::GlicCookieSynchronizer {
 public:
  static TestCookieSynchronizer* InjectForProfile(Profile* profile) {
    GlicKeyedService* service =
        GlicKeyedServiceFactory::GetGlicKeyedService(profile, true);
    auto* auth_controller = service->GetAuthController();
    if (!auth_controller) {
      return nullptr;
    }
    auto cookie_synchronizer = std::make_unique<TestCookieSynchronizer>(
        profile, IdentityManagerFactory::GetForProfile(profile));
    TestCookieSynchronizer* ptr = cookie_synchronizer.get();
    auth_controller->SetCookieSynchronizerForTesting(
        std::move(cookie_synchronizer));
    return ptr;
  }

  using GlicCookieSynchronizer::GlicCookieSynchronizer;

  void CopyCookiesToWebviewStoragePartition(
      base::OnceCallback<void(bool)> callback) override {
    base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
        FROM_HERE, base::BindOnce(std::move(callback), copy_cookies_result_));
  }

  void set_copy_cookies_result(bool result) { copy_cookies_result_ = result; }
  base::WeakPtr<TestCookieSynchronizer> GetWeakPtr() {
    return weak_ptr_factory_.GetWeakPtr();
  }

 private:
  bool copy_cookies_result_ = true;

  base::WeakPtrFactory<TestCookieSynchronizer> weak_ptr_factory_{this};
};

class GlicTestEnvironmentServiceFactory : public ProfileKeyedServiceFactory {
 public:
  static GlicTestEnvironmentService* GetForProfile(Profile* profile,
                                                   bool create) {
    return static_cast<GlicTestEnvironmentService*>(
        GetInstance()->GetServiceForBrowserContext(profile, create));
  }
  static GlicTestEnvironmentServiceFactory* GetInstance() {
    static base::NoDestructor<GlicTestEnvironmentServiceFactory> instance;
    return instance.get();
  }

  // BrowserContextKeyedServiceFactory:
  std::unique_ptr<KeyedService> BuildServiceInstanceForBrowserContext(
      content::BrowserContext* context) const override {
    return std::make_unique<GlicTestEnvironmentService>(
        Profile::FromBrowserContext(context));
  }

 private:
  friend class base::NoDestructor<GlicTestEnvironmentServiceFactory>;

  GlicTestEnvironmentServiceFactory()
      : ProfileKeyedServiceFactory(
            "GlicTestEnvironmentService",
            ProfileSelections::BuildForRegularProfile()) {
    DependsOn(IdentityManagerFactory::GetInstance());
    // It would be sensible to depend on GlicKeyedServiceFactory, but that ends
    // up creating some service factories too early.
  }
  ~GlicTestEnvironmentServiceFactory() override = default;
};

}  // namespace internal

std::vector<base::test::FeatureRef> GetDefaultEnabledGlicTestFeatures() {
  return {features::kGlic, features::kGlicRollout,
          // Live mode is disabled by default on Linux, but we still want to
          // test it.
          features::kGlicLiveMode,
          // The anchor entry point is disabled by default globally, but we want
          // all glic tests to run with the intended future behavior. Explicit
          // enabling is required here because some tests instantiate their own
          // ScopedFeatureList which can clobber the field trial testing config.
          features::kGlicAnchorEntryPointForOnboardedUsers,
  };
}
std::vector<base::test::FeatureRef> GetDefaultDisabledGlicTestFeatures() {
  return {features::kGlicWarming, features::kGlicCountryFiltering,
          features::kGlicLocaleFiltering, features::kSkillsServiceApi};
}

GlicTestEnvironment::GlicTestEnvironment(
    const GlicTestEnvironmentConfig& config,
    std::vector<base::test::FeatureRef> enabled_features,
    std::vector<base::test::FeatureRef> disabled_features)
    : config_(config),
      current_config_resetter_(&internal::g_current_config, &config_),
      shared_(enabled_features, disabled_features) {
  // The service factory needs to be created before any services are created.
  internal::GlicTestEnvironmentServiceFactory::GetInstance();
  create_services_subscription_ =
      BrowserContextDependencyManager::GetInstance()
          ->RegisterCreateServicesCallbackForTesting(base::BindRepeating(
              &GlicTestEnvironment::OnWillCreateBrowserContextKeyedServices,
              base::Unretained(this)));

}

GlicTestEnvironment::~GlicTestEnvironment() = default;

void GlicTestEnvironment::SetForceSigninAndModelExecutionCapability(
    bool force) {
  config_.force_signin_and_glic_capability = force;
}

void GlicTestEnvironment::SetFreStatusForNewProfiles(
    std::optional<prefs::FreStatus> fre_status) {
  config_.fre_status = fre_status;
}

GlicTestEnvironmentService* GlicTestEnvironment::GetService(Profile* profile,
                                                            bool create) {
  return internal::GlicTestEnvironmentServiceFactory::GetForProfile(profile,
                                                                    create);
}

void GlicTestEnvironmentService::SetModelExecutionCapability(bool enabled) {
  ::glic::SetGlicCapability(profile_, enabled);
}

void GlicTestEnvironment::OnWillCreateBrowserContextKeyedServices(
    content::BrowserContext* context) {
  Profile* profile = Profile::FromBrowserContext(context);
  if (!profile || !GlicEnabling::IsProfileEligible(profile)) {
    LOG(WARNING) << "Not creating GlicTestEnvironmentService for "
                    "ineligible profile.";
    return;
  }
  if (config_.force_signin_and_glic_capability) {
    IdentityTestEnvironmentProfileAdaptor::
        SetIdentityTestEnvironmentFactoriesOnBrowserContext(context);

    adaptor_ = std::make_unique<IdentityTestEnvironmentProfileAdaptor>(profile);
  }

  auto observation =
      std::make_unique<base::ScopedObservation<Profile, ProfileObserver>>(this);
  observation->Observe(profile);
  profile_observations_.push_back(std::move(observation));
}

void GlicTestEnvironment::OnProfileWillBeDestroyed(Profile* profile) {
  std::erase_if(profile_observations_, [&profile](const auto& observation) {
    return observation->GetSource() == profile;
  });
}

void GlicTestEnvironment::OnProfileInitializationComplete(Profile* profile) {
  GetService(profile, true);
}

bool GlicTestEnvironment::StartTestServerIfNeeded(
    net::test_server::EmbeddedTestServer* server,
    net::test_server::EmbeddedTestServerHandle& handle) {
  if (server->StartedAcceptingConnection()) {
    return true;
  }

  handle = server->Started()
               ? server->StartAcceptingConnectionsAndReturnHandle()
               : server->StartAndReturnHandle();
  return static_cast<bool>(handle);
}

bool GlicTestEnvironment::InitializeEmbeddedTestServers(
    net::test_server::EmbeddedTestServer* https_server) {
  CHECK(guest_url_.is_empty()) << "InitializeEmbeddedTestServers called twice";
  CHECK(https_server);

  // Initialize the listen socket so the port is known and `GetURL()` can be
  // queried in `SetUpCommandLine()`, without starting the IO thread before the
  // sandbox is initialized.
  if (!https_server->Started() && !https_server->InitializeAndListen()) {
    return false;
  }

  std::ostringstream path;
  path << glic_page_path_;

  // Append the query parameters to the URL.
  bool first_param = true;
  for (const auto& [key, value] : mock_glic_query_params_) {
    path << (first_param ? "?" : "&");
    first_param = false;
    path << url::EncodeUriComponent(key);
    if (!value.empty()) {
      path << "=" << url::EncodeUriComponent(value);
    }
  }

  guest_url_ = https_server->GetURL(path.str());
  auto* command_line = base::CommandLine::ForCurrentProcess();
  command_line->AppendSwitchASCII(::switches::kGlicGuestURL, guest_url_.spec());

  return true;
}

bool GlicTestEnvironment::SetupEmbeddedTestServers(
    net::test_server::EmbeddedTestServer* http_server,
    net::test_server::EmbeddedTestServer* https_server) {
  CHECK(https_server);

  if (guest_url_.is_empty() && !InitializeEmbeddedTestServers(https_server)) {
    return false;
  }

  if (http_server) {
    ServeGlicFiles(http_server, "HTTP test server");
    if (!StartTestServerIfNeeded(http_server, test_server_handle_)) {
      return false;
    }
  }
  ServeGlicFiles(https_server, "HTTPS test server");

  if (!StartTestServerIfNeeded(https_server, https_test_server_handle_)) {
    return false;
  }

  return true;
}

void GlicTestEnvironment::SetGlicPagePath(const std::string& path) {
  CHECK(guest_url_.is_empty())
      << "SetGlicPagePath must be called before InitializeEmbeddedTestServers";
  glic_page_path_ = path;
}

void GlicTestEnvironment::AddMockGlicQueryParam(const std::string_view& key,
                                                const std::string_view& value) {
  CHECK(guest_url_.is_empty()) << "AddMockGlicQueryParam must be called before "
                                  "InitializeEmbeddedTestServers";
  mock_glic_query_params_.emplace(key, value);
}

GURL GlicTestEnvironment::GetGuestURL() const {
  CHECK(guest_url_.is_valid()) << "Guest URL not yet configured.";
  return guest_url_;
}

GlicTestEnvironmentService::GlicTestEnvironmentService(Profile* profile)
    : profile_(profile) {
  internal::TestCookieSynchronizer* cookie_synchronizer =
      internal::TestCookieSynchronizer::InjectForProfile(profile);

  if (cookie_synchronizer) {
    cookie_synchronizer_ = cookie_synchronizer->GetWeakPtr();
  }
  const GlicTestEnvironmentConfig default_config;
  const GlicTestEnvironmentConfig& config =
      internal::g_current_config ? *internal::g_current_config : default_config;
  if (config.fre_status) {
    SetFRECompletion(*config.fre_status);
  }
  if (config.override_cookie_sync_result.has_value()) {
    SetResultForFutureCookieSync(*config.override_cookie_sync_result);
  }
  if (config.force_signin_and_glic_capability) {
    auto* identity_manager = IdentityManagerFactory::GetForProfile(profile);
    if (!identity_manager->HasPrimaryAccount(signin::ConsentLevel::kSignin)) {
      AccountInfo::Builder builder(signin::MakePrimaryAccountAvailable(
          identity_manager, "glic-test@example.com",
          signin::ConsentLevel::kSignin));
      builder.SetFullName("Glic Testing").SetGivenName("Glic");
      if (!config.default_account_hosted_domain.empty()) {
        builder.SetHostedDomain(config.default_account_hosted_domain);
      }
      signin::UpdateAccountInfoForAccount(identity_manager, builder.Build());
    }
    SetModelExecutionCapability(true);
  }
}

GlicTestEnvironmentService::~GlicTestEnvironmentService() = default;

GlicKeyedService* GlicTestEnvironmentService::GetService() {
  return GlicKeyedServiceFactory::GetGlicKeyedService(profile_);
}

void GlicTestEnvironmentService::SetResultForFutureCookieSync(bool result) {
  if (cookie_synchronizer_) {
    cookie_synchronizer_->set_copy_cookies_result(result);
  }
}

void GlicTestEnvironmentService::SetFRECompletion(prefs::FreStatus fre_status) {
  ::glic::SetFRECompletion(profile_, fre_status);
}

GlicUnitTestEnvironment::GlicUnitTestEnvironment(
    const GlicTestEnvironmentConfig& config)
    : config_(config),
      shared_(GetDefaultEnabledGlicTestFeatures(),
              GetDefaultDisabledGlicTestFeatures()) {}

GlicUnitTestEnvironment::~GlicUnitTestEnvironment() = default;

void GlicUnitTestEnvironment::SetupProfile(Profile* profile) {
  if (config_.force_signin_and_glic_capability) {
    ::glic::ForceSigninAndGlicCapability(profile);
  }
  if (config_.fre_status) {
    glic::SetFRECompletion(profile, *config_.fre_status);
  }
}

internal::GlicTestEnvironmentShared::GlicTestEnvironmentShared(
    std::vector<base::test::FeatureRef> enabled_features,
    std::vector<base::test::FeatureRef> disabled_features) {
  scoped_feature_list_.InitWithFeatures(enabled_features, disabled_features);

  // The enabled list above includes chromeos::features::kFeatureManagementGlic
  // so that the system requirement is met, but
  // GlicGlobalEnabling::IsSystemRequirementMet() reads it into a
  // function-local static. That static is settled by whichever test in the
  // process asks first, so without pinning it here the features set up above
  // may have no effect.
  GlicEnabling::SetSystemRequirementMetForTesting(true);
}

internal::GlicTestEnvironmentShared::~GlicTestEnvironmentShared() {
  GlicEnabling::SetSystemRequirementMetForTesting(std::nullopt);
}

}  // namespace glic
