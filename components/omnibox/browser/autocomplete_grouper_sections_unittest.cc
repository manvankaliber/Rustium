// Copyright 2022 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/omnibox/browser/autocomplete_grouper_sections.h"

#include <iterator>
#include <memory>

#include "base/test/scoped_feature_list.h"
#include "components/omnibox/browser/autocomplete_match.h"
#include "components/omnibox/browser/omnibox_field_trial.h"
#include "components/omnibox/common/omnibox_feature_configs.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/omnibox_proto/groups.pb.h"

namespace {
AutocompleteMatch CreateMatch(int relevance,
                              omnibox::GroupId group_id,
                              bool allowed_to_be_default = false) {
  AutocompleteMatch match;
  match.relevance = relevance;
  match.suggestion_group_id = group_id;
  match.allowed_to_be_default_match = allowed_to_be_default;
  return match;
}

void VerifyMatches(const ACMatches& matches,
                   std::vector<int> expected_relevances) {
  std::vector<int> relevances = {};
  std::ranges::transform(matches, std::back_inserter(relevances),
                         [&](const auto& match) { return match.relevance; });

  EXPECT_THAT(relevances, testing::ElementsAreArray(expected_relevances));
}

void VerifyMatches(
    const ACMatches& matches,
    const std::vector<std::pair<int, omnibox::GroupId>>& expected) {
  std::vector<std::pair<int, omnibox::GroupId>> actual;
  std::ranges::transform(
      matches, std::back_inserter(actual), [](const auto& match) {
        return std::make_pair(match.relevance,
                              match.suggestion_group_id.value());
      });
  EXPECT_THAT(actual, testing::ElementsAreArray(expected));
}

}  // namespace

// Tests rules for Section.
TEST(AutocompleteGrouperSectionsTest, Section) {
  class TestSection : public Section {
   public:
    // Up to 1 item of the following types.
    explicit TestSection(omnibox::GroupConfigMap& group_configs)
        : Section(
              1,
              {
                  Group(
                      1,
                      {
                          {omnibox::GROUP_PREVIOUS_SEARCH_RELATED_ENTITY_CHIPS,
                           1},
                      }),
                  Group(1,
                        {
                            {omnibox::GROUP_PREVIOUS_SEARCH_RELATED, 1},
                        }),
              },
              group_configs,
              omnibox::GroupConfig_SideType_DEFAULT_PRIMARY) {}
  };

  auto test = [](ACMatches matches, std::vector<int> expected_relevances) {
    PSections sections;
    // A `Section` ensures all matches in `Group`s have the same `SideType` as
    // the `Section`.
    omnibox::GroupConfig group;
    group.set_side_type(omnibox::GroupConfig_SideType_SECONDARY);
    omnibox::GroupConfigMap group_configs{
        {omnibox::GROUP_PREVIOUS_SEARCH_RELATED_ENTITY_CHIPS, group}};

    sections.push_back(std::make_unique<TestSection>(group_configs));
    auto out_matches = Section::GroupMatches(std::move(sections), matches);
    VerifyMatches(out_matches, expected_relevances);
  };

  // Given no matches, should return no matches.
  test({}, {});

  // Matches not qualifying for the section should not be added.
  test({CreateMatch(1, omnibox::GROUP_SEARCH)}, {});
}

// Tests rules for Section.
TEST(AutocompleteGrouperGroupsTest, Section) {
  class TestSection : public Section {
   public:
    // Up to 2 items of the following types.
    explicit TestSection(omnibox::GroupConfigMap& group_configs)
        : Section(2,
                  {
                      Group(1,
                            {
                                {omnibox::GROUP_MOBILE_SEARCH_READY_OMNIBOX, 1},
                            }),
                      Group(1,
                            {
                                {omnibox::GROUP_MOBILE_CLIPBOARD, 1},
                            }),
                      Group(1,
                            {
                                {omnibox::GROUP_MOBILE_MOST_VISITED, 1},
                            }),
                      Group(1,
                            {
                                {omnibox::GROUP_VISITED_DOC_RELATED, 1},
                            }),
                      Group(1,
                            {
                                {omnibox::GROUP_RELATED_QUERIES, 1},
                            }),
                  },
                  group_configs,
                  omnibox::GroupConfig_SideType_DEFAULT_PRIMARY) {}
  };

  auto test = [](ACMatches matches, std::vector<int> expected_relevances) {
    PSections sections;
    omnibox::GroupConfigMap group_configs;
    sections.push_back(std::make_unique<TestSection>(group_configs));
    auto out_matches = Section::GroupMatches(std::move(sections), matches);
    VerifyMatches(out_matches, expected_relevances);
  };

  {
    SCOPED_TRACE(
        "Matches are ranked by the group order and added up to the limits.");
    test(
        {
            CreateMatch(6, omnibox::GROUP_RELATED_QUERIES),
            CreateMatch(5, omnibox::GROUP_VISITED_DOC_RELATED),
            CreateMatch(4, omnibox::GROUP_MOBILE_MOST_VISITED),
            CreateMatch(3, omnibox::GROUP_MOBILE_CLIPBOARD),
            CreateMatch(2, omnibox::GROUP_MOBILE_SEARCH_READY_OMNIBOX),
            CreateMatch(1, omnibox::GROUP_MOBILE_SEARCH_READY_OMNIBOX),
        },
        {5, 6});
  }
}

// Tests the groups, limits, and rules for the Desktop NTP ZPS section.
TEST(AutocompleteGrouperSectionsTest, DesktopNTPZpsSection) {
  auto test = [](ACMatches matches, std::vector<int> expected_relevances) {
    PSections sections;
    omnibox::GroupConfigMap group_configs;
    sections.push_back(
        std::make_unique<DesktopNTPZpsSection>(group_configs, 8u, false));
    auto out_matches = Section::GroupMatches(std::move(sections), matches);
    VerifyMatches(out_matches, expected_relevances);
  };

  {
    SCOPED_TRACE("Given no matches, should return no matches.");
    test({}, {});
  }
  {
    SCOPED_TRACE("Matches that qualify for no groups should not be added.");
    test(
        {
            CreateMatch(100, omnibox::GROUP_DOCUMENT),
            CreateMatch(99, omnibox::GROUP_SEARCH),
            CreateMatch(98, omnibox::GROUP_PERSONALIZED_ZERO_SUGGEST),
        },
        {98});
  }
  {
    SCOPED_TRACE("Personalized suggestions get precedence over trending ones");
    test(
        {
            // `GROUP_TRENDS` matches are more relevant but will not be added.
            CreateMatch(90, omnibox::GROUP_TRENDS),
            CreateMatch(89, omnibox::GROUP_TRENDS),
            CreateMatch(88, omnibox::GROUP_TRENDS),
            CreateMatch(87, omnibox::GROUP_TRENDS),
            CreateMatch(86, omnibox::GROUP_TRENDS),
            CreateMatch(85, omnibox::GROUP_TRENDS),
            CreateMatch(84, omnibox::GROUP_TRENDS),
            CreateMatch(83, omnibox::GROUP_TRENDS),
            CreateMatch(82, omnibox::GROUP_TRENDS),
            CreateMatch(81, omnibox::GROUP_TRENDS),
            // `GROUP_PERSONALIZED_ZERO_SUGGEST` matches should be added.
            CreateMatch(80, omnibox::GROUP_PERSONALIZED_ZERO_SUGGEST),
            CreateMatch(79, omnibox::GROUP_PERSONALIZED_ZERO_SUGGEST),
            CreateMatch(78, omnibox::GROUP_PERSONALIZED_ZERO_SUGGEST),
            CreateMatch(77, omnibox::GROUP_PERSONALIZED_ZERO_SUGGEST),
            CreateMatch(76, omnibox::GROUP_PERSONALIZED_ZERO_SUGGEST),
            CreateMatch(75, omnibox::GROUP_PERSONALIZED_ZERO_SUGGEST),
            CreateMatch(74, omnibox::GROUP_PERSONALIZED_ZERO_SUGGEST),
            CreateMatch(73, omnibox::GROUP_PERSONALIZED_ZERO_SUGGEST),
            CreateMatch(72, omnibox::GROUP_PERSONALIZED_ZERO_SUGGEST),
            CreateMatch(71, omnibox::GROUP_PERSONALIZED_ZERO_SUGGEST),
        },
        {
            80,
            79,
            78,
            77,
            76,
            75,
            74,
            73,
        });
  }
  {
    SCOPED_TRACE("Matches should be added up to their group limit.");
    test(
        {
            CreateMatch(80, omnibox::GROUP_PERSONALIZED_ZERO_SUGGEST),
            CreateMatch(79, omnibox::GROUP_PERSONALIZED_ZERO_SUGGEST),
            CreateMatch(78, omnibox::GROUP_PERSONALIZED_ZERO_SUGGEST),
            CreateMatch(77, omnibox::GROUP_PERSONALIZED_ZERO_SUGGEST),
            CreateMatch(76, omnibox::GROUP_PERSONALIZED_ZERO_SUGGEST),
            CreateMatch(75, omnibox::GROUP_PERSONALIZED_ZERO_SUGGEST),
            CreateMatch(74, omnibox::GROUP_PERSONALIZED_ZERO_SUGGEST),
            CreateMatch(73, omnibox::GROUP_PERSONALIZED_ZERO_SUGGEST),
            CreateMatch(72, omnibox::GROUP_PERSONALIZED_ZERO_SUGGEST),
            CreateMatch(71, omnibox::GROUP_PERSONALIZED_ZERO_SUGGEST),
        },
        {
            80,
            79,
            78,
            77,
            76,
            75,
            74,
            73,
        });
  }
  {
    SCOPED_TRACE("Matches should be added up to the section limit.");
    test(
        {
            // `GROUP_TRENDS` matches should be added up to the remaining
            // section limit (3).
            CreateMatch(90, omnibox::GROUP_TRENDS),
            CreateMatch(89, omnibox::GROUP_TRENDS),
            CreateMatch(88, omnibox::GROUP_TRENDS),
            CreateMatch(87, omnibox::GROUP_TRENDS),
            CreateMatch(86, omnibox::GROUP_TRENDS),
            CreateMatch(85, omnibox::GROUP_TRENDS),
            CreateMatch(84, omnibox::GROUP_TRENDS),
            CreateMatch(83, omnibox::GROUP_TRENDS),
            CreateMatch(82, omnibox::GROUP_TRENDS),
            CreateMatch(81, omnibox::GROUP_TRENDS),
            // `GROUP_PERSONALIZED_ZERO_SUGGEST` matches should all be added.
            CreateMatch(78, omnibox::GROUP_PERSONALIZED_ZERO_SUGGEST),
            CreateMatch(77, omnibox::GROUP_PERSONALIZED_ZERO_SUGGEST),
            CreateMatch(76, omnibox::GROUP_PERSONALIZED_ZERO_SUGGEST),
            CreateMatch(75, omnibox::GROUP_PERSONALIZED_ZERO_SUGGEST),
            CreateMatch(74, omnibox::GROUP_PERSONALIZED_ZERO_SUGGEST),
        },
        {
            78,
            77,
            76,
            75,
            74,
            90,
            89,
            88,
        });
  }
  {
    SCOPED_TRACE("Up to 5 contextual search suggestions should be added.");
    test(
        {
            CreateMatch(90, omnibox::GROUP_CONTEXTUAL_SEARCH),
            CreateMatch(89, omnibox::GROUP_CONTEXTUAL_SEARCH),
            CreateMatch(88, omnibox::GROUP_CONTEXTUAL_SEARCH),
            CreateMatch(87, omnibox::GROUP_CONTEXTUAL_SEARCH),
            CreateMatch(86, omnibox::GROUP_CONTEXTUAL_SEARCH),
            CreateMatch(85, omnibox::GROUP_CONTEXTUAL_SEARCH),
            CreateMatch(84, omnibox::GROUP_CONTEXTUAL_SEARCH),
            CreateMatch(83, omnibox::GROUP_CONTEXTUAL_SEARCH),
            CreateMatch(82, omnibox::GROUP_CONTEXTUAL_SEARCH),
            CreateMatch(81, omnibox::GROUP_CONTEXTUAL_SEARCH),
        },
        {
            90,
            89,
            88,
            87,
            86,
        });
  }
}

