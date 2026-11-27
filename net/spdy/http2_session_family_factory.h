// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#ifndef NET_SPDY_HTTP2_SESSION_FAMILY_FACTORY_H_
#define NET_SPDY_HTTP2_SESSION_FAMILY_FACTORY_H_

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <utility>

#include "base/memory/raw_ptr.h"
#include "base/time/time.h"
#include "base/types/expected.h"
#include "net/base/load_timing_info.h"
#include "net/base/proxy_value_rust_reference_proxy_chain.h"
#include "net/log/net_log_with_source.h"
#include "net/socket/stream_socket.h"
#include "net/spdy/spdy_http_stream.h"
#include "net/spdy/spdy_http_stream_family_original.h"
#include "net/spdy/spdy_session.h"
#include "net/spdy/spdy_session_family_original.h"
#include "net/spdy/spdy_session_key_rust_original.h"
#include "net/spdy/spdy_session_pool.h"
#include "net/spdy/spdy_session_pool_family_original.h"

namespace net::http2_family_test {

// Providers remain native services and must outlive the factory. No reference
// operation obtains a production pool from HttpNetworkSession.
struct Http2SessionFamilyServices {
  raw_ptr<HostResolver> resolver = nullptr;
  raw_ptr<SSLClientContext> ssl_context = nullptr;
  raw_ptr<HttpServerProperties> server_properties = nullptr;
  raw_ptr<TransportSecurityState> transport_state = nullptr;
  raw_ptr<NetworkQualityEstimator> quality_estimator = nullptr;
};

struct Http2SessionFamilyOptions {
  struct Grease {
    uint8_t type;
    uint8_t flags;
    std::string payload;
  };
  static spdy::SettingsMap DefaultSettings() {
    return {{spdy::SETTINGS_HEADER_TABLE_SIZE, 4096},
            {spdy::SETTINGS_ENABLE_PUSH, 0},
            {spdy::SETTINGS_INITIAL_WINDOW_SIZE, 65535},
            {spdy::SETTINGS_MAX_HEADER_LIST_SIZE, 65536}};
  }
  spdy::SettingsMap initial_settings = DefaultSettings();
  quic::ParsedQuicVersionVector quic_versions;
  std::optional<Grease> grease;
  size_t max_receive_window = 65535;
  int max_queued_capped_frames = 10000;
  bool check_connections_with_ping = false;
  bool http2_enabled = true;
  bool quic_enabled = false;
  bool grease_settings = false;
  bool end_stream_with_data = false;
  bool priority_update = false;
  bool go_away_on_ip_change = false;
  bool cleanup_on_ip_change = false;
  base::TimeTicks (*time_func)() = &base::TimeTicks::Now;
};

template <class Pool>
struct FamilyTypes;
template <>
struct FamilyTypes<SpdySessionPool> {
  using Session = SpdySession;
  using Key = SpdySessionKey;
  using HttpStream = SpdyHttpStream;
};
template <>
struct FamilyTypes<SpdySessionPoolOriginal> {
  using Session = SpdySessionOriginal;
  using Key = SpdySessionKeyOriginal;
  using HttpStream = SpdyHttpStreamOriginal;
};

template <class Pool>
class Http2SessionFamilyFactory {
 public:
  using Session = typename FamilyTypes<Pool>::Session;
  using Key = typename FamilyTypes<Pool>::Key;
  using HttpStream = typename FamilyTypes<Pool>::HttpStream;

  explicit Http2SessionFamilyFactory(
      Http2SessionFamilyServices services,
      Http2SessionFamilyOptions options = {}) {
    std::optional<typename Pool::GreasedHttp2Frame> grease;
    if (options.grease) {
      grease.emplace(typename Pool::GreasedHttp2Frame{
          options.grease->type, options.grease->flags, options.grease->payload});
    }
    pool_ = std::make_unique<Pool>(
        services.resolver, services.ssl_context, services.server_properties,
        services.transport_state, options.quic_versions,
        options.check_connections_with_ping, options.http2_enabled,
        options.quic_enabled, options.max_receive_window,
        options.max_queued_capped_frames, options.initial_settings,
        options.grease_settings, grease, options.end_stream_with_data,
        options.priority_update, options.go_away_on_ip_change, options.time_func,
        services.quality_estimator, options.cleanup_on_ip_change);
  }

  Http2SessionFamilyFactory(const Http2SessionFamilyFactory&) = delete;
  Http2SessionFamilyFactory& operator=(const Http2SessionFamilyFactory&) = delete;
  ~Http2SessionFamilyFactory() = default;

  base::expected<base::WeakPtr<Session>, int> OpenSession(
      const Key& key,
      std::unique_ptr<StreamSocket> socket,
      const LoadTimingInfo::ConnectTiming& timing = {},
      const NetLogWithSource& source = {}) {
    auto result = pool_->CreateAvailableSessionFromSocket(
        key, std::move(socket), timing, source);
    if (result.has_value()) {
      session_ = result.value();
    }
    return result;
  }

  Pool* pool() { return pool_.get(); }
  base::WeakPtr<Session> session() const { return session_; }

 private:
  std::unique_ptr<Pool> pool_;
  base::WeakPtr<Session> session_;
};

}  // namespace net::http2_family_test
#endif  // NET_SPDY_HTTP2_SESSION_FAMILY_FACTORY_H_
