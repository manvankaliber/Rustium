// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include "net/socket/connection_jobs_upper_rust_job_workloads.h"
#include <chrono>
#include <cstdio>
#include <random>
#include "base/at_exit.h"
#include "base/command_line.h"
#include "base/logging.h"
#include "base/logging/logging_settings.h"
#include "base/test/test_timeouts.h"
#include "base/compiler_specific.h"
#include "base/test/scoped_feature_list.h"
#include "net/base/features.h"
namespace net::connection_jobs_job_workloads {
namespace{
volatile uint64_t sink=0;
constexpr unsigned kIterations=32;
template<class Types>std::pair<double,uint64_t> Sample(Case kind,
 base::test::TaskEnvironment&environment,bool timing){
 Fixture<Types> fixture(kind,environment);fixture.Prepare(kIterations);
 // Branch before any measurement clock: verification never enters clocks.
 if(!timing){return{0,fixture.Run(kIterations,true)};}
 const auto start=std::chrono::steady_clock::now();const auto sum=fixture.Run(kIterations);
 const auto end=std::chrono::steady_clock::now();sink=sum;
 return{std::chrono::duration<double,std::nano>(end-start).count()/kIterations,sum};
}
}
}
int main(int argc,char**argv){
  base::AtExitManager at_exit;
  base::CommandLine::Init(argc, argv);
  if (!logging::InitLogging({.logging_dest = logging::LOG_TO_STDERR})) {
    return 1;
  }
  TestTimeouts::Initialize();
 const auto*command=base::CommandLine::ForCurrentProcess();
 const bool timing=command->HasSwitch("timing"),verify=command->HasSwitch("verify-only");
 CHECK(timing!=verify)<<"Use exactly one of --verify-only or --timing";
 base::test::TaskEnvironment environment{base::test::TaskEnvironment::TimeSource::MOCK_TIME};
 base::test::ScopedFeatureList features;
 features.InitAndDisableFeature(net::features::kHappyEyeballsV2);
 using namespace net::connection_jobs_job_workloads;
 for(unsigned i=0;i<kNames.size();++i){
  CHECK(Supported(static_cast<Case>(i)))<<"All16 workloads required; QUIC proxy build flag absent. Record config gap, do not time a reduced set.";
  CHECK_EQ(Sample<Original>(static_cast<Case>(i),environment,false).second,
           Sample<Production>(static_cast<Case>(i),environment,false).second);
 }
 if(verify){std::puts("Verified16 connected job lifecycles; no measurement clock reads");return 0;}
 std::mt19937 random(20261113);
 for(unsigned i=0;i<kNames.size();++i){for(unsigned pair=0;pair<20;++pair){
  const auto kind=static_cast<Case>(i);const bool rust_first=(random()&1)!=0;
  std::pair<double,uint64_t>cpp,rust;
  if(rust_first){rust=Sample<Production>(kind,environment,true);cpp=Sample<Original>(kind,environment,true);}
  else{cpp=Sample<Original>(kind,environment,true);rust=Sample<Production>(kind,environment,true);}
  CHECK_EQ(cpp.second,rust.second);
  // SAFETY: static NUL-terminated format/case names and correctly typed scalar args.
  UNSAFE_BUFFERS(std::printf("{\"case\":\"%s\",\"repeat\":%u,\"iterations\":32,\"rust_first\":%s,"
   "\"baseline_ns\":%.9f,\"rust_ns\":%.9f,\"baseline_checksum\":%llu,\"rust_checksum\":%llu}\n",
   kNames.at(i),pair,rust_first?"true":"false",cpp.first,rust.first,
   static_cast<unsigned long long>(cpp.second),static_cast<unsigned long long>(rust.second)));
 }}return 0;
}