// Tests the groups, limits, and rules for the Desktop NTP ZPS section.
TEST(AutocompleteGrouperSectionsTest, DesktopNTPZpsSectionWithMIA) {
  auto test = [](std::vector<std::pair<int, omnibox::GroupId>> input,
                 bool mia_enabled,
                 std::vector<std::pair<int, omnibox::GroupId>> output) {
    ACMatches in_matches;
    for (const auto& [relevance, group_id] : input) {
      in_matches.push_back(CreateMatch(relevance, group_id));
    }
    PSections sections;
    omnibox::GroupConfigMap group_configs;
    sections.push_back(
        std::make_unique<DesktopNTPZpsSection>(group_configs, 8u, mia_enabled));
    auto out_matches = Section::GroupMatches(std::move(sections), in_matches);
    VerifyMatches(out_matches, output);
  };

  {
    SCOPED_TRACE(
        "MIA above pSuggest - local history zps takes precedence over Trends.");
    test(
        {
            // `GROUP_PERSONALIZED_ZERO_SUGGEST_WITH_MIA` and
            // `GROUP_MIA_RECOMMENDATIONS` matches should all be added and
            // appear first due to their higher relevance scores.
            {90, omnibox::GROUP_PERSONALIZED_ZERO_SUGGEST_WITH_MIA},
            {89, omnibox::GROUP_PERSONALIZED_ZERO_SUGGEST_WITH_MIA},
            {88, omnibox::GROUP_MIA_RECOMMENDATIONS},
            {87, omnibox::GROUP_MIA_RECOMMENDATIONS},
            // remote `GROUP_PERSONALIZED_ZERO_SUGGEST` should all be added.
            {86, omnibox::GROUP_PERSONALIZED_ZERO_SUGGEST},
            {85, omnibox::GROUP_PERSONALIZED_ZERO_SUGGEST},
            // `GROUP_TRENDS` matches should not be added.
            {84, omnibox::GROUP_TRENDS},
            {83, omnibox::GROUP_TRENDS},
            {82, omnibox::GROUP_TRENDS},
            {81, omnibox::GROUP_TRENDS},
            // local `GROUP_PERSONALIZED_ZERO_SUGGEST` should be added up to the
            // remaining section limit (2) despite having lower relevance than
            // `GROUP_TRENDS`.
            {50, omnibox::GROUP_PERSONALIZED_ZERO_SUGGEST},
            {49, omnibox::GROUP_PERSONALIZED_ZERO_SUGGEST},
            {48, omnibox::GROUP_PERSONALIZED_ZERO_SUGGEST},
            {47, omnibox::GROUP_PERSONALIZED_ZERO_SUGGEST},
        },
        /*mia_enabled=*/true,
        {
            {90, omnibox::GROUP_PERSONALIZED_ZERO_SUGGEST_WITH_MIA},
            {89, omnibox::GROUP_PERSONALIZED_ZERO_SUGGEST_WITH_MIA},
            {88, omnibox::GROUP_MIA_RECOMMENDATIONS},
            {87, omnibox::GROUP_MIA_RECOMMENDATIONS},
            {86, omnibox::GROUP_PERSONALIZED_ZERO_SUGGEST},
            {85, omnibox::GROUP_PERSONALIZED_ZERO_SUGGEST},
            {50, omnibox::GROUP_PERSONALIZED_ZERO_SUGGEST},
            {49, omnibox::GROUP_PERSONALIZED_ZERO_SUGGEST},
        });
  }
  {
    SCOPED_TRACE(
        "MIA below pSuggest - Local history zps is grouped with pSuggest but "
        "doesn't take precedence over non-Trends.");
    test(
        {
            // remote `GROUP_PERSONALIZED_ZERO_SUGGEST` should all be added.
            {90, omnibox::GROUP_PERSONALIZED_ZERO_SUGGEST},
            {89, omnibox::GROUP_PERSONALIZED_ZERO_SUGGEST},
            // `GROUP_PERSONALIZED_ZERO_SUGGEST_WITH_MIA` and
            // `GROUP_MIA_RECOMMENDATIONS` matches should should all be added
            // and appear last due to their lower relevance scores.
            {88, omnibox::GROUP_PERSONALIZED_ZERO_SUGGEST_WITH_MIA},
            {87, omnibox::GROUP_PERSONALIZED_ZERO_SUGGEST_WITH_MIA},
            {86, omnibox::GROUP_MIA_RECOMMENDATIONS},
            {85, omnibox::GROUP_MIA_RECOMMENDATIONS},
            // `GROUP_TRENDS` matches should not be added.
            {84, omnibox::GROUP_TRENDS},
            {83, omnibox::GROUP_TRENDS},
            {82, omnibox::GROUP_TRENDS},
            {81, omnibox::GROUP_TRENDS},
            // local `GROUP_PERSONALIZED_ZERO_SUGGEST` should be added up to the
            // remaining section limit (2).
            {50, omnibox::GROUP_PERSONALIZED_ZERO_SUGGEST},
            {49, omnibox::GROUP_PERSONALIZED_ZERO_SUGGEST},
            {48, omnibox::GROUP_PERSONALIZED_ZERO_SUGGEST},
            {47, omnibox::GROUP_PERSONALIZED_ZERO_SUGGEST},
        },
        /*mia_enabled=*/true,
        {
            {90, omnibox::GROUP_PERSONALIZED_ZERO_SUGGEST},
            {89, omnibox::GROUP_PERSONALIZED_ZERO_SUGGEST},
            {50, omnibox::GROUP_PERSONALIZED_ZERO_SUGGEST},
            {49, omnibox::GROUP_PERSONALIZED_ZERO_SUGGEST},
            {88, omnibox::GROUP_PERSONALIZED_ZERO_SUGGEST_WITH_MIA},
            {87, omnibox::GROUP_PERSONALIZED_ZERO_SUGGEST_WITH_MIA},
            {86, omnibox::GROUP_MIA_RECOMMENDATIONS},
            {85, omnibox::GROUP_MIA_RECOMMENDATIONS},
        });
  }
  {
    SCOPED_TRACE(
        "MIA and no pSuggest - Local history zps doesn't take precedence over "
        "non-Trends.");
    test(
        {
            // `GROUP_PERSONALIZED_ZERO_SUGGEST_WITH_MIA` and
            // `GROUP_MIA_RECOMMENDATIONS` matches should all be added and
            // appear first due to their higher relevance scores.
            {90, omnibox::GROUP_PERSONALIZED_ZERO_SUGGEST_WITH_MIA},
            {89, omnibox::GROUP_PERSONALIZED_ZERO_SUGGEST_WITH_MIA},
            {88, omnibox::GROUP_MIA_RECOMMENDATIONS},
            {87, omnibox::GROUP_MIA_RECOMMENDATIONS},
            {86, omnibox::GROUP_MIA_RECOMMENDATIONS},
            {85, omnibox::GROUP_MIA_RECOMMENDATIONS},
            {84, omnibox::GROUP_MIA_RECOMMENDATIONS},
            {83, omnibox::GROUP_MIA_RECOMMENDATIONS},
            // `GROUP_TRENDS` matches should not be added.
            {82, omnibox::GROUP_TRENDS},
            {81, omnibox::GROUP_TRENDS},
            // local `GROUP_PERSONALIZED_ZERO_SUGGEST` should not be added.
            {50, omnibox::GROUP_PERSONALIZED_ZERO_SUGGEST},
            {49, omnibox::GROUP_PERSONALIZED_ZERO_SUGGEST},
            {48, omnibox::GROUP_PERSONALIZED_ZERO_SUGGEST},
            {47, omnibox::GROUP_PERSONALIZED_ZERO_SUGGEST},
        },
        /*mia_enabled=*/true,
        {
            {90, omnibox::GROUP_PERSONALIZED_ZERO_SUGGEST_WITH_MIA},
            {89, omnibox::GROUP_PERSONALIZED_ZERO_SUGGEST_WITH_MIA},
            {88, omnibox::GROUP_MIA_RECOMMENDATIONS},
            {87, omnibox::GROUP_MIA_RECOMMENDATIONS},
            {86, omnibox::GROUP_MIA_RECOMMENDATIONS},
            {85, omnibox::GROUP_MIA_RECOMMENDATIONS},
            {84, omnibox::GROUP_MIA_RECOMMENDATIONS},
            {83, omnibox::GROUP_MIA_RECOMMENDATIONS},
        });
  }
  {
    SCOPED_TRACE(
        "MIA and no pSuggest - Local history zps added but doesn't take "
        "precedence over non-Trends.");
    test(
        {
            // `GROUP_PERSONALIZED_ZERO_SUGGEST_WITH_MIA` and
            // `GROUP_MIA_RECOMMENDATIONS` matches should all be added and
            // appear first due to their higher relevance scores.
            {90, omnibox::GROUP_PERSONALIZED_ZERO_SUGGEST_WITH_MIA},
            {89, omnibox::GROUP_PERSONALIZED_ZERO_SUGGEST_WITH_MIA},
            {88, omnibox::GROUP_MIA_RECOMMENDATIONS},
            {87, omnibox::GROUP_MIA_RECOMMENDATIONS},
            {86, omnibox::GROUP_MIA_RECOMMENDATIONS},
            {85, omnibox::GROUP_MIA_RECOMMENDATIONS},
            // `GROUP_TRENDS` matches should not be added.
            {84, omnibox::GROUP_TRENDS},
            {83, omnibox::GROUP_TRENDS},
            {82, omnibox::GROUP_TRENDS},
            {81, omnibox::GROUP_TRENDS},
            // local `GROUP_PERSONALIZED_ZERO_SUGGEST` should not be added.
            {50, omnibox::GROUP_PERSONALIZED_ZERO_SUGGEST},
            {49, omnibox::GROUP_PERSONALIZED_ZERO_SUGGEST},
            {48, omnibox::GROUP_PERSONALIZED_ZERO_SUGGEST},
            {47, omnibox::GROUP_PERSONALIZED_ZERO_SUGGEST},
        },
        /*mia_enabled=*/true,
        {
            {90, omnibox::GROUP_PERSONALIZED_ZERO_SUGGEST_WITH_MIA},
            {89, omnibox::GROUP_PERSONALIZED_ZERO_SUGGEST_WITH_MIA},
            {88, omnibox::GROUP_MIA_RECOMMENDATIONS},
            {87, omnibox::GROUP_MIA_RECOMMENDATIONS},
            {86, omnibox::GROUP_MIA_RECOMMENDATIONS},
            {85, omnibox::GROUP_MIA_RECOMMENDATIONS},
            {50, omnibox::GROUP_PERSONALIZED_ZERO_SUGGEST},
            {49, omnibox::GROUP_PERSONALIZED_ZERO_SUGGEST},
        });
  }
  {
    SCOPED_TRACE("MIA is not added when mia_enabled is false.");
    test(
        {
            // remote `GROUP_PERSONALIZED_ZERO_SUGGEST` should all be added.
            {90, omnibox::GROUP_PERSONALIZED_ZERO_SUGGEST},
            {89, omnibox::GROUP_PERSONALIZED_ZERO_SUGGEST},
            // `GROUP_PERSONALIZED_ZERO_SUGGEST_WITH_MIA` and
            // `GROUP_MIA_RECOMMENDATIONS` matches should not be added.
            {88, omnibox::GROUP_PERSONALIZED_ZERO_SUGGEST_WITH_MIA},
            {87, omnibox::GROUP_PERSONALIZED_ZERO_SUGGEST_WITH_MIA},
            {86, omnibox::GROUP_MIA_RECOMMENDATIONS},
            {85, omnibox::GROUP_MIA_RECOMMENDATIONS},
            // `GROUP_TRENDS` matches should all be added.
            {84, omnibox::GROUP_TRENDS},
            {83, omnibox::GROUP_TRENDS},
            {82, omnibox::GROUP_TRENDS},
            {81, omnibox::GROUP_TRENDS},
            // local `GROUP_PERSONALIZED_ZERO_SUGGEST` should be added.
            {50, omnibox::GROUP_PERSONALIZED_ZERO_SUGGEST},
        },
        /*mia_enabled=*/false,
        {
            {90, omnibox::GROUP_PERSONALIZED_ZERO_SUGGEST},
            {89, omnibox::GROUP_PERSONALIZED_ZERO_SUGGEST},
            {50, omnibox::GROUP_PERSONALIZED_ZERO_SUGGEST},
            {84, omnibox::GROUP_TRENDS},
            {83, omnibox::GROUP_TRENDS},
            {82, omnibox::GROUP_TRENDS},
            {81, omnibox::GROUP_TRENDS},
        });
  }
}

