#include "net/log/connection_original_log.h"
#include "net/base/net_error_rust_reference.h"
#include "net/http/connection_auth_original_policy.h"
#include "net/http/connection_header_original_policy.h"
// Copyright 2012 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/spdy/spdy_proxy_client_socket_rust_original.h"

#include <algorithm>  // min
#include <utility>
#include <vector>
#include "base/strings/stringprintf.h"
#include "net/socket/connection_jobs_upper_rust_original_helpers.h"

#include "base/check_op.h"
#include "base/functional/bind.h"
#include "base/functional/callback_helpers.h"
#include "base/location.h"
#include "base/notimplemented.h"
#include "base/notreached.h"
#include "base/strings/string_util.h"
#include "base/task/single_thread_task_runner.h"
#include "base/types/expected.h"
#include "base/types/expected_macros.h"
#include "base/values.h"
#include "net/base/auth.h"
#include "net/base/completion_once_callback.h"
#include "net/base/io_buffer.h"
#include "net/base/net_errors.h"
#include "net/base/proxy_chain.h"
#include "net/base/proxy_delegate.h"
#include "net/http/http_auth_cache.h"
#include "net/http/http_auth_handler_factory.h"
#include "net/http/http_log_util.h"
#include "net/http/http_request_info.h"
#include "net/http/http_response_headers.h"
#include "net/log/net_log_event_type.h"
#include "net/log/net_log_source_type.h"
#include "net/spdy/spdy_http_utils_family_original.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "url/gurl.h"

