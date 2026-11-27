// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license in LICENSE.
#ifndef NET_DNS_DNS_REQUEST_FAMILY_ASYNC_WORKLOADS_H_
#define NET_DNS_DNS_REQUEST_FAMILY_ASYNC_WORKLOADS_H_

#include <array>
#include <chrono>
#include <memory>
#include <string>
#include <vector>
#include "base/check.h"
#include "base/functional/bind.h"
#include "base/memory/raw_ptr.h"
#include "base/run_loop.h"
#include "base/test/bind.h"
#include "base/test/task_environment.h"
#include "net/dns/dns_config.h"
#include "net/dns/dns_names_util.h"
#include "net/dns/dns_query.h"
#include "net/dns/dns_response.h"
#include "net/dns/dns_response_rust_names_reference.h"
#include "net/dns/dns_resolution_original_dns_transaction.h"
#include "net/dns/dns_resolution_original_resolve_context.h"
#include "net/dns/dns_resolution_rust_original_types.h"
#include "net/dns/dns_session.h"
#include "net/dns/dns_transaction.h"
#include "net/dns/resolve_context.h"
#include "net/socket/socket_test_util.h"
#include "net/url_request/url_request_context.h"
#include "net/url_request/url_request_test_util.h"
#include "net/url_request/url_request_context_builder.h"

