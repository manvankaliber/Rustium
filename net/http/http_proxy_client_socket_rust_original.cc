#include "net/log/connection_original_log.h"
#include "net/base/net_error_rust_reference.h"
#include "net/http/connection_auth_original_policy.h"
#include "net/http/connection_header_original_policy.h"
#include "net/socket/connection_jobs_upper_rust_original_helpers.h"
// Copyright 2012 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/http/http_proxy_client_socket_rust_original.h"

#include <memory>
#include <utility>

#include "base/functional/bind.h"
#include "base/functional/callback_helpers.h"
#include "base/strings/string_util.h"
#include "base/types/expected.h"
#include "base/types/expected_macros.h"
#include "base/values.h"
#include "net/base/auth.h"
#include "net/base/completion_once_callback.h"
#include "net/base/host_port_pair.h"
#include "net/base/io_buffer.h"
#include "net/base/net_errors.h"
#include "net/base/proxy_chain.h"
#include "net/base/proxy_delegate.h"
#include "net/http/http_basic_stream.h"
#include "net/http/http_log_util.h"
#include "net/http/http_network_session.h"
#include "net/http/http_request_headers.h"
#include "net/http/http_request_info.h"
#include "net/http/http_response_headers.h"
#include "net/http/http_stream_parser_rust_original.h"
#include "net/log/net_log.h"
#include "net/log/net_log_event_type.h"
#include "net/socket/stream_socket.h"
#include "url/gurl.h"