TEST(AutocompleteGrouperSectionsTest, IosNTPZpsSectionWithMIA) {
  auto test = [](std::vector<std::pair<int, omnibox::GroupId>> input,
                 bool mia_enabled,
                 std::vector<std::pair<int, omnibox::GroupId>> output) {
    ACMatches in_matches;
    for (const auto& [relevance, group_id] : input) {
      in_matches.push_back(CreateMatch(relevance, group_id));
    }
    PSections sections;
    omnibox::GroupConfigMap group_configs;

    sections.push_back(
        std::make_unique<IOSNTPZpsSection>(group_configs, mia_enabled));
    auto out_matches = Section::GroupMatches(std::move(sections), in_matches);

    VerifyMatches(out_matches, output);
  };

  {
    SCOPED_TRACE("MIA above pSuggest - local history suppressed.");
    test(
        {
            {90, omnibox::GROUP_PERSONALIZED_ZERO_SUGGEST_WITH_MIA},
            {89, omnibox::GROUP_PERSONALIZED_ZERO_SUGGEST_WITH_MIA},
            {88, omnibox::GROUP_MIA_RECOMMENDATIONS},
            {87, omnibox::GROUP_MIA_RECOMMENDATIONS},
            {86, omnibox::GROUP_PERSONALIZED_ZERO_SUGGEST},
            {85, omnibox::GROUP_PERSONALIZED_ZERO_SUGGEST},
            {84, omnibox::GROUP_TRENDS},
            {83, omnibox::GROUP_TRENDS},
            {82, omnibox::GROUP_TRENDS},
            {81, omnibox::GROUP_TRENDS},
            {50, omnibox::GROUP_PERSONALIZED_ZERO_SUGGEST},
            {49, omnibox::GROUP_PERSONALIZED_ZERO_SUGGEST},
            {48, omnibox::GROUP_PERSONALIZED_ZERO_SUGGEST},
            {47, omnibox::GROUP_PERSONALIZED_ZERO_SUGGEST},
        },
        /*mia_enabled=*/true,
        {
            // First: MIA
            {90, omnibox::GROUP_PERSONALIZED_ZERO_SUGGEST_WITH_MIA},
            {89, omnibox::GROUP_PERSONALIZED_ZERO_SUGGEST_WITH_MIA},
            {88, omnibox::GROUP_MIA_RECOMMENDATIONS},
            {87, omnibox::GROUP_MIA_RECOMMENDATIONS},
            // No backfill this time
            // Lastly - Inspire Me
            {84, omnibox::GROUP_TRENDS},
            {83, omnibox::GROUP_TRENDS},
            {82, omnibox::GROUP_TRENDS},
            {81, omnibox::GROUP_TRENDS},
        });
  }

  {
    SCOPED_TRACE("PSuggest backfill present with no MIA");
    test(
        {
            {86, omnibox::GROUP_PERSONALIZED_ZERO_SUGGEST},
            {85, omnibox::GROUP_PERSONALIZED_ZERO_SUGGEST},
            {84, omnibox::GROUP_TRENDS},
            {83, omnibox::GROUP_TRENDS},
            {82, omnibox::GROUP_TRENDS},
            {81, omnibox::GROUP_TRENDS},
            {50, omnibox::GROUP_PERSONALIZED_ZERO_SUGGEST},
            {49, omnibox::GROUP_PERSONALIZED_ZERO_SUGGEST},
            {48, omnibox::GROUP_PERSONALIZED_ZERO_SUGGEST},
            {47, omnibox::GROUP_PERSONALIZED_ZERO_SUGGEST},
        },
        /*mia_enabled=*/true,
        {
            // Show backfill
            {86, omnibox::GROUP_PERSONALIZED_ZERO_SUGGEST},
            {85, omnibox::GROUP_PERSONALIZED_ZERO_SUGGEST},
            {50, omnibox::GROUP_PERSONALIZED_ZERO_SUGGEST},
            {49, omnibox::GROUP_PERSONALIZED_ZERO_SUGGEST},
            {48, omnibox::GROUP_PERSONALIZED_ZERO_SUGGEST},
            {47, omnibox::GROUP_PERSONALIZED_ZERO_SUGGEST},
            // and then Inspire Me
            {84, omnibox::GROUP_TRENDS},
            {83, omnibox::GROUP_TRENDS},
            {82, omnibox::GROUP_TRENDS},
            {81, omnibox::GROUP_TRENDS},
        });
  }
}

TEST(AutocompleteGrouperSectionsTest, IOSComposeboxZpsSection) {
  auto test = [](ACMatches matches, std::vector<int> expected_relevances) {
    PSections sections;
    omnibox::GroupConfigMap group_configs;
    sections.push_back(std::make_unique<IOSComposeboxZpsSection>(
        group_configs, /*max_suggestions=*/5u,
        /*max_aim_suggestions=*/5u,
        /*max_contextual_suggestions=*/5u));
    auto out_matches = Section::GroupMatches(std::move(sections), matches);
    VerifyMatches(out_matches, expected_relevances);
  };
  {
    SCOPED_TRACE("Given no matches, should return no matches.");
    test({}, {});
  }
  {
    SCOPED_TRACE("ZPS personalized zero suggest and aim suggestions only");
    test(
        {
            CreateMatch(103, omnibox::GROUP_PERSONALIZED_ZERO_SUGGEST),
            CreateMatch(102, omnibox::GROUP_PERSONALIZED_ZERO_SUGGEST),
            CreateMatch(101, omnibox::GROUP_MIA_RECOMMENDATIONS),
            CreateMatch(99, omnibox::GROUP_MOST_VISITED),
            CreateMatch(98, omnibox::GROUP_VISITED_DOC_RELATED),
            CreateMatch(97, omnibox::GROUP_PERSONALIZED_ZERO_SUGGEST),
            CreateMatch(96, omnibox::GROUP_MIA_RECOMMENDATIONS),
            CreateMatch(95, omnibox::GROUP_CONTEXTUAL_SEARCH),
            CreateMatch(94, omnibox::GROUP_CONTEXTUAL_SEARCH_ACTION),
            CreateMatch(93, omnibox::GROUP_CONTEXTUAL_SEARCH),
        },
        // Nothing but personalized zero suggest and aim recommendations.
        {103, 102, 97, 101, 96});
  }
}

