// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <memory>
#include <optional>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "base/scoped_observation.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/test_future.h"
#include "base/uuid.h"
#include "chrome/browser/sync/data_type_store_service_factory.h"
#include "chrome/browser/sync/test/integration/fake_server_match_status_checker.h"
#include "chrome/browser/sync/test/integration/single_client_status_change_checker.h"
#include "chrome/browser/sync/test/integration/status_change_checker.h"
#include "chrome/browser/sync/test/integration/sync_service_impl_harness.h"
#include "chrome/browser/sync/test/integration/sync_test.h"
#include "components/prefs/pref_service.h"
#include "components/skills/features.h"
#include "components/skills/public/skill.h"
#include "components/skills/public/skills_prefs.h"
#include "components/skills/public/skills_service.h"
#include "components/sync/base/data_type.h"
#include "components/sync/engine/loopback_server/persistent_tombstone_entity.h"
#include "components/sync/engine/loopback_server/persistent_unique_client_entity.h"
#include "components/sync/model/data_type_store.h"
#include "components/sync/model/data_type_store_service.h"
#include "components/sync/model/metadata_batch.h"
#include "components/sync/model/metadata_change_list.h"
#include "components/sync/model/model_error.h"
#include "components/sync/protocol/entity_metadata.pb.h"
#include "components/sync/protocol/entity_specifics.pb.h"
#include "components/sync/protocol/skill_specifics.pb.h"
#include "components/sync/service/sync_service_impl.h"
#include "components/sync/test/unknown_field_util.h"
#include "content/public/test/browser_test.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

class SkillSyncActiveChecker : public SingleClientStatusChangeChecker {
 public:
  explicit SkillSyncActiveChecker(syncer::SyncServiceImpl* service)
      : SingleClientStatusChangeChecker(service) {}

  bool IsExitConditionSatisfied(std::ostream* os) override {
    *os << "Waiting for SKILL to become active.";
    return service()->GetActiveDataTypes().Has(syncer::SKILL);
  }
};

class SkillSyncInactiveChecker : public SingleClientStatusChangeChecker {
 public:
  explicit SkillSyncInactiveChecker(syncer::SyncServiceImpl* service)
      : SingleClientStatusChangeChecker(service) {}

  bool IsExitConditionSatisfied(std::ostream* os) override {
    *os << "Waiting for SKILL to become inactive.";
    return !service()->GetActiveDataTypes().Has(syncer::SKILL);
  }
};

using syncer::test::HasUnknownField;
using testing::AllOf;
using testing::Contains;
using testing::IsEmpty;
using testing::Pointee;
using testing::SizeIs;
using testing::UnorderedElementsAre;

MATCHER_P(HasSimpleSkill, matcher, "") {
  return arg.has_simple_skill() &&
         testing::ExplainMatchResult(matcher, arg.simple_skill(),
                                     result_listener);
}

MATCHER_P5(HasSkill, source_skill_id, name, icon, prompt, description, "") {
  return arg.source_skill_id == source_skill_id && arg.name == name &&
         arg.icon == icon && arg.prompt == prompt &&
         arg.description == description;
}

MATCHER_P6(HasSkillSpecifics,
           guid,
           source_skill_id,
           name,
           icon,
           prompt,
           description,
           "") {
  return arg.guid() == guid && arg.source_skill_id() == source_skill_id &&
         arg.name() == name && arg.icon() == icon &&
         arg.simple_skill().prompt() == prompt &&
         arg.simple_skill().description() == description;
}

sync_pb::SkillSpecifics CreateSkillSpecifics(std::string guid,
                                             std::string source_skill_id,
                                             std::string name,
                                             std::string icon,
                                             std::string prompt,
                                             std::string description) {
  sync_pb::SkillSpecifics specifics;
  specifics.set_guid(std::move(guid));
  specifics.set_source_skill_id(std::move(source_skill_id));
  specifics.set_name(std::move(name));
  specifics.set_icon(std::move(icon));
  specifics.mutable_simple_skill()->set_prompt(std::move(prompt));
  specifics.mutable_simple_skill()->set_description(std::move(description));
  specifics.set_skill_source(sync_pb::SKILL_SOURCE_USER_CREATED);
  specifics.set_schema_version(1);
  return specifics;
}

std::vector<sync_pb::SkillSpecifics> SyncEntitiesToSkillSpecifics(
    std::vector<sync_pb::SyncEntity> entities) {
  std::vector<sync_pb::SkillSpecifics> skills;
  for (sync_pb::SyncEntity& entity : entities) {
    CHECK(entity.specifics().has_skill());
    sync_pb::SkillSpecifics specifics;
    specifics.Swap(entity.mutable_specifics()->mutable_skill());
    skills.push_back(std::move(specifics));
  }
  return skills;
}

