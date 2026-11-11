// Copyright 2015 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/sync/sync_service_factory.h"

#include "base/command_line.h"
#include "base/feature_list.h"
#include "base/task/thread_pool/thread_pool_instance.h"
#include "base/test/bind.h"
#include "base/test/scoped_feature_list.h"
#include "build/build_config.h"
#include "chrome/browser/favicon/favicon_service_factory.h"
#include "chrome/browser/history/history_service_factory.h"
#include "chrome/browser/search_engines/template_url_service_factory.h"
#include "chrome/browser/sync/cross_device_theme_tracker_factory.h"
#include "chrome/browser/sync/sync_service_factory.h"
#include "chrome/browser/trusted_vault/trusted_vault_service_factory.h"
#include "chrome/browser/ui/ui_features.h"
#include "chrome/browser/webdata_services/web_data_service_factory.h"
#include "chrome/common/buildflags.h"
#include "chrome/test/base/testing_profile.h"
#include "components/data_sharing/public/features.h"
#include "components/saved_tab_groups/public/features.h"
#include "components/skills/features.h"
#include "components/spellcheck/spellcheck_buildflags.h"
#include "components/sync/base/command_line_switches.h"
#include "components/sync/base/data_type.h"
#include "components/sync/base/features.h"
#include "components/sync/service/sync_service_impl.h"
#include "components/themes/cross_device/cross_device_theme_tracker.h"
#include "components/themes/cross_device/features.h"
#include "content/public/test/browser_task_environment.h"
#include "extensions/buildflags/buildflags.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/webui/buildflags.h"

#if BUILDFLAG(ENABLE_SPELLCHECK)
#include "chrome/browser/spellchecker/spellcheck_factory.h"
#include "chrome/browser/spellchecker/spellcheck_service.h"
#endif  // BUILDFLAG(ENABLE_SPELLCHECK)


class SyncServiceFactoryTest : public testing::Test {
 public:
  void SetUp() override {
    TestingProfile::Builder builder;
    builder.AddTestingFactory(FaviconServiceFactory::GetInstance(),
                              FaviconServiceFactory::GetDefaultFactory());
    builder.AddTestingFactory(HistoryServiceFactory::GetInstance(),
                              HistoryServiceFactory::GetDefaultFactory());
    builder.AddTestingFactory(SyncServiceFactory::GetInstance(),
                              SyncServiceFactory::GetDefaultFactory());
    builder.AddTestingFactory(
        TemplateURLServiceFactory::GetInstance(),
        base::BindRepeating(&TemplateURLServiceFactory::BuildInstanceFor));
    builder.AddTestingFactory(TrustedVaultServiceFactory::GetInstance(),
                              TrustedVaultServiceFactory::GetDefaultFactory());
    // Some services will only be created if there is a WebDataService.
    builder.AddTestingFactory(WebDataServiceFactory::GetInstance(),
                              WebDataServiceFactory::GetDefaultFactory());

#if BUILDFLAG(ENABLE_SPELLCHECK)
    builder.AddTestingFactory(
        SpellcheckServiceFactory::GetInstance(),
        base::BindRepeating(base::BindLambdaForTesting(
            [](content::BrowserContext* browser_context) {
              return std::unique_ptr<KeyedService>(
                  std::make_unique<SpellcheckService>(browser_context));
            })));
#endif  // BUILDFLAG(ENABLE_SPELLCHECK)

    profile_ = builder.Build();
  }

  void TearDown() override {
    // There may tasks in flight referencing fields owned by the test fixture.
    // Make sure they are flushed now to prevent memory safety errors, e.g.
    // use-after-destruction errors.

    // Flush any pending initialization tasks (e.g. WebData) before destroying
    // the profile, to avoid accessing closed DBs.
    task_environment_.RunUntilIdle();

    // Destroy the profile. This may post cleanup tasks.
    profile_.reset();

    // Flush cleanup tasks while NetworkHandler is still alive.
    task_environment_.RunUntilIdle();
  }

 protected:
  SyncServiceFactoryTest() = default;
  ~SyncServiceFactoryTest() override = default;

