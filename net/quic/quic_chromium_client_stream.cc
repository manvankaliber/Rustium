// Copyright 2012 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/quic/quic_chromium_client_stream.h"

#include <array>
#include <string_view>
#include <utility>

#include "base/containers/span.h"
#include "base/functional/bind.h"
#include "base/functional/callback_helpers.h"
#include "base/location.h"
#include "base/memory/ptr_util.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/scoped_refptr.h"
#include "base/metrics/histogram_functions.h"
#include "base/strings/strcat.h"
#include "base/task/single_thread_task_runner.h"
#include "net/base/io_buffer.h"
#include "net/base/net_errors.h"
#include "net/base/url_util.h"
#include "net/http/http_status_code.h"
#include "net/log/net_log_event_type.h"
#include "net/quic/http3_client_stream_rust_ffi.h"
#include "net/quic/quic_chromium_client_session.h"
#include "net/quic/quic_http_utils.h"
#include "net/spdy/spdy_log_util.h"
#include "net/third_party/quiche/src/quiche/quic/core/http/quic_spdy_session.h"
#include "net/third_party/quiche/src/quiche/quic/core/http/spdy_utils.h"
#include "net/third_party/quiche/src/quiche/quic/core/quic_utils.h"
#include "net/third_party/quiche/src/quiche/quic/core/quic_write_blocked_list.h"

namespace net {
static_assert(ERR_IO_PENDING == -1 && ERR_UNEXPECTED == -9 &&
                  ERR_CONNECTION_CLOSED == -100 &&
                  ERR_INVALID_RESPONSE == -320 &&
                  ERR_QUIC_PROTOCOL_ERROR == -356,
              "Rust policy error ABI");
namespace {
// Sets a boolean to a value, and restores it to the previous value once
// the saver goes out of scope.
class ScopedBoolSaver {
 public:
  ScopedBoolSaver(bool* var, bool new_val) : var_(var), old_val_(*var) {
    *var_ = new_val;
  }

  ~ScopedBoolSaver() { *var_ = old_val_; }