// Tests the groups, limits, and rules for the Desktop NTP ZPS section.
TEST(AutocompleteGrouperSectionsTest, DesktopNTPZpsSection_WithIPH) {
  base::test::ScopedFeatureList feature_list;
  feature_list.InitAndEnableFeature(omnibox::kStarterPackIPH);

  auto test = [](ACMatches matches, std::vector<int> expected_relevances) {
    PSections sections;
    omnibox::GroupConfigMap group_configs;
    sections.push_back(
        std::make_unique<DesktopNTPZpsSection>(group_configs, 7u, false));
    sections.push_back(
        std::make_unique<DesktopNTPZpsIPHSection>(group_configs));
    auto out_matches = Section::GroupMatches(std::move(sections), matches);
    VerifyMatches(out_matches, expected_relevances);
  };
  {
    SCOPED_TRACE("Given no matches, should return no matches.");
    test({}, {});
  }
  {
    SCOPED_TRACE("Matches that qualify for no groups should not be added.");
    test(
        {
            CreateMatch(100, omnibox::GROUP_DOCUMENT),
            CreateMatch(99, omnibox::GROUP_SEARCH),
            CreateMatch(98, omnibox::GROUP_PERSONALIZED_ZERO_SUGGEST),
        },
        {98});
  }
  {
    SCOPED_TRACE("Personalized suggestions get precedence over trending ones");
    test(
        {
            // `GROUP_TRENDS` matches are more relevant but will not be added.
            CreateMatch(90, omnibox::GROUP_TRENDS),
            CreateMatch(89, omnibox::GROUP_TRENDS),
            CreateMatch(88, omnibox::GROUP_TRENDS),
            CreateMatch(87, omnibox::GROUP_TRENDS),
            CreateMatch(86, omnibox::GROUP_TRENDS),
            CreateMatch(85, omnibox::GROUP_TRENDS),
            CreateMatch(84, omnibox::GROUP_TRENDS),
            CreateMatch(83, omnibox::GROUP_TRENDS),
            CreateMatch(82, omnibox::GROUP_TRENDS),
            CreateMatch(81, omnibox::GROUP_TRENDS),
            // `GROUP_PERSONALIZED_ZERO_SUGGEST` matches should be added.
            CreateMatch(80, omnibox::GROUP_PERSONALIZED_ZERO_SUGGEST),
            CreateMatch(79, omnibox::GROUP_PERSONALIZED_ZERO_SUGGEST),
            CreateMatch(78, omnibox::GROUP_PERSONALIZED_ZERO_SUGGEST),
            CreateMatch(77, omnibox::GROUP_PERSONALIZED_ZERO_SUGGEST),
            CreateMatch(76, omnibox::GROUP_PERSONALIZED_ZERO_SUGGEST),
            CreateMatch(75, omnibox::GROUP_PERSONALIZED_ZERO_SUGGEST),
            CreateMatch(74, omnibox::GROUP_PERSONALIZED_ZERO_SUGGEST),
            CreateMatch(73, omnibox::GROUP_PERSONALIZED_ZERO_SUGGEST),
            CreateMatch(72, omnibox::GROUP_PERSONALIZED_ZERO_SUGGEST),
            CreateMatch(71, omnibox::GROUP_PERSONALIZED_ZERO_SUGGEST),
            // `GROUP_PREVIOUS_SEARCH_RELATED` matches should not be added.
            CreateMatch(70, omnibox::GROUP_PREVIOUS_SEARCH_RELATED),
            CreateMatch(69, omnibox::GROUP_PREVIOUS_SEARCH_RELATED),
            CreateMatch(68, omnibox::GROUP_PREVIOUS_SEARCH_RELATED),
            CreateMatch(67, omnibox::GROUP_PREVIOUS_SEARCH_RELATED),
            CreateMatch(66, omnibox::GROUP_PREVIOUS_SEARCH_RELATED),
            CreateMatch(65, omnibox::GROUP_PREVIOUS_SEARCH_RELATED),
            CreateMatch(64, omnibox::GROUP_PREVIOUS_SEARCH_RELATED),
            CreateMatch(63, omnibox::GROUP_PREVIOUS_SEARCH_RELATED),
            CreateMatch(62, omnibox::GROUP_PREVIOUS_SEARCH_RELATED),
            CreateMatch(61, omnibox::GROUP_PREVIOUS_SEARCH_RELATED),
            // `GROUP_ZERO_SUGGEST_IN_PRODUCT_HELP` suggestions have their
            // own section, so one should be added.
            CreateMatch(60, omnibox::GROUP_ZERO_SUGGEST_IN_PRODUCT_HELP),
            CreateMatch(59, omnibox::GROUP_ZERO_SUGGEST_IN_PRODUCT_HELP),
        },
        {
            80,
            79,
            78,
            77,
            76,
            75,
            74,
            60,
        });
  }
  {
    SCOPED_TRACE("Matches should be added up to their group limit.");
    test(
        {
            CreateMatch(80, omnibox::GROUP_PERSONALIZED_ZERO_SUGGEST),
            CreateMatch(79, omnibox::GROUP_PERSONALIZED_ZERO_SUGGEST),
            CreateMatch(78, omnibox::GROUP_PERSONALIZED_ZERO_SUGGEST),
            CreateMatch(77, omnibox::GROUP_PERSONALIZED_ZERO_SUGGEST),
            CreateMatch(76, omnibox::GROUP_PERSONALIZED_ZERO_SUGGEST),
            CreateMatch(75, omnibox::GROUP_PERSONALIZED_ZERO_SUGGEST),
            CreateMatch(74, omnibox::GROUP_PERSONALIZED_ZERO_SUGGEST),
            CreateMatch(73, omnibox::GROUP_PERSONALIZED_ZERO_SUGGEST),
            CreateMatch(72, omnibox::GROUP_PERSONALIZED_ZERO_SUGGEST),
            CreateMatch(71, omnibox::GROUP_PERSONALIZED_ZERO_SUGGEST),
        },
        {
            80,
            79,
            78,
            77,
            76,
            75,
            74,
        });
  }
  {
    SCOPED_TRACE("Matches should be added up to the section limit.");
    test(
        {
            // `GROUP_TRENDS` matches should be added up to the remaining
            // section limit (2).
            CreateMatch(90, omnibox::GROUP_TRENDS),
            CreateMatch(89, omnibox::GROUP_TRENDS),
            CreateMatch(88, omnibox::GROUP_TRENDS),
            CreateMatch(87, omnibox::GROUP_TRENDS),
            CreateMatch(86, omnibox::GROUP_TRENDS),
            CreateMatch(85, omnibox::GROUP_TRENDS),
            CreateMatch(84, omnibox::GROUP_TRENDS),
            CreateMatch(83, omnibox::GROUP_TRENDS),
            CreateMatch(82, omnibox::GROUP_TRENDS),
            CreateMatch(81, omnibox::GROUP_TRENDS),
            // `GROUP_PERSONALIZED_ZERO_SUGGEST` matches should all be added.
            CreateMatch(80, omnibox::GROUP_PERSONALIZED_ZERO_SUGGEST),
            CreateMatch(79, omnibox::GROUP_PERSONALIZED_ZERO_SUGGEST),
            CreateMatch(78, omnibox::GROUP_PERSONALIZED_ZERO_SUGGEST),
            CreateMatch(77, omnibox::GROUP_PERSONALIZED_ZERO_SUGGEST),
            CreateMatch(76, omnibox::GROUP_PERSONALIZED_ZERO_SUGGEST),
            // `GROUP_ZERO_SUGGEST_IN_PRODUCT_HELP` should be added up to its
            // section limit (1).
            CreateMatch(75, omnibox::GROUP_ZERO_SUGGEST_IN_PRODUCT_HELP),
            CreateMatch(74, omnibox::GROUP_ZERO_SUGGEST_IN_PRODUCT_HELP),
        },
        {
            80,
            79,
            78,
            77,
            76,
            90,
            89,
            75,
        });
  }
  {
    SCOPED_TRACE(
        "IPH match should be added regardless of matches in first section.");
    test(
        {
            // `GROUP_PERSONALIZED_ZERO_SUGGEST` matches should all be added.
            CreateMatch(80, omnibox::GROUP_PERSONALIZED_ZERO_SUGGEST),
            CreateMatch(77, omnibox::GROUP_PERSONALIZED_ZERO_SUGGEST),
            CreateMatch(76, omnibox::GROUP_PERSONALIZED_ZERO_SUGGEST),
            // `GROUP_ZERO_SUGGEST_IN_PRODUCT_HELP` should be added up to its
            // section limit even if the first section is not full. (1).
            CreateMatch(75, omnibox::GROUP_ZERO_SUGGEST_IN_PRODUCT_HELP),
            CreateMatch(74, omnibox::GROUP_ZERO_SUGGEST_IN_PRODUCT_HELP),
        },
        {
            80,
            77,
            76,
            75,
        });
  }
}