  // Returns the collection of default datatypes.
  syncer::DataTypeSet DefaultDatatypes() {
    static_assert(64 == syncer::GetNumDataTypes(),
                  "When adding a new type, you probably want to add it here as "
                  "well (assuming it is already enabled). Check similar "
                  "function in "
                  "ios/c/b/sync/model/sync_service_factory_unittest.cc");

    syncer::DataTypeSet datatypes;

    // These preprocessor conditions and their order should be in sync with
    // preprocessor conditions in ChromeSyncClient::CreateDataTypeControllers:

    // ChromeSyncClient types.
    datatypes.Put(syncer::READING_LIST);
    datatypes.Put(syncer::SECURITY_EVENTS);
    datatypes.Put(syncer::SUPERVISED_USER_SETTINGS);

#if BUILDFLAG(ENABLE_EXTENSIONS_CORE)
    datatypes.Put(syncer::EXTENSIONS);
    datatypes.Put(syncer::EXTENSION_SETTINGS);
#endif  // BUILDFLAG(ENABLE_EXTENSIONS_CORE)

#if BUILDFLAG(ENABLE_EXTENSIONS)
    datatypes.Put(syncer::WEB_APPS);
    if (!base::FeatureList::IsEnabled(
            syncer::kSyncDoNotSyncAppsAndAppSettings)) {
      datatypes.Put(syncer::APPS);
      datatypes.Put(syncer::APP_SETTINGS);
    }
#endif  // BUILDFLAG(ENABLE_EXTENSIONS)

    datatypes.Put(syncer::THEMES);

    datatypes.Put(syncer::SEARCH_ENGINES);

    datatypes.Put(syncer::SAVED_TAB_GROUP);

#if BUILDFLAG(IS_LINUX) || BUILDFLAG(IS_WIN)
    datatypes.Put(syncer::DICTIONARY);
#endif


    // Common types. This excludes PASSWORDS,
    // INCOMING_PASSWORD_SHARING_INVITATION and
    // INCOMING_PASSWORD_SHARING_INVITATION, because the password store factory
    // is null for testing and hence no controller gets instantiated for those
    // types.
    datatypes.Put(syncer::AUTOFILL);
    datatypes.Put(syncer::AUTOFILL_PROFILE);
    datatypes.Put(syncer::AUTOFILL_WALLET_CREDENTIAL);
    datatypes.Put(syncer::AUTOFILL_WALLET_DATA);
    datatypes.Put(syncer::AUTOFILL_WALLET_METADATA);
    datatypes.Put(syncer::AUTOFILL_WALLET_USAGE);
    datatypes.Put(syncer::BOOKMARKS);
    datatypes.Put(syncer::CONTACT_INFO);
    datatypes.Put(syncer::DEVICE_INFO);
    datatypes.Put(syncer::HISTORY);
    datatypes.Put(syncer::HISTORY_DELETE_DIRECTIVES);
    datatypes.Put(syncer::PREFERENCES);
    datatypes.Put(syncer::PRIORITY_PREFERENCES);
    datatypes.Put(syncer::SESSIONS);
    datatypes.Put(syncer::USER_EVENTS);
    datatypes.Put(syncer::USER_CONSENTS);
    datatypes.Put(syncer::SEND_TAB_TO_SELF);
    datatypes.Put(syncer::SHARING_MESSAGE);
    datatypes.Put(syncer::WEBAUTHN_CREDENTIAL);
    if (base::FeatureList::IsEnabled(
            data_sharing::features::kDataSharingFeature)) {
      datatypes.Put(syncer::COLLABORATION_GROUP);
      datatypes.Put(syncer::SHARED_TAB_GROUP_DATA);
      datatypes.Put(syncer::SHARED_TAB_GROUP_ACCOUNT_DATA);
      if (base::FeatureList::IsEnabled(syncer::kSyncSharedComment)) {
        datatypes.Put(syncer::SHARED_COMMENT);
      }
    }

    datatypes.Put(syncer::AUTOFILL_VALUABLE);

    if (base::FeatureList::IsEnabled(syncer::kSyncAutofillValuableMetadata)) {
      datatypes.Put(syncer::AUTOFILL_VALUABLE_METADATA);
    }

    if (base::FeatureList::IsEnabled(syncer::kSyncAccountSettings)) {
      datatypes.Put(syncer::ACCOUNT_SETTING);
    }

    if (base::FeatureList::IsEnabled(syncer::kSyncAIThread)) {
      datatypes.Put(syncer::AI_THREAD);
    }

    if (base::FeatureList::IsEnabled(syncer::kSyncContextualTask)) {
      datatypes.Put(syncer::CONTEXTUAL_TASK);
    }

    if (base::FeatureList::IsEnabled(features::kSkillsEnabled)) {
      datatypes.Put(syncer::SKILL);
    }

    if (base::FeatureList::IsEnabled(syncer::kSyncGeminiThread)) {
      datatypes.Put(syncer::GEMINI_THREAD);
    }

    if (base::FeatureList::IsEnabled(syncer::kSyncEncryptedTabContext)) {
      datatypes.Put(syncer::ENCRYPTED_TAB_CONTEXT_CONTAINER);
      datatypes.Put(syncer::ENCRYPTED_TAB_CONTEXT_ITEM);
    }

    if (base::FeatureList::IsEnabled(themes::kCrossDeviceThemeTracker)) {
      datatypes.Put(syncer::THEMES_IOS);
    }

    if (base::FeatureList::IsEnabled(themes::kCrossDeviceThemeTracker)) {
      datatypes.Put(syncer::THEMES_ANDROID);
    }

    if (base::FeatureList::IsEnabled(syncer::kSyncNotebook)) {
      datatypes.Put(syncer::NOTEBOOK);
    }

    if (base::FeatureList::IsEnabled(syncer::kSyncJourney)) {
      datatypes.Put(syncer::JOURNEY);
    }

    if (base::FeatureList::IsEnabled(syncer::kSyncAutofillEntitySuppression)) {
      datatypes.Put(syncer::AUTOFILL_ENTITY_SUPPRESSION);
    }

    return datatypes;
  }