 private:
  raw_ptr<bool> var_;
  bool old_val_;
};
}  // namespace

QuicChromiumClientStream::Handle::Handle(
    QuicChromiumClientStream* stream,
    base::TimeDelta max_stream_limit_pending_delay)
    : stream_(stream),
      net_log_(stream->net_log()),
      max_stream_limit_pending_delay_(max_stream_limit_pending_delay) {
  SaveState();
}

base::TimeDelta
QuicChromiumClientStream::Handle::max_stream_limit_pending_delay() const {
  return max_stream_limit_pending_delay_;
}

QuicChromiumClientStream::Handle::~Handle() {
  auto services = [&](uint32_t operation, int64_t arg) -> int64_t {
    switch (operation) {
      case 0: {
        UnregisterHttp3DatagramVisitor();
        return 0;
      }
      case 1: {
        return static_cast<int64_t>(stream_ != nullptr);
      }
      case 2: {
        stream_->ClearHandle();
        return 0;
      }
      default:
        CHECK(false);
    }
  };
  RunHttp3StreamPolicy(0, services);
}

void QuicChromiumClientStream::Handle::OnEarlyHintsAvailable() {
  int rv = 0;
  auto services = [&](uint32_t operation, int64_t arg) -> int64_t {
    switch (operation) {
      case 0: {
        return static_cast<int64_t>(first_early_hints_time_.is_null());
      }
      case 1: {
        first_early_hints_time_ = base::TimeTicks::Now();
        return 0;
      }
      case 2: {
        return static_cast<int64_t>(!read_headers_callback_.is_null());
      }
      case 3: {
        DCHECK(read_headers_buffer_);
        return 0;
      }
      case 4: {
        rv = stream_->DeliverEarlyHints(read_headers_buffer_);
        DCHECK_NE(ERR_IO_PENDING, rv);
        return 0;
      }
      case 5: {
        ResetAndRun(std::move(read_headers_callback_), rv);
        return 0;
      }
      default:
        CHECK(false);
    }
  };
  RunHttp3StreamPolicy(1, services);
}

void QuicChromiumClientStream::Handle::OnInitialHeadersAvailable() {
  int rv = 0;
  auto services = [&](uint32_t operation, int64_t arg) -> int64_t {
    switch (operation) {
      case 0: {
        return static_cast<int64_t>(headers_received_start_time_.is_null());
      }
      case 1: {
        headers_received_start_time_ = base::TimeTicks::Now();
        return 0;
      }
      case 2: {
        return static_cast<int64_t>(!read_headers_callback_.is_null());
      }
      case 3: {
        rv = stream_->DeliverInitialHeaders(read_headers_buffer_);
        DCHECK_NE(ERR_IO_PENDING, rv);
        return 0;
      }
      case 4: {
        ResetAndRun(std::move(read_headers_callback_), rv);
        return 0;
      }
      default:
        CHECK(false);
    }
  };
  RunHttp3StreamPolicy(2, services);
}

void QuicChromiumClientStream::Handle::OnTrailingHeadersAvailable() {
  int rv = ERR_QUIC_PROTOCOL_ERROR;
  auto services = [&](uint32_t operation, int64_t arg) -> int64_t {
    switch (operation) {
      case 0: {
        return static_cast<int64_t>(!read_headers_callback_.is_null());
      }
      case 1: {
        return static_cast<int64_t>(
            stream_->DeliverTrailingHeaders(read_headers_buffer_, &rv));
      }
      case 2: {
        rv = ERR_QUIC_PROTOCOL_ERROR;
        return 0;
      }
      case 3: {
        base::UmaHistogramBoolean(
            "Net.QuicChromiumClientStream.TrailingHeadersProcessSuccess",
            rv >= 0);
        return 0;
      }
      case 4: {
        ResetAndRun(std::move(read_headers_callback_), rv);
        return 0;
      }
      default:
        CHECK(false);
    }
  };
  RunHttp3StreamPolicy(3, services);
}

void QuicChromiumClientStream::Handle::OnDataAvailable() {
  int rv = 0;
  auto services = [&](uint32_t operation, int64_t arg) -> int64_t {
    switch (operation) {
      case 0: {
        return static_cast<int64_t>(!read_body_callback_.is_null());
      }
      case 1: {
        DCHECK(read_body_buffer_);
        DCHECK_GT(read_body_buffer_len_, 0);
        return 0;
      }
      case 2: {
        return static_cast<int64_t>(
            rv = stream_->Read(read_body_buffer_.get(), read_body_buffer_len_));
      }
      case 3: {
        read_body_buffer_ = nullptr;
        read_body_buffer_len_ = 0;
        return 0;
      }
      case 4: {
        ResetAndRun(std::move(read_body_callback_), rv);
        return 0;
      }
      default:
        CHECK(false);
    }
  };
  RunHttp3StreamPolicy(4, services);
}

void QuicChromiumClientStream::Handle::OnCanWrite() {
  auto services = [&](uint32_t operation, int64_t arg) -> int64_t {
    switch (operation) {
      case 0: {
        return static_cast<int64_t>(!write_callback_.is_null());
      }
      case 1: {
        ResetAndRun(std::move(write_callback_), OK);
        return 0;
      }
      default:
        CHECK(false);
    }
  };
  RunHttp3StreamPolicy(5, services);
}

void QuicChromiumClientStream::Handle::OnClose() {
  auto services = [&](uint32_t operation, int64_t arg) -> int64_t {
    switch (operation) {
      case 0: {
        return static_cast<int64_t>(net_error_);
      }
      case 1: {
        return static_cast<int64_t>(stream_error() ==
                                    quic::QUIC_STREAM_NO_ERROR);
      }
      case 2: {
        return static_cast<int64_t>(connection_error() == quic::QUIC_NO_ERROR);
      }
      case 3: {
        return static_cast<int64_t>(fin_sent());
      }
      case 4: {
        return static_cast<int64_t>(fin_received());
      }
      case 5: {
        net_error_ = static_cast<int>(arg);
        return 0;
      }
      case 6: {
        base::UmaHistogramSparse(
            "Net.QuicChromiumClientStream.HandleOnCloseNetError", -net_error_);
        base::UmaHistogramSparse(
            "Net.QuicChromiumClientStream.HandleOnCloseStreamError",
            stream_error());
        base::UmaHistogramSparse(
            "Net.QuicChromiumClientStream.HandleOnCloseConnectionError",
            connection_error());
        return 0;
      }
      case 7: {
        OnError(net_error_);
        return 0;
      }
      default:
        CHECK(false);
    }
  };
  RunHttp3StreamPolicy(6, services);
}

void QuicChromiumClientStream::Handle::OnError(int error) {
  auto services = [&](uint32_t operation, int64_t arg) -> int64_t {
    switch (operation) {
      case 0: {
        net_error_ = error;
        return 0;
      }
      case 1: {
        UnregisterHttp3DatagramVisitor();
        return 0;
      }
      case 2: {
        return static_cast<int64_t>(stream_ != nullptr);
      }
      case 3: {
        SaveState();
        return 0;
      }
      case 4: {
        stream_ = nullptr;
        return 0;
      }
      case 5: {
        base::SingleThreadTaskRunner::GetCurrentDefault()->PostTask(
            FROM_HERE,
            base::BindOnce(
                &QuicChromiumClientStream::Handle::InvokeCallbacksOnClose,
                weak_factory_.GetWeakPtr(), error));
        return 0;
      }
      default:
        CHECK(false);
    }
  };
  RunHttp3StreamPolicy(7, services);
}

void QuicChromiumClientStream::Handle::InvokeCallbacksOnClose(int error) {
  auto guard = weak_factory_.GetWeakPtr();
  std::array callbacks = {&read_headers_callback_, &read_body_callback_,
                          &write_callback_};
  auto services = [&](uint32_t operation, int64_t arg) -> int64_t {
    switch (operation) {
      case 0: {
        read_body_buffer_ = nullptr;
        read_body_buffer_len_ = 0;
        return 0;
      }
      case 1: {
        return static_cast<int64_t>(
            !callbacks[static_cast<size_t>(arg)]->is_null());
      }
      case 2: {
        ResetAndRun(std::move(*callbacks[static_cast<size_t>(arg)]), error);
        return 0;
      }
      case 3: {
        return static_cast<int64_t>(guard.get() != nullptr);
      }
      default:
        CHECK(false);
    }
  };
  RunHttp3StreamPolicy(8, services);
}

int QuicChromiumClientStream::Handle::ReadInitialHeaders(
    quiche::HttpHeaderBlock* header_block,
    CompletionOnceCallback callback) {
  ScopedBoolSaver saver(&may_invoke_callbacks_, false);
  int rv = 0;
  auto services = [&](uint32_t operation, int64_t arg) -> int64_t {
    switch (operation) {
      case 0: {
        return static_cast<int64_t>(stream_ != nullptr);
      }
      case 1: {
        return static_cast<int64_t>(net_error_);
      }
      case 2: {
        return static_cast<int64_t>(
            rv = stream_->DeliverEarlyHints(header_block));
      }
      case 3: {
        return static_cast<int64_t>(
            rv = stream_->DeliverInitialHeaders(header_block));
      }
      case 4: {
        read_headers_buffer_ = header_block;
        DCHECK(!read_headers_callback_);
        return 0;
      }
      case 5: {
        SetCallback(std::move(callback), &read_headers_callback_);
        return 0;
      }
      default:
        CHECK(false);
    }
  };
  return static_cast<int>(RunHttp3StreamPolicy(9, services));
}

int QuicChromiumClientStream::Handle::ReadBody(
    IOBuffer* buffer,
    int buffer_len,
    CompletionOnceCallback callback) {
  ScopedBoolSaver saver(&may_invoke_callbacks_, false);
  auto services = [&](uint32_t operation, int64_t arg) -> int64_t {
    switch (operation) {
      case 0: {
        return static_cast<int64_t>(IsDoneReading());
      }
      case 1: {
        return static_cast<int64_t>(stream_ != nullptr);
      }
      case 2: {
        return static_cast<int64_t>(net_error_);
      }
      case 3: {
        return static_cast<int64_t>(stream_->read_side_closed());
      }
      case 4: {
        return static_cast<int64_t>(stream_->Read(buffer, buffer_len));
      }
      case 5: {
        DCHECK(buffer);
        DCHECK_GT(buffer_len, 0);
        return 0;
      }
      case 6: {
        SetCallback(std::move(callback), &read_body_callback_);
        return 0;
      }
      case 7: {
        read_body_buffer_ = buffer;
        read_body_buffer_len_ = buffer_len;
        return 0;
      }
      default:
        CHECK(false);
    }
  };
  return static_cast<int>(RunHttp3StreamPolicy(10, services));
}

int QuicChromiumClientStream::Handle::ReadTrailingHeaders(
    quiche::HttpHeaderBlock* header_block,
    CompletionOnceCallback callback) {
  ScopedBoolSaver saver(&may_invoke_callbacks_, false);
  int frame_len = 0;
  auto services = [&](uint32_t operation, int64_t arg) -> int64_t {
    switch (operation) {
      case 0: {
        return static_cast<int64_t>(stream_ != nullptr);
      }
      case 1: {
        return static_cast<int64_t>(net_error_);
      }
      case 2: {
        return static_cast<int64_t>(
            stream_->DeliverTrailingHeaders(header_block, &frame_len));
      }
      case 3: {
        return static_cast<int64_t>(frame_len);
      }
      case 4: {
        read_headers_buffer_ = header_block;
        return 0;
      }
      case 5: {
        SetCallback(std::move(callback), &read_headers_callback_);
        return 0;
      }
      default:
        CHECK(false);
    }
  };
  return static_cast<int>(RunHttp3StreamPolicy(11, services));
}

int QuicChromiumClientStream::Handle::WriteHeaders(
    quiche::HttpHeaderBlock header_block,
    bool fin,
    quiche::QuicheReferenceCountedPointer<quic::QuicAckListenerInterface>
        ack_notifier_delegate) {
  auto services = [&](uint32_t operation, int64_t arg) -> int64_t {
    switch (operation) {
      case 0: {
        return static_cast<int64_t>(stream_ != nullptr);
      }
      case 1: {
        return static_cast<int64_t>(HandleIOComplete(stream_->WriteHeaders(
            std::move(header_block), fin, ack_notifier_delegate)));
      }
      default:
        CHECK(false);
    }
  };
  return static_cast<int>(RunHttp3StreamPolicy(12, services));
}

int QuicChromiumClientStream::Handle::WriteStreamData(
    std::string_view data,
    bool fin,
    CompletionOnceCallback callback) {
  ScopedBoolSaver saver(&may_invoke_callbacks_, false);
  auto services = [&](uint32_t operation, int64_t arg) -> int64_t {
    switch (operation) {
      case 0: {
        return static_cast<int64_t>(stream_ != nullptr);
      }
      case 1: {
        return static_cast<int64_t>(net_error_);
      }
      case 2: {
        return static_cast<int64_t>(stream_->WriteStreamData(data, fin));
      }
      case 3: {
        return static_cast<int64_t>(HandleIOComplete(OK));
      }
      case 4: {
        SetCallback(std::move(callback), &write_callback_);
        return 0;
      }
      default:
        CHECK(false);
    }
  };
  return static_cast<int>(RunHttp3StreamPolicy(13, services));
}

int QuicChromiumClientStream::Handle::WritevStreamData(
    const std::vector<scoped_refptr<IOBuffer>>& buffers,
    const std::vector<int>& lengths,
    bool fin,
    CompletionOnceCallback callback) {
  ScopedBoolSaver saver(&may_invoke_callbacks_, false);
  auto services = [&](uint32_t operation, int64_t arg) -> int64_t {
    switch (operation) {
      case 0: {
        return static_cast<int64_t>(stream_ != nullptr);
      }
      case 1: {
        return static_cast<int64_t>(net_error_);
      }
      case 2: {
        return static_cast<int64_t>(
            stream_->WritevStreamData(buffers, lengths, fin));
      }
      case 3: {
        return static_cast<int64_t>(HandleIOComplete(OK));
      }
      case 4: {
        SetCallback(std::move(callback), &write_callback_);
        return 0;
      }
      default:
        CHECK(false);
    }
  };
  return static_cast<int>(RunHttp3StreamPolicy(14, services));
}

int QuicChromiumClientStream::Handle::WriteConnectUdpPayload(
    std::string_view packet) {
  ScopedBoolSaver saver(&may_invoke_callbacks_, false);
  std::string http_payload;
  quic::DatagramStatus message_status;
  auto services = [&](uint32_t operation, int64_t arg) -> int64_t {
    switch (operation) {
      case 0: {
        return static_cast<int64_t>(stream_ != nullptr);
      }
      case 1: {
        return static_cast<int64_t>(net_error_);
      }
      case 2: {
        base::UmaHistogramBoolean(kHttp3DatagramDroppedHistogram,
                                  !stream_->SupportsH3Datagram());
        return 0;
      }
      case 3: {
        return static_cast<int64_t>(stream_->SupportsH3Datagram());
      }
      case 4: {
        DLOG(WARNING)
            << "Dropping datagram because the session has either not received "
               "settings frame with H3_DATAGRAM yet or received settings that "
               "indicate datagrams are not supported (i.e., H3_DATAGRAM=0).";
        return 0;
      }
      case 5: {
        http_payload = base::StrCat({std::string(1, '\0'), packet});
        return 0;
      }
      case 6: {
        message_status = stream_->SendHttp3Datagram(http_payload);
        return 0;
      }
      case 7: {
        return static_cast<int64_t>(
            message_status == quic::DatagramStatus::DATAGRAM_STATUS_SUCCESS);
      }
      case 8: {
        return static_cast<int64_t>(
            message_status == quic::DatagramStatus::DATAGRAM_STATUS_BLOCKED);
      }
      case 9: {
        return static_cast<int64_t>(HandleIOComplete(OK));
      }
      case 10: {
        DCHECK(
            message_status !=
            quic::DatagramStatus::DATAGRAM_STATUS_ENCRYPTION_NOT_ESTABLISHED);
        DCHECK(message_status !=
               quic::DatagramStatus::DATAGRAM_STATUS_TOO_LARGE);
        DLOG(ERROR) << "Failed to send Http3 Datagram on " << stream_->id();
        return 0;
      }
      case 11: {
        stream_->Reset(quic::QUIC_STREAM_CANCELLED);
        return 0;
      }
      default:
        CHECK(false);
    }
  };
  return static_cast<int>(RunHttp3StreamPolicy(15, services));
}

int QuicChromiumClientStream::Handle::Read(IOBuffer* buf, int buf_len) {
  auto services = [&](uint32_t operation, int64_t arg) -> int64_t {
    switch (operation) {
      case 0: {
        return static_cast<int64_t>(stream_ != nullptr);
      }
      case 1: {
        return static_cast<int64_t>(net_error_);
      }
      case 2: {
        return static_cast<int64_t>(stream_->Read(buf, buf_len));
      }
      default:
        CHECK(false);
    }
  };
  return static_cast<int>(RunHttp3StreamPolicy(16, services));
}

void QuicChromiumClientStream::Handle::OnFinRead() {
  auto services = [&](uint32_t operation, int64_t arg) -> int64_t {
    switch (operation) {
      case 0: {
        read_headers_callback_.Reset();
        return 0;
      }
      case 1: {
        return static_cast<int64_t>(stream_ != nullptr);
      }
      case 2: {
        stream_->OnFinRead();
        return 0;
      }
      default:
        CHECK(false);
    }
  };
  RunHttp3StreamPolicy(17, services);
}

void QuicChromiumClientStream::Handle::
    DisableConnectionMigrationToCellularNetwork() {
  auto services = [&](uint32_t operation, int64_t arg) -> int64_t {
    switch (operation) {
      case 0: {
        return static_cast<int64_t>(stream_ != nullptr);
      }
      case 1: {
        stream_->DisableConnectionMigrationToCellularNetwork();
        return 0;
      }
      default:
        CHECK(false);
    }
  };
  RunHttp3StreamPolicy(18, services);
}

void QuicChromiumClientStream::Handle::SetPriority(
    const quic::QuicStreamPriority& priority) {
  auto services = [&](uint32_t operation, int64_t arg) -> int64_t {
    switch (operation) {
      case 0: {
        return static_cast<int64_t>(stream_ != nullptr);
      }
      case 1: {
        stream_->SetPriority(priority);
        return 0;
      }
      default:
        CHECK(false);
    }
  };
  RunHttp3StreamPolicy(19, services);
}

void QuicChromiumClientStream::Handle::Reset(
    quic::QuicRstStreamErrorCode error_code) {
  auto services = [&](uint32_t operation, int64_t arg) -> int64_t {
    switch (operation) {
      case 0: {
        UnregisterHttp3DatagramVisitor();
        return 0;
      }
      case 1: {
        return static_cast<int64_t>(stream_ != nullptr);
      }
      case 2: {
        stream_->Reset(error_code);
        return 0;
      }
      default:
        CHECK(false);
    }
  };
  RunHttp3StreamPolicy(20, services);
}

void QuicChromiumClientStream::Handle::RegisterHttp3DatagramVisitor(
    Http3DatagramVisitor* visitor) {
  auto services = [&](uint32_t operation, int64_t arg) -> int64_t {
    switch (operation) {
      case 0: {
        return static_cast<int64_t>(stream_ != nullptr);
      }
      case 1: {
        stream_->RegisterHttp3DatagramVisitor(visitor);
        datagram_visitor_registered_ = true;
        return 0;
      }
      default:
        CHECK(false);
    }
  };
  RunHttp3StreamPolicy(21, services);
}

void QuicChromiumClientStream::Handle::UnregisterHttp3DatagramVisitor() {
  auto services = [&](uint32_t operation, int64_t arg) -> int64_t {
    switch (operation) {
      case 0: {
        return static_cast<int64_t>(stream_ != nullptr);
      }
      case 1: {
        return static_cast<int64_t>(datagram_visitor_registered_);
      }
      case 2: {
        stream_->UnregisterHttp3DatagramVisitor();
        return 0;
      }
      case 3: {
        datagram_visitor_registered_ = false;
        return 0;
      }
      default:
        CHECK(false);
    }
  };
  RunHttp3StreamPolicy(22, services);
}

quic::QuicStreamId QuicChromiumClientStream::Handle::id() const {
  auto services = [&](uint32_t operation, int64_t arg) -> int64_t {
    switch (operation) {
      case 0: {
        return static_cast<int64_t>(stream_ != nullptr);
      }
      case 1: {
        return static_cast<int64_t>(id_);
      }
      case 2: {
        return static_cast<int64_t>(stream_->id());
      }
      default:
        CHECK(false);
    }
  };
  return static_cast<quic::QuicStreamId>(RunHttp3StreamPolicy(23, services));
}

quic::QuicErrorCode QuicChromiumClientStream::Handle::connection_error() const {
  auto services = [&](uint32_t operation, int64_t arg) -> int64_t {
    switch (operation) {
      case 0: {
        return static_cast<int64_t>(stream_ != nullptr);
      }
      case 1: {
        return static_cast<int64_t>(connection_error_);
      }
      case 2: {
        return static_cast<int64_t>(stream_->connection_error());
      }
      default:
        CHECK(false);
    }
  };
  return static_cast<quic::QuicErrorCode>(RunHttp3StreamPolicy(24, services));
}

quic::QuicRstStreamErrorCode QuicChromiumClientStream::Handle::stream_error()
    const {
  auto services = [&](uint32_t operation, int64_t arg) -> int64_t {
    switch (operation) {
      case 0: {
        return static_cast<int64_t>(stream_ != nullptr);
      }
      case 1: {
        return static_cast<int64_t>(stream_error_);
      }
      case 2: {
        return static_cast<int64_t>(stream_->stream_error());
      }
      default:
        CHECK(false);
    }
  };
  return static_cast<quic::QuicRstStreamErrorCode>(
      RunHttp3StreamPolicy(25, services));
}

uint64_t QuicChromiumClientStream::Handle::connection_wire_error() const {
  auto services = [&](uint32_t operation, int64_t arg) -> int64_t {
    switch (operation) {
      case 0: {
        return static_cast<int64_t>(stream_ != nullptr);
      }
      case 1: {
        return static_cast<int64_t>(connection_wire_error_);
      }
      case 2: {
        CHECK(stream_->session());
        return 0;
      }
      case 3: {
        return static_cast<int64_t>(stream_->session()->wire_error());
      }
      default:
        CHECK(false);
    }
  };
  return static_cast<uint64_t>(RunHttp3StreamPolicy(26, services));
}

uint64_t QuicChromiumClientStream::Handle::ietf_application_error() const {
  auto services = [&](uint32_t operation, int64_t arg) -> int64_t {
    switch (operation) {
      case 0: {
        return static_cast<int64_t>(stream_ != nullptr);
      }
      case 1: {
        return static_cast<int64_t>(ietf_application_error_);
      }
      case 2: {
        return static_cast<int64_t>(stream_->ietf_application_error());
      }
      default:
        CHECK(false);
    }
  };
  return static_cast<uint64_t>(RunHttp3StreamPolicy(27, services));
}

bool QuicChromiumClientStream::Handle::fin_sent() const {
  auto services = [&](uint32_t operation, int64_t arg) -> int64_t {
    switch (operation) {
      case 0: {
        return static_cast<int64_t>(stream_ != nullptr);
      }
      case 1: {
        return static_cast<int64_t>(fin_sent_);
      }
      case 2: {
        return static_cast<int64_t>(stream_->fin_sent());
      }
      default:
        CHECK(false);
    }
  };
  return static_cast<bool>(RunHttp3StreamPolicy(28, services));
}

bool QuicChromiumClientStream::Handle::fin_received() const {
  auto services = [&](uint32_t operation, int64_t arg) -> int64_t {
    switch (operation) {
      case 0: {
        return static_cast<int64_t>(stream_ != nullptr);
      }
      case 1: {
        return static_cast<int64_t>(fin_received_);
      }
      case 2: {
        return static_cast<int64_t>(stream_->fin_received());
      }
      default:
        CHECK(false);
    }
  };
  return static_cast<bool>(RunHttp3StreamPolicy(29, services));
}

uint64_t QuicChromiumClientStream::Handle::stream_bytes_read() const {
  auto services = [&](uint32_t operation, int64_t arg) -> int64_t {
    switch (operation) {
      case 0: {
        return static_cast<int64_t>(stream_ != nullptr);
      }
      case 1: {
        return static_cast<int64_t>(stream_bytes_read_);
      }
      case 2: {
        return static_cast<int64_t>(stream_->stream_bytes_read());
      }
      default:
        CHECK(false);
    }
  };
  return static_cast<uint64_t>(RunHttp3StreamPolicy(30, services));
}

uint64_t QuicChromiumClientStream::Handle::stream_bytes_written() const {
  auto services = [&](uint32_t operation, int64_t arg) -> int64_t {
    switch (operation) {
      case 0: {
        return static_cast<int64_t>(stream_ != nullptr);
      }
      case 1: {
        return static_cast<int64_t>(stream_bytes_written_);
      }
      case 2: {
        return static_cast<int64_t>(stream_->stream_bytes_written());
      }
      default:
        CHECK(false);
    }
  };
  return static_cast<uint64_t>(RunHttp3StreamPolicy(31, services));
}

size_t QuicChromiumClientStream::Handle::NumBytesConsumed() const {
  auto services = [&](uint32_t operation, int64_t arg) -> int64_t {
    switch (operation) {
      case 0: {
        return static_cast<int64_t>(stream_ != nullptr);
      }
      case 1: {
        return static_cast<int64_t>(num_bytes_consumed_);
      }
      case 2: {
        return static_cast<int64_t>(stream_->sequencer()->NumBytesConsumed());
      }
      default:
        CHECK(false);
    }
  };
  return static_cast<size_t>(RunHttp3StreamPolicy(32, services));
}

bool QuicChromiumClientStream::Handle::HasBytesToRead() const {
  auto services = [&](uint32_t operation, int64_t arg) -> int64_t {
    switch (operation) {
      case 0: {
        return static_cast<int64_t>(stream_ != nullptr);
      }
      case 1: {
        return static_cast<int64_t>(false);
      }
      case 2: {
        return static_cast<int64_t>(stream_->HasBytesToRead());
      }
      default:
        CHECK(false);
    }
  };
  return static_cast<bool>(RunHttp3StreamPolicy(33, services));
}

bool QuicChromiumClientStream::Handle::IsDoneReading() const {
  auto services = [&](uint32_t operation, int64_t arg) -> int64_t {
    switch (operation) {
      case 0: {
        return static_cast<int64_t>(stream_ != nullptr);
      }
      case 1: {
        return static_cast<int64_t>(is_done_reading_);
      }
      case 2: {
        return static_cast<int64_t>(stream_->IsDoneReading());
      }
      default:
        CHECK(false);
    }
  };
  return static_cast<bool>(RunHttp3StreamPolicy(34, services));
}

bool QuicChromiumClientStream::Handle::IsFirstStream() const {
  auto services = [&](uint32_t operation, int64_t arg) -> int64_t {
    switch (operation) {
      case 0: {
        return static_cast<int64_t>(stream_ != nullptr);
      }
      case 1: {
        return static_cast<int64_t>(is_first_stream_);
      }
      case 2: {
        return static_cast<int64_t>(stream_->IsFirstStream());
      }
      default:
        CHECK(false);
    }
  };
  return static_cast<bool>(RunHttp3StreamPolicy(35, services));
}

bool QuicChromiumClientStream::Handle::can_migrate_to_cellular_network() {
  auto services = [&](uint32_t operation, int64_t arg) -> int64_t {
    switch (operation) {
      case 0: {
        return static_cast<int64_t>(stream_ != nullptr);
      }
      case 1: {
        return static_cast<int64_t>(false);
      }
      case 2: {
        return static_cast<int64_t>(stream_->CanMigrateToCellularNetwork());
      }
      default:
        CHECK(false);
    }
  };
  return static_cast<bool>(RunHttp3StreamPolicy(36, services));
}

const NetLogWithSource& QuicChromiumClientStream::Handle::net_log() const {
  return net_log_;
}

void QuicChromiumClientStream::Handle::SaveState() {
  DCHECK(stream_);
  fin_sent_ = stream_->fin_sent();
  fin_received_ = stream_->fin_received();
  num_bytes_consumed_ = stream_->sequencer()->NumBytesConsumed();
  id_ = stream_->id();
  connection_error_ = stream_->connection_error();
  stream_error_ = stream_->stream_error();
  // TODO(crbug.com/40715622): Don't access stream_->session(). Instead, update
  // quic::QuicStream::OnConnectionClosed() to take the wire error code.
  CHECK(stream_->session());
  connection_wire_error_ = stream_->session()->wire_error();
  ietf_application_error_ = stream_->ietf_application_error();
  is_done_reading_ = stream_->IsDoneReading();
  is_first_stream_ = stream_->IsFirstStream();
  stream_bytes_read_ = stream_->stream_bytes_read();
  stream_bytes_written_ = stream_->stream_bytes_written();
}

void QuicChromiumClientStream::Handle::SetCallback(
    CompletionOnceCallback new_callback,
    CompletionOnceCallback* callback) {
  // TODO(rch): Convert this to a DCHECK once we ensure the API is stable and
  // bug free.
  CHECK(http3_client_stream_callback_gate(may_invoke_callbacks_, false));
  *callback = std::move(new_callback);
}

void QuicChromiumClientStream::Handle::ResetAndRun(
    CompletionOnceCallback callback,
    int rv) {
  // TODO(rch): Convert this to a DCHECK once we ensure the API is stable and
  // bug free.
  CHECK(http3_client_stream_callback_gate(may_invoke_callbacks_, true));
  std::move(callback).Run(rv);
}

int QuicChromiumClientStream::Handle::HandleIOComplete(int rv) {
  auto services = [&](uint32_t operation, int64_t arg) -> int64_t {
    switch (operation) {
      case 0: {
        return static_cast<int64_t>(rv);
      }
      case 1: {
        return static_cast<int64_t>(stream_ != nullptr);
      }
      case 2: {
        return static_cast<int64_t>(stream_error_ ==
                                    quic::QUIC_STREAM_NO_ERROR);
      }
      case 3: {
        return static_cast<int64_t>(connection_error_ == quic::QUIC_NO_ERROR);
      }
      case 4: {
        return static_cast<int64_t>(fin_sent_);
      }
      case 5: {
        return static_cast<int64_t>(fin_received_);
      }
      case 6: {
        return static_cast<int64_t>(net_error_);
      }
      default:
        CHECK(false);
    }
  };
  return static_cast<int>(RunHttp3StreamPolicy(38, services));
}

void QuicChromiumClientStream::Handle::SetRequestIdempotency(
    Idempotency idempotency) {
  idempotency_ = idempotency;
}

Idempotency QuicChromiumClientStream::Handle::GetRequestIdempotency() const {
  return idempotency_;
}

quic::QuicPacketLength
QuicChromiumClientStream::Handle::GetGuaranteedLargestMessagePayload() const {
  auto services = [&](uint32_t operation, int64_t arg) -> int64_t {
    switch (operation) {
      case 0: {
        return static_cast<int64_t>(stream_ != nullptr);
      }
      case 1: {
        return static_cast<int64_t>(0);
      }
      case 2: {
        return static_cast<int64_t>(
            stream_->GetGuaranteedLargestMessagePayload());
      }
      default:
        CHECK(false);
    }
  };
  return static_cast<quic::QuicPacketLength>(
      RunHttp3StreamPolicy(37, services));
}

QuicChromiumClientStream::QuicChromiumClientStream(
    quic::QuicStreamId id,
    quic::QuicSpdyClientSessionBase* session,
    quic::QuicServerId server_id,
    quic::StreamType type,
    const NetLogWithSource& net_log,
    const NetworkTrafficAnnotationTag& traffic_annotation,
    std::optional<base::TimeDelta> max_stream_limit_pending_delay)
    : QuicChromiumClientStreamBase(id, session, type),
      net_log_(net_log),
      session_(session),
      server_id_(std::move(server_id)),
      quic_version_(session->connection()->transport_version()),
      max_stream_limit_pending_delay_(max_stream_limit_pending_delay) {}

QuicChromiumClientStream::~QuicChromiumClientStream() {
  auto services = [&](uint32_t operation, int64_t arg) -> int64_t {
    switch (operation) {
      case 0: {
        return static_cast<int64_t>(handle_ != nullptr);
      }
      case 1: {
        handle_->OnClose();
        return 0;
      }
      default:
        CHECK(false);
    }
  };
  RunHttp3StreamPolicy(39, services);
}

void QuicChromiumClientStream::OnInitialHeadersComplete(
    bool fin,
    size_t frame_len,
    const quic::QuicHeaderList& header_list) {
  quiche::HttpHeaderBlock header_block;
  int64_t length = -1;
  int response_code = 0;
  auto services = [&](uint32_t operation, int64_t arg) -> int64_t {
    switch (operation) {
      case 0: {
        DCHECK(!initial_headers_arrived_);
        return 0;
      }
      case 1: {
        quic::QuicSpdyStream::OnInitialHeadersComplete(fin, frame_len,
                                                       header_list);
        return 0;
      }
      case 2: {
        return static_cast<int64_t>(header_decoding_delay().has_value());
      }
      case 3: {
        base::UmaHistogramTimes(
            "Net.QuicChromiumClientStream.HeaderDecodingDelay",
            base::Milliseconds(header_decoding_delay()->ToMilliseconds()));
        return 0;
      }
      case 4: {
        return static_cast<int64_t>(IsGoogleHost(server_id_.host()));
      }
      case 5: {
        base::UmaHistogramTimes(
            "Net.QuicChromiumClientStream.HeaderDecodingDelayGoogle",
            base::Milliseconds(header_decoding_delay()->ToMilliseconds()));
        return 0;
      }
      case 6: {
        return static_cast<int64_t>(quic::SpdyUtils::CopyAndValidateHeaders(
            header_list, &length, &header_block));
      }
      case 7: {
        DLOG(ERROR) << "Failed to parse header list: "
                    << header_list.DebugString();
        return 0;
      }
      case 8: {
        ConsumeHeaderList();
        return 0;
      }
      case 9: {
        Reset(quic::QUIC_BAD_APPLICATION_PAYLOAD);
        return 0;
      }
      case 10: {
        return static_cast<int64_t>(
            ParseHeaderStatusCode(header_block, &response_code));
      }
      case 11: {
        DLOG(ERROR) << "Received invalid response code: '"
                    << header_block[":status"].as_string() << "' on stream "
                    << id();
        return 0;
      }
      case 12: {
        Reset(quic::QUIC_BAD_APPLICATION_PAYLOAD);
        return 0;
      }
      case 13: {
        return static_cast<int64_t>(response_code);
      }
      case 14: {
        DLOG(ERROR) << "Received forbidden 101 response code on stream "
                    << id();
        return 0;
      }
      case 15: {
        Reset(quic::QUIC_BAD_APPLICATION_PAYLOAD);
        return 0;
      }
      case 16: {
        set_headers_decompressed(false);
        return 0;
      }
      case 17: {
        ConsumeHeaderList();
        return 0;
      }
      case 18: {
        early_hints_.emplace_back(std::move(header_block), frame_len);
        return 0;
      }
      case 19: {
        return static_cast<int64_t>(handle_ != nullptr);
      }
      case 20: {
        handle_->OnEarlyHintsAvailable();
        return 0;
      }
      case 21: {
        DVLOG(1) << "Ignore informational response " << response_code
                 << " on stream" << id();
        return 0;
      }
      case 22: {
        ConsumeHeaderList();
        return 0;
      }
      case 23: {
        initial_headers_arrived_ = true;
        initial_headers_ = std::move(header_block);
        initial_headers_frame_len_ = frame_len;
        return 0;
      }
      case 24: {
        return static_cast<int64_t>(handle_ != nullptr);
      }
      case 25: {
        NotifyHandleOfInitialHeadersAvailableLater();
        return 0;
      }
      default:
        CHECK(false);
    }
  };
  RunHttp3StreamPolicy(40, services);
}

void QuicChromiumClientStream::OnTrailingHeadersComplete(
    bool fin,
    size_t frame_len,
    const quic::QuicHeaderList& header_list) {
  auto services = [&](uint32_t operation, int64_t arg) -> int64_t {
    switch (operation) {
      case 0: {
        quic::QuicSpdyStream::OnTrailingHeadersComplete(fin, frame_len,
                                                        header_list);
        return 0;
      }
      case 1: {
        trailing_headers_frame_len_ = frame_len;
        return 0;
      }
      case 2: {
        return static_cast<int64_t>(handle_ != nullptr);
      }
      case 3: {
        NotifyHandleOfTrailingHeadersAvailableLater();
        return 0;
      }
      default:
        CHECK(false);
    }
  };
  RunHttp3StreamPolicy(41, services);
}

void QuicChromiumClientStream::OnBodyAvailable() {
  auto services = [&](uint32_t operation, int64_t arg) -> int64_t {
    switch (operation) {
      case 0: {
        return static_cast<int64_t>(FinishedReadingHeaders());
      }
      case 1: {
        return static_cast<int64_t>(headers_delivered_);
      }
      case 2: {
        return static_cast<int64_t>(HasBytesToRead());
      }
      case 3: {
        return static_cast<int64_t>(FinishedReadingTrailers());
      }
      case 4: {
        return static_cast<int64_t>(handle_ != nullptr);
      }
      case 5: {
        NotifyHandleOfDataAvailableLater();
        return 0;
      }
      default:
        CHECK(false);
    }
  };
  RunHttp3StreamPolicy(42, services);
}

void QuicChromiumClientStream::OnClose() {
  auto services = [&](uint32_t operation, int64_t arg) -> int64_t {
    switch (operation) {
      case 0: {
        return static_cast<int64_t>(handle_ != nullptr);
      }
      case 1: {
        handle_->OnClose();
        return 0;
      }
      case 2: {
        handle_ = nullptr;
        return 0;
      }
      case 3: {
        quic::QuicStream::OnClose();
        return 0;
      }
      default:
        CHECK(false);
    }
  };
  RunHttp3StreamPolicy(43, services);
}

void QuicChromiumClientStream::OnCanWrite() {
  auto services = [&](uint32_t operation, int64_t arg) -> int64_t {
    switch (operation) {
      case 0: {
        quic::QuicStream::OnCanWrite();
        return 0;
      }
      case 1: {
        return static_cast<int64_t>(HasBufferedData());
      }
      case 2: {
        return static_cast<int64_t>(handle_ != nullptr);
      }
      case 3: {
        handle_->OnCanWrite();
        return 0;
      }
      default:
        CHECK(false);
    }
  };
  RunHttp3StreamPolicy(44, services);
}

size_t QuicChromiumClientStream::WriteHeaders(
    quiche::HttpHeaderBlock header_block,
    bool fin,
    quiche::QuicheReferenceCountedPointer<quic::QuicAckListenerInterface>
        ack_listener) {
  size_t len = 0;
  auto services = [&](uint32_t operation, int64_t arg) -> int64_t {
    switch (operation) {
      case 0: {
        return static_cast<int64_t>(session()->OneRttKeysAvailable());
      }
      case 1: {
        CHECK(header_block.find(":method") != header_block.end());
        return 0;
      }
      case 2: {
        return static_cast<int64_t>(header_block.find(":method")->second ==
                                    "POST");
      }
      case 3: {
        return static_cast<int64_t>(handle_ != nullptr);
      }
      case 4: {
        return static_cast<int64_t>(handle_->GetRequestIdempotency() ==
                                    IDEMPOTENT);
      }
      case 5: {
        DCHECK(header_block.find(":method")->second != "POST" ||
               (handle_ != nullptr &&
                handle_->GetRequestIdempotency() == IDEMPOTENT));
        return 0;
      }
      case 6: {
        net_log_.AddEvent(
            NetLogEventType::QUIC_CHROMIUM_CLIENT_STREAM_SEND_REQUEST_HEADERS,
            [&](NetLogCaptureMode capture_mode) {
              return QuicRequestNetLogParams(id(), &header_block, priority(),
                                             capture_mode);
            });
        return 0;
      }
      case 7: {
        return static_cast<int64_t>(
            len = quic::QuicSpdyStream::WriteHeaders(
                std::move(header_block), fin, std::move(ack_listener)));
      }
      case 8: {
        initial_headers_sent_ = true;
        return 0;
      }
      default:
        CHECK(false);
    }
  };
  return static_cast<size_t>(RunHttp3StreamPolicy(45, services));
}

bool QuicChromiumClientStream::WriteStreamData(std::string_view data,
                                               bool fin) {
  auto services = [&](uint32_t operation, int64_t arg) -> int64_t {
    switch (operation) {
      case 0: {
        WriteOrBufferBody(data, fin);
        return 0;
      }
      case 1: {
        return static_cast<int64_t>(HasBufferedData());
      }
      default:
        CHECK(false);
    }
  };
  return static_cast<bool>(RunHttp3StreamPolicy(46, services));
}

bool QuicChromiumClientStream::WritevStreamData(
    const std::vector<scoped_refptr<IOBuffer>>& buffers,
    const std::vector<int>& lengths,
    bool fin) {
  auto services = [&](uint32_t operation, int64_t arg) -> int64_t {
    switch (operation) {
      case 0: {
        return static_cast<int64_t>(buffers.size());
      }
      case 1: {
        return static_cast<int64_t>(fin);
      }
      case 2: {
        const size_t i = static_cast<size_t>(arg >> 1);
        WriteOrBufferBody(std::string_view(buffers[i]->data(), lengths[i]),
                          (arg & 1) != 0);
        return 0;
      }
      case 3: {
        return static_cast<int64_t>(HasBufferedData());
      }
      default:
        CHECK(false);
    }
  };
  return static_cast<bool>(RunHttp3StreamPolicy(47, services));
}

std::unique_ptr<QuicChromiumClientStream::Handle>
QuicChromiumClientStream::CreateHandle() {
  std::unique_ptr<Handle> handle;
  auto services = [&](uint32_t operation, int64_t arg) -> int64_t {
    switch (operation) {
      case 0: {
        DCHECK(!handle_);
        CHECK(max_stream_limit_pending_delay_.has_value());
        return 0;
      }
      case 1: {
        handle = base::WrapUnique(
            new Handle(this, *max_stream_limit_pending_delay_));
        handle_ = handle.get();
        return 0;
      }
      case 2: {
        return static_cast<int64_t>(initial_headers_arrived_);
      }
      case 3: {
        handle_->OnInitialHeadersAvailable();
        return 0;
      }
      default:
        CHECK(false);
    }
  };
  RunHttp3StreamPolicy(48, services);
  return handle;
}

void QuicChromiumClientStream::ClearHandle() {
  handle_ = nullptr;
}

void QuicChromiumClientStream::OnError(int error) {
  Handle* handle = nullptr;
  auto services = [&](uint32_t operation, int64_t arg) -> int64_t {
    switch (operation) {
      case 0: {
        return static_cast<int64_t>(handle_ != nullptr);
      }
      case 1: {
        handle = std::exchange(handle_, nullptr);
        return 0;
      }
      case 2: {
        handle->OnError(error);
        return 0;
      }
      default:
        CHECK(false);
    }
  };
  RunHttp3StreamPolicy(49, services);
}

bool QuicChromiumClientStream::Handle::SupportsH3Datagram() const {
  auto services = [&](uint32_t operation, int64_t) -> int64_t {
    switch (operation) {
      case 0:
        return stream_ != nullptr;
      case 1:
        return stream_->SupportsH3Datagram();
      default:
        CHECK(false);
    }
  };
  return RunHttp3StreamPolicy(58, services) != 0;
}

bool QuicChromiumClientStream::SupportsH3Datagram() const {
  return session_->SupportsH3Datagram();
}

int QuicChromiumClientStream::Read(IOBuffer* buf, int buf_len) {
  iovec iov;
  size_t bytes_read = 0;
  auto services = [&](uint32_t operation, int64_t arg) -> int64_t {
    switch (operation) {
      case 0: {
        DCHECK_GT(buf_len, 0);
        DCHECK(buf->data());
        return 0;
      }
      case 1: {
        return static_cast<int64_t>(IsDoneReading());
      }
      case 2: {
        return static_cast<int64_t>(HasBytesToRead());
      }
      case 3: {
        iov.iov_base = buf->data();
        iov.iov_len = buf_len;
        return 0;
      }
      case 4: {
        return static_cast<int64_t>(bytes_read = Readv(&iov, 1));
      }
      case 5: {
        DCHECK_NE(0u, bytes_read);
        return 0;
      }
      default:
        CHECK(false);
    }
  };
  return static_cast<int>(RunHttp3StreamPolicy(50, services));
}

void QuicChromiumClientStream::NotifyHandleOfInitialHeadersAvailableLater() {
  DCHECK(handle_);
  base::SingleThreadTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE,
      base::BindOnce(
          &QuicChromiumClientStream::NotifyHandleOfInitialHeadersAvailable,
          weak_factory_.GetWeakPtr()));
}

