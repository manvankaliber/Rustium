// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include "net/socket/connection_jobs_upper_rust_job_workloads.h"

namespace net::connection_jobs_job_workloads {
#if BUILDFLAG(ENABLE_QUIC_PROXY_SUPPORT)
QuicProvider::~QuicProvider() = default;

QuicProvider::QuicProvider(HttpNetworkSession& session):QuicSessionPool(nullptr,nullptr,
    session.ssl_config_service(),nullptr,session.http_server_properties(),
    session.cert_verifier(),session.context().transport_security_state,nullptr,
    nullptr,nullptr,nullptr,nullptr,session.context().quic_context) {}

void QuicProvider::CancelRequest(QuicSessionRequest*) {++cancels;}

int QuicProvider::RequestSession(const QuicSessionKey&,url::SchemeHostPort,
      quic::ParsedQuicVersion version,const std::optional<NetworkTrafficAnnotationTag>,
      MultiplexedSessionCreationInitiator,std::optional<ConnectionManagementConfig>,
      const HttpUserAgentSettings*,RequestPriority,bool,int,const GURL&,
      const NetLogWithSource&,QuicSessionRequest*) {
    ++requests;last_version=version;return result;
  }

#endif

Script::Script() = default;

Script::~Script() = default;
}  // namespace net::connection_jobs_job_workloads