namespace net {

const int HttpProxyClientSocketOriginal::kDrainBodyBufferSize;

HttpProxyClientSocketOriginal::HttpProxyClientSocketOriginal(
    std::unique_ptr<StreamSocket> socket,
    const std::string& user_agent,
    const HostPortPair& endpoint,
    const ProxyChain& proxy_chain,
    size_t proxy_chain_index,
    scoped_refptr<HttpAuthController> http_auth_controller,
    ProxyDelegate* proxy_delegate,
    const NetworkTrafficAnnotationTag& traffic_annotation)
    : user_agent_(user_agent),
      socket_(std::move(socket)),
      endpoint_(endpoint),
      auth_(std::move(http_auth_controller)),
      proxy_chain_(proxy_chain),
      proxy_chain_index_(proxy_chain_index),
      proxy_delegate_(proxy_delegate),
      traffic_annotation_(traffic_annotation),
      net_log_(socket_->NetLog()) {
  // Synthesize the bits of a request that are actually used.
  request_.url = GURL("https://" + ConnectionJobsUpperRustOriginalStorage::HostString(endpoint));
  request_.method = "CONNECT";
}

HttpProxyClientSocketOriginal::~HttpProxyClientSocketOriginal() {
  Disconnect();
}

int HttpProxyClientSocketOriginal::RestartWithAuth(CompletionOnceCallback callback) {
  DCHECK_EQ(STATE_NONE, next_state_);
  DCHECK(user_callback_.is_null());

  int rv = PrepareForAuthRestart();
  if (rv != OK)
    return rv;

  rv = DoLoop(OK);
  if (rv == ERR_IO_PENDING) {
    if (!callback.is_null())
      user_callback_ = std::move(callback);
  }

  return rv;
}

const scoped_refptr<HttpAuthController>&
HttpProxyClientSocketOriginal::GetAuthController() const {
  return auth_;
}

const HttpResponseInfo* HttpProxyClientSocketOriginal::GetConnectResponseInfo() const {
  return response_.headers.get() ? &response_ : nullptr;
}

int HttpProxyClientSocketOriginal::Connect(CompletionOnceCallback callback) {
  DCHECK(socket_);
  DCHECK(user_callback_.is_null());

  if (next_state_ == STATE_DONE)
    return OK;

  DCHECK_EQ(STATE_NONE, next_state_);
  next_state_ = STATE_GENERATE_AUTH_TOKEN;

  int rv = DoLoop(OK);
  if (rv == ERR_IO_PENDING)
    user_callback_ = std::move(callback);
  return rv;
}

void HttpProxyClientSocketOriginal::Disconnect() {
  if (socket_)
    socket_->Disconnect();

  // Reset other states to make sure they aren't mistakenly used later.
  // These are the states initialized by Connect().
  next_state_ = STATE_NONE;
  user_callback_.Reset();
}

bool HttpProxyClientSocketOriginal::IsConnected() const {
  return next_state_ == STATE_DONE && socket_->IsConnected();
}

bool HttpProxyClientSocketOriginal::IsConnectedAndIdle() const {
  return next_state_ == STATE_DONE && socket_->IsConnectedAndIdle();
}

const NetLogWithSource& HttpProxyClientSocketOriginal::NetLog() const {
  return net_log_;
}

bool HttpProxyClientSocketOriginal::WasEverUsed() const {
  if (socket_)
    return socket_->WasEverUsed();
  NOTREACHED();
}

NextProto HttpProxyClientSocketOriginal::GetNegotiatedProtocol() const {
  // Do not delegate to `socket_`. While `socket_` may negotiate ALPN with the
  // proxy, this object represents the tunneled TCP connection to the origin.
  return NextProto::kProtoUnknown;
}

bool HttpProxyClientSocketOriginal::GetSSLInfo(SSLInfo* ssl_info) {
  // Do not delegate to `socket_`. While `socket_` may connect to the proxy with
  // TLS, this object represents the tunneled TCP connection to the origin.
  return false;
}

int64_t HttpProxyClientSocketOriginal::GetTotalReceivedBytes() const {
  return socket_->GetTotalReceivedBytes();
}

void HttpProxyClientSocketOriginal::ApplySocketTag(const SocketTag& tag) {
  return socket_->ApplySocketTag(tag);
}

int HttpProxyClientSocketOriginal::Read(IOBuffer* buf,
                                int buf_len,
                                CompletionOnceCallback callback) {
  DCHECK(user_callback_.is_null());
  if (!CheckDone())
    return ERR_TUNNEL_CONNECTION_FAILED;

  return socket_->Read(buf, buf_len, std::move(callback));
}

int HttpProxyClientSocketOriginal::ReadIfReady(IOBuffer* buf,
                                       int buf_len,
                                       CompletionOnceCallback callback) {
  DCHECK(user_callback_.is_null());
  if (!CheckDone())
    return ERR_TUNNEL_CONNECTION_FAILED;

  return socket_->ReadIfReady(buf, buf_len, std::move(callback));
}

int HttpProxyClientSocketOriginal::CancelReadIfReady() {
  return socket_->CancelReadIfReady();
}

int HttpProxyClientSocketOriginal::Write(
    IOBuffer* buf,
    int buf_len,
    CompletionOnceCallback callback,
    const NetworkTrafficAnnotationTag& traffic_annotation) {
  DCHECK_EQ(STATE_DONE, next_state_);
  DCHECK(user_callback_.is_null());

  return socket_->Write(buf, buf_len, std::move(callback), traffic_annotation);
}

int HttpProxyClientSocketOriginal::SetReceiveBufferSize(int32_t size) {
  return socket_->SetReceiveBufferSize(size);
}

int HttpProxyClientSocketOriginal::SetSendBufferSize(int32_t size) {
  return socket_->SetSendBufferSize(size);
}

int HttpProxyClientSocketOriginal::GetPeerAddress(IPEndPoint* address) const {
  return socket_->GetPeerAddress(address);
}

int HttpProxyClientSocketOriginal::GetLocalAddress(IPEndPoint* address) const {
  return socket_->GetLocalAddress(address);
}

int HttpProxyClientSocketOriginal::PrepareForAuthRestart() {
  if (!response_.headers.get())
    return ERR_CONNECTION_RESET;

  // If the connection can't be reused, return
  // ERR_UNABLE_TO_REUSE_CONNECTION_FOR_PROXY_AUTH.  The request will be retried
  // at a higher layer.
  if (!response_.headers->IsKeepAlive() ||
      !http_stream_parser_->CanFindEndOfResponse() || !socket_->IsConnected()) {
    socket_->Disconnect();
    return ERR_UNABLE_TO_REUSE_CONNECTION_FOR_PROXY_AUTH;
  }

  // If the auth request had a body, need to drain it before reusing the socket.
  if (!http_stream_parser_->IsResponseBodyComplete()) {
    next_state_ = STATE_DRAIN_BODY;
    drain_buf_ = base::MakeRefCounted<IOBufferWithSize>(kDrainBodyBufferSize);
    return OK;
  }

  return DidDrainBodyForAuthRestart();
}

int HttpProxyClientSocketOriginal::DidDrainBodyForAuthRestart() {
  // Can't reuse the socket if there's still unread data on it.
  if (!socket_->IsConnectedAndIdle())
    return ERR_UNABLE_TO_REUSE_CONNECTION_FOR_PROXY_AUTH;

  next_state_ = STATE_GENERATE_AUTH_TOKEN;
  is_reused_ = true;

  // Reset the other member variables.
  drain_buf_ = nullptr;
  parser_buf_ = nullptr;
  http_stream_parser_.reset();
  request_line_.clear();
  request_headers_.Clear();
  authorization_headers_.Clear();
  proxy_delegate_headers_.Clear();
  response_ = HttpResponseInfo();
  return OK;
}

void HttpProxyClientSocketOriginal::DoCallback(int result) {
  DCHECK_NE(ERR_IO_PENDING, result);
  DCHECK(!user_callback_.is_null());

  // Since Run() may result in Read being called,
  // clear user_callback_ up front.
  std::move(user_callback_).Run(result);
}

void HttpProxyClientSocketOriginal::OnIOComplete(int result) {
  DCHECK_NE(STATE_NONE, next_state_);
  DCHECK_NE(STATE_DONE, next_state_);
  int rv = DoLoop(result);
  if (rv != ERR_IO_PENDING)
    DoCallback(rv);
}

int HttpProxyClientSocketOriginal::DoLoop(int last_io_result) {
  DCHECK_NE(next_state_, STATE_NONE);
  DCHECK_NE(next_state_, STATE_DONE);
  int rv = last_io_result;
  do {
    State state = next_state_;
    next_state_ = STATE_NONE;
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
        rv = DoSendRequestComplete(rv);
        ConnectionOriginalLog::EndEventWithNetErrorCode(net_log_, 
            NetLogEventType::HTTP_TRANSACTION_TUNNEL_SEND_REQUEST, rv);
        break;
      case STATE_READ_HEADERS:
        DCHECK_EQ(OK, rv);
        net_log_.BeginEvent(
            NetLogEventType::HTTP_TRANSACTION_TUNNEL_READ_HEADERS);
        rv = DoReadHeaders();
        break;
      case STATE_READ_HEADERS_COMPLETE:
        rv = DoReadHeadersComplete(rv);
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
      case STATE_DRAIN_BODY:
        DCHECK_EQ(OK, rv);
        rv = DoDrainBody();
        break;
      case STATE_DRAIN_BODY_COMPLETE:
        rv = DoDrainBodyComplete(rv);
        break;
      case STATE_DONE:
        break;
      default:
        NOTREACHED() << "bad state";
    }
  } while (rv != ERR_IO_PENDING && next_state_ != STATE_NONE &&
           next_state_ != STATE_DONE);
  return rv;
}

