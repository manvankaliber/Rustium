// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license in LICENSE.
#include "net/dns/dns_request_family_async_workloads.h"
#include "net/dns/dns_request_family_http_workloads.h"
#include "net/dns/dns_request_family_platform_workloads.h"
#include <chrono>
#include <cstdio>
#include <random>
#include <string_view>
#include "base/compiler_specific.h"
#include "base/at_exit.h"
#include "base/test/task_environment.h"
#include "net/test/test_with_task_environment.h"
namespace {
volatile uint64_t g_checksum=0;
}
int main(int argc,char** argv){
  if(argc!=2){return 2;}
  const std::string_view argument=UNSAFE_BUFFERS(argv[1]);
  if(argument!="--verify-only"&&argument!="--benchmark"){return 2;}
  base::AtExitManager at_exit;
  net::NetTaskEnvironment environment(base::test::TaskEnvironment::MainThreadType::IO, base::test::TaskEnvironment::TimeSource::MOCK_TIME);
  using namespace net::dns_request_family_test;
  std::mt19937 random(20261113);
  constexpr unsigned iterations=20,pairs=20;
  for(unsigned kind=0;kind<kAsyncCases.size()+kHttpCases.size()+kPlatformCases.size();++kind){
    const auto http_end=kAsyncCases.size()+kHttpCases.size();
    const char* name=kind<kAsyncCases.size()?kAsyncCases.at(kind):kind<http_end?kHttpCases.at(kind-kAsyncCases.size()):kPlatformCases.at(kind-http_end);
    auto work=[&](bool production,unsigned n,double* elapsed=nullptr){
      if(kind<kAsyncCases.size()){return production?AsyncWorkload<Production>(kind,n,environment,elapsed):AsyncWorkload<Original>(kind,n,environment,elapsed);}
      if(kind<http_end){return production?HttpWorkload<Production>(kind-kAsyncCases.size(),n,environment,elapsed):HttpWorkload<Original>(kind-kAsyncCases.size(),n,environment,elapsed);}
      return production?PlatformWorkload<PlatformProduction>(kind-http_end,n,environment,elapsed):PlatformWorkload<PlatformOriginal>(kind-http_end,n,environment,elapsed);
    };
    CHECK_EQ(work(true,1),work(false,1));
    if(argument=="--verify-only"){continue;}
    for(unsigned pair=0;pair<pairs;++pair){
      double cpp=0,rust=0;const bool rust_first=(random()&1)!=0;
      auto measure=[&](bool production){
        double elapsed=0;
        const auto checksum=work(production,iterations,&elapsed);g_checksum=checksum;
        return elapsed/iterations;
      };
      if(rust_first){rust=measure(true);cpp=measure(false);}else{cpp=measure(false);rust=measure(true);}
      UNSAFE_BUFFERS(std::printf("{\"case\":\"%s\",\"repeat\":%u,\"baseline_ns\":%.9f,\"rust_ns\":%.9f,\"iterations\":%u,\"rust_first\":%s}\n",name,pair,cpp,rust,iterations,rust_first?"true":"false"));
    }
  }
  return 0;
}
