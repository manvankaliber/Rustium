// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include "net/socket/connection_jobs_upper_rust_test_support.h"
#include <chrono>
#include <cstdio>
#include <random>
#include "base/at_exit.h"
#include "base/command_line.h"
#include "base/compiler_specific.h"
#include "base/test/scoped_feature_list.h"
#include "net/base/features.h"
namespace net::connection_jobs_upper_test {
namespace{
volatile uint64_t sink=0;
template<class Types>uint64_t Batch(Case kind){
  uint64_t result=0;for(unsigned i=0;i<256;++i){result=result*131+Run<Types>(kind);}return result;
}
template<class Types>std::pair<double,uint64_t> Measure(Case kind){
  const auto start=std::chrono::steady_clock::now();const auto sum=Batch<Types>(kind);
  const auto end=std::chrono::steady_clock::now();sink=sum;
  return{std::chrono::duration<double,std::nano>(end-start).count()/256,sum};
}
}
}
int main(int argc,char**argv){
  base::AtExitManager at_exit;base::CommandLine::Init(argc,argv);
  const auto*command=base::CommandLine::ForCurrentProcess();
  const bool timing=command->HasSwitch("timing"),verify=command->HasSwitch("verify-only");
  CHECK(timing!=verify)<<"Use exactly one of --verify-only or --timing";
  base::test::TaskEnvironment environment{base::test::TaskEnvironment::TimeSource::MOCK_TIME};
  base::test::ScopedFeatureList features;
  features.InitAndEnableFeature(net::features::kPartitionConnectionsByNetworkIsolationKey);
  using namespace net::connection_jobs_upper_test;
  for(unsigned i=0;i<kNames.size();++i){const auto kind=static_cast<Case>(i);
    CHECK_EQ(Run<Original>(kind,true),Run<Production>(kind,true));
    CHECK_EQ(Batch<Original>(kind),Batch<Production>(kind));}
  if(verify){std::puts("Verified21 upper-family public lifecycle workloads; no measurement clock reads");return 0;}
  std::mt19937 random(20261113);
  for(unsigned i=0;i<kNames.size();++i){for(unsigned pair=0;pair<20;++pair){
    const auto kind=static_cast<Case>(i);const bool rust_first=(random()&1)!=0;
    std::pair<double,uint64_t> cpp,rust;
    if(rust_first){rust=Measure<Production>(kind);cpp=Measure<Original>(kind);}
    else{cpp=Measure<Original>(kind);rust=Measure<Production>(kind);}
    CHECK_EQ(cpp.second,rust.second);
    // SAFETY: format and case strings are static NUL-terminated literals;
    // scalar arguments match their printf specifiers.
    UNSAFE_BUFFERS(std::printf("{\"case\":\"%s\",\"repeat\":%u,\"iterations\":256,\"rust_first\":%s,"
      "\"baseline_ns\":%.9f,\"rust_ns\":%.9f,\"baseline_checksum\":%llu,\"rust_checksum\":%llu}\n",
      kNames.at(i),pair,rust_first?"true":"false",cpp.first,rust.first,
      static_cast<unsigned long long>(cpp.second),static_cast<unsigned long long>(rust.second)));
  }}return 0;
}