namespace net {

SpdyProxyClientSocketOriginal::SpdyProxyClientSocketOriginal(
    const base::WeakPtr<SpdyStreamOriginal>& spdy_stream,
    const ProxyChain& proxy_chain,
    size_t proxy_chain_index,
    const std::string& user_agent,
    const HostPortPair& endpoint,
    const NetLogWithSource& source_net_log,
    scoped_refptr<HttpAuthController> auth_controller,
    ProxyDelegate* proxy_delegate)
    : spdy_stream_(spdy_stream),
      endpoint_(endpoint),
      auth_(std::move(auth_controller)),
      proxy_chain_(proxy_chain),
      proxy_chain_index_(proxy_chain_index),
      proxy_delegate_(proxy_delegate),
      user_agent_(user_agent),
      net_log_(NetLogWithSource::Make(spdy_stream->net_log().net_log(),
                                      NetLogSourceType::PROXY_CLIENT_SOCKET)),
      source_dependency_(source_net_log.source()) {
  request_.method = "CONNECT";
  request_.url = GURL("https://" + ConnectionJobsUpperRustOriginalStorage::HostString(endpoint));
  net_log_.BeginEventReferencingSource(NetLogEventType::SOCKET_ALIVE,
                                       source_net_log.source());
  net_log_.AddEventReferencingSource(
      NetLogEventType::HTTP2_PROXY_CLIENT_SESSION,
      spdy_stream->net_log().source());

  spdy_stream_->SetDelegate(this);
  was_ever_used_ = spdy_stream_->WasEverUsed();
}

SpdyProxyClientSocketOriginal::~SpdyProxyClientSocketOriginal() {
  Disconnect();
  net_log_.EndEvent(NetLogEventType::SOCKET_ALIVE);
}

const HttpResponseInfo* SpdyProxyClientSocketOriginal::GetConnectResponseInfo() const {
  return response_.headers.get() ? &response_ : nullptr;
}

const scoped_refptr<HttpAuthController>&
SpdyProxyClientSocketOriginal::GetAuthController() const {
  return auth_;
}

int SpdyProxyClientSocketOriginal::RestartWithAuth(CompletionOnceCallback callback) {
  // A SPDY Stream can only handle a single request, so the underlying
  // stream may not be reused and a new SpdyProxyClientSocketOriginal must be
  // created (possibly on top of the same SPDY Session).
  next_state_ = STATE_DISCONNECTED;
  return ERR_UNABLE_TO_REUSE_CONNECTION_FOR_PROXY_AUTH;
}

// Ignore priority changes, just use priority of initial request. Since multiple
// requests are pooled on the SpdyProxyClientSocketOriginal, reprioritization doesn't
// really work.
//
// TODO(mmenke):  Use a single priority value for all SpdyProxyClientSockets,
// regardless of what priority they're created with.
void SpdyProxyClientSocketOriginal::SetStreamPriority(RequestPriority priority) {}

// Sends a HEADERS frame to the proxy with a CONNECT request
// for the specified endpoint.  Waits for the server to send back
// a HEADERS frame.  OK will be returned if the status is 200.
// ERR_TUNNEL_CONNECTION_FAILED will be returned for any other status.
// In any of these cases, Read() may be called to retrieve the HTTP
// response body.  Any other return values should be considered fatal.
// TODO(rch): handle 407 proxy auth requested correctly, perhaps
// by creating a new stream for the subsequent request.
// TODO(rch): create a more appropriate error code to disambiguate
// the HTTPS Proxy tunnel failure from an HTTP Proxy tunnel failure.
int SpdyProxyClientSocketOriginal::Connect(CompletionOnceCallback callback) {
  DCHECK(read_callback_.is_null());
  if (next_state_ == STATE_OPEN)
    return OK;

  DCHECK_EQ(STATE_DISCONNECTED, next_state_);
  next_state_ = STATE_GENERATE_AUTH_TOKEN;

  int rv = DoLoop(OK);
  if (rv == ERR_IO_PENDING)
    read_callback_ = std::move(callback);
  return rv;
}

void SpdyProxyClientSocketOriginal::Disconnect() {
  read_buffer_queue_.Clear();
  user_buffer_ = nullptr;
  user_buffer_len_ = 0;
  read_callback_.Reset();

  write_buffer_len_ = 0;
  write_callback_.Reset();

  next_state_ = STATE_DISCONNECTED;

  if (spdy_stream_.get()) {
    // This will cause OnClose to be invoked, which takes care of
    // cleaning up all the internal state.
    spdy_stream_->Cancel(ERR_ABORTED);
    DCHECK(!spdy_stream_.get());
  }
}

bool SpdyProxyClientSocketOriginal::IsConnected() const {
  return next_state_ == STATE_OPEN;
}

bool SpdyProxyClientSocketOriginal::IsConnectedAndIdle() const {
  return IsConnected() && read_buffer_queue_.IsEmpty() &&
      spdy_stream_->IsOpen();
}

const NetLogWithSource& SpdyProxyClientSocketOriginal::NetLog() const {
  return net_log_;
}

bool SpdyProxyClientSocketOriginal::WasEverUsed() const {
  return was_ever_used_ || (spdy_stream_.get() && spdy_stream_->WasEverUsed());
}

NextProto SpdyProxyClientSocketOriginal::GetNegotiatedProtocol() const {
  // Do not delegate to `spdy_stream_`. While `spdy_stream_` negotiated ALPN
  // with the proxy, this object represents the tunneled TCP connection to the
  // origin.
  return NextProto::kProtoUnknown;
}

bool SpdyProxyClientSocketOriginal::GetSSLInfo(SSLInfo* ssl_info) {
  // Do not delegate to `spdy_stream_`. While `spdy_stream_` connected to the
  // proxy with TLS, this object represents the tunneled TCP connection to the
  // origin.
  return false;
}

int64_t SpdyProxyClientSocketOriginal::GetTotalReceivedBytes() const {
  NOTIMPLEMENTED();
  return 0;
}

void SpdyProxyClientSocketOriginal::ApplySocketTag(const SocketTag& tag) {
  // In the case of a connection to the proxy using HTTP/2 or HTTP/3 where the
  // underlying socket may multiplex multiple streams, applying this request's
  // socket tag to the multiplexed session would incorrectly apply the socket
  // tag to all mutliplexed streams. Fortunately socket tagging is only
  // supported on Android without the data reduction proxy, so only simple HTTP
  // proxies are supported, so proxies won't be using HTTP/2 or HTTP/3. Enforce
  // that a specific (non-default) tag isn't being applied.
  CHECK(tag == SocketTag());
}

int SpdyProxyClientSocketOriginal::Read(IOBuffer* buf,
                                int buf_len,
                                CompletionOnceCallback callback) {
  // ReadIfReady() can fire consume callbacks that trigger session
  // teardown and destroy `this`.
  base::WeakPtr<SpdyProxyClientSocketOriginal> self = weak_factory_.GetWeakPtr();
  int rv = ReadIfReady(buf, buf_len, std::move(callback));
  if (!self) {
    return ERR_CONNECTION_CLOSED;
  }
  if (rv == ERR_IO_PENDING) {
    user_buffer_ = buf;
    user_buffer_len_ = static_cast<size_t>(buf_len);
  }
  return rv;
}

int SpdyProxyClientSocketOriginal::ReadIfReady(IOBuffer* buf,
                                       int buf_len,
                                       CompletionOnceCallback callback) {
  DCHECK(!read_callback_);
  DCHECK(!user_buffer_);

  if (next_state_ == STATE_DISCONNECTED)
    return ERR_SOCKET_NOT_CONNECTED;

  if (next_state_ == STATE_CLOSED && read_buffer_queue_.IsEmpty()) {
    return 0;
  }

  DCHECK(next_state_ == STATE_OPEN || next_state_ == STATE_CLOSED);
  DCHECK(buf);
  // PopulateUserReadBuffer() can fire consume callbacks that trigger session
  // teardown and destroy `this`.
  base::WeakPtr<SpdyProxyClientSocketOriginal> self = weak_factory_.GetWeakPtr();
  size_t result = PopulateUserReadBuffer(buf->first(buf_len));
  if (!self) {
    return ERR_CONNECTION_CLOSED;
  }
  if (result == 0) {
    read_callback_ = std::move(callback);
    return ERR_IO_PENDING;
  }
  return result;
}

int SpdyProxyClientSocketOriginal::CancelReadIfReady() {
  // Only a pending ReadIfReady() can be canceled.
  DCHECK(!user_buffer_) << "Pending Read() cannot be canceled";
  read_callback_.Reset();
  return OK;
}

size_t SpdyProxyClientSocketOriginal::PopulateUserReadBuffer(base::span<uint8_t> data) {
  return read_buffer_queue_.Dequeue(data);
}

int SpdyProxyClientSocketOriginal::Write(
    IOBuffer* buf,
    int buf_len,
    CompletionOnceCallback callback,
    const NetworkTrafficAnnotationTag& traffic_annotation) {
  DCHECK(write_callback_.is_null());
  if (next_state_ != STATE_OPEN)
    return ERR_SOCKET_NOT_CONNECTED;
  if (end_stream_state_ == EndStreamState::kEndStreamSent)
    return ERR_CONNECTION_CLOSED;

  DCHECK(spdy_stream_.get());
  spdy_stream_->SendData(buf, buf_len, MORE_DATA_TO_SEND_ORIGINAL);
  ConnectionOriginalLog::AddByteTransferEvent(net_log_, NetLogEventType::SOCKET_BYTES_SENT, buf_len,
                                buf->data());
  write_callback_ = std::move(callback);
  write_buffer_len_ = buf_len;
  return ERR_IO_PENDING;
}

int SpdyProxyClientSocketOriginal::SetReceiveBufferSize(int32_t size) {
  // Since this StreamSocket sits on top of a shared SpdySession, it
  // is not safe for callers to change this underlying socket.
  return ERR_NOT_IMPLEMENTED;
}

int SpdyProxyClientSocketOriginal::SetSendBufferSize(int32_t size) {
  // Since this StreamSocket sits on top of a shared SpdySession, it
  // is not safe for callers to change this underlying socket.
  return ERR_NOT_IMPLEMENTED;
}

int SpdyProxyClientSocketOriginal::GetPeerAddress(IPEndPoint* address) const {
  if (!IsConnected())
    return ERR_SOCKET_NOT_CONNECTED;
  return spdy_stream_->GetPeerAddress(address);
}

int SpdyProxyClientSocketOriginal::GetLocalAddress(IPEndPoint* address) const {
  if (!IsConnected())
    return ERR_SOCKET_NOT_CONNECTED;
  return spdy_stream_->GetLocalAddress(address);
}

void SpdyProxyClientSocketOriginal::RunWriteCallback(int result) {
  base::WeakPtr<SpdyProxyClientSocketOriginal> weak_ptr = weak_factory_.GetWeakPtr();
  // `write_callback_` might be consumed by OnClose().
  if (write_callback_) {
    std::move(write_callback_).Run(result);
  }
  if (!weak_ptr) {
    // `this` was already destroyed while running `write_callback_`. Must
    // return immediately without touching any field member.
    return;
  }

  if (end_stream_state_ == EndStreamState::kEndStreamReceived) {
    base::SingleThreadTaskRunner::GetCurrentDefault()->PostTask(
        FROM_HERE, base::BindOnce(&SpdyProxyClientSocketOriginal::MaybeSendEndStream,
                                  weak_factory_.GetMutableWeakPtr()));
  }
}

void SpdyProxyClientSocketOriginal::OnIOComplete(int result) {
  DCHECK_NE(STATE_DISCONNECTED, next_state_);
  int rv = DoLoop(result);
  if (rv != ERR_IO_PENDING) {
    std::move(read_callback_).Run(rv);
  }
}

void SpdyProxyClientSocketOriginal::OnBeforeTunnelRequestComplete(
    base::expected<HttpRequestHeaders, Error> result) {
  if (result.has_value()) {
    proxy_delegate_headers_ = std::move(result.value());
    OnIOComplete(OK);
  } else {
    // OnBeforeTunnelRequestComplete should never report ERR_IO_PENDING since
    // it's used to signal that IO has completed.
    CHECK_NE(ERR_IO_PENDING, result.error());
    // Success should always be reported via a base::expected containing an
    // HttpRequestHeaders, see ProxyDelegate::OnBeforeTunnelRequest.
    CHECK_NE(OK, result.error());
    OnIOComplete(result.error());
  }
}

int SpdyProxyClientSocketOriginal::DoLoop(int last_io_result) {
  DCHECK_NE(next_state_, STATE_DISCONNECTED);
  int rv = last_io_result;
  do {
    State state = next_state_;
    next_state_ = STATE_DISCONNECTED;
    switch (state) {
      case STATE_GENERATE_AUTH_TOKEN:
        DCHECK_EQ(OK, rv);
        rv = DoGenerateAuthToken();
        break;
      case STATE_GENERATE_AUTH_TOKEN_COMPLETE:
        rv = DoGenerateAuthTokenComplete(rv);
        break;
      case STATE_CALCULATE_HEADERS:
        DCHECK_EQ(OK, rv);
        rv = DoCalculateHeaders();
        break;
      case STATE_CALCULATE_HEADERS_COMPLETE:
        rv = DoCalculateHeadersComplete(rv);
        break;
      case STATE_SEND_REQUEST:
        DCHECK_EQ(OK, rv);
        net_log_.BeginEvent(
            NetLogEventType::HTTP_TRANSACTION_TUNNEL_SEND_REQUEST);
        rv = DoSendRequest();
        break;
      case STATE_SEND_REQUEST_COMPLETE:
        ConnectionOriginalLog::EndEventWithNetErrorCode(net_log_,
            NetLogEventType::HTTP_TRANSACTION_TUNNEL_SEND_REQUEST, rv);
        rv = DoSendRequestComplete(rv);
        if (rv >= 0 || rv == ERR_IO_PENDING) {
          // Emit extra event so can use the same events as
          // HttpProxyClientSocket.
          net_log_.BeginEvent(
              NetLogEventType::HTTP_TRANSACTION_TUNNEL_READ_HEADERS);
        }
        break;
      case STATE_READ_REPLY_COMPLETE:
        rv = DoReadReplyComplete(rv);
        ConnectionOriginalLog::EndEventWithNetErrorCode(net_log_,
            NetLogEventType::HTTP_TRANSACTION_TUNNEL_READ_HEADERS, rv);
        break;
      case STATE_PROCESS_RESPONSE_HEADERS:
        DCHECK_EQ(OK, rv);
        rv = DoProcessResponseHeaders();
        break;
      case STATE_PROCESS_RESPONSE_HEADERS_COMPLETE:
        rv = DoProcessResponseHeadersComplete(rv);
        break;
      case STATE_PROCESS_RESPONSE_CODE:
        DCHECK_EQ(OK, rv);
        rv = DoProcessResponseCode();
        break;
      default:
        NOTREACHED() << "bad state";
    }
  } while (rv != ERR_IO_PENDING && next_state_ != STATE_DISCONNECTED &&
           next_state_ != STATE_OPEN);
  return rv;
}

int SpdyProxyClientSocketOriginal::DoGenerateAuthToken() {
  next_state_ = STATE_GENERATE_AUTH_TOKEN_COMPLETE;
  return ConnectionAuthOriginalPolicy::MaybeGenerateAuthToken(auth_.get(),
      &request_, response_.ssl_info,
      base::BindOnce(&SpdyProxyClientSocketOriginal::OnIOComplete,
                     weak_factory_.GetWeakPtr()),
      net_log_);
}

int SpdyProxyClientSocketOriginal::DoGenerateAuthTokenComplete(int result) {
  DCHECK_NE(ERR_IO_PENDING, result);
  if (result == OK) {
    next_state_ = STATE_CALCULATE_HEADERS;
  }
  return result;
}

int SpdyProxyClientSocketOriginal::DoCalculateHeaders() {
  next_state_ = STATE_CALCULATE_HEADERS_COMPLETE;

  authorization_headers_.Clear();
  proxy_delegate_headers_.Clear();

  // Add Proxy-Authentication header if necessary.
  if (ConnectionAuthOriginalPolicy::HaveAuth(auth_.get())) {
    ConnectionHeaderOriginalPolicy::AddAuthorizationHeader(*auth_, &authorization_headers_);
  }

  if (proxy_delegate_) {
    ASSIGN_OR_RETURN(
        proxy_delegate_headers_,
        proxy_delegate_->OnBeforeTunnelRequest(
            proxy_chain_, proxy_chain_index_,
            base::BindOnce(
                &SpdyProxyClientSocketOriginal::OnBeforeTunnelRequestComplete,
                weak_factory_.GetWeakPtr())),
        [](const auto& e) {
          // Success should always be reported via a base::expected containing
          // an HttpRequestHeaders, see ProxyDelegate::OnBeforeTunnelRequest.
          CHECK_NE(OK, e);
          return e;
        });
  }
  return OK;
}

int SpdyProxyClientSocketOriginal::DoCalculateHeadersComplete(int result) {
  DCHECK_NE(ERR_IO_PENDING, result);
  if (result != OK) {
    return result;
  }
  next_state_ = STATE_SEND_REQUEST;
  ConnectionHeaderOriginalPolicy::MergeFrom(request_.extra_headers, proxy_delegate_headers_);
  return result;
}

int SpdyProxyClientSocketOriginal::DoSendRequest() {
  next_state_ = STATE_SEND_REQUEST_COMPLETE;

  std::string request_line;
  BuildTunnelRequest(endpoint_, authorization_headers_, user_agent_,
                     &request_line, &request_.extra_headers);

  ConnectionOriginalNetLogRequestHeaders(net_log_,
                       NetLogEventType::HTTP_TRANSACTION_SEND_TUNNEL_HEADERS,
                       request_line, &request_.extra_headers);

  quiche::HttpHeaderBlock headers;
  CreateSpdyHeadersFromHttpRequestOriginal(request_, std::nullopt,
                                   request_.extra_headers, &headers);

  return spdy_stream_->SendRequestHeaders(std::move(headers),
                                          MORE_DATA_TO_SEND_ORIGINAL);
}

int SpdyProxyClientSocketOriginal::DoSendRequestComplete(int result) {
  if (result < 0)
    return result;

  // Wait for HEADERS frame from the server
  next_state_ = STATE_READ_REPLY_COMPLETE;
  return ERR_IO_PENDING;
}

int SpdyProxyClientSocketOriginal::DoReadReplyComplete(int result) {
  // We enter this method directly from DoSendRequestComplete, since
  // we are notified by a callback when the HEADERS frame arrives.

  if (result < 0)
    return result;

  // Require the "HTTP/1.x" status line for SSL CONNECT.
  if (response_.headers->GetHttpVersion() < HttpVersion(1, 0))
    return ERR_TUNNEL_CONNECTION_FAILED;

  next_state_ = STATE_PROCESS_RESPONSE_HEADERS;

  ConnectionOriginalNetLogResponseHeaders(
      net_log_, NetLogEventType::HTTP_TRANSACTION_READ_TUNNEL_RESPONSE_HEADERS,
      response_.headers.get());

  return OK;
}

int SpdyProxyClientSocketOriginal::DoProcessResponseHeaders() {
  next_state_ = STATE_PROCESS_RESPONSE_HEADERS_COMPLETE;

  if (proxy_delegate_) {
    return proxy_delegate_->OnTunnelHeadersReceived(
        proxy_chain_, proxy_chain_index_, *response_.headers,
        base::BindOnce(&SpdyProxyClientSocketOriginal::OnIOComplete,
                       weak_factory_.GetWeakPtr()));
  }

  return OK;
}

int SpdyProxyClientSocketOriginal::DoProcessResponseHeadersComplete(int result) {
  DCHECK_NE(ERR_IO_PENDING, result);
  if (result != OK) {
    return result;
  }

  next_state_ = STATE_PROCESS_RESPONSE_CODE;
  return OK;
}

int SpdyProxyClientSocketOriginal::DoProcessResponseCode() {
  switch (response_.headers->response_code()) {
    case 200:  // OK
      next_state_ = STATE_OPEN;
      return OK;

    case 407:  // Proxy Authentication Required
      next_state_ = STATE_OPEN;
      SanitizeProxyAuth(response_);
      return HandleProxyAuthChallenge(auth_.get(), &response_, net_log_);

    default:
      // Ignore response to avoid letting the proxy impersonate the target
      // server.  (See http://crbug.com/137891.)
      return ERR_TUNNEL_CONNECTION_FAILED;
  }
}

// SpdyStreamOriginal::Delegate methods:
// Called when SYN frame has been sent.
// Returns true if no more data to be sent after SYN frame.
void SpdyProxyClientSocketOriginal::OnHeadersSent() {
  DCHECK_EQ(next_state_, STATE_SEND_REQUEST_COMPLETE);

  OnIOComplete(OK);
}

void SpdyProxyClientSocketOriginal::OnEarlyHintsReceived(
    const quiche::HttpHeaderBlock& headers) {}

void SpdyProxyClientSocketOriginal::OnHeadersReceived(
    const quiche::HttpHeaderBlock& response_headers) {
  // If we've already received the reply, existing headers are too late.
  // TODO(mbelshe): figure out a way to make HEADERS frames useful after the
  //                initial response.
  if (next_state_ != STATE_READ_REPLY_COMPLETE)
    return;

  // Save the response
  const int rv = SpdyHeadersToHttpResponseOriginal(response_headers, &response_);
  DCHECK_NE(rv, ERR_INCOMPLETE_HTTP2_HEADERS);

  OnIOComplete(OK);
}

// Called when data is received or on EOF (if `buffer is nullptr).
void SpdyProxyClientSocketOriginal::OnDataReceived(std::unique_ptr<SpdyBufferOriginal> buffer) {
  if (buffer) {
    ConnectionOriginalLog::AddByteTransferEvent(net_log_, NetLogEventType::SOCKET_BYTES_RECEIVED,
                                  buffer->GetRemaining());
    read_buffer_queue_.Enqueue(std::move(buffer));
  } else {
    ConnectionOriginalLog::AddByteTransferEvent(net_log_, NetLogEventType::SOCKET_BYTES_RECEIVED, 0,
                                  nullptr);

    if (end_stream_state_ == EndStreamState::kNone) {
      // The peer sent END_STREAM. Schedule a DATA frame with END_STREAM.
      end_stream_state_ = EndStreamState::kEndStreamReceived;
      base::SingleThreadTaskRunner::GetCurrentDefault()->PostTask(
          FROM_HERE, base::BindOnce(&SpdyProxyClientSocketOriginal::MaybeSendEndStream,
                                    weak_factory_.GetWeakPtr()));
    }
  }

  if (read_callback_) {
    if (user_buffer_) {
      // PopulateUserReadBuffer() can fire consume callbacks that trigger
      // session teardown and destroy `this`.
      base::WeakPtr<SpdyProxyClientSocketOriginal> self = weak_factory_.GetWeakPtr();
      int rv = PopulateUserReadBuffer(user_buffer_->first(user_buffer_len_));
      if (!self) {
        return;
      }
      user_buffer_ = nullptr;
      user_buffer_len_ = 0;
      std::move(read_callback_).Run(rv);
    } else {
      // If ReadIfReady() is used instead of Read(), tell the caller that data
      // is available for reading.
      std::move(read_callback_).Run(OK);
    }
  }
}

void SpdyProxyClientSocketOriginal::OnDataSent() {
  if (end_stream_state_ == EndStreamState::kEndStreamSent) {
    CHECK(write_callback_.is_null());
    return;
  }

  DCHECK(!write_callback_.is_null());

  int rv = write_buffer_len_;
  write_buffer_len_ = 0;

  // Proxy write callbacks result in deep callback chains. Post to allow the
  // stream's write callback chain to unwind (see crbug.com/355511).
  base::SingleThreadTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE, base::BindOnce(&SpdyProxyClientSocketOriginal::RunWriteCallback,
                                weak_factory_.GetWeakPtr(), rv));
}

