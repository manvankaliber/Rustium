// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license.
// Prepared paired lifecycle harness. Only --timing enables duration measurement.
#include "net/cookies/cookie_family_store_test_support.h"
#include <cstdio>
#include <cstring>
#include "base/at_exit.h"
#include "base/command_line.h"
#include "base/test/allow_check_is_test_for_testing.h"
namespace {
using namespace net::cookie_family_store_test;
constexpr size_t kLifecycles=4,kPairs=20;
volatile uint64_t consumed;
template<class Family> double Sample(const Case& scenario,const Inputs& input,base::test::TaskEnvironment& env,uint64_t seed) {
  const auto start=base::TimeTicks::Now();
  for(size_t i=0;i<kLifecycles;++i)seed=Run<Family>(scenario,input,env).Digest(seed+i);
  const auto elapsed=base::TimeTicks::Now()-start;consumed=seed;
  return static_cast<double>(elapsed.InNanoseconds())/kLifecycles;
}
}
int main(int argc,char** argv) {
  base::AtExitManager at_exit;base::CommandLine::Init(argc,argv);base::test::AllowCheckIsTestForTesting();
  bool timing=false,verify_only=false;
  for(int i=1;i<argc;++i){if(!std::strcmp(argv[i],"--timing"))timing=true;
    else{CHECK(!std::strcmp(argv[i],"--verify-only"));verify_only=true;}}
  // Verification uses a fixed business clock; timed samples keep system time.
  const bool measure = timing && !verify_only;
  base::test::TaskEnvironment env(measure
      ? base::test::TaskEnvironment::TimeSource::SYSTEM_TIME
      : base::test::TaskEnvironment::TimeSource::MOCK_TIME);
  if (!measure) {
    const auto fixed = base::Time::UnixEpoch() + base::Days(20000);
    CHECK_LE(base::Time::Now(), fixed);
    env.AdvanceClock(fixed - base::Time::Now());
  }
  const Inputs input{base::Time::Now()};
  std::fprintf(stderr,"{\"fixture_creation_internal_us\":%lld,\"order_seed\":20261113}\n",static_cast<long long>(input.creation.ToInternalValue()));
  for(const auto& scenario:kCases){const auto cpp=Run<Original>(scenario,input,env),rust=Run<Production>(scenario,input,env);
    CHECK(cpp==rust)<<scenario.id;
    std::fprintf(stderr,"{\"verify_case\":\"%s\",\"checksum\":\"%016llx\"}\n",scenario.id,static_cast<unsigned long long>(cpp.Digest(1469598103934665603ULL)));}
  if(!timing||verify_only)return 0;
  uint64_t order=20261113;
  for(const auto& scenario:kCases)for(size_t pair=0;pair<kPairs;++pair){order^=order<<13;order^=order>>7;order^=order<<17;
    const auto seed=1469598103934665603ULL+pair;double cpp,rust;
    if(order&1){cpp=Sample<Original>(scenario,input,env,seed);rust=Sample<Production>(scenario,input,env,seed);}
    else{rust=Sample<Production>(scenario,input,env,seed);cpp=Sample<Original>(scenario,input,env,seed);}
    std::printf("{\"case\":\"%s\",\"pair\":%zu,\"cpp_ns\":%.17g,\"rust_ns\":%.17g,\"lifecycles\":%zu,\"cpp_first\":%s}\n",
      scenario.id,pair,cpp,rust,kLifecycles,(order&1)?"true":"false");
  }
}
