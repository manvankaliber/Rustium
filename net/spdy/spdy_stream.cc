// Copyright 2012 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/spdy/spdy_stream.h"
#include "net/spdy/http2_family_rust_ffi.h"

#include <algorithm>
#include <limits>
#include <string_view>
#include <utility>
#include <optional>
#include "base/notreached.h"

#include "base/check_op.h"
#include "base/compiler_specific.h"
#include "base/functional/bind.h"
#include "base/location.h"
#include "base/metrics/histogram_functions.h"
#include "base/strings/strcat.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/stringprintf.h"
#include "base/task/single_thread_task_runner.h"
#include "base/time/time.h"
#include "base/trace_event/memory_usage_estimator.h"
#include "base/values.h"
#include "net/base/load_timing_info.h"
#include "net/http/http_status_code.h"
#include "net/log/net_log.h"
#include "net/log/net_log_capture_mode.h"
#include "net/log/net_log_event_type.h"
#include "net/spdy/spdy_buffer_producer.h"
#include "net/spdy/spdy_http_utils.h"
#include "net/spdy/spdy_log_util.h"
#include "net/spdy/spdy_session.h"

namespace net {

namespace {

base::DictValue NetLogSpdyStreamErrorParams(spdy::SpdyStreamId stream_id,
                                            int net_error,
                                            std::string_view description) {
  return base::DictValue()
      .Set("stream_id", static_cast<int>(stream_id))
      .Set("net_error", ErrorToShortString(net_error))
      .Set("description", description);
}

base::DictValue NetLogSpdyStreamWindowUpdateParams(spdy::SpdyStreamId stream_id,
                                                   int32_t delta,
                                                   int32_t window_size) {
  return base::DictValue()
      .Set("stream_id", static_cast<int>(stream_id))
      .Set("delta", delta)
      .Set("window_size", window_size);
}

base::DictValue NetLogSpdyDataParams(spdy::SpdyStreamId stream_id,
                                     int size,
                                     bool fin) {
  return base::DictValue()
      .Set("stream_id", static_cast<int>(stream_id))
      .Set("size", size)
      .Set("fin", fin);
}

}  // namespace

// A wrapper around a stream that calls into ProduceHeadersFrame().
class SpdyStream::HeadersBufferProducer : public SpdyBufferProducer {
 public:
  explicit HeadersBufferProducer(const base::WeakPtr<SpdyStream>& stream)
      : stream_(stream) {
    DCHECK(stream_.get());
  }

  ~HeadersBufferProducer() override = default;

  std::unique_ptr<SpdyBuffer> ProduceBuffer() override {
  // Proposed family boundary: Rust owns method control flow.
  std::unique_ptr<SpdyBuffer> result;
  Http2FamilyRun(1, 48, [&](uint32_t op, int64_t arg) -> int64_t {
    (void)arg;
    switch (op) {
      case 0: {  // Valid
        return stream_.get() != nullptr;
      }
      case 1: {  // Invalid
        NOTREACHED();
      }
      case 2: {  // IdCheck
        DCHECK_GT(stream_->stream_id(), 0u);
        return 0;
      }
      case 3: {  // Make
        result = std::make_unique<SpdyBuffer>(stream_->ProduceHeadersFrame());
        return 0;
      }
      default: NOTREACHED();
    }
  });
  return result;
}

