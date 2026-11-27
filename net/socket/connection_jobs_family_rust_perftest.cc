// Source-only timing harness; no measurements have been collected.
#include <chrono>
#include "base/at_exit.h"
#include "base/command_line.h"
#include "base/logging.h"
#include "base/logging/logging_settings.h"
#include "base/test/test_timeouts.h"
#include "base/compiler_specific.h"
#include <cstdio>
#include <random>
#include <string_view>
#include "net/socket/connection_jobs_family_rust_test_support.h"
namespace net::connection_jobs_family_test {
volatile uint64_t sink;
template<class Traits>
std::pair<double,uint64_t> Measure(Fixture<Traits>& f,size_t kind,size_t iterations) {
 const auto start=std::chrono::steady_clock::now();
 const uint64_t checksum=Run(f,kind,iterations);
 const auto stop=std::chrono::steady_clock::now();
 sink=checksum;
 return {std::chrono::duration<double,std::nano>(stop-start).count()/iterations,checksum};
}
}
int main(int argc,char** argv) {
  base::AtExitManager at_exit;
  base::CommandLine::Init(argc, argv);
  if (!logging::InitLogging({.logging_dest = logging::LOG_TO_STDERR})) {
    return 1;
  }
  TestTimeouts::Initialize();
 using namespace net;using namespace net::connection_jobs_family_test;
 const bool verify=argc==2&&UNSAFE_BUFFERS(std::string_view(argv[1]))=="--verify-only";
 const bool timing=argc==2&&UNSAFE_BUFFERS(std::string_view(argv[1]))=="--timing";
 if(!verify&&!timing)return 2;
 base::test::TaskEnvironment tasks;
 std::mt19937 random(20261113);
 constexpr size_t iterations=64;
 for(size_t kind=0;kind<std::size(kCases);++kind) {
  base::test::ScopedFeatureList features;
  if(kind>=6&&kind<12)features.InitAndEnableFeature(features::kHappyEyeballsV2);
  else features.InitAndDisableFeature(features::kHappyEyeballsV2);
  Fixture<Original> original;Fixture<Production> rust;
  CHECK_EQ(Run(original,kind,3),Run(rust,kind,3));
  if(verify)continue;
  for(unsigned repeat=0;repeat<20;++repeat) {
   const bool rust_first=(random()&1)!=0;
   std::pair<double,uint64_t> cpp,rs;
   if(rust_first){rs=Measure(rust,kind,iterations);cpp=Measure(original,kind,iterations);}
   else{cpp=Measure(original,kind,iterations);rs=Measure(rust,kind,iterations);}
   CHECK_EQ(cpp.second,rs.second);
   UNSAFE_BUFFERS(std::printf(
    "{\"case\":\"%s\",\"repeat\":%u,\"cpp_ns\":%.9f,\"rust_ns\":%.9f,\"rust_first\":%s,\"checksum\":%llu}\n",
    kCases[kind],repeat,cpp.first,rs.first,rust_first?"true":"false",
    static_cast<unsigned long long>(rs.second)));
  }
 }
 if(verify)UNSAFE_BUFFERS(std::puts("Verified sixteen full-path fixtures; no timing."));
 return 0;
}