// A checker that waits for the skills in the service on the client to match the
// provided matcher.


// A checker that waits for the skills in the fake server to match the provided
// matcher.
class ServerSkillsMatchChecker
    : public fake_server::FakeServerMatchStatusChecker {
 public:
  using Matcher = testing::Matcher<std::vector<sync_pb::SkillSpecifics>>;

  explicit ServerSkillsMatchChecker(const Matcher& matcher)
      : matcher_(matcher) {}

  // fake_server::FakeServerMatchStatusChecker overrides.
  bool IsExitConditionSatisfied(std::ostream* os) override {
    *os << "Waiting for server skills to match. ";

    testing::StringMatchResultListener result_listener;
    bool matches = testing::ExplainMatchResult(
        matcher_,
        SyncEntitiesToSkillSpecifics(
            fake_server()->GetSyncEntitiesByDataType(syncer::SKILL)),
        &result_listener);
    *os << result_listener.str();
    return matches;
  }

 private:
  const Matcher matcher_;
};

class SingleClientSkillsSyncTest
    : public SyncTest,
      public testing::WithParamInterface<SyncTest::SetupSyncMode> {
 public:
  SingleClientSkillsSyncTest() : SyncTest(SINGLE_CLIENT) {}

  SyncTest::SetupSyncMode GetSetupSyncMode() const override {
    return GetParam();
  }


  void InjectSpecificsToFakeServer(const sync_pb::SkillSpecifics& specifics) {
    sync_pb::EntitySpecifics entity_specifics;
    *entity_specifics.mutable_skill() = specifics;
    fake_server_->InjectEntity(
        syncer::PersistentUniqueClientEntity::CreateFromSpecificsForTesting(
            /*non_unique_name=*/specifics.guid(),
            /*client_tag=*/specifics.guid(), entity_specifics,
            /*creation_time=*/0,
            /*last_modified_time=*/0));
  }

  void InjectTombstoneToFakeServer(const std::string& skill_id) {
    fake_server_->InjectEntity(
        syncer::PersistentTombstoneEntity::CreateNewForTest(syncer::SKILL,
                                                            skill_id));
  }

  void SetPossiblyTrimmedSpecificsInStore(
      syncer::DataType data_type,
      const std::string& storage_key,
      const sync_pb::EntitySpecifics& trimmed_specifics) {
    base::test::TestFuture<const std::optional<syncer::ModelError>&,
                           std::unique_ptr<syncer::DataTypeStore>>
        init_future;
    DataTypeStoreServiceFactory::GetForProfile(GetProfile(0))
        ->GetStoreFactory()
        .Run(data_type, init_future.GetCallback());
    ASSERT_FALSE(init_future.Get<0>().has_value());
    std::unique_ptr<syncer::DataTypeStore> store =
        std::get<1>(init_future.Take());
    ASSERT_NE(store, nullptr);

    base::test::TestFuture<const std::optional<syncer::ModelError>&,
                           std::unique_ptr<syncer::MetadataBatch>>
        read_future;
    store->ReadAllMetadata(read_future.GetCallback());
    ASSERT_FALSE(read_future.Get<0>().has_value());
    std::unique_ptr<syncer::MetadataBatch> metadata_batch =
        std::get<1>(read_future.Take());
    ASSERT_NE(metadata_batch, nullptr);

    syncer::EntityMetadataMap metadata_map = metadata_batch->TakeAllMetadata();
    auto it = metadata_map.find(storage_key);
    ASSERT_NE(it, metadata_map.end());

    sync_pb::EntityMetadata metadata = *it->second;
    *metadata.mutable_possibly_trimmed_base_specifics() = trimmed_specifics;

    std::unique_ptr<syncer::DataTypeStore::WriteBatch> write_batch =
        store->CreateWriteBatch();
    write_batch->GetMetadataChangeList()->UpdateMetadata(storage_key, metadata);
    base::test::TestFuture<const std::optional<syncer::ModelError>&>
        commit_future;
    store->CommitWriteBatch(std::move(write_batch),
                            commit_future.GetCallback());
    ASSERT_FALSE(commit_future.Get().has_value());
  }

  void SetPossiblyTrimmedSkillSpecificsInStore(
      const std::string& storage_key,
      const sync_pb::SkillSpecifics& trimmed_specifics) {
    sync_pb::EntitySpecifics entity_specifics;
    *entity_specifics.mutable_skill() = trimmed_specifics;
    SetPossiblyTrimmedSpecificsInStore(syncer::SKILL, storage_key,
                                       entity_specifics);
  }

 private:
  base::test::ScopedFeatureList feature_overrides_{features::kSkillsEnabled};
};