void SpdyProxyClientSocketOriginal::OnTrailers(
    const quiche::HttpHeaderBlock& trailers) {
  // |spdy_stream_| is of type SPDY_BIDIRECTIONAL_STREAM, so trailers are
  // combined with response headers and this method will not be calld.
  DUMP_WILL_BE_NOTREACHED();
}

void SpdyProxyClientSocketOriginal::OnClose(int status)  {
  was_ever_used_ = spdy_stream_->WasEverUsed();
  spdy_stream_.reset();

  bool connecting = next_state_ != STATE_DISCONNECTED &&
      next_state_ < STATE_OPEN;
  if (next_state_ == STATE_OPEN)
    next_state_ = STATE_CLOSED;
  else
    next_state_ = STATE_DISCONNECTED;

  base::WeakPtr<SpdyProxyClientSocketOriginal> weak_ptr = weak_factory_.GetWeakPtr();
  CompletionOnceCallback write_callback = std::move(write_callback_);
  write_buffer_len_ = 0;

  // If we're in the middle of connecting, we need to make sure
  // we invoke the connect callback.
  if (connecting) {
    DCHECK(!read_callback_.is_null());
    std::move(read_callback_).Run(status);
  } else if (!read_callback_.is_null()) {
    // If we have a read_callback_, the we need to make sure we call it back.
    OnDataReceived(std::unique_ptr<SpdyBufferOriginal>());
  }
  // This may have been deleted by read_callback_, so check first.
  if (weak_ptr.get() && !write_callback.is_null())
    std::move(write_callback).Run(ERR_CONNECTION_CLOSED);
}

