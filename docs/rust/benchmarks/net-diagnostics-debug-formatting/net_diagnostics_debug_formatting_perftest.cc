#include <chrono>
#include <cstdio>
#include <cstdint>
#include <random>
#include <string>
#include "net/base/load_flags_to_string.h"
#include "net/base/net_diagnostics_load_flags_reference.h"
#include "net/base/net_diagnostics_privacy_mode_reference.h"
#include "net/base/privacy_mode.h"
namespace {
volatile uint64_t sink=0;
struct Case { const char* key; int value; bool privacy; };
double Measure(Case c, bool rust, int iterations) { uint64_t sum=0; auto start=std::chrono::steady_clock::now(); for(int i=0;i<iterations;++i) { if(c.privacy) { const char* s=rust?net::PrivacyModeToDebugString(static_cast<net::PrivacyMode>(c.value)):net::net_diagnostics_privacy_mode_reference::PrivacyModeToDebugString(c.value); sum += static_cast<unsigned char>(s[0]); } else { std::string s=rust?net::LoadFlagsToString(c.value):net::net_diagnostics_load_flags_reference::LoadFlagsToString(c.value); sum += s.size()+static_cast<unsigned char>(s[0]); } } sink=sum; return std::chrono::duration<double,std::nano>(std::chrono::steady_clock::now()-start).count()/iterations; }
}
int main() { constexpr Case cases[]={{"normal",0,false},{"single",1,false},{"combined-small",0x7,false},{"combined-wide",0x81001,false},{"top-flag",0x80000,false},{"disabled",0,true},{"enabled",1,true},{"no-client-certs",2,true},{"partitioned",3,true}}; std::mt19937 rng(20261112); for(auto c:cases) { Measure(c,false,4096); Measure(c,true,4096); for(int repeat=0;repeat<20;++repeat) { double baseline,rust; bool rust_first=(rng()&1)!=0; if(rust_first){rust=Measure(c,true,1000000);baseline=Measure(c,false,1000000);}else{baseline=Measure(c,false,1000000);rust=Measure(c,true,1000000);} std::printf("{\"case\":\"%s\",\"repeat\":%d,\"baseline_ns\":%.9f,\"rust_ns\":%.9f,\"rust_first\":%s}\n",c.key,repeat,baseline,rust,rust_first?"true":"false"); } } }