// Tests the groups, limits, and rules for the Desktop non-ZPS section.
TEST(AutocompleteGrouperSectionsTest, DesktopNonZpsSection) {
  auto test = [](ACMatches matches, std::vector<int> expected_relevances) {
    PSections sections;
    omnibox::GroupConfigMap group_configs;
    sections.push_back(std::make_unique<DesktopNonZpsSection>(group_configs));
    auto out_matches = Section::GroupMatches(std::move(sections), matches);
    VerifyMatches(out_matches, expected_relevances);
  };

  {
    SCOPED_TRACE("Given no matches, should return no matches.");
    test({}, {});
  }

  {
    SCOPED_TRACE("Rank groups: default > starter pack > searches > navs.");
    test(
        {
            CreateMatch(100, omnibox::GROUP_SEARCH),
            CreateMatch(99, omnibox::GROUP_OTHER_NAVS),
            CreateMatch(98, omnibox::GROUP_SEARCH),
            CreateMatch(96, omnibox::GROUP_SEARCH, true),
            // Only the 1st default-able suggestion should be ranked 1st.
            CreateMatch(97, omnibox::GROUP_OTHER_NAVS, true),
            CreateMatch(95, omnibox::GROUP_STARTER_PACK),
        },
        {96, 95, 100, 98, 99, 97});
  }

  {
    SCOPED_TRACE("Matches that qualify for no groups, should not be added.");
    test(
        {
            CreateMatch(100, omnibox::GROUP_SEARCH, true),
            CreateMatch(99, omnibox::GROUP_PERSONALIZED_ZERO_SUGGEST),
            CreateMatch(98, omnibox::GROUP_MOBILE_CLIPBOARD),
        },
        {100});
  }

  {
    SCOPED_TRACE(
        "A match that qualifies for multiple groups, should only be added "
        "once.");
    test(
        {
            CreateMatch(100, omnibox::GROUP_SEARCH, true),
        },
        {100});
  }

  {
    SCOPED_TRACE("Show at least 1 search.");
    test(
        {
            CreateMatch(100, omnibox::GROUP_OTHER_NAVS),
            CreateMatch(99, omnibox::GROUP_OTHER_NAVS, true),
            CreateMatch(98, omnibox::GROUP_OTHER_NAVS),
            CreateMatch(97, omnibox::GROUP_OTHER_NAVS),
            CreateMatch(96, omnibox::GROUP_OTHER_NAVS),
            CreateMatch(95, omnibox::GROUP_OTHER_NAVS),
            CreateMatch(94, omnibox::GROUP_OTHER_NAVS),
            CreateMatch(93, omnibox::GROUP_OTHER_NAVS),
            CreateMatch(92, omnibox::GROUP_OTHER_NAVS),
            CreateMatch(91, omnibox::GROUP_OTHER_NAVS),
            CreateMatch(90, omnibox::GROUP_OTHER_NAVS),
            CreateMatch(89, omnibox::GROUP_SEARCH),
            CreateMatch(88, omnibox::GROUP_SEARCH),
            CreateMatch(87, omnibox::GROUP_SEARCH),
        },
        {99, 89, 100, 98, 97, 96, 95, 94});
  }

  {
    SCOPED_TRACE("Show at least 1 search unless there are no searches.");
    test(
        {
            CreateMatch(100, omnibox::GROUP_OTHER_NAVS),
            CreateMatch(99, omnibox::GROUP_OTHER_NAVS, true),
            CreateMatch(98, omnibox::GROUP_OTHER_NAVS),
            CreateMatch(97, omnibox::GROUP_OTHER_NAVS),
            CreateMatch(96, omnibox::GROUP_OTHER_NAVS),
            CreateMatch(95, omnibox::GROUP_OTHER_NAVS),
            CreateMatch(94, omnibox::GROUP_OTHER_NAVS),
            CreateMatch(93, omnibox::GROUP_OTHER_NAVS),
            CreateMatch(92, omnibox::GROUP_OTHER_NAVS),
            CreateMatch(91, omnibox::GROUP_OTHER_NAVS),
            CreateMatch(90, omnibox::GROUP_OTHER_NAVS),
        },
        {99, 100, 98, 97, 96, 95, 94, 93});
  }

  {
    SCOPED_TRACE(
        "Show at least 1 search; if the default is a search, that counts too.");
    test(
        {
            CreateMatch(100, omnibox::GROUP_OTHER_NAVS),
            CreateMatch(99, omnibox::GROUP_OTHER_NAVS),
            CreateMatch(98, omnibox::GROUP_OTHER_NAVS),
            CreateMatch(97, omnibox::GROUP_OTHER_NAVS),
            CreateMatch(96, omnibox::GROUP_OTHER_NAVS),
            CreateMatch(95, omnibox::GROUP_OTHER_NAVS),
            CreateMatch(94, omnibox::GROUP_OTHER_NAVS),
            CreateMatch(93, omnibox::GROUP_OTHER_NAVS),
            CreateMatch(92, omnibox::GROUP_OTHER_NAVS),
            CreateMatch(91, omnibox::GROUP_OTHER_NAVS),
            CreateMatch(90, omnibox::GROUP_OTHER_NAVS),
            CreateMatch(89, omnibox::GROUP_SEARCH, true),
            CreateMatch(88, omnibox::GROUP_SEARCH),
            CreateMatch(87, omnibox::GROUP_SEARCH),
        },
        {89, 100, 99, 98, 97, 96, 95, 94});
  }

  {
    SCOPED_TRACE("Show at most 8 suggestions.");
    test(
        {
            CreateMatch(100, omnibox::GROUP_OTHER_NAVS),
            CreateMatch(99, omnibox::GROUP_SEARCH),
            CreateMatch(98, omnibox::GROUP_SEARCH, true),
            CreateMatch(97, omnibox::GROUP_SEARCH),
            CreateMatch(96, omnibox::GROUP_SEARCH),
            CreateMatch(95, omnibox::GROUP_SEARCH),
            CreateMatch(94, omnibox::GROUP_SEARCH),
            CreateMatch(93, omnibox::GROUP_SEARCH),
            CreateMatch(92, omnibox::GROUP_SEARCH),
            CreateMatch(91, omnibox::GROUP_SEARCH),
            CreateMatch(90, omnibox::GROUP_SEARCH),
        },
        {98, 99, 97, 96, 95, 94, 93, 100});
  }

  {
    SCOPED_TRACE(
        "Show at most 8 suggestions; unless there are no navs, then show up to "
        "10.");
    test(
        {
            CreateMatch(100, omnibox::GROUP_SEARCH),
            CreateMatch(99, omnibox::GROUP_SEARCH),
            CreateMatch(98, omnibox::GROUP_SEARCH, true),
            CreateMatch(97, omnibox::GROUP_SEARCH),
            CreateMatch(96, omnibox::GROUP_SEARCH),
            CreateMatch(95, omnibox::GROUP_SEARCH),
            CreateMatch(94, omnibox::GROUP_SEARCH),
            CreateMatch(93, omnibox::GROUP_SEARCH),
            CreateMatch(92, omnibox::GROUP_SEARCH),
            CreateMatch(91, omnibox::GROUP_SEARCH),
            CreateMatch(90, omnibox::GROUP_SEARCH),
        },
        {98, 100, 99, 97, 96, 95, 94, 93, 92, 91});
  }

  {
    SCOPED_TRACE(
        "Show at most 8 suggestions; unless there are no navs, then show up to "
        "10, even if there are navs after the 10th suggestion.");
    test(
        {
            CreateMatch(100, omnibox::GROUP_SEARCH),
            CreateMatch(99, omnibox::GROUP_SEARCH),
            CreateMatch(98, omnibox::GROUP_SEARCH, true),
            CreateMatch(97, omnibox::GROUP_SEARCH),
            CreateMatch(96, omnibox::GROUP_SEARCH),
            CreateMatch(95, omnibox::GROUP_SEARCH),
            CreateMatch(94, omnibox::GROUP_SEARCH),
            CreateMatch(93, omnibox::GROUP_SEARCH),
            CreateMatch(92, omnibox::GROUP_SEARCH),
            CreateMatch(91, omnibox::GROUP_SEARCH),
            CreateMatch(90, omnibox::GROUP_SEARCH),
            CreateMatch(89, omnibox::GROUP_OTHER_NAVS),
        },
        {98, 100, 99, 97, 96, 95, 94, 93, 92, 91});
  }

  {
    SCOPED_TRACE(
        "Show at most 8 suggestions; unless the 10th suggestion is the 1st "
        "nav, then show up to 9.");
    test(
        {
            CreateMatch(100, omnibox::GROUP_SEARCH),
            CreateMatch(99, omnibox::GROUP_SEARCH),
            CreateMatch(98, omnibox::GROUP_SEARCH, true),
            CreateMatch(97, omnibox::GROUP_SEARCH),
            CreateMatch(96, omnibox::GROUP_SEARCH),
            CreateMatch(95, omnibox::GROUP_SEARCH),
            CreateMatch(94, omnibox::GROUP_SEARCH),
            CreateMatch(93, omnibox::GROUP_SEARCH),
            CreateMatch(92, omnibox::GROUP_SEARCH),
            CreateMatch(91, omnibox::GROUP_OTHER_NAVS),
            CreateMatch(90, omnibox::GROUP_SEARCH),
        },
        {98, 100, 99, 97, 96, 95, 94, 93, 92});
  }

  {
    SCOPED_TRACE(
        "Show at most 8 suggestions if the 9th suggestion is the 1st nav.");
    test(
        {
            CreateMatch(100, omnibox::GROUP_SEARCH),
            CreateMatch(99, omnibox::GROUP_SEARCH),
            CreateMatch(98, omnibox::GROUP_SEARCH, true),
            CreateMatch(97, omnibox::GROUP_SEARCH),
            CreateMatch(96, omnibox::GROUP_SEARCH),
            CreateMatch(95, omnibox::GROUP_SEARCH),
            CreateMatch(94, omnibox::GROUP_SEARCH),
            CreateMatch(93, omnibox::GROUP_SEARCH),
            CreateMatch(92, omnibox::GROUP_OTHER_NAVS),
            CreateMatch(91, omnibox::GROUP_SEARCH),
            CreateMatch(90, omnibox::GROUP_SEARCH),
        },
        {98, 100, 99, 97, 96, 95, 94, 93});
  }

  {
    SCOPED_TRACE(
        "Show at most 8 suggestions if the default suggestion is a nav.");
    test(
        {
            CreateMatch(100, omnibox::GROUP_OTHER_NAVS, true),
            CreateMatch(99, omnibox::GROUP_SEARCH),
            CreateMatch(98, omnibox::GROUP_SEARCH),
            CreateMatch(97, omnibox::GROUP_SEARCH),
            CreateMatch(96, omnibox::GROUP_SEARCH),
            CreateMatch(95, omnibox::GROUP_SEARCH),
            CreateMatch(94, omnibox::GROUP_SEARCH),
            CreateMatch(93, omnibox::GROUP_SEARCH),
            CreateMatch(92, omnibox::GROUP_SEARCH),
            CreateMatch(91, omnibox::GROUP_SEARCH),
            CreateMatch(90, omnibox::GROUP_SEARCH),
        },
        {100, 99, 98, 97, 96, 95, 94, 93});
  }

  {
    SCOPED_TRACE("Show at most 1 default.");
    test(
        {
            CreateMatch(100, omnibox::GROUP_OTHER_NAVS, true),
            CreateMatch(99, omnibox::GROUP_OTHER_NAVS, true),
            CreateMatch(98, omnibox::GROUP_OTHER_NAVS, true),
            CreateMatch(97, omnibox::GROUP_STARTER_PACK, true),
            CreateMatch(96, omnibox::GROUP_SEARCH, true),
            CreateMatch(95, omnibox::GROUP_SEARCH, true),
        },
        {100, 97, 96, 95, 99, 98});
  }

  {
    SCOPED_TRACE("Group history clusters with searches.");
    test(
        {
            CreateMatch(100, omnibox::GROUP_SEARCH, true),
            CreateMatch(99, omnibox::GROUP_OTHER_NAVS),
            CreateMatch(98, omnibox::GROUP_OTHER_NAVS),
            CreateMatch(97, omnibox::GROUP_SEARCH),
            CreateMatch(96, omnibox::GROUP_HISTORY_CLUSTER),
            CreateMatch(95, omnibox::GROUP_SEARCH),
        },
        {100, 97, 96, 95, 99, 98});
  }

  {
    SCOPED_TRACE("Show at most 1 history cluster.");
    test(
        {
            CreateMatch(100, omnibox::GROUP_SEARCH, true),
            CreateMatch(99, omnibox::GROUP_OTHER_NAVS),
            CreateMatch(98, omnibox::GROUP_OTHER_NAVS),
            CreateMatch(97, omnibox::GROUP_SEARCH),
            CreateMatch(96, omnibox::GROUP_HISTORY_CLUSTER),
            CreateMatch(95, omnibox::GROUP_HISTORY_CLUSTER),
            CreateMatch(94, omnibox::GROUP_SEARCH),
        },
        {100, 97, 96, 94, 99, 98});
  }

  {
    SCOPED_TRACE("History cluster should count against search limit.");
    test(
        {
            CreateMatch(100, omnibox::GROUP_SEARCH, true),
            CreateMatch(99, omnibox::GROUP_SEARCH),
            CreateMatch(98, omnibox::GROUP_SEARCH),
            CreateMatch(97, omnibox::GROUP_SEARCH),
            CreateMatch(96, omnibox::GROUP_HISTORY_CLUSTER),
            CreateMatch(95, omnibox::GROUP_SEARCH),
            CreateMatch(94, omnibox::GROUP_SEARCH),
            CreateMatch(93, omnibox::GROUP_SEARCH),
            CreateMatch(92, omnibox::GROUP_SEARCH),
            CreateMatch(91, omnibox::GROUP_SEARCH),
            CreateMatch(90, omnibox::GROUP_SEARCH),
            CreateMatch(89, omnibox::GROUP_SEARCH),
            CreateMatch(88, omnibox::GROUP_SEARCH),
            CreateMatch(87, omnibox::GROUP_SEARCH),
            CreateMatch(86, omnibox::GROUP_SEARCH),
            CreateMatch(85, omnibox::GROUP_SEARCH),
            CreateMatch(84, omnibox::GROUP_SEARCH),
            CreateMatch(83, omnibox::GROUP_SEARCH),
        },
        {100, 99, 98, 97, 96, 95, 94, 93, 92, 91});
  }
}