bool SpdyProxyClientSocketOriginal::CanGreaseFrameType() const {
  return false;
}

NetLogSource SpdyProxyClientSocketOriginal::source_dependency() const {
  return source_dependency_;
}

void SpdyProxyClientSocketOriginal::MaybeSendEndStream() {
  DCHECK_NE(end_stream_state_, EndStreamState::kNone);
  if (end_stream_state_ == EndStreamState::kEndStreamSent)
    return;

  if (!spdy_stream_)
    return;

  // When there is a pending write, wait until the write completes.
  if (write_callback_)
    return;

  auto buffer = base::MakeRefCounted<IOBufferWithSize>(/*buffer_size=*/0);
  spdy_stream_->SendData(buffer.get(), /*length=*/0, NO_MORE_DATA_TO_SEND_ORIGINAL);
  end_stream_state_ = EndStreamState::kEndStreamSent;
}

// static
void SpdyProxyClientSocketOriginal::BuildTunnelRequest(
    const HostPortPair& endpoint,
    const HttpRequestHeaders& extra_headers,
    const std::string& user_agent,
    std::string* request_line,
    HttpRequestHeaders* request_headers) {
  // RFC 7230 Section 5.4 says a client MUST send a Host header field in all
  // HTTP/1.1 request messages, and Host SHOULD be the first header field
  // following the request-line.  Add "Proxy-Connection: keep-alive" for compat
  // with HTTP/1.0 proxies such as Squid (required for NTLM authentication).
  std::string host_and_port = ConnectionJobsUpperRustOriginalStorage::HostString(endpoint);
  *request_line =
      base::StringPrintf("CONNECT %s HTTP/1.1\r\n", host_and_port.c_str());
  ConnectionHeaderOriginalPolicy::SetHeader(*request_headers, HttpRequestHeaders::kHost, host_and_port);
  ConnectionHeaderOriginalPolicy::SetHeader(*request_headers, HttpRequestHeaders::kProxyConnection,
                             "keep-alive");
  if (!user_agent.empty())
    ConnectionHeaderOriginalPolicy::SetHeader(*request_headers, HttpRequestHeaders::kUserAgent, user_agent);

  ConnectionHeaderOriginalPolicy::MergeFrom(*request_headers, extra_headers);
}

