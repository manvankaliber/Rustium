// Copyright 2012 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_SOCKET_SSL_CONNECT_JOB_RUST_ORIGINAL_H_
#define NET_SOCKET_SSL_CONNECT_JOB_RUST_ORIGINAL_H_

#include <stdint.h>

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "base/containers/flat_set.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/ref_counted.h"
#include "base/time/time.h"
#include "net/base/completion_once_callback.h"
#include "net/base/completion_repeating_callback.h"
#include "net/base/net_export.h"
#include "net/base/network_anonymization_key.h"
#include "net/dns/public/host_resolver_results.h"
#include "net/dns/public/resolution_details.h"
#include "net/dns/public/resolve_error_info.h"
#include "net/socket/connect_job_rust_original.h"
#include "net/socket/connect_job_params_rust_original.h"
#include "net/socket/connection_attempts.h"
#include "net/socket/ssl_client_socket.h"
#include "net/ssl/ssl_cert_request_info.h"
#include "net/ssl/ssl_config_service.h"

// All family reference objects link together in one test-only source_set.
#define CONNECTION_JOBS_ORIGINAL_PRIVATE_EXPORT
#define CONNECTION_JOBS_ORIGINAL_EXPORT

namespace net {

class HostPortPair;
class HttpProxySocketParamsRustOriginal;
class SocketTag;
class SOCKSSocketParamsRustOriginal;
class TcpConnectJobRustOriginal;
class TransportSocketParamsRustOriginal;

class CONNECTION_JOBS_ORIGINAL_PRIVATE_EXPORT SSLSocketParamsRustOriginal
    : public base::RefCounted<SSLSocketParamsRustOriginal> {
 public:
  enum ConnectionType { DIRECT, SOCKS_PROXY, HTTP_PROXY };

  // Exactly one of |direct_params|, |socks_proxy_params|, and
  // |http_proxy_params| must be non-NULL.
  SSLSocketParamsRustOriginal(ConnectJobParamsRustOriginal params,
                  const HostPortPair& host_and_port,
                  const SSLConfig& ssl_config,
                  NetworkAnonymizationKey network_anonymization_key);

  SSLSocketParamsRustOriginal(const SSLSocketParamsRustOriginal&) = delete;
  SSLSocketParamsRustOriginal& operator=(const SSLSocketParamsRustOriginal&) = delete;

  // Returns the type of the underlying connection.
  ConnectionType GetConnectionType() const;

  // Must be called only when GetConnectionType() returns DIRECT.
  const scoped_refptr<TransportSocketParamsRustOriginal>& GetDirectConnectionParams()
      const {
    return nested_params_.transport();
  }

  // Must be called only when GetConnectionType() returns SOCKS_PROXY.
  const scoped_refptr<SOCKSSocketParamsRustOriginal>& GetSocksProxyConnectionParams()
      const {
    return nested_params_.socks();
  }

  // Must be called only when GetConnectionType() returns HTTP_PROXY.
  const scoped_refptr<HttpProxySocketParamsRustOriginal>& GetHttpProxyConnectionParams()
      const {
    return nested_params_.http_proxy();
  }

  const HostPortPair& host_and_port() const { return host_and_port_; }
  const SSLConfig& ssl_config() const { return ssl_config_; }
  const NetworkAnonymizationKey& network_anonymization_key() const {
    return network_anonymization_key_;
  }

 private:
  friend class base::RefCounted<SSLSocketParamsRustOriginal>;
  ~SSLSocketParamsRustOriginal();

  const ConnectJobParamsRustOriginal nested_params_;
  const HostPortPair host_and_port_;
  const SSLConfig ssl_config_;
  const NetworkAnonymizationKey network_anonymization_key_;
};

// SSLConnectJob establishes a connection, through a proxy if needed, and then
// handles the SSL handshake. It returns an SSLClientSocket on success.
class CONNECTION_JOBS_ORIGINAL_PRIVATE_EXPORT SSLConnectJobRustOriginal : public ConnectJobRustOriginal,
                                         public ConnectJobRustOriginal::Delegate {
 public:
  class CONNECTION_JOBS_ORIGINAL_PRIVATE_EXPORT Factory {
   public:
    Factory() = default;
    virtual ~Factory() = default;

    virtual std::unique_ptr<SSLConnectJobRustOriginal> Create(
        RequestPriority priority,
        const SocketTag& socket_tag,
        const CommonConnectJobParamsRustOriginal* common_connect_job_params,
        scoped_refptr<SSLSocketParamsRustOriginal> params,
        ConnectJobRustOriginal::Delegate* delegate,
        const NetLogWithSource* net_log);
  };

  SSLConnectJobRustOriginal(RequestPriority priority,
                const SocketTag& socket_tag,
                const CommonConnectJobParamsRustOriginal* common_connect_job_params,
                scoped_refptr<SSLSocketParamsRustOriginal> params,
                ConnectJobRustOriginal::Delegate* delegate,
                const NetLogWithSource* net_log);

  SSLConnectJobRustOriginal(const SSLConnectJobRustOriginal&) = delete;
  SSLConnectJobRustOriginal& operator=(const SSLConnectJobRustOriginal&) = delete;

  ~SSLConnectJobRustOriginal() override;

  // ConnectJob methods.
  LoadState GetLoadState() const override;
  bool HasEstablishedConnection() const override;

  // ConnectJob::Delegate methods.
  void OnConnectJobComplete(int result, ConnectJobRustOriginal* job) override;
  void OnNeedsProxyAuth(const HttpResponseInfo& response,
                        HttpAuthController* auth_controller,
                        base::OnceClosure restart_with_auth_callback,
                        ConnectJobRustOriginal* job) override;
  ConnectionAttempts GetConnectionAttempts() const override;
  ResolveErrorInfo GetResolveErrorInfo() const override;
  bool IsSSLError() const override;
  scoped_refptr<SSLCertRequestInfo> GetCertRequestInfo() override;
  std::optional<ResolutionDetails> GetResolutionDetails() const override;

  // Returns the timeout for the SSL handshake. This is the same for all
  // connections regardless of whether or not there is a proxy in use.
  static base::TimeDelta HandshakeTimeoutForTesting();

 private:
  enum State {
    STATE_TRANSPORT_CONNECT,
    STATE_TRANSPORT_CONNECT_COMPLETE,
    STATE_SOCKS_CONNECT,
    STATE_SOCKS_CONNECT_COMPLETE,
    STATE_TUNNEL_CONNECT,
    STATE_TUNNEL_CONNECT_COMPLETE,
    STATE_SSL_CONNECT,
    STATE_SSL_CONNECT_COMPLETE,
    STATE_NONE,
  };

  void OnIOComplete(int result);

  // Runs the state transition loop.
  int DoLoop(int result);

  int DoTransportConnect();
  int DoTransportConnectComplete(int result);
  int DoSOCKSConnect();
  int DoSOCKSConnectComplete(int result);
  int DoTunnelConnect();
  int DoTunnelConnectComplete(int result);
  int DoSSLConnect();
  int DoSSLConnectComplete(int result);

  // Returns the initial state for the state machine based on the
  // |connection_type|.
  static State GetInitialState(SSLSocketParamsRustOriginal::ConnectionType connection_type);

  // Starts the SSL connection process.  Returns OK on success and
  // ERR_IO_PENDING if it cannot immediately service the request.
  // Otherwise, it returns a net error code.
  int ConnectInternal() override;

  void ResetStateForRestart();

  void ChangePriorityInternal(RequestPriority priority) override;

  scoped_refptr<SSLSocketParamsRustOriginal> params_;
  std::optional<ResolutionDetails> resolution_details_;

  State next_state_;
  CompletionRepeatingCallback callback_;
  std::unique_ptr<ConnectJobRustOriginal> nested_connect_job_;
  // Points to `nested_connect_job_` if it's of type TcpConnectJob. Used to call
  // GetServiceEndpoint() on successful connect if non-null.
  raw_ptr<TcpConnectJobRustOriginal> tcp_connect_job_;
  std::unique_ptr<StreamSocket> nested_socket_;
  std::unique_ptr<SSLClientSocket> ssl_socket_;

  // True once SSL negotiation has started.
  bool ssl_negotiation_started_ = false;

  // True if legacy crypto should be disabled for the job's current connection
  // attempt. On error, the connection will be retried with legacy crypto
  // enabled.
  bool disable_legacy_crypto_with_fallback_ = true;

  scoped_refptr<SSLCertRequestInfo> ssl_cert_request_info_;

  ConnectionAttempts connection_attempts_;
  ResolveErrorInfo resolve_error_info_;
  // The address of the server the connect job is connected to. Populated if
  // and only if the connect job is connected *directly* to the server (not
  // through an HTTPS CONNECT request or a SOCKS proxy).
  IPEndPoint server_address_;

  // Any DNS aliases for the remote endpoint. Includes all known aliases, e.g.
  // from A, AAAA, or HTTPS, not just from the address used for the connection,
  // in no particular order. Stored because `nested_connect_job_` has a limited
  // lifetime and the aliases can no longer be retrieved from there by the
  // time that the aliases are needed to be passed in SetSocket.
  base::flat_set<std::string> dns_aliases_;

  // The endpoint result used by `nested_connect_job_`. Stored because
  // `nested_connect_job_` has a limited lifetime.
  std::optional<HostResolverEndpointResult> endpoint_result_;

  // Same as `endpoint_result_`, except in the case that TcpConnectJob is in
  // use.
  std::optional<ServiceEndpoint> service_endpoint_result_;

  // If not `std::nullopt`, the ECH retry configs to use in the ECH recovery
  // flow. `endpoint_result_` will then contain the endpoint to reconnect to.
  std::optional<std::vector<uint8_t>> ech_retry_configs_;


  // True if the connection was established using a stale DNS result. Passed
  // up from the nested ConnectJob.
  bool is_connected_via_stale_dns_ = false;

  // True if the use of stale DNS results should be disabled for this connection
  // attempt. Set to true when a previous connection attempt via stale DNS
  // failed and the job needs to retry with fresh results.
  bool disable_stale_dns_ = false;
};

}  // namespace net

#endif  // NET_SOCKET_SSL_CONNECT_JOB_RUST_ORIGINAL_H_