// Tests the groups, limits, and rules for the iOS NTP ZPS.
TEST(AutocompleteGrouperSectionsTest, IOSNTPZpsSection) {
  auto test = [](ACMatches matches, std::vector<int> expected_relevances) {
    PSections sections;
    omnibox::GroupConfigMap group_configs;
    sections.push_back(
        std::make_unique<IOSNTPZpsSection>(group_configs, false));
    auto out_matches = Section::GroupMatches(std::move(sections), matches);
    VerifyMatches(out_matches, expected_relevances);
  };

  {
    SCOPED_TRACE("Given no matches, should return no matches.");
    test({}, {});
  }

  {
    SCOPED_TRACE(
        "Given no trend matches and only psuggest, should only display "
        "psuggest following the psuggest count limit");
    test({CreateMatch(100, omnibox::GROUP_PERSONALIZED_ZERO_SUGGEST),
          CreateMatch(99, omnibox::GROUP_PERSONALIZED_ZERO_SUGGEST),
          CreateMatch(98, omnibox::GROUP_PERSONALIZED_ZERO_SUGGEST),
          CreateMatch(97, omnibox::GROUP_PERSONALIZED_ZERO_SUGGEST)},
         {100, 99, 98, 97});
  }

  {
    SCOPED_TRACE(
        "Given no psuggest matches and only trends, should only display trends "
        "following the trends count limit");
    test({CreateMatch(100, omnibox::GROUP_TRENDS),
          CreateMatch(99, omnibox::GROUP_TRENDS),
          CreateMatch(98, omnibox::GROUP_TRENDS),
          CreateMatch(97, omnibox::GROUP_TRENDS),
          CreateMatch(96, omnibox::GROUP_TRENDS),
          CreateMatch(95, omnibox::GROUP_TRENDS)},
         {100, 99, 98, 97, 96});
  }

  {
    SCOPED_TRACE(
        "Given both psuggest and trends matches, should display both groups "
        "following their count limit");
    test({CreateMatch(100, omnibox::GROUP_PERSONALIZED_ZERO_SUGGEST),
          CreateMatch(99, omnibox::GROUP_PERSONALIZED_ZERO_SUGGEST),
          CreateMatch(98, omnibox::GROUP_PERSONALIZED_ZERO_SUGGEST),
          CreateMatch(97, omnibox::GROUP_PERSONALIZED_ZERO_SUGGEST),
          CreateMatch(96, omnibox::GROUP_TRENDS),
          CreateMatch(95, omnibox::GROUP_TRENDS),
          CreateMatch(94, omnibox::GROUP_TRENDS),
          CreateMatch(93, omnibox::GROUP_TRENDS)},
         {100, 99, 98, 97, 96, 95, 94, 93});
  }

  {
    SCOPED_TRACE(
        "Given cross-device tab matches, should display at most 1 above "
        "psuggest and trends");
    test({CreateMatch(102, omnibox::GROUP_MOBILE_CLIPBOARD),
          CreateMatch(101, omnibox::GROUP_CROSS_DEVICE_TABS),
          CreateMatch(100, omnibox::GROUP_CROSS_DEVICE_TABS),
          CreateMatch(99, omnibox::GROUP_PERSONALIZED_ZERO_SUGGEST),
          CreateMatch(98, omnibox::GROUP_TRENDS)},
         {102, 101, 99, 98});
  }
}

// Tests the groups and limits for DesktopSecondaryNTPZpsSection.
TEST(AutocompleteGrouperSectionsTest, DesktopSecondaryNTPZpsSection) {
  auto test = [](ACMatches matches, std::vector<int> expected_relevances) {
    PSections sections;
    omnibox::GroupConfigMap group_configs;
    const auto group1 = omnibox::GROUP_PREVIOUS_SEARCH_RELATED_ENTITY_CHIPS;
    const auto group2 = omnibox::GROUP_PREVIOUS_SEARCH_RELATED;
    const auto group3 = omnibox::GROUP_TRENDS;
    group_configs[group1].set_side_type(
        omnibox::GroupConfig_SideType_SECONDARY);
    group_configs[group2].set_side_type(
        omnibox::GroupConfig_SideType_SECONDARY);
    group_configs[group3].set_side_type(
        omnibox::GroupConfig_SideType_SECONDARY);
    sections.push_back(
        std::make_unique<DesktopSecondaryNTPZpsSection>(group_configs));
    auto out_matches = Section::GroupMatches(std::move(sections), matches);
    VerifyMatches(out_matches, expected_relevances);
  };
  {
    SCOPED_TRACE("Given no matches, should return no matches.");
    test({}, {});
  }
  {
    SCOPED_TRACE("Matches should be added up to their group limit.");
    // Groups do not enforce a minimum number of matches shown. They do enforce
    // a maximum number of matches shown for that Group.
    test({CreateMatch(100, omnibox::GROUP_TRENDS),
          CreateMatch(99, omnibox::GROUP_TRENDS),
          CreateMatch(98, omnibox::GROUP_TRENDS),
          CreateMatch(97, omnibox::GROUP_TRENDS),
          CreateMatch(96, omnibox::GROUP_TRENDS)},
         {100, 99, 98, 97});
  }
  {
    SCOPED_TRACE("Matches should be added up to the section limit.");
    // Sections do not enforce a minimum number of matches shown. They do
    // enforce a maximum number of matches shown.
    test({CreateMatch(100, omnibox::GROUP_PREVIOUS_SEARCH_RELATED_ENTITY_CHIPS),
          CreateMatch(99, omnibox::GROUP_PREVIOUS_SEARCH_RELATED_ENTITY_CHIPS),
          // `GROUP_PREVIOUS_SEARCH_RELATED` matches will be displayed
          // simultaneously as `GROUP_PREVIOUS_SEARCH_RELATED_ENTITY_CHIPS`
          // matches. Showing both may occur but is not likely to happen.
          CreateMatch(200, omnibox::GROUP_PREVIOUS_SEARCH_RELATED),
          CreateMatch(201, omnibox::GROUP_PREVIOUS_SEARCH_RELATED),
          CreateMatch(202, omnibox::GROUP_PREVIOUS_SEARCH_RELATED)},
         {100, 99, 200, 201});
  }
  {
    SCOPED_TRACE(
        "Matches added up to their group limit, but not section limit.");
    test({CreateMatch(100, omnibox::GROUP_PREVIOUS_SEARCH_RELATED_ENTITY_CHIPS),
          CreateMatch(99, omnibox::GROUP_PREVIOUS_SEARCH_RELATED_ENTITY_CHIPS),
          CreateMatch(98, omnibox::GROUP_PREVIOUS_SEARCH_RELATED_ENTITY_CHIPS),
          CreateMatch(97, omnibox::GROUP_PREVIOUS_SEARCH_RELATED_ENTITY_CHIPS)},
         {100, 99, 98});
  }
  {
    SCOPED_TRACE(
        "Given no matches that can be added to this section because of their "
        "GroupId, should return no matches.");
    test({CreateMatch(100, omnibox::GROUP_TRENDS_ENTITY_CHIPS),
          CreateMatch(99, omnibox::GROUP_PERSONALIZED_ZERO_SUGGEST),
          CreateMatch(98, omnibox::GROUP_PERSONALIZED_ZERO_SUGGEST)},
         {});
  }
}

// Tests the behavior when DesktopNTPZpsSection and
// DesktopSecondaryNTPZpsSection are both created.
TEST(AutocompleteGrouperSectionsTest,
     DesktopNTPZpsSectionAndDesktopSecondaryNTPZpsSection) {
  auto test = [](ACMatches matches, std::vector<int> expected_relevances,
                 bool trends_has_default_side_type = true) {
    PSections sections;
    const auto group1 = omnibox::GROUP_PERSONALIZED_ZERO_SUGGEST;
    const auto group2 = omnibox::GROUP_TRENDS;
    const auto group3 = omnibox::GROUP_PREVIOUS_SEARCH_RELATED_ENTITY_CHIPS;
    const auto group4 = omnibox::GROUP_PREVIOUS_SEARCH_RELATED;
    omnibox::GroupConfigMap group_configs;
    group_configs[group1];
    group_configs[group2];
    if (!trends_has_default_side_type) {
      group_configs[group2].set_side_type(
          omnibox::GroupConfig_SideType_SECONDARY);
    }
    group_configs[group3].set_side_type(
        omnibox::GroupConfig_SideType_SECONDARY);
    group_configs[group4].set_side_type(
        omnibox::GroupConfig_SideType_SECONDARY);
    sections.push_back(
        std::make_unique<DesktopNTPZpsSection>(group_configs, 8u, false));
    sections.push_back(
        std::make_unique<DesktopSecondaryNTPZpsSection>(group_configs));
    auto out_matches = Section::GroupMatches(std::move(sections), matches);
    VerifyMatches(out_matches, expected_relevances);
  };
  {
    SCOPED_TRACE(
        "Given 8 psuggest matches, and trending matches with a secondary side "
        "type, display psuggest matches on LHS and trending on the RHS.");
    test(
        {
            CreateMatch(100, omnibox::GROUP_PERSONALIZED_ZERO_SUGGEST),
            CreateMatch(99, omnibox::GROUP_PERSONALIZED_ZERO_SUGGEST),
            CreateMatch(98, omnibox::GROUP_PERSONALIZED_ZERO_SUGGEST),
            CreateMatch(97, omnibox::GROUP_PERSONALIZED_ZERO_SUGGEST),
            CreateMatch(96, omnibox::GROUP_PERSONALIZED_ZERO_SUGGEST),
            CreateMatch(95, omnibox::GROUP_PERSONALIZED_ZERO_SUGGEST),
            CreateMatch(94, omnibox::GROUP_PERSONALIZED_ZERO_SUGGEST),
            CreateMatch(93, omnibox::GROUP_PERSONALIZED_ZERO_SUGGEST),
            CreateMatch(91, omnibox::GROUP_TRENDS),
            CreateMatch(90, omnibox::GROUP_TRENDS),
            CreateMatch(89, omnibox::GROUP_TRENDS),
            CreateMatch(88, omnibox::GROUP_TRENDS),
        },
        {100, 99, 98, 97, 96, 95, 94, 93, 91, 90, 89, 88}, false);
  }
  {
    SCOPED_TRACE(
        "Given psuggests and trending suggestions with a default side type, "
        "display psuggest and trending on the LHS and entity chip suggestions "
        "on the RHS.");
    test(
        {
            CreateMatch(200, omnibox::GROUP_PERSONALIZED_ZERO_SUGGEST),
            CreateMatch(199, omnibox::GROUP_PERSONALIZED_ZERO_SUGGEST),
            CreateMatch(198, omnibox::GROUP_PERSONALIZED_ZERO_SUGGEST),
            CreateMatch(197, omnibox::GROUP_PERSONALIZED_ZERO_SUGGEST),
            CreateMatch(100, omnibox::GROUP_TRENDS),
            CreateMatch(99, omnibox::GROUP_TRENDS),
            CreateMatch(98, omnibox::GROUP_TRENDS),
            CreateMatch(97, omnibox::GROUP_TRENDS),
            CreateMatch(96, omnibox::GROUP_TRENDS),
            CreateMatch(92,
                        omnibox::GROUP_PREVIOUS_SEARCH_RELATED_ENTITY_CHIPS),
            CreateMatch(91,
                        omnibox::GROUP_PREVIOUS_SEARCH_RELATED_ENTITY_CHIPS),
            CreateMatch(90,
                        omnibox::GROUP_PREVIOUS_SEARCH_RELATED_ENTITY_CHIPS),
        },
        {200, 199, 198, 197, 100, 99, 98, 97, 92, 91, 90});
  }
}