INSTANTIATE_TEST_SUITE_P(,
                         SingleClientSkillsSyncTest,
                         GetSyncTestModes(),
                         testing::PrintToStringParamName());

IN_PROC_BROWSER_TEST_P(SingleClientSkillsSyncTest, ShouldInitializeDataType) {
  ASSERT_TRUE(SetupSync());
  EXPECT_TRUE(GetSyncService(0)->GetActiveDataTypes().Has(syncer::SKILL));
}

IN_PROC_BROWSER_TEST_P(SingleClientSkillsSyncTest,
                       PRE_ShouldLoadDataOnRestart) {
  ASSERT_TRUE(SetupSync());

}

IN_PROC_BROWSER_TEST_P(SingleClientSkillsSyncTest, ShouldLoadDataOnRestart) {
  ASSERT_TRUE(SetupClients());
  ASSERT_TRUE(GetClient(0)->AwaitSyncTransportActive());

  EXPECT_TRUE(GetSyncService(0)->GetActiveDataTypes().Has(syncer::SKILL));
}

IN_PROC_BROWSER_TEST_P(SingleClientSkillsSyncTest, ShouldApplyRemoteUpdates) {
  ASSERT_TRUE(SetupSync());

  const std::string skill_id_to_add =
      base::Uuid::GenerateRandomV4().AsLowercaseString();

  ASSERT_TRUE(
      ServerSkillsMatchChecker(
          UnorderedElementsAre(
              HasSkillSpecifics(skill1->id, "source_skill_id", "skill1",
                                "icon1", "prompt1",
                                /*description=*/""),
              HasSkillSpecifics(skill_to_update->id, "", "skill2 to update",
                                "icon2", "prompt2", /*description=*/""),
              HasSkillSpecifics(skill_to_delete->id, "", "skill3 to delete",
                                "icon3", "prompt3", /*description=*/"")))
          .Wait());

  InjectSpecificsToFakeServer(CreateSkillSpecifics(
      skill_to_update->id, "", "updated name", "updated icon", "updated prompt",
      "updated description"));
  InjectSpecificsToFakeServer(CreateSkillSpecifics(
      skill_id_to_add, "new_source_skill_id", "new_skill_name",
      "new_skill_icon", "new_skill_prompt", "new_skill_description"));
  InjectTombstoneToFakeServer(skill_to_delete->id);

}

IN_PROC_BROWSER_TEST_P(SingleClientSkillsSyncTest, ShouldMergeRemoteData) {
  InjectSpecificsToFakeServer(CreateSkillSpecifics(
      base::Uuid::GenerateRandomV4().AsLowercaseString(), "source_skill_id1",
      "skill1", "icon1", "prompt1", "description1"));
  InjectSpecificsToFakeServer(CreateSkillSpecifics(
      base::Uuid::GenerateRandomV4().AsLowercaseString(), "source_skill_id2",
      "skill2", "icon2", "prompt2", "description2"));

  ASSERT_TRUE(SetupSync());

}

// TODO(crbug.com/471795213): add a test to verify that skills can't be created
// when sync is disabled.

// Verifies that fields unknown to the current client version are preserved when
// committing local updates back to the server (via sync metadata's
// `possibly_trimmed_base_specifics`).
IN_PROC_BROWSER_TEST_P(SingleClientSkillsSyncTest,
                       ShouldPreserveUnknownFields) {
  const std::string kSkillId =
      base::Uuid::GenerateRandomV4().AsLowercaseString();
  sync_pb::SkillSpecifics specifics_with_unknown_fields = CreateSkillSpecifics(
      kSkillId, "source_skill_id", "name", "icon", "prompt", "description");

  // Add unknown fields that should be preserved by the client.
  syncer::test::AddUnknownFieldToProto(
      *specifics_with_unknown_fields.mutable_simple_skill(),
      "simple_skill_unknown_field");
  syncer::test::AddUnknownFieldToProto(specifics_with_unknown_fields,
                                       "specifics_unknown_field");

  InjectSpecificsToFakeServer(specifics_with_unknown_fields);

  ASSERT_TRUE(SetupSync());

  // Update the skill on the client.
  ASSERT_NE(skill, nullptr);

  // Verify that the server received the update and that unknown fields are
  // preserved.
  EXPECT_TRUE(
      ServerSkillsMatchChecker(
          UnorderedElementsAre(AllOf(
              HasSkillSpecifics(kSkillId, "source_skill_id", "updated name",
                                "updated icon", "updated prompt",
                                /*description=*/""),
              HasUnknownField("specifics_unknown_field"),
              HasSimpleSkill(HasUnknownField("simple_skill_unknown_field")))))
          .Wait());
}

