// Copyright 2026 The Chromium Authors
// BSD-style license; see LICENSE.
#include "net/dns/dns_resolution_family_rust_test_support.h"
#include <chrono>
#include <cstdio>
#include <random>
#include <string_view>
#include "base/at_exit.h"
#include "base/command_line.h"
#include "base/logging.h"
#include "base/logging/logging_settings.h"
#include "base/test/test_timeouts.h"
#include "base/compiler_specific.h"
#include "base/test/task_environment.h"
#include "net/test/test_with_task_environment.h"
#include "net/base/mock_network_change_notifier.h"
namespace net::dns_resolution_family_test {
namespace {
volatile uint64_t sink=0;
template<class Impl>double Sample(size_t kind,unsigned count){
  Fixture<Impl> fixture(kind);  // All manager/context/cache preload outside sample.
  const auto start=std::chrono::steady_clock::now();
  const uint64_t result=fixture.Run(count);
  const auto finish=std::chrono::steady_clock::now();sink=result;
  // Destruction is outside timing; each request's own destruction is inside Run.
  return std::chrono::duration<double,std::nano>(finish-start).count()/count;
}
void Run(bool timing){
  NetTaskEnvironment environment(base::test::TaskEnvironment::MainThreadType::IO, base::test::TaskEnvironment::TimeSource::MOCK_TIME);
  test::ScopedMockNetworkChangeNotifier notifier;std::mt19937 random(20261113);
  constexpr unsigned count=512;
  for(size_t kind=0;kind<kCases.size();++kind){
    {Fixture<Original> a(kind);Fixture<Production>b(kind);CHECK_EQ(a.Run(8),b.Run(8));}
    if(!timing)continue;
    for(unsigned pair=0;pair<20;++pair){
      double cpp=0,rust=0;const bool rust_first=(random()&1)!=0;
      if(rust_first){rust=Sample<Production>(kind,count);cpp=Sample<Original>(kind,count);}
      else{cpp=Sample<Original>(kind,count);rust=Sample<Production>(kind,count);}
      UNSAFE_BUFFERS(std::printf("{\"case\":\"%s\",\"repeat\":%u,\"baseline_ns\":%.9f,\"rust_ns\":%.9f,\"iterations\":%u,\"rust_first\":%s}\n",kCases.at(kind),pair,cpp,rust,count,rust_first?"true":"false"));
    }
  }
}
}
}
int main(int argc,char** argv){
  base::AtExitManager at_exit;
  base::CommandLine::Init(argc, argv);
  if (!logging::InitLogging({.logging_dest = logging::LOG_TO_STDERR})) {
    return 1;
  }
  TestTimeouts::Initialize();
  if(argc!=2)return 2;const std::string_view mode=UNSAFE_BUFFERS(argv[1]);
  if(mode!="--verify-only"&&mode!="--benchmark")return 2;
  net::dns_resolution_family_test::Run(mode=="--benchmark");return 0;
}