// Tests the groups, limits, and rules for the Desktop SRP section with URL
// suggestions enabled.
TEST(AutocompleteGrouperSectionsTest, DesktopSRPZpsSectionWithUrls) {
  omnibox_feature_configs::ScopedConfigForTesting<
      omnibox_feature_configs::OmniboxZpsSuggestionLimit>
      scoped_config;
  scoped_config.Get().enabled = true;
  scoped_config.Get().max_search_suggestions = 4;
  scoped_config.Get().max_url_suggestions = 4;
  auto test = [](size_t action_count, ACMatches matches,
                 std::vector<int> expected_relevances) {
    PSections sections;
    omnibox::GroupConfigMap group_configs;
    group_configs[omnibox::GROUP_MOST_VISITED];
    group_configs[omnibox::GROUP_PREVIOUS_SEARCH_RELATED];
    // Max 8 suggestions, with an upper limit of 4 search suggestions.
    sections.push_back(std::make_unique<DesktopSRPZpsSection>(
        group_configs, 8u + action_count, 4u, 4u, action_count));
    auto out_matches = Section::GroupMatches(std::move(sections), matches);
    VerifyMatches(out_matches, expected_relevances);
  };
  {
    SCOPED_TRACE(
        "Given 12 srp zps matches, the group should respect the search "
        "suggestion limit");
    test(0,
         {
             CreateMatch(100, omnibox::GROUP_PREVIOUS_SEARCH_RELATED),
             CreateMatch(99, omnibox::GROUP_PREVIOUS_SEARCH_RELATED),
             CreateMatch(98, omnibox::GROUP_PREVIOUS_SEARCH_RELATED),
             CreateMatch(97, omnibox::GROUP_PREVIOUS_SEARCH_RELATED),
             CreateMatch(96, omnibox::GROUP_PREVIOUS_SEARCH_RELATED),
             CreateMatch(95, omnibox::GROUP_PREVIOUS_SEARCH_RELATED),
             CreateMatch(94, omnibox::GROUP_MOST_VISITED),
             CreateMatch(93, omnibox::GROUP_MOST_VISITED),
             CreateMatch(92, omnibox::GROUP_MOST_VISITED),
             CreateMatch(91, omnibox::GROUP_MOST_VISITED),
             CreateMatch(89, omnibox::GROUP_MOST_VISITED),
             CreateMatch(88, omnibox::GROUP_MOST_VISITED),
         },
         {100, 99, 98, 97, 94, 93, 92, 91});
  }
  {
    SCOPED_TRACE(
        "Given 12 srp zps matches, the group should respect the url suggestion "
        "limit");
    test(0,
         {
             CreateMatch(100, omnibox::GROUP_PREVIOUS_SEARCH_RELATED),
             CreateMatch(99, omnibox::GROUP_MOST_VISITED),
             CreateMatch(98, omnibox::GROUP_MOST_VISITED),
             CreateMatch(97, omnibox::GROUP_MOST_VISITED),
             CreateMatch(96, omnibox::GROUP_MOST_VISITED),
             CreateMatch(95, omnibox::GROUP_MOST_VISITED),
             CreateMatch(94, omnibox::GROUP_MOST_VISITED),
             CreateMatch(93, omnibox::GROUP_MOST_VISITED),
             CreateMatch(92, omnibox::GROUP_MOST_VISITED),
             CreateMatch(91, omnibox::GROUP_MOST_VISITED),
             CreateMatch(90, omnibox::GROUP_MOST_VISITED),
             CreateMatch(89, omnibox::GROUP_MOST_VISITED),
         },
         {100, 99, 98, 97, 96});
  }
  {
    SCOPED_TRACE("Contextual search action is excluded when disabled.");
    test(0,
         {
             CreateMatch(100, omnibox::GROUP_PREVIOUS_SEARCH_RELATED),
             CreateMatch(99, omnibox::GROUP_PREVIOUS_SEARCH_RELATED),
             CreateMatch(98, omnibox::GROUP_PREVIOUS_SEARCH_RELATED),
             CreateMatch(97, omnibox::GROUP_PREVIOUS_SEARCH_RELATED),
             CreateMatch(96, omnibox::GROUP_PREVIOUS_SEARCH_RELATED),
             CreateMatch(95, omnibox::GROUP_PREVIOUS_SEARCH_RELATED),
             CreateMatch(94, omnibox::GROUP_MOST_VISITED),
             CreateMatch(93, omnibox::GROUP_MOST_VISITED),
             CreateMatch(92, omnibox::GROUP_MOST_VISITED),
             CreateMatch(91, omnibox::GROUP_MOST_VISITED),
             CreateMatch(89, omnibox::GROUP_MOST_VISITED),
             CreateMatch(88, omnibox::GROUP_MOST_VISITED),
             CreateMatch(87, omnibox::GROUP_CONTEXTUAL_SEARCH_ACTION),
         },
         {100, 99, 98, 97, 94, 93, 92, 91});
  }
  {
    SCOPED_TRACE("Contextual search action is included when enabled.");
    test(1,
         {
             CreateMatch(100, omnibox::GROUP_PREVIOUS_SEARCH_RELATED),
             CreateMatch(99, omnibox::GROUP_PREVIOUS_SEARCH_RELATED),
             CreateMatch(98, omnibox::GROUP_PREVIOUS_SEARCH_RELATED),
             CreateMatch(97, omnibox::GROUP_PREVIOUS_SEARCH_RELATED),
             CreateMatch(96, omnibox::GROUP_PREVIOUS_SEARCH_RELATED),
             CreateMatch(95, omnibox::GROUP_PREVIOUS_SEARCH_RELATED),
             CreateMatch(94, omnibox::GROUP_MOST_VISITED),
             CreateMatch(93, omnibox::GROUP_MOST_VISITED),
             CreateMatch(92, omnibox::GROUP_MOST_VISITED),
             CreateMatch(91, omnibox::GROUP_MOST_VISITED),
             CreateMatch(89, omnibox::GROUP_MOST_VISITED),
             CreateMatch(88, omnibox::GROUP_MOST_VISITED),
             CreateMatch(87, omnibox::GROUP_CONTEXTUAL_SEARCH_ACTION),
         },
         {100, 99, 98, 97, 94, 93, 92, 91, 87});
  }
}

// Tests the groups, limits, and rules for the Desktop Web section with URL
// suggestions enabled.
TEST(AutocompleteGrouperSectionsTest, DesktopWebZpsSectionWithUrls) {
  omnibox_feature_configs::ScopedConfigForTesting<
      omnibox_feature_configs::OmniboxUrlSuggestionsOnFocus>
      scoped_config;
  scoped_config.Get().enabled = true;
  auto test = [](ACMatches matches, std::vector<int> expected_relevances,
                 bool trends_has_default_side_type = true) {
    PSections sections;
    omnibox::GroupConfigMap group_configs;
    group_configs[omnibox::GROUP_MOST_VISITED];
    group_configs[omnibox::GROUP_PREVIOUS_SEARCH_RELATED];
    group_configs[omnibox::GROUP_PERSONALIZED_ZERO_SUGGEST];
    // Max 4 url suggestions.
    sections.push_back(
        std::make_unique<DesktopWebURLZpsSection>(group_configs, 4u));
    // Max 4 suggestions, with an upper limit of 4 contextual search
    // suggestions and no contextual actions.
    sections.push_back(std::make_unique<DesktopWebSearchZpsSection>(
        group_configs, /*limit=*/4u, /*contextual_action_limit=*/0u,
        /*contextual_search_limit=*/4u));
    auto out_matches = Section::GroupMatches(std::move(sections), matches);
    VerifyMatches(out_matches, expected_relevances);
  };
  {
    SCOPED_TRACE(
        "Given 12 web zps matches, the group should respect the url "
        "limit as well as show them first in the suggestion list.");
    test(
        {
            CreateMatch(100, omnibox::GROUP_MOST_VISITED),
            CreateMatch(99, omnibox::GROUP_MOST_VISITED),
            CreateMatch(98, omnibox::GROUP_MOST_VISITED),
            CreateMatch(97, omnibox::GROUP_MOST_VISITED),
            CreateMatch(96, omnibox::GROUP_MOST_VISITED),
            CreateMatch(95, omnibox::GROUP_MOST_VISITED),
            CreateMatch(94, omnibox::GROUP_VISITED_DOC_RELATED),
            CreateMatch(93, omnibox::GROUP_VISITED_DOC_RELATED),
            CreateMatch(92, omnibox::GROUP_VISITED_DOC_RELATED),
            CreateMatch(91, omnibox::GROUP_PERSONALIZED_ZERO_SUGGEST),
            CreateMatch(90, omnibox::GROUP_PERSONALIZED_ZERO_SUGGEST),
            CreateMatch(89, omnibox::GROUP_PERSONALIZED_ZERO_SUGGEST),
        },
        {100, 99, 98, 97, 94, 93, 92, 91});
  }
  {
    SCOPED_TRACE(
        "Given 12 web zps matches, if there aren't enough search suggestions, "
        "url suggestions should not take their place. Instead less "
        "overall suggestions should be shown.");
    test(
        {
            CreateMatch(100, omnibox::GROUP_VISITED_DOC_RELATED),
            CreateMatch(99, omnibox::GROUP_PERSONALIZED_ZERO_SUGGEST),
            CreateMatch(98, omnibox::GROUP_MOST_VISITED),
            CreateMatch(97, omnibox::GROUP_MOST_VISITED),
            CreateMatch(96, omnibox::GROUP_MOST_VISITED),
            CreateMatch(95, omnibox::GROUP_MOST_VISITED),
            CreateMatch(94, omnibox::GROUP_MOST_VISITED),
            CreateMatch(93, omnibox::GROUP_MOST_VISITED),
            CreateMatch(91, omnibox::GROUP_MOST_VISITED),
            CreateMatch(90, omnibox::GROUP_MOST_VISITED),
            CreateMatch(89, omnibox::GROUP_MOST_VISITED),
            CreateMatch(88, omnibox::GROUP_MOST_VISITED),
        },
        {98, 97, 96, 95, 100, 99});
  }
}