namespace net::dns_request_family_test {
struct Production {
  using Query = ::net::DnsQuery;
  using Record = ::net::DnsResourceRecord;
  using Response = ::net::DnsResponse;
  using Factory = ::net::DnsTransactionFactory;
  using Context = ::net::ResolveContext;
  static auto Encode(std::string_view name) {
    return ::net::dns_names_util::DottedNameToNetwork(name, true);
  }
};
struct Original {
  using Query = dns_resolution_original::DnsQuery;
  using Record = dns_resolution_original::DnsResourceRecord;
  using Response = dns_resolution_original::DnsResponse;
  using Factory = dns_resolution_original::DnsTransactionFactory;
  using Context = dns_resolution_original::ResolveContext;
  static auto Encode(std::string_view name) {
    return ::net::dns_response_rust_names_reference::DottedNameToNetwork(name, true);
  }
};

inline constexpr std::array<const char*,10> kAsyncCases{
    "dns_udp_async_ok", "dns_udp_sync_ok", "dns_udp_nxdomain",
    "dns_udp_server_failure", "dns_udp_truncated_tcp_retry",
    "dns_suffix_nxdomain_then_success", "dns_cancel_pending",
    "dns_invalid_name_posted_error", "dns_target_network_error",
    "dns_transaction_survives_factory"};

// Only the requested execution window reads a harness clock. Fixture generation,
// context/session setup and post-verification remain outside measured windows.
class ActiveWindow {
 public:
  explicit ActiveWindow(double* elapsed):elapsed_(elapsed){if(elapsed_){begin_=Clock::now();}}
  ~ActiveWindow(){if(elapsed_){*elapsed_+=std::chrono::duration<double,std::nano>(Clock::now()-begin_).count();}}
 private:
  using Clock=std::chrono::steady_clock;
  raw_ptr<double> elapsed_;
  Clock::time_point begin_;
};

// Each implementation independently builds its typed query, expected write,
// synthetic response and context. No production MockDnsClient/response pointer
// is passed into the independent original factory. Native socket and URL
// ownership fixtures are explicit shared providers, with configured hashes.
template<class Api>
struct Wire {
  Wire(std::string_view name, bool tcp, int rcode, bool truncated,
       IoMode mode, bool pending) {
    auto encoded=Api::Encode(name); CHECK(encoded);
    query=std::make_unique<typename Api::Query>(0,*encoded,dns_protocol::kTypeA);
    auto bytes=query->io_buffer()->span();
    query_bytes.assign(bytes.begin(),bytes.end()); response=query_bytes;
    CHECK_GE(response.size(),12u);
    response.at(2)=static_cast<uint8_t>(0x81u|(truncated?0x02u:0u));
    response.at(3)=static_cast<uint8_t>(0x80u|rcode);
    if(tcp){
      const size_t q=query_bytes.size(),r=response.size(); CHECK_LE(q,65535u);CHECK_LE(r,65535u);
      query_length={static_cast<uint8_t>(q>>8),static_cast<uint8_t>(q)};
      response_length={static_cast<uint8_t>(r>>8),static_cast<uint8_t>(r)};
      writes.emplace_back(mode,base::span(query_length),0,0);
      reads.emplace_back(mode,base::span(response_length),0,0);
    }
    writes.emplace_back(mode,base::span(query_bytes),0,0);
    if(pending){reads.emplace_back(ASYNC,ERR_IO_PENDING);}
    else{reads.emplace_back(mode,base::span(response),0,0);}
    provider=std::make_unique<StaticSocketDataProvider>(reads,writes);
    provider->set_connect_data(MockConnect(SYNCHRONOUS,OK));
  }
  uint64_t InputHash() const {
    uint64_t hash=0;for(uint8_t byte:query_bytes){hash=hash*131+byte;}return hash;
  }
  std::unique_ptr<typename Api::Query> query;
  std::vector<uint8_t> query_bytes,response;
  std::array<uint8_t,2> query_length{},response_length{};
  std::vector<MockRead> reads;
  std::vector<MockWrite> writes;
  std::unique_ptr<StaticSocketDataProvider> provider;
};

template<class Api>
uint64_t AsyncWorkload(unsigned kind,unsigned count,
                       base::test::TaskEnvironment& environment,double* elapsed_ns=nullptr){
  uint64_t checksum=0;
  for(unsigned iteration=0;iteration<count;++iteration){
    MockClientSocketFactory socket_factory;
    auto builder=CreateTestURLRequestContextBuilder();
    builder->set_client_socket_factory_for_testing(&socket_factory);
    auto url_context=builder->Build();
    auto context=std::make_unique<typename Api::Context>(url_context.get(),false);
    DnsConfig config;config.nameservers={IPEndPoint(IPAddress(192,0,2,53),53)};
    config.attempts=1;config.fallback_period=base::Seconds(1);
    config.append_to_multi_label_name=true;
    if(kind==5){config.search={"suffix.test"};config.ndots=2;}
    auto session=base::MakeRefCounted<DnsSession>(config,
        base::BindRepeating([](int minimum,int maximum){CHECK_LE(minimum,0);CHECK_GE(maximum,0);return 0;}),nullptr);
    context->InvalidateCachesAndPerSessionData(session.get(),false);
    auto factory=Api::Factory::CreateFactory(session.get());
    const std::string name=kind==5?"two.labels":kind==7?"bad..host":"host.test.";
    std::vector<std::unique_ptr<Wire<Api>>> wires;
    auto add=[&](std::string_view wire_name,bool tcp,int rcode,bool truncated){
      auto wire=std::make_unique<Wire<Api>>(wire_name,tcp,rcode,truncated,
          kind==1?SYNCHRONOUS:ASYNC,kind==6);
      checksum+=wire->InputHash();socket_factory.AddSocketDataProvider(wire->provider.get());
      wires.push_back(std::move(wire));
    };
    if(kind!=7&&kind!=8){
      add(kind==5?"two.labels.suffix.test":"host.test",false,kind==2||kind==5?3:kind==3?2:0,kind==4);
      if(kind==4){add("host.test",true,0,false);}
      if(kind==5){add("two.labels",false,0,false);}
    }
    bool completed=false;int result=ERR_IO_PENDING;base::RunLoop loop;
    {
    ActiveWindow window(elapsed_ns);
    auto transaction=factory->CreateTransaction(name,dns_protocol::kTypeA,NetLogWithSource(),
        Api::Factory::AttemptMode::kClassic,SecureDnsMode::kOff,
        kind==8?7:handles::kInvalidNetworkHandle,context.get(),true);
    if(kind==9){factory.reset();}
    transaction->Start(base::BindLambdaForTesting([&](int error,const typename Api::Response* response){
      CHECK(!completed);completed=true;result=error;
      checksum+=static_cast<uint32_t>(error);
      if(response){checksum+=response->rcode()+response->answer_count()+1;}
      loop.Quit();
    }));
    CHECK(!completed); // Every transaction must complete asynchronously.
    if(kind==6){environment.RunUntilIdle();transaction.reset();environment.RunUntilIdle();}
    else{
      loop.Run();transaction.reset();
    }
    }
    if(kind==6){CHECK(!completed);}
    else{
      CHECK(completed);
      if(kind==2){CHECK_EQ(result,ERR_NAME_NOT_RESOLVED);}
      else if(kind==3){CHECK_EQ(result,ERR_DNS_SERVER_FAILURE);}
      else if(kind==7||kind==8){CHECK_EQ(result,ERR_INVALID_ARGUMENT);}
      else{CHECK_EQ(result,OK);}
      for(const auto& wire:wires){CHECK(wire->provider->AllWriteDataConsumed());CHECK(wire->provider->AllReadDataConsumed());}
    }
    factory.reset();context.reset();session.reset();url_context.reset();
    environment.FastForwardUntilNoTasksRemain();
  }
  return checksum;
}
}  // namespace net::dns_request_family_test
#endif