 private:
  const base::WeakPtr<SpdyStream> stream_;
};

SpdyStream::SpdyStream(SpdyStreamType type,
                       const base::WeakPtr<SpdySession>& session,
                       const GURL& url,
                       RequestPriority priority,
                       int32_t initial_send_window_size,
                       int32_t max_recv_window_size,
                       const NetLogWithSource& net_log,
                       const NetworkTrafficAnnotationTag& traffic_annotation,
                       bool detect_broken_connection)
    : type_(type),
      url_(url),
      priority_(priority),
      send_window_size_(initial_send_window_size),
      max_recv_window_size_(max_recv_window_size),
      recv_window_size_(max_recv_window_size),
      last_recv_window_update_(base::TimeTicks::Now()),
      session_(session),
      request_time_(base::Time::Now()),
      net_log_(net_log),
      traffic_annotation_(traffic_annotation),
      detect_broken_connection_(detect_broken_connection) {
  // Proposed family boundary: Rust owns method control flow.
  Http2FamilyRun(1, 0, [&](uint32_t op, int64_t arg) -> int64_t {
    (void)arg;
    switch (op) {
      case 0: {  // Diagnostics
        static_assert(STATE_IDLE == 0 && STATE_OPEN == 1 && STATE_HALF_CLOSED_LOCAL == 2 && STATE_HALF_CLOSED_REMOTE == 3 && STATE_RESERVED_REMOTE == 4 && STATE_CLOSED == 5);
        static_assert(READY_FOR_HEADERS == 0 && READY_FOR_DATA_OR_TRAILERS == 1 && TRAILERS_RECEIVED == 2);
        static_assert(MORE_DATA_TO_SEND == 0 && NO_MORE_DATA_TO_SEND == 1);
        static_assert(HTTP_SWITCHING_PROTOCOLS == 101 && HTTP_EARLY_HINTS == 103);
        CHECK(type_ == SPDY_BIDIRECTIONAL_STREAM || type_ == SPDY_REQUEST_RESPONSE_STREAM);
        CHECK_GE(priority_, MINIMUM_PRIORITY); CHECK_LE(priority_, MAXIMUM_PRIORITY);
        return 0;
      }
      default: NOTREACHED();
    }
  });
}

SpdyStream::~SpdyStream() {
  // Proposed family boundary: Rust owns method control flow.
  Http2FamilyRun(1, 1, [&](uint32_t op, int64_t arg) -> int64_t {
    (void)arg;
    switch (op) {
      case 0: {  // Guard
        CHECK(!write_handler_guard_);
        return 0;
      }
      default: NOTREACHED();
    }
  });
}

void SpdyStream::SetDelegate(Delegate* delegate) {
  // Proposed family boundary: Rust owns method control flow.
  Http2FamilyRun(1, 2, [&](uint32_t op, int64_t arg) -> int64_t {
    (void)arg;
    switch (op) {
      case 0: {  // Set
        CHECK(!delegate_); CHECK(delegate); delegate_ = delegate;
        CHECK(io_state_ == STATE_IDLE || io_state_ == STATE_RESERVED_REMOTE);
        return 0;
      }
      default: NOTREACHED();
    }
  });
}

std::unique_ptr<spdy::SpdySerializedFrame> SpdyStream::ProduceHeadersFrame() {
  // Proposed family boundary: Rust owns method control flow.
  std::unique_ptr<spdy::SpdySerializedFrame> frame;
  Http2FamilyRun(1, 3, [&](uint32_t op, int64_t arg) -> int64_t {
    (void)arg;
    switch (op) {
      case 0: {  // Checks
        CHECK_EQ(io_state_, STATE_IDLE); CHECK(request_headers_valid_); CHECK_GT(stream_id_, 0u);
        return 0;
      }
      case 1: {  // Status
        return static_cast<int>(pending_send_status_);
      }
      case 2: {  // Fin
        return spdy::CONTROL_FLAG_FIN;
      }
      case 3: {  // None
        return spdy::CONTROL_FLAG_NONE;
      }
      case 4: {  // Create
        frame = session_->CreateHeaders(stream_id_, priority_, static_cast<spdy::SpdyControlFlags>(arg), std::move(request_headers_), delegate_->source_dependency());
        return 0;
      }
      case 5: {  // Invalidate
        request_headers_valid_ = false;
        return 0;
      }
      case 6: {  // Time
        send_time_ = base::TimeTicks::Now();
        return 0;
      }
      default: NOTREACHED();
    }
  });
  return frame;
}

void SpdyStream::DetachDelegate() {
  // Proposed family boundary: Rust owns method control flow.
  Http2FamilyRun(1, 4, [&](uint32_t op, int64_t arg) -> int64_t {
    (void)arg;
    switch (op) {
      case 0: {  // Check
        DCHECK(!IsClosed());
        return 0;
      }
      case 1: {  // Detach
        delegate_ = nullptr;
        return 0;
      }
      case 2: {  // Cancel
        Cancel(ERR_ABORTED);
        return 0;
      }
      default: NOTREACHED();
    }
  });
}

void SpdyStream::SetPriority(RequestPriority priority) {
  // Proposed family boundary: Rust owns method control flow.
  Http2FamilyRun(1, 5, [&](uint32_t op, int64_t arg) -> int64_t {
    (void)arg;
    switch (op) {
      case 0: {  // Old
        return priority_;
      }
      case 1: {  // New
        return priority;
      }
      case 2: {  // Update
        session_->UpdateStreamPriority(this, priority_, priority);
        return 0;
      }
      case 3: {  // Commit
        priority_ = priority;
        return 0;
      }
      default: NOTREACHED();
    }
  });
}

bool SpdyStream::AdjustSendWindowSize(int32_t delta_window_size) {
  // Proposed family boundary: Rust owns method control flow.
  const int64_t family_result = Http2FamilyRun(1, 6, [&](uint32_t op, int64_t arg) -> int64_t {
    (void)arg;
    switch (op) {
      case 0: {  // Closed
        return static_cast<int>(io_state_);
      }
      case 1: {  // Delta
        return delta_window_size;
      }
      case 2: {  // Window
        return send_window_size_;
      }
      case 3: {  // LowerCheck
        DCHECK_LE(std::numeric_limits<int32_t>::min() - delta_window_size, send_window_size_);
        return 0;
      }
      case 4: {  // Set
        send_window_size_ = static_cast<int32_t>(arg);
        return 0;
      }
      case 5: {  // Log
        net_log_.AddEvent(NetLogEventType::HTTP2_STREAM_UPDATE_SEND_WINDOW, [&] {
        return NetLogSpdyStreamWindowUpdateParams(stream_id_, delta_window_size, send_window_size_); });
        return 0;
      }
      case 6: {  // Resume
        PossiblyResumeIfSendStalled();
        return 0;
      }
      default: NOTREACHED();
    }
  });
  return family_result != 0;
}

void SpdyStream::OnWriteBufferConsumed(
    size_t frame_payload_size,
    size_t consume_size,
    SpdyBuffer::ConsumeSource consume_source) {
  // Proposed family boundary: Rust owns method control flow.
  Http2FamilyRun(1, 7, [&](uint32_t op, int64_t arg) -> int64_t {
    (void)arg;
    switch (op) {
      case 0: {  // Source
        return static_cast<int>(consume_source);
      }
      case 1: {  // Consumed
        return static_cast<int64_t>(consume_size);
      }
      case 2: {  // Payload
        return static_cast<int64_t>(frame_payload_size);
      }
      case 3: {  // Check
        DCHECK_GT(static_cast<size_t>(arg), 0u);
        return 0;
      }
      case 4: {  // Increase
        IncreaseSendWindowSize(static_cast<int32_t>(arg));
        return 0;
      }
      default: NOTREACHED();
    }
  });
}

void SpdyStream::IncreaseSendWindowSize(int32_t delta_window_size) {
  // Proposed family boundary: Rust owns method control flow.
  Http2FamilyRun(1, 8, [&](uint32_t op, int64_t arg) -> int64_t {
    (void)arg;
    switch (op) {
      case 0: {  // Check
        DCHECK_GE(delta_window_size, 1);
        return 0;
      }
      case 1: {  // Active
        return session_->IsStreamActive(stream_id_);
      }
      case 2: {  // Adjust
        return AdjustSendWindowSize(delta_window_size);
      }
      case 3: {  // Reset
        std::string desc = base::StringPrintf("Received WINDOW_UPDATE [delta: %d] for stream %d overflows send_window_size_ [current: %d]", delta_window_size, stream_id_, send_window_size_);
        session_->ResetStream(stream_id_, ERR_HTTP2_FLOW_CONTROL_ERROR, desc);
        return 0;
      }
      default: NOTREACHED();
    }
  });
}

void SpdyStream::DecreaseSendWindowSize(int32_t delta_window_size) {
  // Proposed family boundary: Rust owns method control flow.
  Http2FamilyRun(1, 9, [&](uint32_t op, int64_t arg) -> int64_t {
    (void)arg;
    switch (op) {
      case 0: {  // Closed
        return static_cast<int>(io_state_);
      }
      case 1: {  // Checks
        DCHECK_GE(delta_window_size, 1); DCHECK_LE(delta_window_size, kMaxSpdyFrameChunkSize); DCHECK_GE(send_window_size_, delta_window_size);
        return 0;
      }
      case 2: {  // Window
        return send_window_size_;
      }
      case 3: {  // Delta
        return delta_window_size;
      }
      case 4: {  // Set
        send_window_size_ = static_cast<int32_t>(arg);
        return 0;
      }
      case 5: {  // Log
        net_log_.AddEvent(NetLogEventType::HTTP2_STREAM_UPDATE_SEND_WINDOW, [&] {
        return NetLogSpdyStreamWindowUpdateParams(stream_id_, -delta_window_size, send_window_size_); });
        return 0;
      }
      default: NOTREACHED();
    }
  });
}

void SpdyStream::OnReadBufferConsumed(
    size_t consume_size,
    SpdyBuffer::ConsumeSource consume_source) {
  // Proposed family boundary: Rust owns method control flow.
  Http2FamilyRun(1, 10, [&](uint32_t op, int64_t arg) -> int64_t {
    (void)arg;
    switch (op) {
      case 0: {  // Check
        DCHECK_GE(consume_size, 1u); DCHECK_LE(consume_size, static_cast<size_t>(std::numeric_limits<int32_t>::max()));
        return 0;
      }
      case 1: {  // Increase
        IncreaseRecvWindowSize(static_cast<int32_t>(consume_size));
        return 0;
      }
      default: NOTREACHED();
    }
  });
}

void SpdyStream::IncreaseRecvWindowSize(int32_t delta_window_size) {
  // Proposed family boundary: Rust owns method control flow.
  base::TimeDelta elapsed; base::WeakPtr<SpdyStream> weak_this;
  Http2FamilyRun(1, 11, [&](uint32_t op, int64_t arg) -> int64_t {
    (void)arg;
    switch (op) {
      case 0: {  // Active
        return session_->IsStreamActive(stream_id_);
      }
      case 1: {  // Checks
        DCHECK_GE(unacked_recv_window_bytes_, 0); DCHECK_GE(recv_window_size_, unacked_recv_window_bytes_); DCHECK_GE(delta_window_size, 1); DCHECK_LE(delta_window_size, std::numeric_limits<int32_t>::max() - recv_window_size_);
        return 0;
      }
      case 2: {  // Window
        return recv_window_size_;
      }
      case 3: {  // Delta
        return delta_window_size;
      }
      case 4: {  // SetWindow
        recv_window_size_ = static_cast<int32_t>(arg);
        return 0;
      }
      case 5: {  // Log
        net_log_.AddEvent(NetLogEventType::HTTP2_STREAM_UPDATE_RECV_WINDOW, [&] {
        return NetLogSpdyStreamWindowUpdateParams(stream_id_, delta_window_size, recv_window_size_); });
        return 0;
      }
      case 6: {  // Unacked
        return unacked_recv_window_bytes_;
      }
      case 7: {  // SetUnacked
        unacked_recv_window_bytes_ = static_cast<int32_t>(arg);
        return 0;
      }
      case 8: {  // ObserveElapsed
        elapsed = base::TimeTicks::Now() - last_recv_window_update_;
        return 0;
      }
      case 9: {  // MaxWindow
        return max_recv_window_size_;
      }
      case 10: {  // Elapsed
        return elapsed.InMicroseconds();
      }
      case 11: {  // Delay
        return session_->TimeToBufferSmallWindowUpdates().InMicroseconds();
      }
      case 12: {  // UpdateTime
        last_recv_window_update_ = base::TimeTicks::Now();
        return 0;
      }
      case 13: {  // CaptureWeak
        weak_this = weak_ptr_factory_.GetWeakPtr();
        return 0;
      }
      case 14: {  // SendUpdate
        session_->SendStreamWindowUpdate(stream_id_, static_cast<uint32_t>(unacked_recv_window_bytes_));
        return 0;
      }
      case 15: {  // Alive
        return static_cast<bool>(weak_this);
      }
      default: NOTREACHED();
    }
  });
}

void SpdyStream::DecreaseRecvWindowSize(int32_t delta_window_size) {
  // Proposed family boundary: Rust owns method control flow.
  Http2FamilyRun(1, 12, [&](uint32_t op, int64_t arg) -> int64_t {
    (void)arg;
    switch (op) {
      case 0: {  // Checks
        DCHECK(session_->IsStreamActive(stream_id_)); DCHECK_GE(delta_window_size, 1);
        return 0;
      }
      case 1: {  // Delta
        return delta_window_size;
      }
      case 2: {  // Window
        return recv_window_size_;
      }
      case 3: {  // Unacked
        return unacked_recv_window_bytes_;
      }
      case 4: {  // Reset
        session_->ResetStream(stream_id_, ERR_HTTP2_FLOW_CONTROL_ERROR,
        base::StrCat({"delta_window_size is ", base::NumberToString(delta_window_size), " in DecreaseRecvWindowSize, which is larger than the receive window size of ", base::NumberToString(recv_window_size_)}));
        return 0;
      }
      case 5: {  // Set
        recv_window_size_ = static_cast<int32_t>(arg);
        return 0;
      }
      case 6: {  // Log
        net_log_.AddEvent(NetLogEventType::HTTP2_STREAM_UPDATE_RECV_WINDOW, [&] {
        return NetLogSpdyStreamWindowUpdateParams(stream_id_, -delta_window_size, recv_window_size_); });
        return 0;
      }
      default: NOTREACHED();
    }
  });
}

int SpdyStream::GetPeerAddress(IPEndPoint* address) const {
  // Proposed family boundary: Rust owns method control flow.
  const int64_t family_result = Http2FamilyRun(1, 13, [&](uint32_t op, int64_t arg) -> int64_t {
    (void)arg;
    switch (op) {
      case 0: {  // Native
        return session_->GetPeerAddress(address);
      }
      default: NOTREACHED();
    }
  });
  return static_cast<int>(family_result);
}

int SpdyStream::GetLocalAddress(IPEndPoint* address) const {
  // Proposed family boundary: Rust owns method control flow.
  const int64_t family_result = Http2FamilyRun(1, 14, [&](uint32_t op, int64_t arg) -> int64_t {
    (void)arg;
    switch (op) {
      case 0: {  // Native
        return session_->GetLocalAddress(address);
      }
      default: NOTREACHED();
    }
  });
  return static_cast<int>(family_result);
}

bool SpdyStream::WasEverUsed() const {
  // Proposed family boundary: Rust owns method control flow.
  const int64_t family_result = Http2FamilyRun(1, 15, [&](uint32_t op, int64_t arg) -> int64_t {
    (void)arg;
    switch (op) {
      case 0: {  // Native
        return session_->WasEverUsed();
      }
      default: NOTREACHED();
    }
  });
  return family_result != 0;
}

base::Time SpdyStream::GetRequestTime() const {
  // Proposed family boundary: Rust owns method control flow.
  base::Time result;
  Http2FamilyRun(1, 18, [&](uint32_t op, int64_t arg) -> int64_t {
    (void)arg;
    switch (op) {
      case 0: {  // Get
        result = request_time_;
        return 0;
      }
      default: NOTREACHED();
    }
  });
  return result;
}

void SpdyStream::SetRequestTime(base::Time t) {
  // Proposed family boundary: Rust owns method control flow.
  Http2FamilyRun(1, 19, [&](uint32_t op, int64_t arg) -> int64_t {
    (void)arg;
    switch (op) {
      case 0: {  // Set
        request_time_ = t;
        return 0;
      }
      default: NOTREACHED();
    }
  });
}

void SpdyStream::OnHeadersReceived(
    const quiche::HttpHeaderBlock& response_headers,
    base::Time response_time,
    base::TimeTicks recv_first_byte_time) {
  // Proposed family boundary: Rust owns method control flow.
  std::optional<quiche::HttpHeaderBlock::const_iterator> it; int status = 0;
  Http2FamilyRun(1, 20, [&](uint32_t op, int64_t arg) -> int64_t {
    (void)arg;
    switch (op) {
      case 0: {  // ResponseState
        return static_cast<int>(response_state_);
      }
      case 1: {  // EmptyCheck
        DCHECK(response_headers_.empty());
        return 0;
      }
      case 2: {  // FindStatus
        it.emplace(response_headers.find(spdy::kHttp2StatusHeader));
        return 0;
      }
      case 3: {  // HasStatus
        return *it != response_headers.end();
      }
      case 4: {  // ErrorMissing
        const std::string error("Response headers do not include :status."); LogStreamError(ERR_HTTP2_PROTOCOL_ERROR, error); session_->ResetStream(stream_id_, ERR_HTTP2_PROTOCOL_ERROR, error);
        return 0;
      }
      case 5: {  // ParseStatus
        return base::StringToInt((*it)->second, &status);
      }
      case 6: {  // ErrorParse
        const std::string error("Cannot parse :status."); LogStreamError(ERR_HTTP2_PROTOCOL_ERROR, error); session_->ResetStream(stream_id_, ERR_HTTP2_PROTOCOL_ERROR, error);
        return 0;
      }
      case 7: {  // Status
        return status;
      }
      case 8: {  // Histogram
        base::UmaHistogramSparse("Net.SpdyResponseCode", status);
        return 0;
      }
      case 9: {  // FirstNull
        return recv_first_byte_time_.is_null();
      }
      case 10: {  // SetFirst
        recv_first_byte_time_ = recv_first_byte_time;
        return 0;
      }
      case 11: {  // SetNonInformational
        DCHECK(recv_first_byte_time_for_non_informational_response_.is_null()); recv_first_byte_time_for_non_informational_response_ = recv_first_byte_time;
        return 0;
      }
      case 12: {  // EarlyHints
        OnEarlyHintsReceived(response_headers, recv_first_byte_time);
        return 0;
      }
      case 13: {  // SetResponseState
        response_state_ = static_cast<ResponseState>(arg);
        return 0;
      }
      case 14: {  // Type
        return static_cast<int>(type_);
      }
      case 15: {  // Bidirectional
        return SPDY_BIDIRECTIONAL_STREAM;
      }
      case 16: {  // RequestResponse
        return SPDY_REQUEST_RESPONSE_STREAM;
      }
      case 17: {  // IoState
        return static_cast<int>(io_state_);
      }
      case 18: {  // ErrorBeforeRequest
        const std::string error("Response received before request sent."); LogStreamError(ERR_HTTP2_PROTOCOL_ERROR, error); DVLOG(1) << "Response received before request sent, possibly crbug.com/41180906"; session_->ResetStream(stream_id_, ERR_HTTP2_PROTOCOL_ERROR, error);
        return 0;
      }
      case 19: {  // NotIdleCheck
        DCHECK_NE(io_state_, STATE_IDLE);
        return 0;
      }
      case 20: {  // ResponseTime
        response_time_ = response_time;
        return 0;
      }
      case 21: {  // Save
        SaveResponseHeaders(response_headers, status);
        return 0;
      }
      case 22: {  // Trailers
        delegate_->OnTrailers(response_headers);
        return 0;
      }
      case 23: {  // ErrorAfterTrailers
        const std::string error("Header block received after trailers."); LogStreamError(ERR_HTTP2_PROTOCOL_ERROR, error); session_->ResetStream(stream_id_, ERR_HTTP2_PROTOCOL_ERROR, error);
        return 0;
      }
      default: NOTREACHED();
    }
  });
}

void SpdyStream::OnDataReceived(std::unique_ptr<SpdyBuffer> buffer) {
  // Proposed family boundary: Rust owns method control flow.
  base::WeakPtr<SpdyStream> weak_this;
  Http2FamilyRun(1, 21, [&](uint32_t op, int64_t arg) -> int64_t {
    (void)arg;
    switch (op) {
      case 0: {  // ActiveCheck
        DCHECK(session_->IsStreamActive(stream_id_));
        return 0;
      }
      case 1: {  // ResponseState
        return static_cast<int>(response_state_);
      }
      case 2: {  // BeforeHeaders
        const std::string error("DATA received before headers."); LogStreamError(ERR_HTTP2_PROTOCOL_ERROR, error); session_->ResetStream(stream_id_, ERR_HTTP2_PROTOCOL_ERROR, error);
        return 0;
      }
      case 3: {  // HasBuffer
        return static_cast<bool>(buffer);
      }
      case 4: {  // AfterTrailers
        const std::string error("DATA received after trailers."); LogStreamError(ERR_HTTP2_PROTOCOL_ERROR, error); session_->ResetStream(stream_id_, ERR_HTTP2_PROTOCOL_ERROR, error);
        return 0;
      }
      case 5: {  // State
        return static_cast<int>(io_state_);
      }
      case 6: {  // HalfClosed
        const std::string error("DATA received on half-closed (remove) stream."); LogStreamError(ERR_HTTP2_STREAM_CLOSED, error); session_->ResetStream(stream_id_, ERR_HTTP2_STREAM_CLOSED, error);
        return 0;
      }
      case 7: {  // Length
        return static_cast<int64_t>(buffer->GetRemainingSize());
      }
      case 8: {  // Received
        return recv_bytes_;
      }
      case 9: {  // SetReceived
        recv_bytes_ = static_cast<int>(arg);
        return 0;
      }
      case 10: {  // LastTime
        recv_last_byte_time_ = base::TimeTicks::Now();
        return 0;
      }
      case 11: {  // NotClosedCheck
        CHECK(!IsClosed());
        return 0;
      }
      case 12: {  // SetState
        io_state_ = static_cast<State>(arg);
        return 0;
      }
      case 13: {  // Eof
        delegate_->OnDataReceived(nullptr);
        return 0;
      }
      case 14: {  // CloseActive
        session_->CloseActiveStream(stream_id_, OK);
        return 0;
      }
      case 15: {  // Unreachable
        NOTREACHED() << io_state_;
      }
      case 16: {  // LengthCheck
        DCHECK_LE(static_cast<size_t>(arg), spdy::kHttp2DefaultFramePayloadLimit);
        return 0;
      }
      case 17: {  // CaptureWeak
        weak_this = GetWeakPtr();
        return 0;
      }
      case 18: {  // Decrease
        DecreaseRecvWindowSize(static_cast<int32_t>(arg));
        return 0;
      }
      case 19: {  // Alive
        return static_cast<bool>(weak_this);
      }
      case 20: {  // BindConsume
        buffer->AddConsumeCallback(base::BindRepeating(&SpdyStream::OnReadBufferConsumed, GetWeakPtr()));
        return 0;
      }
      case 21: {  // Deliver
        delegate_->OnDataReceived(std::move(buffer));
        return 0;
      }
      default: NOTREACHED();
    }
  });
}

void SpdyStream::OnPaddingConsumed(size_t len) {
  // Proposed family boundary: Rust owns method control flow.
  base::WeakPtr<SpdyStream> weak_this;
  Http2FamilyRun(1, 22, [&](uint32_t op, int64_t arg) -> int64_t {
    (void)arg;
    switch (op) {
      case 0: {  // Capture
        weak_this = GetWeakPtr();
        return 0;
      }
      case 1: {  // Decrease
        DecreaseRecvWindowSize(static_cast<int32_t>(len));
        return 0;
      }
      case 2: {  // Alive
        return static_cast<bool>(weak_this);
      }
      case 3: {  // Increase
        IncreaseRecvWindowSize(static_cast<int32_t>(len));
        return 0;
      }
      default: NOTREACHED();
    }
  });
}

void SpdyStream::OnFrameWriteComplete(spdy::SpdyFrameType frame_type,
                                      size_t frame_size) {
  // Proposed family boundary: Rust owns method control flow.
  base::WeakPtr<SpdyStream> weak_this;
  Http2FamilyRun(1, 23, [&](uint32_t op, int64_t arg) -> int64_t {
    (void)arg;
    switch (op) {
      case 0: {  // FrameType
        return static_cast<int>(frame_type);
      }
      case 1: {  // HeadersType
        return static_cast<int>(spdy::SpdyFrameType::HEADERS);
      }
      case 2: {  // DataType
        return static_cast<int>(spdy::SpdyFrameType::DATA);
      }
      case 3: {  // HeadersSent
        return OnHeadersSent();
      }
      case 4: {  // DataSent
        return OnDataSent(frame_size);
      }
      case 5: {  // Pending
        return ERR_IO_PENDING;
      }
      case 6: {  // SendStatus
        return static_cast<int>(pending_send_status_);
      }
      case 7: {  // State
        return static_cast<int>(io_state_);
      }
      case 8: {  // SetState
        io_state_ = static_cast<State>(arg);
        return 0;
      }
      case 9: {  // Unreachable
        NOTREACHED() << io_state_;
      }
      case 10: {  // DelegateCheck
        CHECK(delegate_);
        return 0;
      }
      case 11: {  // Capture
        weak_this = GetWeakPtr();
        return 0;
      }
      case 12: {  // Guard
        write_handler_guard_ = static_cast<bool>(arg);
        return 0;
      }
      case 13: {  // NotifyHeaders
        delegate_->OnHeadersSent();
        return 0;
      }
      case 14: {  // NotifyData
        delegate_->OnDataSent();
        return 0;
      }
      case 15: {  // AliveCheck
        CHECK(weak_this);
        return 0;
      }
      case 16: {  // ReleaseWeak
        weak_this.reset();
        return 0;
      }
      case 17: {  // Close
        session_->CloseActiveStream(stream_id_, OK);
        return 0;
      }
      default: NOTREACHED();
    }
  });
}

int SpdyStream::OnHeadersSent() {
  // Proposed family boundary: Rust owns method control flow.
  const int64_t family_result = Http2FamilyRun(1, 24, [&](uint32_t op, int64_t arg) -> int64_t {
    (void)arg;
    switch (op) {
      case 0: {  // Checks
        CHECK_EQ(io_state_, STATE_IDLE); CHECK_NE(stream_id_, 0u);
        return 0;
      }
      case 1: {  // State
        io_state_ = static_cast<State>(arg);
        return 0;
      }
      case 2: {  // Ok
        return OK;
      }
      default: NOTREACHED();
    }
  });
  return static_cast<int>(family_result);
}

int SpdyStream::OnDataSent(size_t frame_size) {
  // Proposed family boundary: Rust owns method control flow.
  const int64_t family_result = Http2FamilyRun(1, 25, [&](uint32_t op, int64_t arg) -> int64_t {
    (void)arg;
    switch (op) {
      case 0: {  // StateCheck
        CHECK(io_state_ == STATE_OPEN || io_state_ == STATE_HALF_CLOSED_REMOTE) << io_state_;
        return 0;
      }
      case 1: {  // Size
        return static_cast<int64_t>(frame_size);
      }
      case 2: {  // Minimum
        return spdy::kDataFrameMinimumSize;
      }
      case 3: {  // SizeCheck
        CHECK_GE(frame_size, spdy::kDataFrameMinimumSize); CHECK_LE(static_cast<size_t>(arg), spdy::kHttp2DefaultFramePayloadLimit);
        return 0;
      }
      case 4: {  // Consume
        pending_send_data_->DidConsume(static_cast<size_t>(arg));
        return 0;
      }
      case 5: {  // Remaining
        return pending_send_data_->BytesRemaining();
      }
      case 6: {  // Queue
        QueueNextDataFrame();
        return 0;
      }
      case 7: {  // Pending
        return ERR_IO_PENDING;
      }
      case 8: {  // Clear
        pending_send_data_ = nullptr;
        return 0;
      }
      case 9: {  // Ok
        return OK;
      }
      default: NOTREACHED();
    }
  });
  return static_cast<int>(family_result);
}

void SpdyStream::LogStreamError(int error, std::string_view description) {
  // Proposed family boundary: Rust owns method control flow.
  Http2FamilyRun(1, 26, [&](uint32_t op, int64_t arg) -> int64_t {
    (void)arg;
    switch (op) {
      case 0: {  // Log
        net_log_.AddEvent(NetLogEventType::HTTP2_STREAM_ERROR, [&] { return NetLogSpdyStreamErrorParams(stream_id_, error, description); });
        return 0;
      }
      default: NOTREACHED();
    }
  });
}

void SpdyStream::OnClose(int status) {
  // Proposed family boundary: Rust owns method control flow.
  Delegate* saved_delegate = nullptr;
  Http2FamilyRun(1, 27, [&](uint32_t op, int64_t arg) -> int64_t {
    (void)arg;
    switch (op) {
      case 0: {  // State
        io_state_ = static_cast<State>(arg);
        return 0;
      }
      case 1: {  // Status
        return status;
      }
      case 2: {  // NoErrorReset
        return ERR_HTTP2_RST_STREAM_NO_ERROR_RECEIVED;
      }
      case 3: {  // ResponseState
        return static_cast<int>(response_state_);
      }
      case 4: {  // ProtocolError
        return ERR_HTTP2_PROTOCOL_ERROR;
      }
      case 5: {  // Ok
        return OK;
      }
      case 6: {  // SaveDelegate
        saved_delegate = delegate_;
        return 0;
      }
      case 7: {  // ClearDelegate
        delegate_ = nullptr;
        return 0;
      }
      case 8: {  // HasDelegate
        return saved_delegate != nullptr;
      }
      case 9: {  // Notify
        saved_delegate->OnClose(static_cast<int>(arg));
        return 0;
      }
      case 10: {  // ClearId
        stream_id_ = 0;
        return 0;
      }
      default: NOTREACHED();
    }
  });
}

void SpdyStream::Cancel(int error) {
  // Proposed family boundary: Rust owns method control flow.
  Http2FamilyRun(1, 28, [&](uint32_t op, int64_t arg) -> int64_t {
    (void)arg;
    switch (op) {
      case 0: {  // State
        return static_cast<int>(io_state_);
      }
      case 1: {  // Id
        return stream_id_;
      }
      case 2: {  // Active
        session_->ResetStream(stream_id_, error, std::string());
        return 0;
      }
      case 3: {  // Created
        session_->CloseCreatedStream(GetWeakPtr(), error);
        return 0;
      }
      default: NOTREACHED();
    }
  });
}

void SpdyStream::Close() {
  // Proposed family boundary: Rust owns method control flow.
  Http2FamilyRun(1, 29, [&](uint32_t op, int64_t arg) -> int64_t {
    (void)arg;
    switch (op) {
      case 0: {  // State
        return static_cast<int>(io_state_);
      }
      case 1: {  // Id
        return stream_id_;
      }
      case 2: {  // Active
        session_->CloseActiveStream(stream_id_, OK);
        return 0;
      }
      case 3: {  // Created
        session_->CloseCreatedStream(GetWeakPtr(), OK);
        return 0;
      }
      default: NOTREACHED();
    }
  });
}

base::WeakPtr<SpdyStream> SpdyStream::GetWeakPtr() {
  // Proposed family boundary: Rust owns method control flow.
  base::WeakPtr<SpdyStream> result;
  Http2FamilyRun(1, 30, [&](uint32_t op, int64_t arg) -> int64_t {
    (void)arg;
    switch (op) {
      case 0: {  // Get
        result = weak_ptr_factory_.GetWeakPtr();
        return 0;
      }
      default: NOTREACHED();
    }
  });
  return result;
}

int SpdyStream::SendRequestHeaders(quiche::HttpHeaderBlock request_headers,
                                   SpdySendStatus send_status) {
  // Proposed family boundary: Rust owns method control flow.
  const int64_t family_result = Http2FamilyRun(1, 31, [&](uint32_t op, int64_t arg) -> int64_t {
    (void)arg;
    switch (op) {
      case 0: {  // Log
        net_log_.AddEvent(NetLogEventType::HTTP_TRANSACTION_HTTP2_SEND_REQUEST_HEADERS, [&](NetLogCaptureMode capture_mode) { return HttpHeaderBlockNetLogParams(&request_headers, capture_mode); });
        return 0;
      }
      case 1: {  // Checks
        CHECK_EQ(pending_send_status_, MORE_DATA_TO_SEND); CHECK(!request_headers_valid_); CHECK(!pending_send_data_.get()); CHECK_EQ(io_state_, STATE_IDLE);
        return 0;
      }
      case 2: {  // Move
        request_headers_ = std::move(request_headers);
        return 0;
      }
      case 3: {  // Valid
        request_headers_valid_ = true;
        return 0;
      }
      case 4: {  // Status
        pending_send_status_ = send_status;
        return 0;
      }
      case 5: {  // Enqueue
        session_->EnqueueStreamWrite(GetWeakPtr(), spdy::SpdyFrameType::HEADERS, std::make_unique<HeadersBufferProducer>(GetWeakPtr()));
        return 0;
      }
      case 6: {  // Pending
        return ERR_IO_PENDING;
      }
      default: NOTREACHED();
    }
  });
  return static_cast<int>(family_result);
}

void SpdyStream::SendData(IOBuffer* data,
                          int length,
                          SpdySendStatus send_status) {
  // Proposed family boundary: Rust owns method control flow.
  Http2FamilyRun(1, 32, [&](uint32_t op, int64_t arg) -> int64_t {
    (void)arg;
    switch (op) {
      case 0: {  // Checks
        CHECK_EQ(pending_send_status_, MORE_DATA_TO_SEND); CHECK(io_state_ == STATE_OPEN || io_state_ == STATE_HALF_CLOSED_REMOTE) << io_state_; CHECK(!pending_send_data_.get());
        return 0;
      }
      case 1: {  // Buffer
        pending_send_data_ = base::MakeRefCounted<DrainableIOBuffer>(data, length);
        return 0;
      }
      case 2: {  // Status
        pending_send_status_ = send_status;
        return 0;
      }
      case 3: {  // Queue
        QueueNextDataFrame();
        return 0;
      }
      default: NOTREACHED();
    }
  });
}

bool SpdyStream::GetSSLInfo(SSLInfo* ssl_info) const {
  // Proposed family boundary: Rust owns method control flow.
  const int64_t family_result = Http2FamilyRun(1, 16, [&](uint32_t op, int64_t arg) -> int64_t {
    (void)arg;
    switch (op) {
      case 0: {  // Native
        return session_->GetSSLInfo(ssl_info);
      }
      default: NOTREACHED();
    }
  });
  return family_result != 0;
}

NextProto SpdyStream::GetNegotiatedProtocol() const {
  // Proposed family boundary: Rust owns method control flow.
  const int64_t family_result = Http2FamilyRun(1, 17, [&](uint32_t op, int64_t arg) -> int64_t {
    (void)arg;
    switch (op) {
      case 0: {  // Native
        return static_cast<int>(session_->GetNegotiatedProtocol());
      }
      default: NOTREACHED();
    }
  });
  return static_cast<NextProto>(family_result);
}

SpdyStream::ShouldRequeueStream SpdyStream::PossiblyResumeIfSendStalled() {
  // Proposed family boundary: Rust owns method control flow.
  const int64_t family_result = Http2FamilyRun(1, 33, [&](uint32_t op, int64_t arg) -> int64_t {
    (void)arg;
    switch (op) {
      case 0: {  // State
        return static_cast<int>(io_state_);
      }
      case 1: {  // Stalled
        return send_stalled_by_flow_control_;
      }
      case 2: {  // No
        return DoNotRequeue;
      }
      case 3: {  // SessionStalled
        return session_->IsSendStalled();
      }
      case 4: {  // Window
        return send_window_size_;
      }
      case 5: {  // Yes
        return Requeue;
      }
      case 6: {  // Log
        net_log_.AddEventWithIntParams(NetLogEventType::HTTP2_STREAM_FLOW_CONTROL_UNSTALLED, "stream_id", stream_id_);
        return 0;
      }
      case 7: {  // Clear
        send_stalled_by_flow_control_ = false;
        return 0;
      }
      case 8: {  // Queue
        QueueNextDataFrame();
        return 0;
      }
      default: NOTREACHED();
    }
  });
  return static_cast<ShouldRequeueStream>(family_result);
}

bool SpdyStream::IsClosed() const {
  // Proposed family boundary: Rust owns method control flow.
  const int64_t family_result = Http2FamilyRun(1, 34, [&](uint32_t op, int64_t arg) -> int64_t {
    (void)arg;
    switch (op) {
      case 0: {  // State
        return static_cast<int>(io_state_);
      }
      default: NOTREACHED();
    }
  });
  return family_result != 0;
}

bool SpdyStream::IsLocallyClosed() const {
  // Proposed family boundary: Rust owns method control flow.
  const int64_t family_result = Http2FamilyRun(1, 35, [&](uint32_t op, int64_t arg) -> int64_t {
    (void)arg;
    switch (op) {
      case 0: {  // State
        return static_cast<int>(io_state_);
      }
      default: NOTREACHED();
    }
  });
  return family_result != 0;
}

bool SpdyStream::IsRemoteClosed() const {
  // Proposed family boundary: Rust owns method control flow.
  const int64_t family_result = Http2FamilyRun(1, 36, [&](uint32_t op, int64_t arg) -> int64_t {
    (void)arg;
    switch (op) {
      case 0: {  // State
        return static_cast<int>(io_state_);
      }
      default: NOTREACHED();
    }
  });
  return family_result != 0;
}

bool SpdyStream::IsIdle() const {
  // Proposed family boundary: Rust owns method control flow.
  const int64_t family_result = Http2FamilyRun(1, 37, [&](uint32_t op, int64_t arg) -> int64_t {
    (void)arg;
    switch (op) {
      case 0: {  // State
        return static_cast<int>(io_state_);
      }
      default: NOTREACHED();
    }
  });
  return family_result != 0;
}

bool SpdyStream::IsOpen() const {
  // Proposed family boundary: Rust owns method control flow.
  const int64_t family_result = Http2FamilyRun(1, 38, [&](uint32_t op, int64_t arg) -> int64_t {
    (void)arg;
    switch (op) {
      case 0: {  // State
        return static_cast<int>(io_state_);
      }
      default: NOTREACHED();
    }
  });
  return family_result != 0;
}

bool SpdyStream::IsReservedRemote() const {
  // Proposed family boundary: Rust owns method control flow.
  const int64_t family_result = Http2FamilyRun(1, 39, [&](uint32_t op, int64_t arg) -> int64_t {
    (void)arg;
    switch (op) {
      case 0: {  // State
        return static_cast<int>(io_state_);
      }
      default: NOTREACHED();
    }
  });
  return family_result != 0;
}

void SpdyStream::AddRawReceivedBytes(size_t received_bytes) {
  // Proposed family boundary: Rust owns method control flow.
  Http2FamilyRun(1, 40, [&](uint32_t op, int64_t arg) -> int64_t {
    (void)arg;
    switch (op) {
      case 0: {  // Add
        raw_received_bytes_ += base::ByteSize(received_bytes);
        return 0;
      }
      default: NOTREACHED();
    }
  });
}

void SpdyStream::AddRawSentBytes(size_t sent_bytes) {
  // Proposed family boundary: Rust owns method control flow.
  Http2FamilyRun(1, 41, [&](uint32_t op, int64_t arg) -> int64_t {
    (void)arg;
    switch (op) {
      case 0: {  // Add
        raw_sent_bytes_ += base::ByteSize(sent_bytes);
        return 0;
      }
      default: NOTREACHED();
    }
  });
}

bool SpdyStream::GetLoadTimingInfo(LoadTimingInfo* load_timing_info) const {
  // Proposed family boundary: Rust owns method control flow.
  const int64_t family_result = Http2FamilyRun(1, 42, [&](uint32_t op, int64_t arg) -> int64_t {
    (void)arg;
    switch (op) {
      case 0: {  // Id
        return stream_id_;
      }
      case 1: {  // Get
        return session_->GetLoadTimingInfo(stream_id_, load_timing_info);
      }
      case 2: {  // Times
        load_timing_info->receive_headers_start = recv_first_byte_time_; load_timing_info->receive_non_informational_headers_start = recv_first_byte_time_for_non_informational_response_; load_timing_info->first_early_hints_time = first_early_hints_time_;
        return 0;
      }
      default: NOTREACHED();
    }
  });
  return family_result != 0;
}

base::DictValue SpdyStream::GetInfoAsValue() const {
  // Proposed family boundary: Rust owns method control flow.
  base::DictValue dict;
  Http2FamilyRun(1, 43, [&](uint32_t op, int64_t arg) -> int64_t {
    (void)arg;
    switch (op) {
      case 0: {  // Id
        dict.Set("stream_id", static_cast<int>(stream_id_));
        return 0;
      }
      case 1: {  // State
        dict.Set("io_state", DescribeState(io_state_));
        return 0;
      }
      case 2: {  // Stalled
        dict.Set("send_stalled_by_flow_control", send_stalled_by_flow_control_);
        return 0;
      }
      case 3: {  // Status
        dict.Set("pending_send_status", pending_send_status_);
        return 0;
      }
      default: NOTREACHED();
    }
  });
  return dict;
}

void SpdyStream::QueueNextDataFrame() {
  // Proposed family boundary: Rust owns method control flow.
  int effective_len = 0; bool end_stream = false; std::unique_ptr<SpdyBuffer> data_buffer;
  Http2FamilyRun(1, 44, [&](uint32_t op, int64_t arg) -> int64_t {
    (void)arg;
    switch (op) {
      case 0: {  // Checks
        CHECK(io_state_ == STATE_OPEN || io_state_ == STATE_HALF_CLOSED_REMOTE) << io_state_; CHECK_GT(stream_id_, 0u); CHECK(pending_send_data_.get());
        return 0;
      }
      case 1: {  // Status
        return static_cast<int>(pending_send_status_);
      }
      case 2: {  // FinalCheck
        CHECK_GE(pending_send_data_->BytesRemaining(), 0);
        return 0;
      }
      case 3: {  // MoreCheck
        CHECK_GT(pending_send_data_->BytesRemaining(), 0);
        return 0;
      }
      case 4: {  // Fin
        return spdy::DATA_FLAG_FIN;
      }
      case 5: {  // None
        return spdy::DATA_FLAG_NONE;
      }
      case 6: {  // Create
        data_buffer = session_->CreateDataBuffer(stream_id_, pending_send_data_.get(), pending_send_data_->BytesRemaining(), static_cast<spdy::SpdyDataFlags>(arg), &effective_len, &end_stream);
        return 0;
      }
      case 7: {  // HasBuffer
        return static_cast<bool>(data_buffer);
      }
      case 8: {  // MinimumCheck
        DCHECK_GE(data_buffer->GetRemainingSize(), spdy::kDataFrameMinimumSize);
        return 0;
      }
      case 9: {  // Size
        return static_cast<int64_t>(data_buffer->GetRemainingSize());
      }
      case 10: {  // Minimum
        return spdy::kDataFrameMinimumSize;
      }
      case 11: {  // PayloadCheck
        DCHECK_LE(static_cast<size_t>(arg), spdy::kHttp2DefaultFramePayloadLimit);
        return 0;
      }
      case 12: {  // Decrease
        DecreaseSendWindowSize(static_cast<int32_t>(arg));
        return 0;
      }
      case 13: {  // BindConsume
        data_buffer->AddConsumeCallback(base::BindRepeating(&SpdyStream::OnWriteBufferConsumed, GetWeakPtr(), static_cast<size_t>(arg)));
        return 0;
      }
      case 14: {  // Greased
        return session_->GreasedFramesEnabled();
      }
      case 15: {  // HasDelegate
        return delegate_ != nullptr;
      }
      case 16: {  // CanGrease
        return delegate_->CanGreaseFrameType();
      }
      case 17: {  // Grease
        session_->EnqueueGreasedFrame(GetWeakPtr());
        return 0;
      }
      case 18: {  // Log
        session_->net_log().AddEvent(NetLogEventType::HTTP2_SESSION_SEND_DATA, [&] { return NetLogSpdyDataParams(stream_id_, effective_len, end_stream); });
        return 0;
      }
      case 19: {  // Enqueue
        session_->EnqueueStreamWrite(GetWeakPtr(), spdy::SpdyFrameType::DATA, std::make_unique<SimpleBufferProducer>(std::move(data_buffer)));
        return 0;
      }
      default: NOTREACHED();
    }
  });
}

void SpdyStream::OnEarlyHintsReceived(
    const quiche::HttpHeaderBlock& response_headers,
    base::TimeTicks recv_first_byte_time) {
  // Proposed family boundary: Rust owns method control flow.
  Http2FamilyRun(1, 45, [&](uint32_t op, int64_t arg) -> int64_t {
    (void)arg;
    switch (op) {
      case 0: {  // FirstNull
        return first_early_hints_time_.is_null();
      }
      case 1: {  // SetFirst
        first_early_hints_time_ = recv_first_byte_time;
        return 0;
      }
      case 2: {  // TransferEncoding
        return response_headers.find("transfer-encoding") != response_headers.end();
      }
      case 3: {  // ErrorEncoding
        const char error[] = "Received transfer-encoding header"; LogStreamError(ERR_HTTP2_PROTOCOL_ERROR, error); session_->ResetStream(stream_id_, ERR_HTTP2_PROTOCOL_ERROR, error);
        return 0;
      }
      case 4: {  // Type
        return static_cast<int>(type_);
      }
      case 5: {  // RequestResponse
        return SPDY_REQUEST_RESPONSE_STREAM;
      }
      case 6: {  // State
        return static_cast<int>(io_state_);
      }
      case 7: {  // ErrorBefore
        const char error[] = "Early Hints received before request sent."; LogStreamError(ERR_HTTP2_PROTOCOL_ERROR, error); session_->ResetStream(stream_id_, ERR_HTTP2_PROTOCOL_ERROR, error);
        return 0;
      }
      case 8: {  // DelegateCheck
        CHECK(delegate_);
        return 0;
      }
      case 9: {  // Deliver
        delegate_->OnEarlyHintsReceived(response_headers);
        return 0;
      }
      default: NOTREACHED();
    }
  });
}

void SpdyStream::SaveResponseHeaders(
    const quiche::HttpHeaderBlock& response_headers,
    int status) {
  // Proposed family boundary: Rust owns method control flow.
  Http2FamilyRun(1, 46, [&](uint32_t op, int64_t arg) -> int64_t {
    (void)arg;
    switch (op) {
      case 0: {  // TransferEncoding
        return response_headers.contains("transfer-encoding");
      }
      case 1: {  // Reset
        session_->ResetStream(stream_id_, ERR_HTTP2_PROTOCOL_ERROR, "Received transfer-encoding header");
        return 0;
      }
      case 2: {  // Check
        DCHECK(response_headers_.empty());
        return 0;
      }
      case 3: {  // Clone
        response_headers_ = response_headers.Clone();
        return 0;
      }
      case 4: {  // HasDelegate
        return delegate_ != nullptr;
      }
      case 5: {  // Deliver
        delegate_->OnHeadersReceived(response_headers_);
        return 0;
      }
      default: NOTREACHED();
    }
  });
}

std::string SpdyStream::DescribeState(State state) {
  // Proposed family boundary: Rust owns method control flow.
  std::string description;
  Http2FamilyRun(1, 47, [&](uint32_t op, int64_t arg) -> int64_t {
    (void)arg;
    switch (op) {
      case 0: {  // State
        return static_cast<uint32_t>(state);
      }
      case 1: {  // Idle
        description = base::StringPrintf("%s (0x%08X)", "STATE_IDLE", state);
        return 0;
      }
      case 2: {  // Open
        description = base::StringPrintf("%s (0x%08X)", "STATE_OPEN", state);
        return 0;
      }
      case 3: {  // Local
        description = base::StringPrintf("%s (0x%08X)", "STATE_HALF_CLOSED_LOCAL", state);
        return 0;
      }
      case 4: {  // Closed
        description = base::StringPrintf("%s (0x%08X)", "STATE_CLOSED", state);
        return 0;
      }
      case 5: {  // Unknown
        description = base::StringPrintf("Unknown state 0x%08X (%u)", state, state);
        return 0;
      }
      default: NOTREACHED();
    }
  });
  return description;
}

#undef STATE_CASE

}  // namespace net
