// Copyright 2012 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/spdy/spdy_http_stream.h"
#include "net/spdy/http2_family_rust_ffi.h"

#include <algorithm>
#include <list>
#include <string>
#include <string_view>
#include <utility>
#include <optional>
#include "base/notreached.h"

#include "base/check_op.h"
#include "base/containers/flat_set.h"
#include "base/feature_list.h"
#include "base/functional/bind.h"
#include "base/location.h"
#include "base/task/single_thread_task_runner.h"
#include "base/time/time.h"
#include "base/values.h"
#include "net/base/features.h"
#include "net/base/ip_endpoint.h"
#include "net/base/net_errors.h"
#include "net/base/upload_data_stream.h"
#include "net/http/http_request_headers.h"
#include "net/http/http_request_info.h"
#include "net/http/http_response_info.h"
#include "net/log/net_log_event_type.h"
#include "net/log/net_log_with_source.h"
#include "net/socket/next_proto.h"
#include "net/spdy/spdy_http_utils.h"
#include "net/spdy/spdy_session.h"
#include "net/third_party/quiche/src/quiche/http2/core/spdy_protocol.h"
#include "url/scheme_host_port.h"

namespace net {

// Align our request body with |kMaxSpdyFrameChunkSize| to prevent unexpected
// buffer chunking. This is 16KB - frame header size.
const size_t SpdyHttpStream::kRequestBodyBufferSize = kMaxSpdyFrameChunkSize;

SpdyHttpStream::SpdyHttpStream(const base::WeakPtr<SpdySession>& spdy_session,
                               NetLogSource source_dependency,
                               base::flat_set<std::string> dns_aliases)
    : MultiplexedHttpStream(
          std::make_unique<MultiplexedSessionHandle>(spdy_session)),
      spdy_session_(spdy_session),
      is_reused_(spdy_session_->IsReused()),
      source_dependency_(source_dependency),
      dns_aliases_(std::move(dns_aliases)) {
  // Proposed family boundary: Rust owns method control flow.
  Http2FamilyRun(2, 0, [&](uint32_t op, int64_t arg) -> int64_t {
    (void)arg;
    switch (op) {
      case 0: {  // Check
        DCHECK(spdy_session_.get());
        return 0;
      }
      case 40000: {  // Shared UploadContract
        CHECK(request_info_); return 0;
      }
      case 40001: {  // Shared UploadPresent
        return request_info_->upload_data_stream != nullptr;
      }
      case 40002: {  // Shared UploadSize
        return static_cast<int64_t>(request_info_->upload_data_stream->size());
      }
      case 40003: {  // Shared UploadChunked
        return request_info_->upload_data_stream->is_chunked();
      }
      default: NOTREACHED();
    }
  });
}

SpdyHttpStream::~SpdyHttpStream() {
  // Proposed family boundary: Rust owns method control flow.
  Http2FamilyRun(2, 1, [&](uint32_t op, int64_t arg) -> int64_t {
    (void)arg;
    switch (op) {
      case 0: {  // HasStream
        return stream_ != nullptr;
      }
      case 1: {  // Detach
        stream_->DetachDelegate();
        return 0;
      }
      case 2: {  // Check
        DCHECK(!stream_);
        return 0;
      }
      case 40000: {  // Shared UploadContract
        CHECK(request_info_); return 0;
      }
      case 40001: {  // Shared UploadPresent
        return request_info_->upload_data_stream != nullptr;
      }
      case 40002: {  // Shared UploadSize
        return static_cast<int64_t>(request_info_->upload_data_stream->size());
      }
      case 40003: {  // Shared UploadChunked
        return request_info_->upload_data_stream->is_chunked();
      }
      default: NOTREACHED();
    }
  });
}

void SpdyHttpStream::RegisterRequest(const HttpRequestInfo* request_info) {
  // Proposed family boundary: Rust owns method control flow.
  Http2FamilyRun(2, 2, [&](uint32_t op, int64_t arg) -> int64_t {
    (void)arg;
    switch (op) {
      case 0: {  // Check
        DCHECK(request_info);
        return 0;
      }
      case 1: {  // Store
        request_info_ = request_info;
        return 0;
      }
      case 40000: {  // Shared UploadContract
        CHECK(request_info_); return 0;
      }
      case 40001: {  // Shared UploadPresent
        return request_info_->upload_data_stream != nullptr;
      }
      case 40002: {  // Shared UploadSize
        return static_cast<int64_t>(request_info_->upload_data_stream->size());
      }
      case 40003: {  // Shared UploadChunked
        return request_info_->upload_data_stream->is_chunked();
      }
      default: NOTREACHED();
    }
  });
}

int SpdyHttpStream::InitializeStream(bool can_send_early,
                                     RequestPriority priority,
                                     const NetLogWithSource& stream_net_log,
                                     CompletionOnceCallback callback) {
  // Proposed family boundary: Rust owns method control flow.
  const int64_t family_result = Http2FamilyRun(2, 3, [&](uint32_t op, int64_t arg) -> int64_t {
    (void)arg;
    switch (op) {
      case 0: {  // Checks
        DCHECK(!stream_); DCHECK(request_info_);
        return 0;
      }
      case 1: {  // Session
        return static_cast<bool>(spdy_session_);
      }
      case 2: {  // Closed
        return ERR_CONNECTION_CLOSED;
      }
      case 3: {  // Priority
        priority_ = priority;
        return 0;
      }
      case 4: {  // Start
        return stream_request_.StartRequest(SPDY_REQUEST_RESPONSE_STREAM, spdy_session_, request_info_->url, can_send_early, priority, request_info_->socket_tag, stream_net_log, base::BindOnce(&SpdyHttpStream::OnStreamCreated, weak_factory_.GetWeakPtr(), std::move(callback)), NetworkTrafficAnnotationTag{request_info_->traffic_annotation});
      }
      case 5: {  // Ok
        return OK;
      }
      case 6: {  // Release
        stream_ = stream_request_.ReleaseStream().get();
        return 0;
      }
      case 7: {  // Initialize
        InitializeStreamHelper();
        return 0;
      }
      case 40000: {  // Shared UploadContract
        CHECK(request_info_); return 0;
      }
      case 40001: {  // Shared UploadPresent
        return request_info_->upload_data_stream != nullptr;
      }
      case 40002: {  // Shared UploadSize
        return static_cast<int64_t>(request_info_->upload_data_stream->size());
      }
      case 40003: {  // Shared UploadChunked
        return request_info_->upload_data_stream->is_chunked();
      }
      default: NOTREACHED();
    }
  });
  return static_cast<int>(family_result);
}

int SpdyHttpStream::ReadResponseHeaders(CompletionOnceCallback callback) {
  // Proposed family boundary: Rust owns method control flow.
  const int64_t family_result = Http2FamilyRun(2, 4, [&](uint32_t op, int64_t arg) -> int64_t {
    (void)arg;
    switch (op) {
      case 0: {  // CallbackCheck
        CHECK(!callback.is_null());
        return 0;
      }
      case 1: {  // Closed
        return stream_closed_;
      }
      case 2: {  // Status
        return closed_stream_status_;
      }
      case 3: {  // StreamCheck
        CHECK(stream_);
        return 0;
      }
      case 4: {  // Complete
        return response_headers_complete_;
      }
      case 5: {  // IdleCheck
        CHECK(!stream_->IsIdle());
        return 0;
      }
      case 6: {  // Ok
        return OK;
      }
      case 7: {  // PendingCheck
        CHECK(response_callback_.is_null());
        return 0;
      }
      case 8: {  // Store
        response_callback_ = std::move(callback);
        return 0;
      }
      case 9: {  // Pending
        return ERR_IO_PENDING;
      }
      case 40000: {  // Shared UploadContract
        CHECK(request_info_); return 0;
      }
      case 40001: {  // Shared UploadPresent
        return request_info_->upload_data_stream != nullptr;
      }
      case 40002: {  // Shared UploadSize
        return static_cast<int64_t>(request_info_->upload_data_stream->size());
      }
      case 40003: {  // Shared UploadChunked
        return request_info_->upload_data_stream->is_chunked();
      }
      default: NOTREACHED();
    }
  });
  return static_cast<int>(family_result);
}

int SpdyHttpStream::ReadResponseBody(IOBuffer* buf,
                                     int buf_len,
                                     CompletionOnceCallback callback) {
  // Proposed family boundary: Rust owns method control flow.
  base::WeakPtr<SpdyHttpStream> self;
  const int64_t family_result = Http2FamilyRun(2, 5, [&](uint32_t op, int64_t arg) -> int64_t {
    (void)arg;
    switch (op) {
      case 0: {  // HasStream
        return stream_ != nullptr;
      }
      case 1: {  // IdleCheck
        CHECK(!stream_->IsIdle());
        return 0;
      }
      case 2: {  // Checks
        CHECK(buf); CHECK(buf_len); CHECK(!callback.is_null());
        return 0;
      }
      case 3: {  // QueueEmpty
        return response_body_queue_.IsEmpty();
      }
      case 4: {  // Capture
        self = weak_factory_.GetWeakPtr();
        return 0;
      }
      case 5: {  // Dequeue
        return response_body_queue_.Dequeue(buf->first(buf_len));
      }
      case 6: {  // Alive
        return static_cast<bool>(self);
      }
      case 7: {  // ConnectionClosed
        return ERR_CONNECTION_CLOSED;
      }
      case 8: {  // Closed
        return stream_closed_;
      }
      case 9: {  // Status
        return closed_stream_status_;
      }
      case 10: {  // PendingChecks
        CHECK(response_callback_.is_null()); CHECK(!user_buffer_.get()); CHECK_EQ(0, user_buffer_len_);
        return 0;
      }
      case 11: {  // StoreCallback
        response_callback_ = std::move(callback);
        return 0;
      }
      case 12: {  // StoreBuffer
        user_buffer_ = buf;
        return 0;
      }
      case 13: {  // StoreLength
        user_buffer_len_ = buf_len;
        return 0;
      }
      case 14: {  // Pending
        return ERR_IO_PENDING;
      }
      case 40000: {  // Shared UploadContract
        CHECK(request_info_); return 0;
      }
      case 40001: {  // Shared UploadPresent
        return request_info_->upload_data_stream != nullptr;
      }
      case 40002: {  // Shared UploadSize
        return static_cast<int64_t>(request_info_->upload_data_stream->size());
      }
      case 40003: {  // Shared UploadChunked
        return request_info_->upload_data_stream->is_chunked();
      }
      default: NOTREACHED();
    }
  });
  return static_cast<int>(family_result);
}

void SpdyHttpStream::Close(bool not_reusable) {
  // Proposed family boundary: Rust owns method control flow.
  Http2FamilyRun(2, 6, [&](uint32_t op, int64_t arg) -> int64_t {
    (void)arg;
    switch (op) {
      case 0: {  // Cancel
        Cancel();
        return 0;
      }
      case 1: {  // Check
        DCHECK(!stream_);
        return 0;
      }
      case 40000: {  // Shared UploadContract
        CHECK(request_info_); return 0;
      }
      case 40001: {  // Shared UploadPresent
        return request_info_->upload_data_stream != nullptr;
      }
      case 40002: {  // Shared UploadSize
        return static_cast<int64_t>(request_info_->upload_data_stream->size());
      }
      case 40003: {  // Shared UploadChunked
        return request_info_->upload_data_stream->is_chunked();
      }
      default: NOTREACHED();
    }
  });
}

bool SpdyHttpStream::IsResponseBodyComplete() const {
  // Proposed family boundary: Rust owns method control flow.
  const int64_t family_result = Http2FamilyRun(2, 7, [&](uint32_t op, int64_t arg) -> int64_t {
    (void)arg;
    switch (op) {
      case 0: {  // Value
        return stream_closed_;
      }
      case 40000: {  // Shared UploadContract
        CHECK(request_info_); return 0;
      }
      case 40001: {  // Shared UploadPresent
        return request_info_->upload_data_stream != nullptr;
      }
      case 40002: {  // Shared UploadSize
        return static_cast<int64_t>(request_info_->upload_data_stream->size());
      }
      case 40003: {  // Shared UploadChunked
        return request_info_->upload_data_stream->is_chunked();
      }
      default: NOTREACHED();
    }
  });
  return family_result != 0;
}

bool SpdyHttpStream::IsConnectionReused() const {
  // Proposed family boundary: Rust owns method control flow.
  const int64_t family_result = Http2FamilyRun(2, 8, [&](uint32_t op, int64_t arg) -> int64_t {
    (void)arg;
    switch (op) {
      case 0: {  // Value
        return is_reused_;
      }
      case 40000: {  // Shared UploadContract
        CHECK(request_info_); return 0;
      }
      case 40001: {  // Shared UploadPresent
        return request_info_->upload_data_stream != nullptr;
      }
      case 40002: {  // Shared UploadSize
        return static_cast<int64_t>(request_info_->upload_data_stream->size());
      }
      case 40003: {  // Shared UploadChunked
        return request_info_->upload_data_stream->is_chunked();
      }
      default: NOTREACHED();
    }
  });
  return family_result != 0;
}

base::ByteSize SpdyHttpStream::GetTotalReceivedBytes() const {
  // Proposed family boundary: Rust owns method control flow.
  base::ByteSize result;
  Http2FamilyRun(2, 9, [&](uint32_t op, int64_t arg) -> int64_t {
    (void)arg;
    switch (op) {
      case 0: {  // Closed
        return stream_closed_;
      }
      case 1: {  // Saved
        result = closed_stream_received_bytes_;
        return 0;
      }
      case 2: {  // HasStream
        return stream_ != nullptr;
      }
      case 3: {  // Zero
        result = base::ByteSize(0);
        return 0;
      }
      case 4: {  // Current
        result = stream_->raw_received_bytes();
        return 0;
      }
      case 40000: {  // Shared UploadContract
        CHECK(request_info_); return 0;
      }
      case 40001: {  // Shared UploadPresent
        return request_info_->upload_data_stream != nullptr;
      }
      case 40002: {  // Shared UploadSize
        return static_cast<int64_t>(request_info_->upload_data_stream->size());
      }
      case 40003: {  // Shared UploadChunked
        return request_info_->upload_data_stream->is_chunked();
      }
      default: NOTREACHED();
    }
  });
  return result;
}

base::ByteSize SpdyHttpStream::GetTotalSentBytes() const {
  // Proposed family boundary: Rust owns method control flow.
  base::ByteSize result;
  Http2FamilyRun(2, 10, [&](uint32_t op, int64_t arg) -> int64_t {
    (void)arg;
    switch (op) {
      case 0: {  // Closed
        return stream_closed_;
      }
      case 1: {  // Saved
        result = closed_stream_sent_bytes_;
        return 0;
      }
      case 2: {  // HasStream
        return stream_ != nullptr;
      }
      case 3: {  // Zero
        result = base::ByteSize(0);
        return 0;
      }
      case 4: {  // Current
        result = stream_->raw_sent_bytes();
        return 0;
      }
      case 40000: {  // Shared UploadContract
        CHECK(request_info_); return 0;
      }
      case 40001: {  // Shared UploadPresent
        return request_info_->upload_data_stream != nullptr;
      }
      case 40002: {  // Shared UploadSize
        return static_cast<int64_t>(request_info_->upload_data_stream->size());
      }
      case 40003: {  // Shared UploadChunked
        return request_info_->upload_data_stream->is_chunked();
      }
      default: NOTREACHED();
    }
  });
  return result;
}

bool SpdyHttpStream::GetAlternativeService(
    AlternativeService* alternative_service) const {
  // Proposed family boundary: Rust owns method control flow.
  const int64_t family_result = Http2FamilyRun(2, 11, [&](uint32_t op, int64_t arg) -> int64_t {
    (void)arg;
    switch (op) {
      case 40000: {  // Shared UploadContract
        CHECK(request_info_); return 0;
      }
      case 40001: {  // Shared UploadPresent
        return request_info_->upload_data_stream != nullptr;
      }
      case 40002: {  // Shared UploadSize
        return static_cast<int64_t>(request_info_->upload_data_stream->size());
      }
      case 40003: {  // Shared UploadChunked
        return request_info_->upload_data_stream->is_chunked();
      }
      default: NOTREACHED();
    }
  });
  return family_result != 0;
}

bool SpdyHttpStream::GetLoadTimingInfo(LoadTimingInfo* load_timing_info) const {
  // Proposed family boundary: Rust owns method control flow.
  const int64_t family_result = Http2FamilyRun(2, 12, [&](uint32_t op, int64_t arg) -> int64_t {
    (void)arg;
    switch (op) {
      case 0: {  // Closed
        return stream_closed_;
      }
      case 1: {  // SavedValid
        return closed_stream_has_load_timing_info_;
      }
      case 2: {  // Saved
        *load_timing_info = closed_stream_load_timing_info_;
        return 0;
      }
      case 3: {  // HasStream
        return stream_ != nullptr;
      }
      case 4: {  // Id
        return stream_->stream_id();
      }
      case 5: {  // Current
        return stream_->GetLoadTimingInfo(load_timing_info);
      }
      case 6: {  // SslNull
        return load_timing_info->connect_timing.ssl_end.is_null();
      }
      case 7: {  // HandshakeNull
        return stream_request_.confirm_handshake_end().is_null();
      }
      case 8: {  // ShiftSsl
        load_timing_info->connect_timing.ssl_end = stream_request_.confirm_handshake_end();
        return 0;
      }
      case 9: {  // ShiftConnect
        load_timing_info->connect_timing.connect_end = stream_request_.confirm_handshake_end();
        return 0;
      }
      case 40000: {  // Shared UploadContract
        CHECK(request_info_); return 0;
      }
      case 40001: {  // Shared UploadPresent
        return request_info_->upload_data_stream != nullptr;
      }
      case 40002: {  // Shared UploadSize
        return static_cast<int64_t>(request_info_->upload_data_stream->size());
      }
      case 40003: {  // Shared UploadChunked
        return request_info_->upload_data_stream->is_chunked();
      }
      default: NOTREACHED();
    }
  });
  return family_result != 0;
}

void SpdyHttpStream::PopulateLoadTimingInternalInfo(
    LoadTimingInternalInfo* load_timing_internal_info) const {
  // Proposed family boundary: Rust owns method control flow.
  Http2FamilyRun(2, 13, [&](uint32_t op, int64_t arg) -> int64_t {
    (void)arg;
    switch (op) {
      case 0: {  // Check
        CHECK(load_timing_internal_info);
        return 0;
      }
      case 1: {  // Delay
        load_timing_internal_info->max_stream_limit_pending_delay = stream_request_.max_stream_limit_pending_delay();
        return 0;
      }
      case 2: {  // HasSession
        return static_cast<bool>(spdy_session_);
      }
      case 3: {  // Resolution
        load_timing_internal_info->resolution_details = spdy_session_->GetResolutionDetails();
        return 0;
      }
      case 40000: {  // Shared UploadContract
        CHECK(request_info_); return 0;
      }
      case 40001: {  // Shared UploadPresent
        return request_info_->upload_data_stream != nullptr;
      }
      case 40002: {  // Shared UploadSize
        return static_cast<int64_t>(request_info_->upload_data_stream->size());
      }
      case 40003: {  // Shared UploadChunked
        return request_info_->upload_data_stream->is_chunked();
      }
      default: NOTREACHED();
    }
  });
}

int SpdyHttpStream::SendRequest(const HttpRequestHeaders& request_headers,
                                HttpResponseInfo* response,
                                CompletionOnceCallback callback) {
  // Proposed family boundary: Rust owns method control flow.
  base::Time request_time; std::optional<IPEndPoint> address; std::optional<quiche::HttpHeaderBlock> headers;
  const int64_t family_result = Http2FamilyRun(2, 14, [&](uint32_t op, int64_t arg) -> int64_t {
    (void)arg;
    switch (op) {
      case 0: {  // Closed
        return stream_closed_;
      }
      case 1: {  // Status
        return closed_stream_status_;
      }
      case 2: {  // Time
        request_time = base::Time::Now();
        return 0;
      }
      case 3: {  // StreamCheck
        CHECK(stream_);
        return 0;
      }
      case 4: {  // RequestTime
        stream_->SetRequestTime(request_time);
        return 0;
      }
      case 5: {  // HasResponse
        return response_info_ != nullptr;
      }
      case 6: {  // ResponseTime
        response_info_->request_time = request_time;
        return 0;
      }
      case 7: {  // BufferCheck
        CHECK(!request_body_buf_.get());
        return 0;
      }
      case 8: {  // Allocate
        request_body_buf_ = base::MakeRefCounted<IOBufferWithSize>(kRequestBodyBufferSize);
        return 0;
      }
      case 9: {  // EmptyBody
        request_body_buf_size_ = 0;
        return 0;
      }
      case 10: {  // Checks
        CHECK(!callback.is_null()); CHECK(response); DCHECK(!response_info_);
        return 0;
      }
      case 11: {  // Response
        response_info_ = response;
        return 0;
      }
      case 12: {  // CreateAddress
        address.emplace();
        return 0;
      }
      case 13: {  // Peer
        return stream_->GetPeerAddress(&*address);
      }
      case 14: {  // Ok
        return OK;
      }
      case 15: {  // DestroyAddress
        address.reset();
        return 0;
      }
      case 16: {  // Remote
        response_info_->remote_endpoint = *address;
        return 0;
      }
      case 17: {  // CreateHeaders
        headers.emplace();
        return 0;
      }
      case 18: {  // BuildHeaders
        CreateSpdyHeadersFromHttpRequest(*request_info_, priority_, request_headers, &*headers);
        return 0;
      }
      case 19: {  // DispatchHeaders
        DispatchRequestHeadersCallback(*headers);
        return 0;
      }
      case 20: {  // EndWithData
        return spdy_session_->EndStreamWithDataFrame();
      }
      case 21: {  // Send
        return stream_->SendRequestHeaders(std::move(*headers), static_cast<SpdySendStatus>(arg));
      }
      case 22: {  // DestroyHeaders
        headers.reset();
        return 0;
      }
      case 23: {  // Pending
        return ERR_IO_PENDING;
      }
      case 24: {  // RequestCallbackCheck
        CHECK(request_callback_.is_null());
        return 0;
      }
      case 25: {  // SaveCallback
        request_callback_ = std::move(callback);
        return 0;
      }
      case 40000: {  // Shared UploadContract
        CHECK(request_info_); return 0;
      }
      case 40001: {  // Shared UploadPresent
        return request_info_->upload_data_stream != nullptr;
      }
      case 40002: {  // Shared UploadSize
        return static_cast<int64_t>(request_info_->upload_data_stream->size());
      }
      case 40003: {  // Shared UploadChunked
        return request_info_->upload_data_stream->is_chunked();
      }
      default: NOTREACHED();
    }
  });
  return static_cast<int>(family_result);
}

void SpdyHttpStream::Cancel() {
  // Proposed family boundary: Rust owns method control flow.
  Http2FamilyRun(2, 15, [&](uint32_t op, int64_t arg) -> int64_t {
    (void)arg;
    switch (op) {
      case 0: {  // ResetRequest
        request_callback_.Reset();
        return 0;
      }
      case 1: {  // ResetResponse
        response_callback_.Reset();
        return 0;
      }
      case 2: {  // HasStream
        return stream_ != nullptr;
      }
      case 3: {  // Cancel
        stream_->Cancel(ERR_ABORTED);
        return 0;
      }
      case 4: {  // Check
        DCHECK(!stream_);
        return 0;
      }
      case 40000: {  // Shared UploadContract
        CHECK(request_info_); return 0;
      }
      case 40001: {  // Shared UploadPresent
        return request_info_->upload_data_stream != nullptr;
      }
      case 40002: {  // Shared UploadSize
        return static_cast<int64_t>(request_info_->upload_data_stream->size());
      }
      case 40003: {  // Shared UploadChunked
        return request_info_->upload_data_stream->is_chunked();
      }
      default: NOTREACHED();
    }
  });
}

void SpdyHttpStream::OnHeadersSent() {
  // Proposed family boundary: Rust owns method control flow.
  Http2FamilyRun(2, 16, [&](uint32_t op, int64_t arg) -> int64_t {
    (void)arg;
    switch (op) {
      case 0: {  // ReadSend
        ReadAndSendRequestBodyData();
        return 0;
      }
      case 1: {  // EndWithData
        return spdy_session_->EndStreamWithDataFrame();
      }
      case 2: {  // Empty
        SendEmptyBody();
        return 0;
      }
      case 3: {  // Complete
        MaybePostRequestCallback(OK);
        return 0;
      }
      case 40000: {  // Shared UploadContract
        CHECK(request_info_); return 0;
      }
      case 40001: {  // Shared UploadPresent
        return request_info_->upload_data_stream != nullptr;
      }
      case 40002: {  // Shared UploadSize
        return static_cast<int64_t>(request_info_->upload_data_stream->size());
      }
      case 40003: {  // Shared UploadChunked
        return request_info_->upload_data_stream->is_chunked();
      }
      default: NOTREACHED();
    }
  });
}

void SpdyHttpStream::OnEarlyHintsReceived(
    const quiche::HttpHeaderBlock& headers) {
  // Proposed family boundary: Rust owns method control flow.
  Http2FamilyRun(2, 17, [&](uint32_t op, int64_t arg) -> int64_t {
    (void)arg;
    switch (op) {
      case 0: {  // Checks
        DCHECK(!response_headers_complete_); DCHECK(response_info_); DCHECK_EQ(stream_->type(), SPDY_REQUEST_RESPONSE_STREAM);
        return 0;
      }
      case 1: {  // Convert
        return SpdyHeadersToHttpResponse(headers, response_info_);
      }
      case 2: {  // ResultCheck
        CHECK_NE(static_cast<int>(arg), ERR_INCOMPLETE_HTTP2_HEADERS);
        return 0;
      }
      case 3: {  // HasResponseCallback
        return !response_callback_.is_null();
      }
      case 4: {  // Complete
        DoResponseCallback(OK);
        return 0;
      }
      case 40000: {  // Shared UploadContract
        CHECK(request_info_); return 0;
      }
      case 40001: {  // Shared UploadPresent
        return request_info_->upload_data_stream != nullptr;
      }
      case 40002: {  // Shared UploadSize
        return static_cast<int64_t>(request_info_->upload_data_stream->size());
      }
      case 40003: {  // Shared UploadChunked
        return request_info_->upload_data_stream->is_chunked();
      }
      default: NOTREACHED();
    }
  });
}

void SpdyHttpStream::OnHeadersReceived(
    const quiche::HttpHeaderBlock& response_headers) {
  // Proposed family boundary: Rust owns method control flow.
  Http2FamilyRun(2, 18, [&](uint32_t op, int64_t arg) -> int64_t {
    (void)arg;
    switch (op) {
      case 0: {  // Checks
        DCHECK(!response_headers_complete_); DCHECK(response_info_);
        return 0;
      }
      case 1: {  // Complete
        response_headers_complete_ = true;
        return 0;
      }
      case 2: {  // Convert
        return SpdyHeadersToHttpResponse(response_headers, response_info_);
      }
      case 3: {  // ResultCheck
        DCHECK_NE(static_cast<int>(arg), ERR_INCOMPLETE_HTTP2_HEADERS);
        return 0;
      }
      case 4: {  // MultipleLocation
        return ERR_RESPONSE_HEADERS_MULTIPLE_LOCATION;
      }
      case 5: {  // MultipleDisposition
        return ERR_RESPONSE_HEADERS_MULTIPLE_CONTENT_DISPOSITION;
      }
      case 6: {  // Cancel
        stream_->Cancel(static_cast<int>(arg));
        return 0;
      }
      case 7: {  // ResponseTime
        response_info_->response_time = response_info_->original_response_time = stream_->response_time();
        return 0;
      }
      case 8: {  // ProtocolCheck
        CHECK_EQ(stream_->GetNegotiatedProtocol(), NextProto::kProtoHTTP2);
        return 0;
      }
      case 9: {  // Alpn
        response_info_->was_alpn_negotiated = true;
        return 0;
      }
      case 10: {  // RequestTime
        response_info_->request_time = stream_->GetRequestTime();
        return 0;
      }
      case 11: {  // ConnectionInfo
        response_info_->connection_info = HttpConnectionInfo::kHTTP2;
        return 0;
      }
      case 12: {  // ProtocolString
        response_info_->alpn_negotiated_protocol = HttpConnectionInfoToString(response_info_->connection_info);
        return 0;
      }
      case 13: {  // UploadInProgress
        return upload_stream_in_progress_;
      }
      case 14: {  // InvalidateRequest
        request_info_ = nullptr;
        return 0;
      }
      case 15: {  // HasCallback
        return !response_callback_.is_null();
      }
      case 16: {  // Callback
        DoResponseCallback(OK);
        return 0;
      }
      case 40000: {  // Shared UploadContract
        CHECK(request_info_); return 0;
      }
      case 40001: {  // Shared UploadPresent
        return request_info_->upload_data_stream != nullptr;
      }
      case 40002: {  // Shared UploadSize
        return static_cast<int64_t>(request_info_->upload_data_stream->size());
      }
      case 40003: {  // Shared UploadChunked
        return request_info_->upload_data_stream->is_chunked();
      }
      default: NOTREACHED();
    }
  });
}

void SpdyHttpStream::OnDataReceived(std::unique_ptr<SpdyBuffer> buffer) {
  // Proposed family boundary: Rust owns method control flow.
  Http2FamilyRun(2, 19, [&](uint32_t op, int64_t arg) -> int64_t {
    (void)arg;
    switch (op) {
      case 0: {  // Checks
        DCHECK(response_headers_complete_); DCHECK(stream_); DCHECK(!stream_->IsClosed());
        return 0;
      }
      case 1: {  // HasBuffer
        return static_cast<bool>(buffer);
      }
      case 2: {  // Enqueue
        response_body_queue_.Enqueue(std::move(buffer));
        return 0;
      }
      case 3: {  // Schedule
        MaybeScheduleBufferedReadCallback();
        return 0;
      }
      case 40000: {  // Shared UploadContract
        CHECK(request_info_); return 0;
      }
      case 40001: {  // Shared UploadPresent
        return request_info_->upload_data_stream != nullptr;
      }
      case 40002: {  // Shared UploadSize
        return static_cast<int64_t>(request_info_->upload_data_stream->size());
      }
      case 40003: {  // Shared UploadChunked
        return request_info_->upload_data_stream->is_chunked();
      }
      default: NOTREACHED();
    }
  });
}

void SpdyHttpStream::OnDataSent() {
  // Proposed family boundary: Rust owns method control flow.
  Http2FamilyRun(2, 20, [&](uint32_t op, int64_t arg) -> int64_t {
    (void)arg;
    switch (op) {
      case 0: {  // HasRequest
        return request_info_ != nullptr;
      }
      case 1: {  // ResetSize
        request_body_buf_size_ = 0;
        return 0;
      }
      case 2: {  // ReadSend
        ReadAndSendRequestBodyData();
        return 0;
      }
      case 3: {  // Check
        CHECK(spdy_session_->EndStreamWithDataFrame());
        return 0;
      }
      case 4: {  // Complete
        MaybePostRequestCallback(OK);
        return 0;
      }
      case 40000: {  // Shared UploadContract
        CHECK(request_info_); return 0;
      }
      case 40001: {  // Shared UploadPresent
        return request_info_->upload_data_stream != nullptr;
      }
      case 40002: {  // Shared UploadSize
        return static_cast<int64_t>(request_info_->upload_data_stream->size());
      }
      case 40003: {  // Shared UploadChunked
        return request_info_->upload_data_stream->is_chunked();
      }
      default: NOTREACHED();
    }
  });
}

// TODO(xunjieli): Maybe do something with the trailers. crbug.com/422958.
void SpdyHttpStream::OnTrailers(const quiche::HttpHeaderBlock& trailers) {
  // Proposed family boundary: Rust owns method control flow.
  Http2FamilyRun(2, 21, [&](uint32_t op, int64_t arg) -> int64_t {
    (void)arg;
    switch (op) {
      case 40000: {  // Shared UploadContract
        CHECK(request_info_); return 0;
      }
      case 40001: {  // Shared UploadPresent
        return request_info_->upload_data_stream != nullptr;
      }
      case 40002: {  // Shared UploadSize
        return static_cast<int64_t>(request_info_->upload_data_stream->size());
      }
      case 40003: {  // Shared UploadChunked
        return request_info_->upload_data_stream->is_chunked();
      }
      default: NOTREACHED();
    }
  });
}

void SpdyHttpStream::OnClose(int status) {
  // Proposed family boundary: Rust owns method control flow.
  base::WeakPtr<SpdyHttpStream> self;
  Http2FamilyRun(2, 22, [&](uint32_t op, int64_t arg) -> int64_t {
    (void)arg;
    switch (op) {
      case 0: {  // StreamCheck
        DCHECK(stream_);
        return 0;
      }
      case 1: {  // HasRequest
        return request_info_ != nullptr;
      }
      case 2: {  // HasUpload
        return request_info_->upload_data_stream != nullptr;
      }
      case 3: {  // ResetUpload
        request_info_->upload_data_stream->Reset();
        return 0;
      }
      case 4: {  // Closed
        stream_closed_ = true;
        return 0;
      }
      case 5: {  // Status
        closed_stream_status_ = status;
        return 0;
      }
      case 6: {  // Id
        closed_stream_id_ = stream_->stream_id();
        return 0;
      }
      case 7: {  // Timing
        closed_stream_has_load_timing_info_ = stream_->GetLoadTimingInfo(&closed_stream_load_timing_info_);
        return 0;
      }
      case 8: {  // Received
        closed_stream_received_bytes_ = stream_->raw_received_bytes();
        return 0;
      }
      case 9: {  // Sent
        closed_stream_sent_bytes_ = stream_->raw_sent_bytes();
        return 0;
      }
      case 10: {  // ClearStream
        stream_ = nullptr;
        return 0;
      }
      case 11: {  // Capture
        self = weak_factory_.GetWeakPtr();
        return 0;
      }
      case 12: {  // HasRequestCallback
        return !request_callback_.is_null();
      }
      case 13: {  // RequestCallback
        DoRequestCallback(status);
        return 0;
      }
      case 14: {  // Alive
        return static_cast<bool>(self);
      }
      case 15: {  // InputStatus
        return status;
      }
      case 16: {  // Ok
        return OK;
      }
      case 17: {  // ReadCallback
        DoBufferedReadCallback();
        return 0;
      }
      case 18: {  // HasResponseCallback
        return !response_callback_.is_null();
      }
      case 19: {  // ResponseCallback
        DoResponseCallback(status);
        return 0;
      }
      case 40000: {  // Shared UploadContract
        CHECK(request_info_); return 0;
      }
      case 40001: {  // Shared UploadPresent
        return request_info_->upload_data_stream != nullptr;
      }
      case 40002: {  // Shared UploadSize
        return static_cast<int64_t>(request_info_->upload_data_stream->size());
      }
      case 40003: {  // Shared UploadChunked
        return request_info_->upload_data_stream->is_chunked();
      }
      default: NOTREACHED();
    }
  });
}

bool SpdyHttpStream::CanGreaseFrameType() const {
  // Proposed family boundary: Rust owns method control flow.
  const int64_t family_result = Http2FamilyRun(2, 23, [&](uint32_t op, int64_t arg) -> int64_t {
    (void)arg;
    switch (op) {
      case 40000: {  // Shared UploadContract
        CHECK(request_info_); return 0;
      }
      case 40001: {  // Shared UploadPresent
        return request_info_->upload_data_stream != nullptr;
      }
      case 40002: {  // Shared UploadSize
        return static_cast<int64_t>(request_info_->upload_data_stream->size());
      }
      case 40003: {  // Shared UploadChunked
        return request_info_->upload_data_stream->is_chunked();
      }
      default: NOTREACHED();
    }
  });
  return family_result != 0;
}

NetLogSource SpdyHttpStream::source_dependency() const {
  // Proposed family boundary: Rust owns method control flow.
  NetLogSource result;
  Http2FamilyRun(2, 24, [&](uint32_t op, int64_t arg) -> int64_t {
    (void)arg;
    switch (op) {
      case 0: {  // Copy
        result = source_dependency_;
        return 0;
      }
      case 40000: {  // Shared UploadContract
        CHECK(request_info_); return 0;
      }
      case 40001: {  // Shared UploadPresent
        return request_info_->upload_data_stream != nullptr;
      }
      case 40002: {  // Shared UploadSize
        return static_cast<int64_t>(request_info_->upload_data_stream->size());
      }
      case 40003: {  // Shared UploadChunked
        return request_info_->upload_data_stream->is_chunked();
      }
      default: NOTREACHED();
    }
  });
  return result;
}

bool SpdyHttpStream::HasUploadData() const {
  // Proposed family boundary: Rust owns method control flow.
  const int64_t family_result = Http2FamilyRun(2, 25, [&](uint32_t op, int64_t arg) -> int64_t {
    (void)arg;
    switch (op) {
      case 40000: {  // Shared UploadContract
        CHECK(request_info_); return 0;
      }
      case 40001: {  // Shared UploadPresent
        return request_info_->upload_data_stream != nullptr;
      }
      case 40002: {  // Shared UploadSize
        return static_cast<int64_t>(request_info_->upload_data_stream->size());
      }
      case 40003: {  // Shared UploadChunked
        return request_info_->upload_data_stream->is_chunked();
      }
      default: NOTREACHED();
    }
  });
  return family_result != 0;
}

void SpdyHttpStream::OnStreamCreated(CompletionOnceCallback callback, int rv) {
  // Proposed family boundary: Rust owns method control flow.
  Http2FamilyRun(2, 26, [&](uint32_t op, int64_t arg) -> int64_t {
    (void)arg;
    switch (op) {
      case 0: {  // Result
        return rv;
      }
      case 1: {  // Ok
        return OK;
      }
      case 2: {  // Release
        stream_ = stream_request_.ReleaseStream().get();
        return 0;
      }
      case 3: {  // Initialize
        InitializeStreamHelper();
        return 0;
      }
      case 4: {  // Callback
        std::move(callback).Run(static_cast<int>(arg));
        return 0;
      }
      case 40000: {  // Shared UploadContract
        CHECK(request_info_); return 0;
      }
      case 40001: {  // Shared UploadPresent
        return request_info_->upload_data_stream != nullptr;
      }
      case 40002: {  // Shared UploadSize
        return static_cast<int64_t>(request_info_->upload_data_stream->size());
      }
      case 40003: {  // Shared UploadChunked
        return request_info_->upload_data_stream->is_chunked();
      }
      default: NOTREACHED();
    }
  });
}

void SpdyHttpStream::ReadAndSendRequestBodyData() {
  // Proposed family boundary: Rust owns method control flow.
  Http2FamilyRun(2, 27, [&](uint32_t op, int64_t arg) -> int64_t {
    (void)arg;
    switch (op) {
      case 0: {  // UploadCheck
        CHECK(HasUploadData());
        return 0;
      }
      case 1: {  // InProgress
        upload_stream_in_progress_ = static_cast<bool>(arg);
        return 0;
      }
      case 2: {  // BufferCheck
        CHECK_EQ(request_body_buf_size_, 0);
        return 0;
      }
      case 3: {  // Eof
        return request_info_->upload_data_stream->IsEOF();
      }
      case 4: {  // Complete
        MaybePostRequestCallback(OK);
        return 0;
      }
      case 5: {  // HeadersComplete
        return response_headers_complete_;
      }
      case 6: {  // Invalidate
        request_info_ = nullptr;
        return 0;
      }
      case 7: {  // Read
        return request_info_->upload_data_stream->Read(request_body_buf_.get(), request_body_buf_->size(), base::BindOnce(&SpdyHttpStream::OnRequestBodyReadCompleted, weak_factory_.GetWeakPtr()));
      }
      case 8: {  // Pending
        return ERR_IO_PENDING;
      }
      case 9: {  // ReadComplete
        OnRequestBodyReadCompleted(static_cast<int>(arg));
        return 0;
      }
      case 40000: {  // Shared UploadContract
        CHECK(request_info_); return 0;
      }
      case 40001: {  // Shared UploadPresent
        return request_info_->upload_data_stream != nullptr;
      }
      case 40002: {  // Shared UploadSize
        return static_cast<int64_t>(request_info_->upload_data_stream->size());
      }
      case 40003: {  // Shared UploadChunked
        return request_info_->upload_data_stream->is_chunked();
      }
      default: NOTREACHED();
    }
  });
}

void SpdyHttpStream::SendEmptyBody() {
  // Proposed family boundary: Rust owns method control flow.
  scoped_refptr<IOBufferWithSize> buffer;
  Http2FamilyRun(2, 28, [&](uint32_t op, int64_t arg) -> int64_t {
    (void)arg;
    switch (op) {
      case 0: {  // Checks
        CHECK(!HasUploadData()); CHECK(spdy_session_->EndStreamWithDataFrame());
        return 0;
      }
      case 1: {  // Allocate
        buffer = base::MakeRefCounted<IOBufferWithSize>(0);
        return 0;
      }
      case 2: {  // Send
        stream_->SendData(buffer.get(), 0, NO_MORE_DATA_TO_SEND);
        return 0;
      }
      case 40000: {  // Shared UploadContract
        CHECK(request_info_); return 0;
      }
      case 40001: {  // Shared UploadPresent
        return request_info_->upload_data_stream != nullptr;
      }
      case 40002: {  // Shared UploadSize
        return static_cast<int64_t>(request_info_->upload_data_stream->size());
      }
      case 40003: {  // Shared UploadChunked
        return request_info_->upload_data_stream->is_chunked();
      }
      default: NOTREACHED();
    }
  });
}

void SpdyHttpStream::InitializeStreamHelper() {
  // Proposed family boundary: Rust owns method control flow.
  Http2FamilyRun(2, 29, [&](uint32_t op, int64_t arg) -> int64_t {
    (void)arg;
    switch (op) {
      case 0: {  // Set
        stream_->SetDelegate(this);
        return 0;
      }
      case 40000: {  // Shared UploadContract
        CHECK(request_info_); return 0;
      }
      case 40001: {  // Shared UploadPresent
        return request_info_->upload_data_stream != nullptr;
      }
      case 40002: {  // Shared UploadSize
        return static_cast<int64_t>(request_info_->upload_data_stream->size());
      }
      case 40003: {  // Shared UploadChunked
        return request_info_->upload_data_stream->is_chunked();
      }
      default: NOTREACHED();
    }
  });
}

void SpdyHttpStream::ResetStream(int error) {
  // Proposed family boundary: Rust owns method control flow.
  Http2FamilyRun(2, 30, [&](uint32_t op, int64_t arg) -> int64_t {
    (void)arg;
    switch (op) {
      case 0: {  // Reset
        spdy_session_->ResetStream(stream()->stream_id(), error, std::string());
        return 0;
      }
      case 40000: {  // Shared UploadContract
        CHECK(request_info_); return 0;
      }
      case 40001: {  // Shared UploadPresent
        return request_info_->upload_data_stream != nullptr;
      }
      case 40002: {  // Shared UploadSize
        return static_cast<int64_t>(request_info_->upload_data_stream->size());
      }
      case 40003: {  // Shared UploadChunked
        return request_info_->upload_data_stream->is_chunked();
      }
      default: NOTREACHED();
    }
  });
}

void SpdyHttpStream::OnRequestBodyReadCompleted(int status) {
  // Proposed family boundary: Rust owns method control flow.
  const base::Location native_location = FROM_HERE;
  Http2FamilyRun(2, 31, [&](uint32_t op, int64_t arg) -> int64_t {
    (void)arg;
    switch (op) {
      case 0: {  // Status
        return status;
      }
      case 1: {  // PendingCheck
        DCHECK_NE(ERR_IO_PENDING, status);
        return 0;
      }
      case 2: {  // Post
        base::SingleThreadTaskRunner::GetCurrentDefault()->PostTask(native_location, base::BindOnce(&SpdyHttpStream::ResetStream, weak_factory_.GetWeakPtr(), status));
        return 0;
      }
      case 3: {  // Nonnegative
        CHECK_GE(status, 0);
        return 0;
      }
      case 4: {  // Store
        request_body_buf_size_ = static_cast<int>(arg);
        return 0;
      }
      case 5: {  // Eof
        return request_info_->upload_data_stream->IsEOF();
      }
      case 6: {  // FinalCheck
        CHECK_GE(request_body_buf_size_, 0);
        return 0;
      }
      case 7: {  // MoreCheck
        CHECK_GT(request_body_buf_size_, 0);
        return 0;
      }
      case 8: {  // Send
        stream_->SendData(request_body_buf_.get(), request_body_buf_size_, static_cast<SpdySendStatus>(arg));
        return 0;
      }
      case 40000: {  // Shared UploadContract
        CHECK(request_info_); return 0;
      }
      case 40001: {  // Shared UploadPresent
        return request_info_->upload_data_stream != nullptr;
      }
      case 40002: {  // Shared UploadSize
        return static_cast<int64_t>(request_info_->upload_data_stream->size());
      }
      case 40003: {  // Shared UploadChunked
        return request_info_->upload_data_stream->is_chunked();
      }
      default: NOTREACHED();
    }
  });
}

void SpdyHttpStream::MaybeScheduleBufferedReadCallback() {
  // Proposed family boundary: Rust owns method control flow.
  const base::Location native_location = FROM_HERE;
  Http2FamilyRun(2, 32, [&](uint32_t op, int64_t arg) -> int64_t {
    (void)arg;
    switch (op) {
      case 0: {  // Check
        DCHECK(!stream_closed_);
        return 0;
      }
      case 1: {  // HasBuffer
        return user_buffer_.get() != nullptr;
      }
      case 2: {  // QueueSize
        return static_cast<int64_t>(response_body_queue_.GetTotalSize());
      }
      case 3: {  // BufferLength
        return static_cast<int64_t>(static_cast<size_t>(user_buffer_len_));
      }
      case 4: {  // Immediate
        buffered_read_timer_.Start(native_location, base::TimeDelta(), this, &SpdyHttpStream::DoBufferedReadCallback);
        return 0;
      }
      case 5: {  // Delayed
        buffered_read_timer_.Start(native_location, base::Milliseconds(1), this, &SpdyHttpStream::DoBufferedReadCallback);
        return 0;
      }
      case 40000: {  // Shared UploadContract
        CHECK(request_info_); return 0;
      }
      case 40001: {  // Shared UploadPresent
        return request_info_->upload_data_stream != nullptr;
      }
      case 40002: {  // Shared UploadSize
        return static_cast<int64_t>(request_info_->upload_data_stream->size());
      }
      case 40003: {  // Shared UploadChunked
        return request_info_->upload_data_stream->is_chunked();
      }
      default: NOTREACHED();
    }
  });
}

void SpdyHttpStream::DoBufferedReadCallback() {
  // Proposed family boundary: Rust owns method control flow.
  base::WeakPtr<SpdyHttpStream> self;
  Http2FamilyRun(2, 33, [&](uint32_t op, int64_t arg) -> int64_t {
    (void)arg;
    switch (op) {
      case 0: {  // Stop
        buffered_read_timer_.Stop();
        return 0;
      }
      case 1: {  // Closed
        return stream_closed_;
      }
      case 2: {  // Status
        return closed_stream_status_;
      }
      case 3: {  // Ok
        return OK;
      }
      case 4: {  // HasCallback
        return static_cast<bool>(response_callback_);
      }
      case 5: {  // StatusCallback
        DoResponseCallback(closed_stream_status_);
        return 0;
      }
      case 6: {  // HasBuffer
        return user_buffer_.get() != nullptr;
      }
      case 7: {  // QueueEmpty
        return response_body_queue_.IsEmpty();
      }
      case 8: {  // Capture
        self = weak_factory_.GetWeakPtr();
        return 0;
      }
      case 9: {  // Dequeue
        return response_body_queue_.Dequeue(user_buffer_->first(user_buffer_len_));
      }
      case 10: {  // Alive
        return static_cast<bool>(self);
      }
      case 11: {  // ClearBuffer
        user_buffer_ = nullptr;
        return 0;
      }
      case 12: {  // ClearLength
        user_buffer_len_ = 0;
        return 0;
      }
      case 13: {  // ReadCallback
        DoResponseCallback(static_cast<int>(arg));
        return 0;
      }
      case 40000: {  // Shared UploadContract
        CHECK(request_info_); return 0;
      }
      case 40001: {  // Shared UploadPresent
        return request_info_->upload_data_stream != nullptr;
      }
      case 40002: {  // Shared UploadSize
        return static_cast<int64_t>(request_info_->upload_data_stream->size());
      }
      case 40003: {  // Shared UploadChunked
        return request_info_->upload_data_stream->is_chunked();
      }
      default: NOTREACHED();
    }
  });
}

void SpdyHttpStream::DoRequestCallback(int rv) {
  // Proposed family boundary: Rust owns method control flow.
  Http2FamilyRun(2, 34, [&](uint32_t op, int64_t arg) -> int64_t {
    (void)arg;
    switch (op) {
      case 0: {  // Checks
        CHECK_NE(rv, ERR_IO_PENDING); CHECK(!request_callback_.is_null());
        return 0;
      }
      case 1: {  // Run
        std::move(request_callback_).Run(rv);
        return 0;
      }
      case 40000: {  // Shared UploadContract
        CHECK(request_info_); return 0;
      }
      case 40001: {  // Shared UploadPresent
        return request_info_->upload_data_stream != nullptr;
      }
      case 40002: {  // Shared UploadSize
        return static_cast<int64_t>(request_info_->upload_data_stream->size());
      }
      case 40003: {  // Shared UploadChunked
        return request_info_->upload_data_stream->is_chunked();
      }
      default: NOTREACHED();
    }
  });
}

void SpdyHttpStream::MaybeDoRequestCallback(int rv) {
  // Proposed family boundary: Rust owns method control flow.
  Http2FamilyRun(2, 35, [&](uint32_t op, int64_t arg) -> int64_t {
    (void)arg;
    switch (op) {
      case 0: {  // Check
        CHECK_NE(ERR_IO_PENDING, rv);
        return 0;
      }
      case 1: {  // HasCallback
        return static_cast<bool>(request_callback_);
      }
      case 2: {  // Run
        std::move(request_callback_).Run(rv);
        return 0;
      }
      case 40000: {  // Shared UploadContract
        CHECK(request_info_); return 0;
      }
      case 40001: {  // Shared UploadPresent
        return request_info_->upload_data_stream != nullptr;
      }
      case 40002: {  // Shared UploadSize
        return static_cast<int64_t>(request_info_->upload_data_stream->size());
      }
      case 40003: {  // Shared UploadChunked
        return request_info_->upload_data_stream->is_chunked();
      }
      default: NOTREACHED();
    }
  });
}

void SpdyHttpStream::MaybePostRequestCallback(int rv) {
  // Proposed family boundary: Rust owns method control flow.
  const base::Location native_location = FROM_HERE;
  Http2FamilyRun(2, 36, [&](uint32_t op, int64_t arg) -> int64_t {
    (void)arg;
    switch (op) {
      case 0: {  // Check
        CHECK_NE(ERR_IO_PENDING, rv);
        return 0;
      }
      case 1: {  // HasCallback
        return static_cast<bool>(request_callback_);
      }
      case 2: {  // Post
        base::SingleThreadTaskRunner::GetCurrentDefault()->PostTask(native_location, base::BindOnce(&SpdyHttpStream::MaybeDoRequestCallback, weak_factory_.GetWeakPtr(), rv));
        return 0;
      }
      case 40000: {  // Shared UploadContract
        CHECK(request_info_); return 0;
      }
      case 40001: {  // Shared UploadPresent
        return request_info_->upload_data_stream != nullptr;
      }
      case 40002: {  // Shared UploadSize
        return static_cast<int64_t>(request_info_->upload_data_stream->size());
      }
      case 40003: {  // Shared UploadChunked
        return request_info_->upload_data_stream->is_chunked();
      }
      default: NOTREACHED();
    }
  });
}

void SpdyHttpStream::DoResponseCallback(int rv) {
  // Proposed family boundary: Rust owns method control flow.
  Http2FamilyRun(2, 37, [&](uint32_t op, int64_t arg) -> int64_t {
    (void)arg;
    switch (op) {
      case 0: {  // Checks
        CHECK_NE(rv, ERR_IO_PENDING); CHECK(!response_callback_.is_null());
        return 0;
      }
      case 1: {  // Run
        std::move(response_callback_).Run(rv);
        return 0;
      }
      case 40000: {  // Shared UploadContract
        CHECK(request_info_); return 0;
      }
      case 40001: {  // Shared UploadPresent
        return request_info_->upload_data_stream != nullptr;
      }
      case 40002: {  // Shared UploadSize
        return static_cast<int64_t>(request_info_->upload_data_stream->size());
      }
      case 40003: {  // Shared UploadChunked
        return request_info_->upload_data_stream->is_chunked();
      }
      default: NOTREACHED();
    }
  });
}

int SpdyHttpStream::GetRemoteEndpoint(IPEndPoint* endpoint) {
  // Proposed family boundary: Rust owns method control flow.
  const int64_t family_result = Http2FamilyRun(2, 38, [&](uint32_t op, int64_t arg) -> int64_t {
    (void)arg;
    switch (op) {
      case 0: {  // DrainFeature
        return base::FeatureList::IsEnabled(features::kDrainSpdySessionSynchronouslyOnRemoteEndpointDisconnect);
      }
      case 1: {  // Parent
        return MultiplexedHttpStream::GetRemoteEndpoint(endpoint);
      }
      case 2: {  // Session
        return static_cast<bool>(spdy_session_);
      }
      case 3: {  // Disconnected
        return ERR_SOCKET_NOT_CONNECTED;
      }
      case 4: {  // Peer
        return spdy_session_->GetPeerAddress(endpoint);
      }
      case 40000: {  // Shared UploadContract
        CHECK(request_info_); return 0;
      }
      case 40001: {  // Shared UploadPresent
        return request_info_->upload_data_stream != nullptr;
      }
      case 40002: {  // Shared UploadSize
        return static_cast<int64_t>(request_info_->upload_data_stream->size());
      }
      case 40003: {  // Shared UploadChunked
        return request_info_->upload_data_stream->is_chunked();
      }
      default: NOTREACHED();
    }
  });
  return static_cast<int>(family_result);
}

void SpdyHttpStream::PopulateNetErrorDetails(NetErrorDetails* details) {
  // Proposed family boundary: Rust owns method control flow.
  Http2FamilyRun(2, 39, [&](uint32_t op, int64_t arg) -> int64_t {
    (void)arg;
    switch (op) {
      case 0: {  // Set
        details->connection_info = HttpConnectionInfo::kHTTP2;
        return 0;
      }
      case 40000: {  // Shared UploadContract
        CHECK(request_info_); return 0;
      }
      case 40001: {  // Shared UploadPresent
        return request_info_->upload_data_stream != nullptr;
      }
      case 40002: {  // Shared UploadSize
        return static_cast<int64_t>(request_info_->upload_data_stream->size());
      }
      case 40003: {  // Shared UploadChunked
        return request_info_->upload_data_stream->is_chunked();
      }
      default: NOTREACHED();
    }
  });
}

void SpdyHttpStream::SetPriority(RequestPriority priority) {
  // Proposed family boundary: Rust owns method control flow.
  Http2FamilyRun(2, 40, [&](uint32_t op, int64_t arg) -> int64_t {
    (void)arg;
    switch (op) {
      case 0: {  // Store
        priority_ = priority;
        return 0;
      }
      case 1: {  // HasStream
        return stream_ != nullptr;
      }
      case 2: {  // Update
        stream_->SetPriority(priority);
        return 0;
      }
      case 40000: {  // Shared UploadContract
        CHECK(request_info_); return 0;
      }
      case 40001: {  // Shared UploadPresent
        return request_info_->upload_data_stream != nullptr;
      }
      case 40002: {  // Shared UploadSize
        return static_cast<int64_t>(request_info_->upload_data_stream->size());
      }
      case 40003: {  // Shared UploadChunked
        return request_info_->upload_data_stream->is_chunked();
      }
      default: NOTREACHED();
    }
  });
}

const base::flat_set<std::string>& SpdyHttpStream::GetDnsAliases() const {
  // Proposed family boundary: Rust owns method control flow.
  const base::flat_set<std::string>* result = nullptr;
  Http2FamilyRun(2, 41, [&](uint32_t op, int64_t arg) -> int64_t {
    (void)arg;
    switch (op) {
      case 0: {  // Get
        result = &dns_aliases_;
        return 0;
      }
      case 40000: {  // Shared UploadContract
        CHECK(request_info_); return 0;
      }
      case 40001: {  // Shared UploadPresent
        return request_info_->upload_data_stream != nullptr;
      }
      case 40002: {  // Shared UploadSize
        return static_cast<int64_t>(request_info_->upload_data_stream->size());
      }
      case 40003: {  // Shared UploadChunked
        return request_info_->upload_data_stream->is_chunked();
      }
      default: NOTREACHED();
    }
  });
  return *result;
}

std::string_view SpdyHttpStream::GetAcceptChViaAlps() const {
  // Proposed family boundary: Rust owns method control flow.
  std::string_view result;
  Http2FamilyRun(2, 42, [&](uint32_t op, int64_t arg) -> int64_t {
    (void)arg;
    switch (op) {
      case 0: {  // HasRequest
        return request_info_ != nullptr;
      }
      case 1: {  // Get
        result = session()->GetAcceptChViaAlps(url::SchemeHostPort(request_info_->url));
        return 0;
      }
      case 40000: {  // Shared UploadContract
        CHECK(request_info_); return 0;
      }
      case 40001: {  // Shared UploadPresent
        return request_info_->upload_data_stream != nullptr;
      }
      case 40002: {  // Shared UploadSize
        return static_cast<int64_t>(request_info_->upload_data_stream->size());
      }
      case 40003: {  // Shared UploadChunked
        return request_info_->upload_data_stream->is_chunked();
      }
      default: NOTREACHED();
    }
  });
  return result;
}

void SpdyHttpStream::SetHTTP11Required() {
  // Proposed family boundary: Rust owns method control flow.
  Http2FamilyRun(2, 43, [&](uint32_t op, int64_t arg) -> int64_t {
    (void)arg;
    switch (op) {
      case 0: {  // Session
        return static_cast<bool>(spdy_session_);
      }
      case 1: {  // Close
        spdy_session_->CloseSessionOnError(ERR_HTTP_1_1_REQUIRED, std::string(SpdySession::kHTTP11RequiredErrorMessage), true);
        return 0;
      }
      case 40000: {  // Shared UploadContract
        CHECK(request_info_); return 0;
      }
      case 40001: {  // Shared UploadPresent
        return request_info_->upload_data_stream != nullptr;
      }
      case 40002: {  // Shared UploadSize
        return static_cast<int64_t>(request_info_->upload_data_stream->size());
      }
      case 40003: {  // Shared UploadChunked
        return request_info_->upload_data_stream->is_chunked();
      }
      default: NOTREACHED();
    }
  });
}

}  // namespace net
