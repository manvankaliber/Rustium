// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license.
// Prepared only. Parent runs after full-family integration in a quiet window.
#include "net/cookies/cookie_family_test_support.h"
#include <iomanip>
#include <iostream>
#include <string_view>
#include "base/at_exit.h"
#include "base/check.h"
#include "base/command_line.h"
#include "base/test/allow_check_is_test_for_testing.h"
#include "base/time/time.h"
namespace {
using namespace net::cookie_family_test;
constexpr size_t kLifecycles=64;
constexpr size_t kPairs=20;
volatile uint64_t consumed;
template<class Family> double Sample(const Case& input,uint64_t seed) {
  const auto start=base::TimeTicks::Now();
  for(size_t i=0;i<kLifecycles;++i) seed=Pipeline<Family>(input,seed+i);
  const auto elapsed=base::TimeTicks::Now()-start;
  consumed=seed;
  return static_cast<double>(elapsed.InNanoseconds())/kLifecycles;
}
void Verify() {
  for(const auto& input:kCases) {
    const auto cpp=Pipeline<Original>(input,1469598103934665603ULL);
    const auto rust=Pipeline<Production>(input,1469598103934665603ULL);
    CHECK_EQ(cpp,rust)<<input.id;
    std::cerr << "{\"verify_case\":\"" << input.id << "\",\"checksum\":\""
              << std::hex << std::setw(16) << std::setfill('0') << cpp
              << std::dec << std::setfill(' ') << "\"}\n";
  }
}
}
int main(int argc,char** argv) {
  base::AtExitManager at_exit;base::CommandLine::Init(argc,argv);
  base::test::AllowCheckIsTestForTesting();
  bool timing=false;bool verify_only=false;
  for(int i=1;i<argc;++i) {
    if(std::string_view(argv[i])=="--timing") timing=true;
    else {CHECK(std::string_view(argv[i])=="--verify-only");verify_only=true;}
  }
  Verify();
  // No timing helper is reached in the default/--verify-only mode.
  if(!timing||verify_only) return 0;
  uint64_t order=20261113;
  for(const auto& input:kCases) {
    for(size_t pair=0;pair<kPairs;++pair) {
      order^=order<<13;order^=order>>7;order^=order<<17;
      const uint64_t seed=1469598103934665603ULL+pair;
      double cpp,rust;
      if(order&1) {cpp=Sample<Original>(input,seed);rust=Sample<Production>(input,seed);}
      else {rust=Sample<Production>(input,seed);cpp=Sample<Original>(input,seed);}
    std::cout << "{\"case\":\"" << input.id
              << "\",\"pair\":" << pair << ",\"cpp_ns\":" << std::setprecision(17) << cpp
              << ",\"rust_ns\":" << rust << ",\"lifecycles\":" << kLifecycles
              << ",\"cpp_first\":" << ((order & 1) ? "true" : "false") << "}\n";
    }
  }
}
