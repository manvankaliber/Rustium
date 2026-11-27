// Copyright 2016 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_SPDY_FAMILY_ORIGINAL_MULTIPLEXED_SESSION_H_
#define NET_SPDY_FAMILY_ORIGINAL_MULTIPLEXED_SESSION_H_

#include "base/component_export.h"
#include <string_view>
#include "net/spdy/multiplexed_session.h"

#include "net/base/ip_endpoint.h"
#include "net/base/net_errors.h"
#include "net/http/http_stream.h"
#include "net/ssl/ssl_info.h"

namespace url {
class SchemeHostPort;
}

namespace net {

// A handle to a multiplexed session which will be valid even after the
// underlying session is deleted.
class COMPONENT_EXPORT(HTTP2_STREAM_FAMILY_REFERENCE) MultiplexedSessionHandleOriginal {
 public:
  explicit MultiplexedSessionHandleOriginal(base::WeakPtr<MultiplexedSession> session);
  virtual ~MultiplexedSessionHandleOriginal();

  // Gets the remote endpoint of the socket that the HTTP stream is using, if
  // any. Returns OK and fills in |endpoint| if it is available; returns an
  // error and does not modify |endpoint| otherwise.
  int GetRemoteEndpoint(IPEndPoint* endpoint);

  // Fills SSL info in |ssl_info| and returns true when SSL is in use.
  bool GetSSLInfo(SSLInfo* ssl_info) const;

  // Caches SSL info from the underlying session.
  void SaveSSLInfo();

  // The value corresponding to |scheme_host_port| in the ACCEPT_CH frame
  // received during TLS handshake via the ALPS extension, or the empty string
  // if the server did not send one or if the underlying session is not
  // available.
  //
  // Note that this uses url::SchemeHostPort instead of url::Origin because this
  // is based around network authorities, as opposed to general RFC 6454
  // origins.
  virtual std::string_view GetAcceptChViaAlps(
      const url::SchemeHostPort& scheme_host_port) const;

 private:
  base::WeakPtr<MultiplexedSession> session_;
  SSLInfo ssl_info_;
  bool has_ssl_info_;
};

}  // namespace net

#endif  // NET_SPDY_FAMILY_ORIGINAL_MULTIPLEXED_SESSION_H_
