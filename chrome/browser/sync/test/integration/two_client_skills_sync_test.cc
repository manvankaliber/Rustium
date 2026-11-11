// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <string>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "base/scoped_observation.h"
#include "base/strings/stringprintf.h"
#include "base/test/scoped_feature_list.h"
#include "chrome/browser/sync/test/integration/status_change_checker.h"
#include "chrome/browser/sync/test/integration/sync_service_impl_harness.h"
#include "chrome/browser/sync/test/integration/sync_test.h"
#include "components/skills/features.h"
#include "components/skills/public/skill.h"
#include "components/skills/public/skills_service.h"
#include "content/public/test/browser_test.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

class SkillsMatchChecker : public StatusChangeChecker,
                           public skills::SkillsService::Observer {
 public:

  // skills::SkillsService::Observer overrides.

  bool Require1PSkillRefresh() override { return false; }

  // StatusChangeChecker:
  bool IsExitConditionSatisfied(std::ostream* os) override {
    *os << "Waiting for skills to match across all clients. ";

    if (services_.empty()) {
      return true;
    }

    const std::vector<std::unique_ptr<skills::Skill>>& base_skills =
        services_.front()->GetSkills();

    for (size_t i = 1; i < services_.size(); ++i) {
      const std::vector<std::unique_ptr<skills::Skill>>& skills =
          services_[i]->GetSkills();

      if (!SkillsMatch(base_skills, skills, os)) {
        return false;
      }
    }
    return true;
  }

 private:
  bool SkillsMatch(const std::vector<std::unique_ptr<skills::Skill>>& skills1,
                   const std::vector<std::unique_ptr<skills::Skill>>& skills2,
                   std::ostream* os) {
    if (skills1.size() != skills2.size()) {
      *os << "Size mismatch: " << skills1.size() << " vs " << skills2.size();
      return false;
    }

    for (size_t i = 0; i < skills1.size(); ++i) {
      const skills::Skill& skill1 = *skills1[i];
      const skills::Skill& skill2 = *skills2[i];

      if (skill1.id != skill2.id) {
        *os << "Skill id mismatch: " << skill1.id << " vs " << skill2.id;
        return false;
      }

#define COMPARE_SKILL_FIELD(field)                                   \
  if (skill1.field != skill2.field) {                                \
    *os << "Skill " << skill1.name << " " << #field << " mismatch."; \
    return false;                                                    \
  }

      COMPARE_SKILL_FIELD(name);
      COMPARE_SKILL_FIELD(icon);
      COMPARE_SKILL_FIELD(prompt);
      COMPARE_SKILL_FIELD(description);
      COMPARE_SKILL_FIELD(source_skill_id);
      COMPARE_SKILL_FIELD(creation_time);
      COMPARE_SKILL_FIELD(last_update_time);

#undef COMPARE_SKILL_FIELD
    }
    return true;
  }

};

class TwoClientSkillsSyncTest
    : public SyncTest,
      public testing::WithParamInterface<SyncTest::SetupSyncMode> {
 public:
  TwoClientSkillsSyncTest() : SyncTest(TWO_CLIENT) {
    feature_overrides_.InitAndEnableFeature(features::kSkillsEnabled);
  }
  ~TwoClientSkillsSyncTest() override = default;

  SyncTest::SetupSyncMode GetSetupSyncMode() const override {
    return GetParam();
  }


  size_t CountAllSkills(int index) const {
    return GetSkillsService(index).GetSkills().size();
  }


 private:
  base::test::ScopedFeatureList feature_overrides_;
};

INSTANTIATE_TEST_SUITE_P(,
                         TwoClientSkillsSyncTest,
                         GetSyncTestModes(),
                         testing::PrintToStringParamName());

IN_PROC_BROWSER_TEST_P(TwoClientSkillsSyncTest,
                       E2E_ENABLED(OneClientAddsSkill)) {
  ASSERT_TRUE(ResetSyncForPrimaryAccount());
  ASSERT_TRUE(SetupSync());

  // Add one new skill to the first profile.

  EXPECT_EQ(CountAllSkills(1), 1u);
}

IN_PROC_BROWSER_TEST_P(TwoClientSkillsSyncTest,
                       E2E_ENABLED(TwoClientsAddSkills)) {
  ASSERT_TRUE(ResetSyncForPrimaryAccount());
  ASSERT_TRUE(SetupSync());

  // Add one new skill per profile.

  EXPECT_EQ(CountAllSkills(0), 2u);
  EXPECT_EQ(CountAllSkills(1), 2u);
}

IN_PROC_BROWSER_TEST_P(TwoClientSkillsSyncTest,
                       E2E_ENABLED(OneClientUpdatesSkill)) {
  ASSERT_TRUE(ResetSyncForPrimaryAccount());
  ASSERT_TRUE(SetupSync());

  // Add one new skill to the first profile.
  ASSERT_TRUE(skill);
  std::string skill_id = skill->id;


  // Update the skill on the first profile.


  // Verify the update on the second profile.
  ASSERT_TRUE(synced_skill);
  EXPECT_EQ(synced_skill->name, "Updated Skill");
  EXPECT_EQ(synced_skill->icon, "updated_icon");
  EXPECT_EQ(synced_skill->prompt, "updated_prompt");
}

IN_PROC_BROWSER_TEST_P(TwoClientSkillsSyncTest,
                       E2E_ENABLED(CrossClientUpdateAndDelete)) {
  ASSERT_TRUE(ResetSyncForPrimaryAccount());
  ASSERT_TRUE(SetupSync());

  // Add initial skills on different clients.
  ASSERT_TRUE(skill_a);
  std::string skill_a_id = skill_a->id;

  ASSERT_TRUE(skill_b);
  std::string skill_b_id = skill_b->id;


  // Client 0 updates Skill B (which was added by Client 1).

  // Client 1 deletes Skill A (which was added by Client 0).


  // Verify final state.
  EXPECT_EQ(CountAllSkills(0), 1u);
  EXPECT_EQ(CountAllSkills(1), 1u);

  // Verify the remaining skill is Skill B Updated.
  ASSERT_TRUE(remaining_skill_0);
  EXPECT_EQ(remaining_skill_0->name, "Skill B Updated");

  ASSERT_TRUE(remaining_skill_1);
  EXPECT_EQ(remaining_skill_1->name, "Skill B Updated");

  // Verify Skill A is gone.
}

}  // namespace