  Profile* profile() { return profile_.get(); }

 private:
  content::BrowserTaskEnvironment task_environment_;
  std::unique_ptr<TestingProfile> profile_;

};

// Test fixture for testing the kDisableSync flag.
class SyncServiceFactoryTestDisableSync : public SyncServiceFactoryTest {
 public:
  void SetUp() override {
    // Append the switch *before* the profile is built.
    base::CommandLine::ForCurrentProcess()->AppendSwitch(syncer::kDisableSync);
    SyncServiceFactoryTest::SetUp();
  }
};

// Verify that the disable sync flag disables creation of the sync service.
TEST_F(SyncServiceFactoryTestDisableSync, DisableSyncFlag) {
  EXPECT_EQ(nullptr, SyncServiceFactory::GetForProfile(profile()));
}

// Verify that a normal (no command line flags) SyncServiceImpl can be created
// and properly initialized.
TEST_F(SyncServiceFactoryTest, CreateSyncServiceImplDefault) {
  syncer::SyncServiceImpl* sync_service =
      SyncServiceFactory::GetAsSyncServiceImplForProfileForTesting(profile());
  syncer::DataTypeSet types = sync_service->GetRegisteredDataTypesForTest();
  const syncer::DataTypeSet default_types = DefaultDatatypes();
  EXPECT_EQ(default_types.size(), types.size());
  for (syncer::DataType type : default_types) {
    EXPECT_TRUE(types.Has(type))
        << syncer::DataTypeToDebugString(type) << " not found in datatypes map";
  }
}

class SyncServiceFactoryWithNtpThemeCustomizationSyncTest
    : public SyncServiceFactoryTest {
 public:
  SyncServiceFactoryWithNtpThemeCustomizationSyncTest() {
    feature_list_.InitWithFeatures(
        /*enabled_features=*/{syncer::kNewTabPageCustomizationThemeSync},
        /*disabled_features=*/{themes::kCrossDeviceThemeTracker});
  }

 private:
  base::test::ScopedFeatureList feature_list_;
};

// Verifies that enabling kNewTabPageCustomizationThemeSync alone only activates
// continuous 2-way sync for Android's own NTP customizations (THEMES_ANDROID on
// Android via NtpAndroidCustomBackgroundService) and does not activate
// CrossDeviceThemeTracker controllers.
TEST_F(SyncServiceFactoryWithNtpThemeCustomizationSyncTest,
       ContinuousNtpThemeSyncOnly) {
  syncer::SyncServiceImpl* sync_service =
      SyncServiceFactory::GetAsSyncServiceImplForProfileForTesting(profile());
  syncer::DataTypeSet types = sync_service->GetRegisteredDataTypesForTest();

  // On Desktop, THEMES is registered natively via ThemeService.
  EXPECT_TRUE(types.Has(syncer::THEMES));
  // CrossDeviceThemeTracker controllers (THEMES_ANDROID, THEMES_IOS) are
  // disabled.
  EXPECT_FALSE(types.Has(syncer::THEMES_ANDROID));
  EXPECT_FALSE(types.Has(syncer::THEMES_IOS));

  EXPECT_EQ(nullptr, CrossDeviceThemeTrackerFactory::GetForProfile(profile()));
}