IN_PROC_BROWSER_TEST_P(SingleClientSkillsSyncTest,
                       PRE_ShouldPopulateNewlySupportedFieldsOnBrowserUpgrade) {
  const std::string kSkillId =
      base::Uuid::GenerateRandomV4().AsLowercaseString();
  InjectSpecificsToFakeServer(
      CreateSkillSpecifics(kSkillId, /*source_skill_id=*/"", "name", "icon",
                           "prompt", /*description=*/""));

  ASSERT_TRUE(SetupSync());

  // Simulate that `source_skill_id` and `description` were unknown to this
  // browser version when the entity was received, so they were stored in sync
  // metadata's `possibly_trimmed_base_specifics` rather than `SkillLocalData`.
  sync_pb::SkillSpecifics trimmed_specifics;
  trimmed_specifics.set_source_skill_id("newly_supported_source_skill_id");
  trimmed_specifics.mutable_simple_skill()->set_description(
      "newly_supported_description");
  SetPossiblyTrimmedSkillSpecificsInStore(kSkillId, trimmed_specifics);
}

IN_PROC_BROWSER_TEST_P(SingleClientSkillsSyncTest,
                       ShouldPopulateNewlySupportedFieldsOnBrowserUpgrade) {
  ASSERT_TRUE(SetupClients());
  ASSERT_TRUE(GetClient(0)->AwaitSyncTransportActive());

  // Upon restart (simulating a browser upgrade), the newly-supported fields
  // from sync metadata should be merged on load and surfaced in `SkillsService`
  // without requiring a server redownload.

  // Subsequent local edits should preserve the newly-populated
  // `source_skill_id` when committing back to the server.
  ASSERT_NE(skill, nullptr);

  EXPECT_TRUE(ServerSkillsMatchChecker(
                  UnorderedElementsAre(HasSkillSpecifics(
                      skill_id, "newly_supported_source_skill_id",
                      "updated name", "updated icon", "updated prompt",
                      /*description=*/"")))
                  .Wait());
}

// ChromeOS does not support signout.
IN_PROC_BROWSER_TEST_P(SingleClientSkillsSyncTest,
                       ShouldDeleteAllDataOnDisableSync) {
  InjectSpecificsToFakeServer(CreateSkillSpecifics(
      base::Uuid::GenerateRandomV4().AsLowercaseString(), "source_skill_id1",
      "skill1", "icon1", "prompt1", "description1"));
  InjectSpecificsToFakeServer(CreateSkillSpecifics(
      base::Uuid::GenerateRandomV4().AsLowercaseString(), "source_skill_id2",
      "skill2", "icon2", "prompt2", "description2"));

  ASSERT_TRUE(SetupSync());


  // Sign out the primary account to disable sync and verify that all data was
  // deleted.
  GetClient(0)->SignOutPrimaryAccount();


  // Sign in again to re-enable sync and verify that the data was re-synced.
  ASSERT_TRUE(SignIn());

}

IN_PROC_BROWSER_TEST_P(SingleClientSkillsSyncTest,
                       ShouldMirrorSyncOnPrefChange) {
  ASSERT_TRUE(SetupSync());
  EXPECT_TRUE(GetSyncService(0)->GetActiveDataTypes().Has(syncer::SKILL));

  // Add some data to sync.
  InjectSpecificsToFakeServer(CreateSkillSpecifics(
      base::Uuid::GenerateRandomV4().AsLowercaseString(), "source_skill_id1",
      "skill1", "icon1", "prompt1", "description1"));

  // Disable the pref.
  GetProfile(0)->GetPrefs()->SetBoolean(skills::prefs::kChromeSkillsEnabled,
                                        false);

  // Verify that SKILL is no longer active and local data is cleared.
  EXPECT_TRUE(SkillSyncInactiveChecker(GetSyncService(0)).Wait());

  // Re-enable the pref.
  GetProfile(0)->GetPrefs()->SetBoolean(skills::prefs::kChromeSkillsEnabled,
                                        true);

  // Verify that SKILL is active again and data is re-synced.
  EXPECT_TRUE(SkillSyncActiveChecker(GetSyncService(0)).Wait());
}

}  // namespace
