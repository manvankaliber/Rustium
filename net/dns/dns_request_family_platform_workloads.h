// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license in LICENSE.
#ifndef NET_DNS_DNS_REQUEST_FAMILY_PLATFORM_WORKLOADS_H_
#define NET_DNS_DNS_REQUEST_FAMILY_PLATFORM_WORKLOADS_H_
#include "net/dns/dns_request_family_async_workloads.h"
#include <algorithm>
#include "net/dns/dns_platform_attempt_factory.h"
#include "net/dns/dns_resolution_original_dns_platform_attempt_factory.h"
#include "net/log/net_log_values.h"
#include "net/log/net_log_values_rust_reference.h"
#include "base/task/single_thread_task_runner.h"
#include "base/memory/weak_ptr.h"
namespace net::dns_request_family_test {
struct PlatformProduction : Production {
  using Attempt=::net::DnsAttempt;
  using Provider=::net::DnsPlatformAttemptFactory;
  static constexpr bool kOriginal=false;
};
struct PlatformOriginal : Original {
  using Attempt=dns_resolution_original::DnsAttempt;
  using Provider=dns_resolution_original::DnsPlatformAttemptFactory;
  static constexpr bool kOriginal=true;
};
inline constexpr std::array<const char*,5> kPlatformCases{
  "dns_platform_async_success","dns_platform_sync_success",
  "dns_platform_server_failure","dns_platform_cancel_pending",
  "dns_platform_specific_network"};

template<class Api>
struct PlatformRow {
  explicit PlatformRow(unsigned kind):kind(kind){
    auto encoded=Api::Encode("host.test");CHECK(encoded);expected_name=*encoded;
    query=std::make_unique<typename Api::Query>(0,*encoded,dns_protocol::kTypeA);
    response=std::make_unique<typename Api::Response>(0,false,
        std::vector<typename Api::Record>{},std::vector<typename Api::Record>{},
        std::vector<typename Api::Record>{},std::optional<typename Api::Query>(*query));
  }
  unsigned kind,calls=0;
  handles::NetworkHandle network=handles::kInvalidNetworkHandle;
  std::vector<uint8_t> expected_name;
  std::unique_ptr<typename Api::Query> query;
  std::unique_ptr<typename Api::Response> response;
};
template<class Api>
class TypedPlatformAttempt final : public Api::Attempt {
 public:
  TypedPlatformAttempt(size_t server,std::shared_ptr<PlatformRow<Api>> row,
                        const NetLogWithSource& log)
      :Api::Attempt(server),row_(std::move(row)),log_(log),
       query_(std::move(row_->query)),response_(std::move(row_->response)){}
  int Start(CompletionOnceCallback callback) override{
    if(row_->kind==1){return OK;}
    pending_=true;callback_=std::move(callback);
    if(row_->kind!=3){base::SingleThreadTaskRunner::GetCurrentDefault()->PostTask(
        FROM_HERE,base::BindOnce(&TypedPlatformAttempt::Complete,weak_factory_.GetWeakPtr()));}
    return ERR_IO_PENDING;
  }
  const typename Api::Query* GetQuery() const override{return query_.get();}
  const typename Api::Response* GetResponse() const override{
    return !pending_&&row_->kind!=2?response_.get():nullptr;
  }
  base::Value GetRawResponseBufferForLog() const override{
    if constexpr(Api::kOriginal){return net_log_values_rust_reference::NetLogBinaryValue(response_->io_buffer()->data(),response_->io_buffer_size());}
    else{return NetLogBinaryValue(response_->io_buffer()->data(),response_->io_buffer_size());}
  }
  const NetLogWithSource& GetSocketNetLog() const override{return log_;}
  bool IsPending() const override{return pending_;}
 private:
  void Complete(){pending_=false;std::move(callback_).Run(row_->kind==2?ERR_DNS_SERVER_FAILURE:OK);}
  std::shared_ptr<PlatformRow<Api>> row_;
  NetLogWithSource log_;
  std::unique_ptr<typename Api::Query> query_;
  std::unique_ptr<typename Api::Response> response_;
  bool pending_=false;
  CompletionOnceCallback callback_;
  base::WeakPtrFactory<TypedPlatformAttempt> weak_factory_{this};
};
template<class Api>
class TypedPlatformFactory final : public Api::Provider {
 public:
  explicit TypedPlatformFactory(std::shared_ptr<PlatformRow<Api>> row):row_(std::move(row)){}
  std::unique_ptr<typename Api::Attempt> CreateDnsPlatformAttempt(size_t server,
      base::span<const uint8_t> name,uint16_t type,handles::NetworkHandle network,
      const NetLogWithSource& log) override{
    CHECK_EQ(type,dns_protocol::kTypeA);CHECK(std::ranges::equal(name,row_->expected_name));
    ++row_->calls;row_->network=network;
    return std::make_unique<TypedPlatformAttempt<Api>>(server,row_,log);
  }
 private:
  std::shared_ptr<PlatformRow<Api>> row_;
};

template<class Api>
uint64_t PlatformWorkload(unsigned kind,unsigned count,base::test::TaskEnvironment& environment,double* elapsed_ns=nullptr){
  uint64_t checksum=0;
  for(unsigned i=0;i<count;++i){
    auto row=std::make_shared<PlatformRow<Api>>(kind);
    for(uint8_t byte:row->expected_name){checksum=checksum*131+byte;}
    auto builder=CreateTestURLRequestContextBuilder();
    if constexpr(!Api::kOriginal){builder->set_dns_platform_attempt_factory(std::make_unique<TypedPlatformFactory<Api>>(row));}
    auto url_context=builder->Build();
    auto context=std::make_unique<typename Api::Context>(url_context.get(),false);
    if constexpr(Api::kOriginal){context->SetOriginalPlatformAttemptFactoryForTesting(std::make_unique<TypedPlatformFactory<Api>>(row));}
    DnsConfig config;config.nameservers={IPEndPoint(IPAddress(192,0,2,53),53)};config.attempts=1;
    auto session=base::MakeRefCounted<DnsSession>(config,base::BindRepeating([](int,int){return 0;}),nullptr);
    context->InvalidateCachesAndPerSessionData(session.get(),false);
    auto factory=Api::Factory::CreateFactory(session.get());
    bool completed=false;int result=ERR_IO_PENDING;base::RunLoop loop;
    {
      ActiveWindow window(elapsed_ns);
      auto transaction=factory->CreateTransaction("host.test.",dns_protocol::kTypeA,NetLogWithSource(),
          Api::Factory::AttemptMode::kPlatform,SecureDnsMode::kOff,
          kind==4?7:handles::kInvalidNetworkHandle,context.get(),true);
      transaction->Start(base::BindLambdaForTesting([&](int error,const typename Api::Response* response){
        CHECK(!completed);completed=true;result=error;checksum+=static_cast<uint32_t>(error);
        if(response){checksum+=response->question_count();}loop.Quit();
      }));
      CHECK(!completed);
      if(kind==3){environment.RunUntilIdle();transaction.reset();environment.RunUntilIdle();}
      else{loop.Run();transaction.reset();}
    }
    CHECK_EQ(row->calls,1u);CHECK_EQ(row->network,kind==4?7:handles::kInvalidNetworkHandle);
    if(kind==3){CHECK(!completed);}else{CHECK(completed);CHECK_EQ(result,kind==2?ERR_DNS_SERVER_FAILURE:OK);}
    factory.reset();context.reset();session.reset();url_context.reset();
    environment.FastForwardUntilNoTasksRemain();
  }
  return checksum;
}
}  // namespace net::dns_request_family_test
#endif
