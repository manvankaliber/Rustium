// Copyright 2016 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stdint.h>

#include <string>

#include "base/containers/span.h"
#include "base/strings/string_view_util.h"
#include "net/proxy_resolution/proxy_host_matching_rules.h"
#include "testing/libfuzzer/libfuzzer_base_wrappers.h"
#include "url/gurl.h"

// Entry point for LibFuzzer.
DEFINE_LLVM_FUZZER_TEST_ONE_INPUT_SPAN(base::span<const uint8_t> data) {
  // Don't waste time parsing if the input is too large
  // (https://crbug.com/813619). According to
  // //testing/libfuzzer/efficient_fuzzer.md setting max_len in the build
  // target is insufficient since AFL doesn't respect it.
  if (data.size() > 512) {
    return 0;
  }

  net::ProxyHostMatchingRules rules;
  rules.ParseFromString(std::string(base::as_string_view(data)));

  const GURL url(std::string(base::as_string_view(data)));
  static_cast<void>(rules.Matches(url));
  static_cast<void>(rules.Matches(url, true));
  static_cast<void>(rules.MatchesImplicitRules(url));
  net::ProxyHostMatchingRules copy(rules);
  static_cast<void>(copy == rules);
  rules.PrependRuleToBypassSimpleHostnames();
  rules.AddRulesToSubtractImplicit();
  static_cast<void>(rules.Matches(url));
  static_cast<void>(rules.ToString());
  if (!rules.rules().empty()) {
    rules.ReplaceRule(
        0, net::SchemeHostPortMatcherRule::FromUntrimmedRawString("*"));
  }
  rules.Clear();
  return 0;
}
