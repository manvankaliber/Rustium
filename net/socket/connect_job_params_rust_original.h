// Copyright 2024 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_SOCKET_CONNECT_JOB_PARAMS_RUST_ORIGINAL_H_
#define NET_SOCKET_CONNECT_JOB_PARAMS_RUST_ORIGINAL_H_

#include <variant>

#include "base/memory/scoped_refptr.h"
#include "net/base/net_export.h"

namespace net {

class HttpProxySocketParamsRustOriginal;
class SOCKSSocketParamsRustOriginal;
class TransportSocketParamsRustOriginal;
class SSLSocketParamsRustOriginal;

// Abstraction over the param types for various connect jobs.
class ConnectJobParamsRustOriginal {
 public:
  ConnectJobParamsRustOriginal();
  explicit ConnectJobParamsRustOriginal(scoped_refptr<HttpProxySocketParamsRustOriginal> params);
  explicit ConnectJobParamsRustOriginal(scoped_refptr<SOCKSSocketParamsRustOriginal> params);
  explicit ConnectJobParamsRustOriginal(scoped_refptr<TransportSocketParamsRustOriginal> params);
  explicit ConnectJobParamsRustOriginal(scoped_refptr<SSLSocketParamsRustOriginal> params);
  ~ConnectJobParamsRustOriginal();

  ConnectJobParamsRustOriginal(ConnectJobParamsRustOriginal&);
  ConnectJobParamsRustOriginal& operator=(ConnectJobParamsRustOriginal&);
  ConnectJobParamsRustOriginal(ConnectJobParamsRustOriginal&&);
  ConnectJobParamsRustOriginal& operator=(ConnectJobParamsRustOriginal&&);

  bool is_http_proxy() const {
    return std::holds_alternative<scoped_refptr<HttpProxySocketParamsRustOriginal>>(
        params_);
  }

  bool is_socks() const {
    return std::holds_alternative<scoped_refptr<SOCKSSocketParamsRustOriginal>>(params_);
  }

  bool is_transport() const {
    return std::holds_alternative<scoped_refptr<TransportSocketParamsRustOriginal>>(
        params_);
  }

  bool is_ssl() const {
    return std::holds_alternative<scoped_refptr<SSLSocketParamsRustOriginal>>(params_);
  }

  // Get lvalue references to the contained params.
  const scoped_refptr<HttpProxySocketParamsRustOriginal>& http_proxy() const {
    return get<scoped_refptr<HttpProxySocketParamsRustOriginal>>(params_);
  }
  const scoped_refptr<SOCKSSocketParamsRustOriginal>& socks() const {
    return get<scoped_refptr<SOCKSSocketParamsRustOriginal>>(params_);
  }
  const scoped_refptr<TransportSocketParamsRustOriginal>& transport() const {
    return get<scoped_refptr<TransportSocketParamsRustOriginal>>(params_);
  }
  const scoped_refptr<SSLSocketParamsRustOriginal>& ssl() const {
    return get<scoped_refptr<SSLSocketParamsRustOriginal>>(params_);
  }

  // Take params out of this value.
  scoped_refptr<HttpProxySocketParamsRustOriginal>&& take_http_proxy() {
    return get<scoped_refptr<HttpProxySocketParamsRustOriginal>>(std::move(params_));
  }
  scoped_refptr<SOCKSSocketParamsRustOriginal>&& take_socks() {
    return get<scoped_refptr<SOCKSSocketParamsRustOriginal>>(std::move(params_));
  }
  scoped_refptr<TransportSocketParamsRustOriginal>&& take_transport() {
    return get<scoped_refptr<TransportSocketParamsRustOriginal>>(std::move(params_));
  }
  scoped_refptr<SSLSocketParamsRustOriginal>&& take_ssl() {
    return get<scoped_refptr<SSLSocketParamsRustOriginal>>(std::move(params_));
  }

 private:
  std::variant<scoped_refptr<HttpProxySocketParamsRustOriginal>,
               scoped_refptr<SOCKSSocketParamsRustOriginal>,
               scoped_refptr<TransportSocketParamsRustOriginal>,
               scoped_refptr<SSLSocketParamsRustOriginal>>
      params_;
};

}  // namespace net

#endif  // NET_SOCKET_CONNECT_JOB_PARAMS_RUST_ORIGINAL_H_