void QuicChromiumClientStream::NotifyHandleOfInitialHeadersAvailable() {
  auto services = [&](uint32_t operation, int64_t arg) -> int64_t {
    switch (operation) {
      case 0: {
        return static_cast<int64_t>(handle_ != nullptr);
      }
      case 1: {
        return static_cast<int64_t>(headers_delivered_);
      }
      case 2: {
        handle_->OnInitialHeadersAvailable();
        return 0;
      }
      default:
        CHECK(false);
    }
  };
  RunHttp3StreamPolicy(51, services);
}

void QuicChromiumClientStream::NotifyHandleOfTrailingHeadersAvailableLater() {
  DCHECK(handle_);
  base::SingleThreadTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE,
      base::BindOnce(
          &QuicChromiumClientStream::NotifyHandleOfTrailingHeadersAvailable,
          weak_factory_.GetWeakPtr()));
}

void QuicChromiumClientStream::NotifyHandleOfTrailingHeadersAvailable() {
  auto services = [&](uint32_t operation, int64_t arg) -> int64_t {
    switch (operation) {
      case 0: {
        return static_cast<int64_t>(handle_ != nullptr);
      }
      case 1: {
        return static_cast<int64_t>(trailers_decompressed());
      }
      case 2: {
        return static_cast<int64_t>(headers_delivered_);
      }
      case 3: {
        NotifyHandleOfDataAvailableLater();
        return 0;
      }
      case 4: {
        handle_->OnTrailingHeadersAvailable();
        return 0;
      }
      default:
        CHECK(false);
    }
  };
  RunHttp3StreamPolicy(52, services);
}