class SyncServiceFactoryWithCrossDeviceThemeTrackerFlagTest
    : public SyncServiceFactoryTest {
 public:
  SyncServiceFactoryWithCrossDeviceThemeTrackerFlagTest() {
    feature_list_.InitWithFeatures(
        /*enabled_features=*/{themes::kCrossDeviceThemeTracker},
        /*disabled_features=*/{syncer::kNewTabPageCustomizationThemeSync});
  }

 private:
  base::test::ScopedFeatureList feature_list_;
};

// Verifies that enabling kCrossDeviceThemeTracker alone activates remote theme
// controllers across platforms for Synced Set Up without activating continuous
// 2-way NTP background sync on Android.
TEST_F(SyncServiceFactoryWithCrossDeviceThemeTrackerFlagTest,
       RegistersCrossDeviceThemeDataTypes) {
  syncer::SyncServiceImpl* sync_service =
      SyncServiceFactory::GetAsSyncServiceImplForProfileForTesting(profile());
  syncer::DataTypeSet types = sync_service->GetRegisteredDataTypesForTest();

  EXPECT_TRUE(types.Has(syncer::THEMES_IOS));
  EXPECT_TRUE(types.Has(syncer::THEMES));

  // On Desktop, CrossDeviceThemeTracker registers THEMES_ANDROID.
  EXPECT_TRUE(types.Has(syncer::THEMES_ANDROID));

  auto* tracker = CrossDeviceThemeTrackerFactory::GetForProfile(profile());
  ASSERT_NE(nullptr, tracker);
  EXPECT_EQ(themes::ServiceStatus::kInitializing, tracker->GetServiceStatus());
}

class SyncServiceFactoryWithBothThemeFeaturesTest
    : public SyncServiceFactoryTest {
 public:
  SyncServiceFactoryWithBothThemeFeaturesTest() {
    feature_list_.InitWithFeatures(
        /*enabled_features=*/{themes::kCrossDeviceThemeTracker,
                              syncer::kNewTabPageCustomizationThemeSync},
        /*disabled_features=*/{});
  }

 private:
  base::test::ScopedFeatureList feature_list_;
};

// Verifies that enabling both flags activates all continuous and cross-device
// theme controllers.
TEST_F(SyncServiceFactoryWithBothThemeFeaturesTest,
       RegistersAllThemeDataTypes) {
  syncer::SyncServiceImpl* sync_service =
      SyncServiceFactory::GetAsSyncServiceImplForProfileForTesting(profile());
  syncer::DataTypeSet types = sync_service->GetRegisteredDataTypesForTest();

  EXPECT_TRUE(types.Has(syncer::THEMES_IOS));
  EXPECT_TRUE(types.Has(syncer::THEMES));
  EXPECT_TRUE(types.Has(syncer::THEMES_ANDROID));

  auto* tracker = CrossDeviceThemeTrackerFactory::GetForProfile(profile());
  ASSERT_NE(nullptr, tracker);
  EXPECT_EQ(themes::ServiceStatus::kInitializing, tracker->GetServiceStatus());
}

#if BUILDFLAG(ENABLE_EXTENSIONS)
class SyncServiceFactoryTestWithoutAppsAndAppSettings
    : public SyncServiceFactoryTest {
 public:
  SyncServiceFactoryTestWithoutAppsAndAppSettings() {
    feature_list_.InitAndEnableFeature(
        syncer::kSyncDoNotSyncAppsAndAppSettings);
  }

 private:
  base::test::ScopedFeatureList feature_list_;
};

TEST_F(SyncServiceFactoryTestWithoutAppsAndAppSettings,
       AppsAndAppSettingsNotRegistered) {
  syncer::SyncServiceImpl* sync_service =
      SyncServiceFactory::GetAsSyncServiceImplForProfileForTesting(profile());
  syncer::DataTypeSet types = sync_service->GetRegisteredDataTypesForTest();

  EXPECT_FALSE(types.Has(syncer::APPS));
  EXPECT_FALSE(types.Has(syncer::APP_SETTINGS));

  // WEB_APPS should still be registered.
  EXPECT_TRUE(types.Has(syncer::WEB_APPS));
}
#endif