TEST(AutocompleteGrouperSectionsTest, DesktopWebZpsWithActionsSection) {
  auto test = [](ACMatches matches, std::vector<int> expected_relevances) {
    PSections sections;
    omnibox::GroupConfigMap group_configs;
    // Max 3 suggestions, with an upper limit of 3 url suggestions.
    sections.push_back(
        std::make_unique<DesktopWebURLZpsSection>(group_configs, 3u));
    // Max 3 suggestions, with an upper limit of 3 contextual search
    // suggestions and one contextual action.
    sections.push_back(std::make_unique<DesktopWebSearchZpsSection>(
        group_configs, /*limit=*/3u, /*contextual_action_limit=*/1u,
        /*contextual_search_limit=*/3u));
    auto out_matches = Section::GroupMatches(std::move(sections), matches);
    VerifyMatches(out_matches, expected_relevances);
  };
  {
    SCOPED_TRACE("ZPS action matches group before contextual search matches");
    test(
        {
            CreateMatch(300, omnibox::GROUP_CONTEXTUAL_SEARCH_ACTION),
            CreateMatch(299, omnibox::GROUP_CONTEXTUAL_SEARCH_ACTION),
            CreateMatch(200, omnibox::GROUP_CONTEXTUAL_SEARCH),
            CreateMatch(199, omnibox::GROUP_CONTEXTUAL_SEARCH),
            CreateMatch(100, omnibox::GROUP_MOST_VISITED),
            CreateMatch(99, omnibox::GROUP_MOST_VISITED),
            CreateMatch(98, omnibox::GROUP_MOST_VISITED),
            CreateMatch(97, omnibox::GROUP_MOST_VISITED),
            CreateMatch(96, omnibox::GROUP_MOST_VISITED),
            CreateMatch(95, omnibox::GROUP_MOST_VISITED),
            CreateMatch(94, omnibox::GROUP_VISITED_DOC_RELATED),
            CreateMatch(93, omnibox::GROUP_VISITED_DOC_RELATED),
            CreateMatch(92, omnibox::GROUP_VISITED_DOC_RELATED),
            CreateMatch(91, omnibox::GROUP_PERSONALIZED_ZERO_SUGGEST),
            CreateMatch(90, omnibox::GROUP_PERSONALIZED_ZERO_SUGGEST),
            CreateMatch(89, omnibox::GROUP_PERSONALIZED_ZERO_SUGGEST),
        },
        // 3 URLs, 1 action, 2 contextual searches.
        {100, 99, 98, 300, 200, 199});
  }
}

TEST(AutocompleteGrouperSectionsTest, DesktopWebZpsNoContextualSuggestions) {
  auto test = [](ACMatches matches, std::vector<int> expected_relevances) {
    PSections sections;
    omnibox::GroupConfigMap group_configs;
    sections.push_back(
        std::make_unique<DesktopWebURLZpsSection>(group_configs, 3u));
    sections.push_back(std::make_unique<DesktopWebSearchZpsSection>(
        group_configs, /*limit=*/4u, /*contextual_action_limit=*/1u,
        /*contextual_search_limit=*/0u));
    auto out_matches = Section::GroupMatches(std::move(sections), matches);
    VerifyMatches(out_matches, expected_relevances);
  };
  {
    SCOPED_TRACE("ZPS contextual search matches limit 0");
    test(
        {
            CreateMatch(99, omnibox::GROUP_MOST_VISITED),
            CreateMatch(98, omnibox::GROUP_VISITED_DOC_RELATED),
            CreateMatch(97, omnibox::GROUP_PERSONALIZED_ZERO_SUGGEST),
            CreateMatch(96, omnibox::GROUP_CONTEXTUAL_SEARCH_ACTION),
            CreateMatch(95, omnibox::GROUP_CONTEXTUAL_SEARCH),
            CreateMatch(94, omnibox::GROUP_CONTEXTUAL_SEARCH_ACTION),
            CreateMatch(93, omnibox::GROUP_CONTEXTUAL_SEARCH),
        },
        // URLs, then searches, then one action, stable sorted.
        // No contextual search matches due to above configuration.
        {99, 98, 97, 96});
  }
}

TEST(AutocompleteGrouperSectionsTest, DesktopWebZpsContextualSuggestionsOnly) {
  auto test = [](ACMatches matches, std::vector<int> expected_relevances) {
    PSections sections;
    omnibox::GroupConfigMap group_configs;
    sections.push_back(
        std::make_unique<DesktopWebSearchZpsContextualOnlySection>(
            group_configs, /*contextual_action_limit=*/1u,
            /*contextual_search_limit=*/3u));
    auto out_matches = Section::GroupMatches(std::move(sections), matches);
    VerifyMatches(out_matches, expected_relevances);
  };
  {
    SCOPED_TRACE("ZPS contextual search matches only");
    test(
        {
            CreateMatch(99, omnibox::GROUP_MOST_VISITED),
            CreateMatch(98, omnibox::GROUP_VISITED_DOC_RELATED),
            CreateMatch(97, omnibox::GROUP_PERSONALIZED_ZERO_SUGGEST),
            CreateMatch(96, omnibox::GROUP_CONTEXTUAL_SEARCH_ACTION),
            CreateMatch(95, omnibox::GROUP_CONTEXTUAL_SEARCH),
            CreateMatch(94, omnibox::GROUP_CONTEXTUAL_SEARCH_ACTION),
            CreateMatch(93, omnibox::GROUP_CONTEXTUAL_SEARCH),
        },
        // Nothing but contextual action and contextual search matches.
        {96, 95, 93});
  }
  {
    SCOPED_TRACE("ZPS contextual search matches with cross-device tab");
    test(
        {
            CreateMatch(100, omnibox::GROUP_CROSS_DEVICE_TABS),
            CreateMatch(99, omnibox::GROUP_MOST_VISITED),
            CreateMatch(98, omnibox::GROUP_VISITED_DOC_RELATED),
            CreateMatch(97, omnibox::GROUP_PERSONALIZED_ZERO_SUGGEST),
            CreateMatch(96, omnibox::GROUP_CONTEXTUAL_SEARCH_ACTION),
            CreateMatch(95, omnibox::GROUP_CONTEXTUAL_SEARCH),
            CreateMatch(94, omnibox::GROUP_CONTEXTUAL_SEARCH_ACTION),
            CreateMatch(93, omnibox::GROUP_CONTEXTUAL_SEARCH),
        },
        // Cross-device tab at the top, followed by contextual matches.
        {100, 96, 95, 93});
  }
}

TEST(AutocompleteGrouperSectionsTest, DesktopComposeboxZpsSection) {
  auto test = [](ACMatches matches, std::vector<int> expected_relevances) {
    PSections sections;
    omnibox::GroupConfigMap group_configs;
    sections.push_back(std::make_unique<DesktopComposeboxZpsSection>(
        group_configs, /*max_suggestions=*/5u,
        /*max_aim_suggestions=*/5u,
        /*max_contextual_suggestions=*/5u));
    auto out_matches = Section::GroupMatches(std::move(sections), matches);
    VerifyMatches(out_matches, expected_relevances);
  };
  {
    SCOPED_TRACE("ZPS personalized zero suggest and aim suggestions only");
    test(
        {
            CreateMatch(103, omnibox::GROUP_PERSONALIZED_ZERO_SUGGEST),
            CreateMatch(102, omnibox::GROUP_PERSONALIZED_ZERO_SUGGEST),
            CreateMatch(101, omnibox::GROUP_MIA_RECOMMENDATIONS),
            CreateMatch(99, omnibox::GROUP_MOST_VISITED),
            CreateMatch(98, omnibox::GROUP_VISITED_DOC_RELATED),
            CreateMatch(97, omnibox::GROUP_PERSONALIZED_ZERO_SUGGEST),
            CreateMatch(96, omnibox::GROUP_MIA_RECOMMENDATIONS),
            CreateMatch(95, omnibox::GROUP_CONTEXTUAL_SEARCH),
            CreateMatch(94, omnibox::GROUP_CONTEXTUAL_SEARCH_ACTION),
            CreateMatch(93, omnibox::GROUP_CONTEXTUAL_SEARCH),
        },
        // Nothing but personalized zero suggest and aim recommendations.
        {103, 102, 97, 101, 96});
  }
  {
    SCOPED_TRACE("All contextual search with one verbatim match");
    test(
        {
            CreateMatch(96, omnibox::GROUP_SEARCH),
            CreateMatch(95, omnibox::GROUP_CONTEXTUAL_SEARCH),
            CreateMatch(93, omnibox::GROUP_CONTEXTUAL_SEARCH),
        },
        // Group search gets in along with contextual searches.
        {96, 95, 93});
  }
  {
    SCOPED_TRACE("ZPS rich image action matches only");
    test(
        {
            CreateMatch(99, omnibox::GROUP_RICH_IMAGE_ACTION),
            CreateMatch(98, omnibox::GROUP_RICH_IMAGE_ACTION),
            CreateMatch(97, omnibox::GROUP_RICH_IMAGE_ACTION),
        },
        // All rich image actions get in, up to `max_aim_suggestions`.
        {99, 98, 97});
  }
  {
    SCOPED_TRACE("Rich image actions are grouped after all other groups");
    test(
        {
            CreateMatch(100, omnibox::GROUP_RICH_IMAGE_ACTION),
            CreateMatch(99, omnibox::GROUP_SEARCH),
            CreateMatch(98, omnibox::GROUP_PERSONALIZED_ZERO_SUGGEST),
            CreateMatch(97, omnibox::GROUP_AI_MODE_ZERO_SUGGEST_CANNED),
            CreateMatch(96, omnibox::GROUP_CONTEXTUAL_SEARCH),
        },
        // Rich image actions are last despite having the highest relevance.
        {99, 98, 97, 96, 100});
  }
  {
    SCOPED_TRACE("Rich image actions are dropped once the section is full");
    test(
        {
            CreateMatch(99, omnibox::GROUP_SEARCH),
            CreateMatch(98, omnibox::GROUP_PERSONALIZED_ZERO_SUGGEST),
            CreateMatch(97, omnibox::GROUP_PERSONALIZED_ZERO_SUGGEST),
            CreateMatch(96, omnibox::GROUP_PERSONALIZED_ZERO_SUGGEST),
            CreateMatch(95, omnibox::GROUP_PERSONALIZED_ZERO_SUGGEST),
            CreateMatch(94, omnibox::GROUP_RICH_IMAGE_ACTION),
        },
        // `max_suggestions` is reached before the rich image action is added.
        {99, 98, 97, 96, 95});
  }
}

// Test that (on Android) sections are grouped by Search vs URL.