int QuicChromiumClientStream::DeliverEarlyHints(
    quiche::HttpHeaderBlock* headers) {
  size_t frame_len = 0;
  auto services = [&](uint32_t operation, int64_t arg) -> int64_t {
    switch (operation) {
      case 0: {
        return static_cast<int64_t>(early_hints_.empty());
      }
      case 1: {
        DCHECK(!headers_delivered_);
        return 0;
      }
      case 2: {
        *headers = std::move(early_hints_.front().headers);
        frame_len = early_hints_.front().frame_len;
        early_hints_.pop_front();
        return 0;
      }
      case 3: {
        net_log_.AddEvent(
            NetLogEventType::
                QUIC_CHROMIUM_CLIENT_STREAM_READ_EARLY_HINTS_RESPONSE_HEADERS,
            [&](NetLogCaptureMode capture_mode) {
              return QuicResponseNetLogParams(id(), fin_received(), headers,
                                              capture_mode);
            });
        return 0;
      }
      case 4: {
        return static_cast<int64_t>(frame_len);
      }
      default:
        CHECK(false);
    }
  };
  return static_cast<int>(RunHttp3StreamPolicy(53, services));
}

int QuicChromiumClientStream::DeliverInitialHeaders(
    quiche::HttpHeaderBlock* headers) {
  auto services = [&](uint32_t operation, int64_t arg) -> int64_t {
    switch (operation) {
      case 0: {
        return static_cast<int64_t>(initial_headers_arrived_);
      }
      case 1: {
        headers_delivered_ = true;
        return 0;
      }
      case 2: {
        return static_cast<int64_t>(initial_headers_.empty());
      }
      case 3: {
        net_log_.AddEvent(
            NetLogEventType::QUIC_CHROMIUM_CLIENT_STREAM_READ_RESPONSE_HEADERS,
            [&](NetLogCaptureMode capture_mode) {
              return QuicResponseNetLogParams(id(), fin_received(),
                                              &initial_headers_, capture_mode);
            });
        return 0;
      }
      case 4: {
        *headers = std::move(initial_headers_);
        return 0;
      }
      case 5: {
        return static_cast<int64_t>(initial_headers_frame_len_);
      }
      default:
        CHECK(false);
    }
  };
  return static_cast<int>(RunHttp3StreamPolicy(54, services));
}

