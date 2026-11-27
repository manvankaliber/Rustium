// Copyright 2026 The Chromium Authors
// BSD-style license: see LICENSE.
#include "net/socket/connection_jobs_http2_rust_original_context.h"
#include "net/http/http_network_session.h"
#include "net/quic/quic_context.h"
#include "net/spdy/spdy_session_pool_family_original_test_peer.h"
namespace net {
namespace {
// Frozen AddDefaultHttp2Settings from http_network_session.cc. Numeric protocol
// defaults are source-bound in before/; do not invoke a production policy helper.
spdy::SettingsMap OriginalSettings(spdy::SettingsMap settings) {
  settings[spdy::SETTINGS_ENABLE_PUSH] = 0;
  settings.try_emplace(spdy::SETTINGS_HEADER_TABLE_SIZE, 64 * 1024);
  settings.try_emplace(spdy::SETTINGS_INITIAL_WINDOW_SIZE, 6 * 1024 * 1024);
  settings.try_emplace(spdy::SETTINGS_MAX_HEADER_LIST_SIZE, 256 * 1024);
  return settings;
}
}  // namespace
ConnectionJobsHttp2RustOriginalContext::ConnectionJobsHttp2RustOriginalContext(
    HttpNetworkSession& providers, bool enable_sending_initial_data) {
  const auto& params = providers.params();
  const auto& context = providers.context();
  std::optional<SpdySessionPoolOriginal::GreasedHttp2Frame> grease;
  if (params.greased_http2_frame) {
    const auto& value = *params.greased_http2_frame;
    grease.emplace(SpdySessionPoolOriginal::GreasedHttp2Frame{
        value.type, value.flags, value.payload});
  }
  pool_ = std::make_unique<SpdySessionPoolOriginal>(
      context.host_resolver, providers.ssl_client_context(),
      context.http_server_properties, context.transport_security_state,
      context.quic_context->params()->supported_versions,
      params.enable_spdy_ping_based_connection_checking, params.enable_http2,
      params.enable_quic, params.spdy_session_max_recv_window_size,
      params.spdy_session_max_queued_capped_frames,
      OriginalSettings(params.http2_settings), params.enable_http2_settings_grease,
      grease, params.http2_end_stream_with_data_frame, params.enable_priority_update,
      params.spdy_go_away_on_ip_change, params.time_func,
      context.network_quality_estimator, !params.ignore_ip_address_changes);
  SpdySessionPoolOriginalPeer::SetEnableSendingInitialData(
      *pool_, enable_sending_initial_data);
}
ConnectionJobsHttp2RustOriginalContext::~ConnectionJobsHttp2RustOriginalContext() = default;
}  // namespace net