int HttpProxyClientSocketOriginal::DoGenerateAuthToken() {
  next_state_ = STATE_GENERATE_AUTH_TOKEN_COMPLETE;
  return ConnectionAuthOriginalPolicy::MaybeGenerateAuthToken(auth_.get(), 
      &request_, response_.ssl_info,
      base::BindOnce(&HttpProxyClientSocketOriginal::OnIOComplete,
                     weak_factory_.GetWeakPtr()),
      net_log_);
}

int HttpProxyClientSocketOriginal::DoGenerateAuthTokenComplete(int result) {
  DCHECK_NE(ERR_IO_PENDING, result);
  if (result == OK) {
    next_state_ = STATE_CALCULATE_HEADERS;
  }
  return result;
}

int HttpProxyClientSocketOriginal::DoCalculateHeaders() {
  if (!request_line_.empty()) {
    next_state_ = STATE_SEND_REQUEST;
    return OK;
  }
  next_state_ = STATE_CALCULATE_HEADERS_COMPLETE;

  authorization_headers_.Clear();
  proxy_delegate_headers_.Clear();

  if (ConnectionAuthOriginalPolicy::HaveAuth(auth_.get())) {
    ConnectionHeaderOriginalPolicy::AddAuthorizationHeader(*auth_, &authorization_headers_);
  }

  if (proxy_delegate_) {
    ASSIGN_OR_RETURN(
        proxy_delegate_headers_,
        proxy_delegate_->OnBeforeTunnelRequest(
            proxy_chain_, proxy_chain_index_,
            base::BindOnce(
                &HttpProxyClientSocketOriginal::OnBeforeTunnelRequestComplete,
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

void HttpProxyClientSocketOriginal::OnBeforeTunnelRequestComplete(
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

int HttpProxyClientSocketOriginal::DoCalculateHeadersComplete(int result) {
  DCHECK_NE(ERR_IO_PENDING, result);
  if (result != OK) {
    return result;
  }
  next_state_ = STATE_SEND_REQUEST;

  return result;
}

int HttpProxyClientSocketOriginal::DoSendRequest() {
  next_state_ = STATE_SEND_REQUEST_COMPLETE;

  // This is constructed lazily (instead of within our Start method), so that
  // we have proxy info available.
  if (request_line_.empty()) {
    DCHECK(request_headers_.IsEmpty());

    HttpRequestHeaders extra_headers;
    ConnectionHeaderOriginalPolicy::MergeFrom(extra_headers, authorization_headers_);
    ConnectionHeaderOriginalPolicy::MergeFrom(extra_headers, proxy_delegate_headers_);

    BuildTunnelRequest(endpoint_, extra_headers, user_agent_, &request_line_,
                       &request_headers_);

    ConnectionOriginalNetLogRequestHeaders(net_log_,
                         NetLogEventType::HTTP_TRANSACTION_SEND_TUNNEL_HEADERS,
                         request_line_, &request_headers_);
  }

  parser_buf_ = base::MakeRefCounted<GrowableIOBuffer>();
  http_stream_parser_ = std::make_unique<HttpStreamParserOriginal>(
      socket_.get(), is_reused_, request_.url, request_.method,
      /*upload_data_stream=*/nullptr, parser_buf_.get(), net_log_);
  return http_stream_parser_->SendRequest(
      request_line_, request_headers_, traffic_annotation_, &response_,
      base::BindOnce(&HttpProxyClientSocketOriginal::OnIOComplete,
                     base::Unretained(this)));
}

int HttpProxyClientSocketOriginal::DoSendRequestComplete(int result) {
  if (result < 0)
    return result;

  next_state_ = STATE_READ_HEADERS;
  return OK;
}

int HttpProxyClientSocketOriginal::DoReadHeaders() {
  next_state_ = STATE_READ_HEADERS_COMPLETE;
  return http_stream_parser_->ReadResponseHeaders(base::BindOnce(
      &HttpProxyClientSocketOriginal::OnIOComplete, base::Unretained(this)));
}

int HttpProxyClientSocketOriginal::DoReadHeadersComplete(int result) {
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

int HttpProxyClientSocketOriginal::DoProcessResponseHeaders() {
  next_state_ = STATE_PROCESS_RESPONSE_HEADERS_COMPLETE;

  if (proxy_delegate_) {
    return proxy_delegate_->OnTunnelHeadersReceived(
        proxy_chain_, proxy_chain_index_, *response_.headers,
        base::BindOnce(&HttpProxyClientSocketOriginal::OnIOComplete,
                       weak_factory_.GetWeakPtr()));
  }

  return OK;
}

int HttpProxyClientSocketOriginal::DoProcessResponseHeadersComplete(int result) {
  DCHECK_NE(ERR_IO_PENDING, result);
  if (result != OK) {
    return result;
  }

  next_state_ = STATE_PROCESS_RESPONSE_CODE;
  return OK;
}

int HttpProxyClientSocketOriginal::DoProcessResponseCode() {
  switch (response_.headers->response_code()) {
    case 200:  // OK
      if (http_stream_parser_->IsMoreDataBuffered())
        // The proxy sent extraneous data after the headers.
        return ERR_TUNNEL_CONNECTION_FAILED;

      next_state_ = STATE_DONE;
      return OK;

      // We aren't able to CONNECT to the remote host through the proxy.  We
      // need to be very suspicious about the response because an active network
      // attacker can force us into this state by masquerading as the proxy.
      // The only safe thing to do here is to fail the connection because our
      // client is expecting an SSL protected response.
      // See http://crbug.com/7338.

    case 407:  // Proxy Authentication Required
      // We need this status code to allow proxy authentication.  Our
      // authentication code is smart enough to avoid being tricked by an
      // active network attacker.
      // The next state is intentionally not set as it should be STATE_NONE;
      SanitizeProxyAuth(response_);
      return HandleProxyAuthChallenge(auth_.get(), &response_, net_log_);

    default:
      // Ignore response to avoid letting the proxy impersonate the target
      // server.  (See http://crbug.com/137891.)
      // We lose something by doing this.  We have seen proxy 403, 404, and
      // 501 response bodies that contain a useful error message.  For
      // example, Squid uses a 404 response to report the DNS error: "The
      // domain name does not exist."
      return ERR_TUNNEL_CONNECTION_FAILED;
  }
}

int HttpProxyClientSocketOriginal::DoDrainBody() {
  DCHECK(drain_buf_.get());
  next_state_ = STATE_DRAIN_BODY_COMPLETE;
  return http_stream_parser_->ReadResponseBody(
      drain_buf_.get(), kDrainBodyBufferSize,
      base::BindOnce(&HttpProxyClientSocketOriginal::OnIOComplete,
                     base::Unretained(this)));
}

int HttpProxyClientSocketOriginal::DoDrainBodyComplete(int result) {
  if (result < 0)
    return ERR_UNABLE_TO_REUSE_CONNECTION_FOR_PROXY_AUTH;

  if (!http_stream_parser_->IsResponseBodyComplete()) {
    // Keep draining.
    next_state_ = STATE_DRAIN_BODY;
    return OK;
  }

  return DidDrainBodyForAuthRestart();
}

bool HttpProxyClientSocketOriginal::CheckDone() {
  if (next_state_ != STATE_DONE) {
    // We're trying to read the body of the response but we're still trying
    // to establish an SSL tunnel through the proxy.  We can't read these
    // bytes when establishing a tunnel because they might be controlled by
    // an active network attacker.  We don't worry about this for HTTP
    // because an active network attacker can already control HTTP sessions.
    // We reach this case when the user cancels a 407 proxy auth prompt.
    // See http://crbug.com/8473.
    DCHECK_EQ(407, response_.headers->response_code());

    return false;
  }
  return true;
}

//----------------------------------------------------------------

// static
void HttpProxyClientSocketOriginal::BuildTunnelRequest(
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
int HttpProxyClientSocketOriginal::HandleProxyAuthChallenge(
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
void HttpProxyClientSocketOriginal::SanitizeProxyAuth(HttpResponseInfo& response) {
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