bool QuicChromiumClientStream::DeliverTrailingHeaders(
    quiche::HttpHeaderBlock* headers,
    int* frame_len) {
  auto services = [&](uint32_t operation, int64_t arg) -> int64_t {
    switch (operation) {
      case 0: {
        return static_cast<int64_t>(trailing_headers_frame_len_);
      }
      case 1: {
        net_log_.AddEvent(
            NetLogEventType::QUIC_CHROMIUM_CLIENT_STREAM_READ_RESPONSE_TRAILERS,
            [&](NetLogCaptureMode capture_mode) {
              return QuicResponseNetLogParams(
                  id(), fin_received(), &received_trailers(), capture_mode);
            });
        return 0;
      }
      case 2: {
        *headers = received_trailers().Clone();
        *frame_len = trailing_headers_frame_len_;
        return 0;
      }
      case 3: {
        MarkTrailersConsumed();
        return 0;
      }
      default:
        CHECK(false);
    }
  };
  return static_cast<bool>(RunHttp3StreamPolicy(55, services));
}

void QuicChromiumClientStream::NotifyHandleOfDataAvailableLater() {
  DCHECK(handle_);
  base::SingleThreadTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE,
      base::BindOnce(&QuicChromiumClientStream::NotifyHandleOfDataAvailable,
                     weak_factory_.GetWeakPtr()));
}

