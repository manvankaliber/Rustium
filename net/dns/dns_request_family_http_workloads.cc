// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license in LICENSE.
#include "net/dns/dns_request_family_http_workloads.h"

namespace net::dns_request_family_test {
DnsFixtureHttpJob::DnsFixtureHttpJob(URLRequest* request, std::shared_ptr<HttpRow> row)
    : URLRequestJob(request), row_(std::move(row)) {}
DnsFixtureHttpJob::~DnsFixtureHttpJob() = default;
void DnsFixtureHttpJob::SetUpload(UploadDataStream* upload) {upload_=upload;}
void DnsFixtureHttpJob::Start() {
    ++row_->started;
    std::string wire;
    if(row_->post){
      CHECK_EQ(request()->method(),"POST");CHECK(upload_);
      CHECK_EQ(upload_->Init(CompletionOnceCallback(),NetLogWithSource()),OK);
      CHECK(upload_->IsInMemory());
      auto buffer=base::MakeRefCounted<IOBufferWithSize>(upload_->size());
      CHECK_EQ(upload_->Read(buffer.get(),buffer->size(),CompletionOnceCallback()),static_cast<int>(upload_->size()));
      wire=base::as_string_view(buffer->span());
    }else{
      CHECK_EQ(request()->method(),"GET");std::string encoded;
      CHECK(GetValueForKeyInQuery(request()->url(),"dns",&encoded));
      CHECK(base::Base64UrlDecode(encoded,base::Base64UrlDecodePolicy::IGNORE_PADDING,&wire));
    }
    CHECK_EQ(wire,row_->expected_query);++row_->matched;
    if(row_->hang){return;}
    base::SingleThreadTaskRunner::GetCurrentDefault()->PostTask(FROM_HERE,
        base::BindOnce(&DnsFixtureHttpJob::StartAsync,weak_factory_.GetWeakPtr()));
  }
int DnsFixtureHttpJob::ReadRawData(IOBuffer* buffer,int size) {
    CHECK_GE(size,0);
    if(offset_==row_->body.size()){++row_->eof;return 0;}
    const size_t amount=std::min(static_cast<size_t>(size),row_->body.size()-offset_);
    buffer->span().copy_prefix_from(base::as_byte_span(row_->body).subspan(offset_,amount));
    offset_+=amount;return static_cast<int>(amount);
  }
void DnsFixtureHttpJob::GetResponseInfo(HttpResponseInfo* info) {
    const std::string headers="HTTP/1.1 "+std::to_string(row_->status)+" Fixture\nContent-Type: "+row_->mime+
        "\nContent-Length: "+std::to_string(row_->body.size())+"\n";
    info->headers=base::MakeRefCounted<HttpResponseHeaders>(HttpUtil::AssembleRawHeaders(headers));
  }


DnsFixtureInterceptor::DnsFixtureInterceptor(std::shared_ptr<HttpRow> row)
    : row_(std::move(row)) {}
DnsFixtureInterceptor::~DnsFixtureInterceptor() = default;
std::unique_ptr<URLRequestJob> DnsFixtureInterceptor::MaybeInterceptRequest(URLRequest* request) const {
    return std::make_unique<DnsFixtureHttpJob>(request,row_);
  }

}  // namespace net
