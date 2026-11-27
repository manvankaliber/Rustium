// Copyright 2026 The Chromium Authors
// BSD-style license; see LICENSE.
// Draft ordinary fixture. Never compiled or executed by its writer.
#ifndef NET_DNS_DNS_RESOLUTION_FAMILY_RUST_TEST_SUPPORT_H_
#define NET_DNS_DNS_RESOLUTION_FAMILY_RUST_TEST_SUPPORT_H_
#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include "base/check_op.h"
#include "base/containers/flat_set.h"
#include "base/functional/bind.h"
#include "base/test/simple_test_clock.h"
#include "base/test/simple_test_tick_clock.h"
#include "net/base/host_port_pair.h"
#include "net/base/net_errors.h"
#include "net/base/network_anonymization_key.h"
#include "net/dns/host_cache.h"
#include "net/dns/host_resolver_manager.h"
#include "net/dns/resolve_context.h"
#include "net/dns/dns_resolution_original_host_resolver_manager.h"
#include "net/dns/dns_resolution_original_resolve_context.h"
#include "net/dns/dns_resolution_host_cache_rust_original.h"
#include "net/dns/dns_resolution_host_value_rust_original.h"

namespace net::dns_resolution_family_test {
inline constexpr std::array<const char*,15> kCases={
  "literal_ipv4","literal_ipv6","literal_canonical_alias","literal_wrong_family","literal_non_address",
  "localhost_dual","localhost_ipv4","localhost_ipv6","localhost_trailing_dot","empty_name",
  "oversize_name","cache_fresh","cache_negative","cache_stale","cache_disallowed"};
struct Production {
  using Manager=::net::HostResolverManager;using Context=::net::ResolveContext;using Cache=::net::HostCache;
  using Host=::net::HostResolver::Host;using Pair=::net::HostPortPair;
};
struct Original {
  using Manager=::net::dns_resolution_original::HostResolverManager;
  using Context=::net::dns_resolution_original::ResolveContext;
  using Cache=::net::dns_resolution_original::HostCache;
  using Host=::net::dns_resolution_original::DnsResolverHostOriginal;using Pair=::net::HostPortRustReference;
};
struct Outcome {
  int outer=0;int error=0;std::vector<IPEndPoint> endpoints;base::flat_set<std::string> aliases;
  bool operator==(const Outcome&)const=default;
  uint64_t Fingerprint()const {
    uint64_t hash=static_cast<uint32_t>(outer);hash=hash*131+static_cast<uint32_t>(error);
    for(const auto& endpoint:endpoints){hash=hash*131+endpoint.port();for(uint8_t byte:endpoint.address().bytes())hash=hash*131+byte;}
    for(const auto& alias:aliases)for(unsigned char byte:alias)hash=hash*131+byte;
    return hash;
  }
};
template<class Impl>class Fixture {
 public:
  using Cache=typename Impl::Cache;
  explicit Fixture(size_t kind):kind_(kind){
    tick_.SetNowTicks(base::TimeTicks()+base::Seconds(10000));
    wall_.SetNow(base::Time::FromSecondsSinceUnixEpoch(100000));
    HostResolver::ManagerOptions options;options.insecure_dns_mode=InsecureDnsMode::kDisabled;
    manager_=std::make_unique<typename Impl::Manager>(options,nullptr,nullptr);
    manager_->SetTickClockForTesting(&tick_);manager_->SetLastIPv6ProbeResultForTesting(true);
    manager_->SetIsHappyEyeballsV3Enabled(false);
    context_=std::make_unique<typename Impl::Context>(nullptr,true,wall_,tick_);
    manager_->RegisterResolveContext(context_.get());
    params_.source=HostResolverSource::LOCAL_ONLY;
    params_.cache_usage=HostResolver::ResolveHostParameters::CacheUsage::DISALLOWED;
    switch(kind_){
      case 0:host_="192.0.2.7";break;
      case 1:host_="2001:db8::7";break;
      case 2:host_="192.0.2.7";params_.include_canonical_name=true;break;
      case 3:host_="192.0.2.7";params_.dns_query_type=DnsQueryType::AAAA;break;
      case 4:host_="192.0.2.7";params_.dns_query_type=DnsQueryType::TXT;break;
      case 5:host_="localhost";break;
      case 6:host_="localhost";params_.dns_query_type=DnsQueryType::A;break;
      case 7:host_="localhost";params_.dns_query_type=DnsQueryType::AAAA;break;
      case 8:host_="localhost.";break;
      case 9:host_="";break;
      case 10:host_=std::string(4097,'a');break;
      default:{
        host_="cache.test";params_.dns_query_type=DnsQueryType::A;
        params_.cache_usage=kind_==13?HostResolver::ResolveHostParameters::CacheUsage::STALE_ALLOWED:
            kind_==14?HostResolver::ResolveHostParameters::CacheUsage::DISALLOWED:
            HostResolver::ResolveHostParameters::CacheUsage::ALLOWED;
        typename Cache::Key key(std::string(host_),DnsQueryType::A,0,HostResolverSource::ANY,NetworkAnonymizationKey(),handles::kInvalidNetworkHandle);
        if(kind_==12){
          typename Cache::Entry value(ERR_NAME_NOT_RESOLVED,Cache::Entry::SOURCE_DNS);
          context_->host_cache()->Set(key,value,tick_.NowTicks(),base::Hours(1));
        }else{
          typename Cache::Entry value(OK,std::vector<IPEndPoint>{{IPAddress(192,0,2,33),0}},base::flat_set<std::string>{"cache.test"},Cache::Entry::SOURCE_DNS);
          context_->host_cache()->Set(key,value,tick_.NowTicks()-(kind_==13?base::Hours(2):base::TimeDelta()),base::Hours(1));
        }break;
      }
    }
  }
  ~Fixture(){manager_->DeregisterResolveContext(context_.get());context_.reset();manager_.reset();}
  Outcome Once(){
    auto request=manager_->CreateRequest(typename Impl::Host(typename Impl::Pair(host_,443)),NetworkAnonymizationKey(),handles::kInvalidNetworkHandle,NetLogWithSource(),params_,context_.get());
    Outcome value;value.outer=request->Start(base::BindOnce([](int){CHECK(false)<<"local fixture must remain synchronous";}));
    CHECK_NE(ERR_IO_PENDING,value.outer);value.error=request->GetResolveErrorInfo().error;
    value.endpoints=request->GetAddressResults().endpoints();value.aliases=request->GetDnsAliasResults();return value;
  }
  uint64_t Run(unsigned count){uint64_t sum=0;for(unsigned i=0;i<count;++i)sum+=Once().Fingerprint();return sum;}
 private:
  size_t kind_;base::SimpleTestTickClock tick_;base::SimpleTestClock wall_;
  std::unique_ptr<typename Impl::Manager> manager_;std::unique_ptr<typename Impl::Context> context_;
  HostResolver::ResolveHostParameters params_;std::string host_;
};
}
#endif