// static
int SpdyProxyClientSocketOriginal::HandleProxyAuthChallenge(
    HttpAuthController* auth,
    HttpResponseInfo* response,
    const NetLogWithSource& net_log) {
  DCHECK(response->headers.get());
  int rv = ConnectionAuthOriginalPolicy::HandleAuthChallenge(auth, response->headers, response->ssl_info,
                                     false, true, net_log);
  ConnectionAuthOriginalPolicy::TakeAuthInfo(auth, &response->auth_challenge);
  if (rv == OK)
    return ERR_PROXY_AUTH_REQUESTED;
  return rv;
}

// static
void SpdyProxyClientSocketOriginal::SanitizeProxyAuth(HttpResponseInfo& response) {
  DCHECK(response.headers);

  // Copy status line and all hop-by-hop headers to preserve keep-alive
  // behavior.
  const char* kHeadersToKeep[] = {
      "connection",         "proxy-connection", "keep-alive", "trailer",
      "transfer-encoding",  "upgrade",

      "content-length",

      "proxy-authenticate",
  };

  // Create a list of all present header not in |kHeadersToKeep|, and then
  // remove them.
  size_t iter = 0;
  std::string header_name;
  std::string header_value;
  std::vector<std::string> headers_to_remove;
  while (response.headers->EnumerateHeaderLines(&iter, &header_name,
                                                &header_value)) {
    bool remove = true;
    for (const char* header : kHeadersToKeep) {
      if (base::EqualsCaseInsensitiveASCII(header, header_name)) {
        remove = false;
        break;
      }
    }
    if (remove)
      headers_to_remove.push_back(header_name);
  }

  response.headers->RemoveHeaders(headers_to_remove);
}


}  // namespace net