void QuicChromiumClientStream::NotifyHandleOfDataAvailable() {
  auto services = [&](uint32_t operation, int64_t arg) -> int64_t {
    switch (operation) {
      case 0: {
        return static_cast<int64_t>(handle_ != nullptr);
      }
      case 1: {
        handle_->OnDataAvailable();
        return 0;
      }
      default:
        CHECK(false);
    }
  };
  RunHttp3StreamPolicy(56, services);
}

void QuicChromiumClientStream::DisableConnectionMigrationToCellularNetwork() {
  set_can_migrate_to_cellular_network(false);
}

quic::QuicPacketLength
QuicChromiumClientStream::GetGuaranteedLargestMessagePayload() const {
  auto services = [&](uint32_t operation, int64_t arg) -> int64_t {
    switch (operation) {
      case 0: {
        return static_cast<int64_t>(session() != nullptr);
      }
      case 1: {
        return static_cast<int64_t>(
            session()->GetGuaranteedLargestDatagramPayload());
      }
      default:
        CHECK(false);
    }
  };
  return static_cast<quic::QuicPacketLength>(
      RunHttp3StreamPolicy(57, services));
}

bool QuicChromiumClientStream::IsFirstStream() {
  return http3_client_stream_is_first(
      id(), quic::QuicUtils::GetFirstBidirectionalStreamId(
                quic_version_, quic::Perspective::IS_CLIENT));
}

}  // namespace net
