// Copyright 2012 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/spdy/spdy_session_pool.h"

#include "net/spdy/spdy_session_pool_rust_ffi.h"

#include <algorithm>
#include <set>
#include <utility>

#include "base/check_op.h"
#include "base/containers/flat_set.h"
#include "base/containers/span.h"
#include "base/functional/bind.h"
#include "base/metrics/histogram_macros.h"
#include "base/notimplemented.h"
#include "base/notreached.h"
#include "base/task/single_thread_task_runner.h"
#include "base/trace_event/trace_event.h"
#include "base/types/expected.h"
#include "base/values.h"
#include "build/build_config.h"
#include "net/base/features.h"
#include "net/base/ip_endpoint.h"
#include "net/base/trace_constants.h"
#include "net/dns/dns_alias_utility.h"
#include "net/dns/host_resolver.h"
#include "net/dns/public/host_resolver_source.h"
#include "net/http/http_network_session.h"
#include "net/http/http_server_properties.h"
#include "net/http/http_stream_request.h"
#include "net/log/net_log_event_type.h"
#include "net/log/net_log_source.h"
#include "net/log/net_log_with_source.h"
#include "net/socket/stream_socket_handle.h"
#include "net/spdy/multiplexed_session_creation_initiator.h"
#include "net/spdy/spdy_session.h"
#include "net/third_party/quiche/src/quiche/http2/hpack/hpack_constants.h"
#include "net/third_party/quiche/src/quiche/http2/hpack/hpack_static_table.h"

namespace net {

namespace {

enum SpdySessionGetTypes {
  CREATED_NEW                 = 0,
  FOUND_EXISTING              = 1,
  FOUND_EXISTING_FROM_IP_POOL = 2,
  IMPORTED_FROM_SOCKET        = 3,
  SPDY_SESSION_GET_MAX        = 4
};

bool IsEndpointEligibleForH2(const ConnectionEndpointMetadata& metadata) {
const int64_t rust_result = RunSpdySessionPoolRust(36, [&](uint32_t operation, [[maybe_unused]] uint64_t a, [[maybe_unused]] uint64_t b, [[maybe_unused]] uint64_t c) -> int64_t {
    switch(operation) {
      case 0: { return metadata.supported_protocol_alpns.size(); }
      case 1: { return base::span(metadata.supported_protocol_alpns)[a]=="h2"; }
    }
    NOTREACHED();
  });
return rust_result != 0;
}

void AppendUniqueIpEndpoints(std::vector<IPEndPoint>& target,
                             base::span<const IPEndPoint> source) {
RunSpdySessionPoolRust(37, [&](uint32_t operation, [[maybe_unused]] uint64_t a, [[maybe_unused]] uint64_t b, [[maybe_unused]] uint64_t c) -> int64_t {
    switch(operation) {
      case 0: { return source.size(); }
      case 1: { return target.size(); }
      case 2: { return base::span(target)[b]==source[a]; }
      case 3: { target.push_back(source[a]);
return 0; }
    }
    NOTREACHED();
  });
}

}  // namespace

SpdySessionPool::SpdySessionRequest::Delegate::Delegate() = default;
SpdySessionPool::SpdySessionRequest::Delegate::~Delegate() = default;

SpdySessionPool::SpdySessionRequest::SpdySessionRequest(
    const SpdySessionKey& key,
    bool enable_ip_based_pooling_for_h2,
    bool is_websocket,
    bool is_blocking_request_for_session,
    Delegate* delegate,
    SpdySessionPool* spdy_session_pool)
    : key_(key),
      enable_ip_based_pooling_for_h2_(enable_ip_based_pooling_for_h2),
      is_websocket_(is_websocket),
      is_blocking_request_for_session_(is_blocking_request_for_session),
      delegate_(delegate),
      spdy_session_pool_(spdy_session_pool) {}

SpdySessionPool::SpdySessionRequest::~SpdySessionRequest() {
RunSpdySessionPoolRust(33, [&](uint32_t operation, [[maybe_unused]] uint64_t a, [[maybe_unused]] uint64_t b, [[maybe_unused]] uint64_t c) -> int64_t {
    switch(operation) {
      case 0: { return !!spdy_session_pool_; }
      case 1: { spdy_session_pool_->RemoveRequestForSpdySession(this);
return 0; }
    }
    NOTREACHED();
  });
}

void SpdySessionPool::SpdySessionRequest::OnRemovedFromPool() {
  DCHECK(spdy_session_pool_);
  spdy_session_pool_ = nullptr;
}

SpdySessionPool::SpdySessionPool(
    HostResolver* resolver,
    SSLClientContext* ssl_client_context,
    HttpServerProperties* http_server_properties,
    TransportSecurityState* transport_security_state,
    const quic::ParsedQuicVersionVector& quic_supported_versions,
    bool enable_ping_based_connection_checking,
    bool is_http2_enabled,
    bool is_quic_enabled,
    size_t session_max_recv_window_size,
    int session_max_queued_capped_frames,
    const spdy::SettingsMap& initial_settings,
    bool enable_http2_settings_grease,
    const std::optional<GreasedHttp2Frame>& greased_http2_frame,
    bool http2_end_stream_with_data_frame,
    bool enable_priority_update,
    bool go_away_on_ip_change,
    SpdySessionPool::TimeFunc time_func,
    NetworkQualityEstimator* network_quality_estimator,
    bool cleanup_sessions_on_ip_address_changed)
    : http_server_properties_(http_server_properties),
      transport_security_state_(transport_security_state),
      ssl_client_context_(ssl_client_context),
      resolver_(resolver),
      quic_supported_versions_(quic_supported_versions),
      enable_ping_based_connection_checking_(
          enable_ping_based_connection_checking),
      is_http2_enabled_(is_http2_enabled),
      is_quic_enabled_(is_quic_enabled),
      session_max_recv_window_size_(session_max_recv_window_size),
      session_max_queued_capped_frames_(session_max_queued_capped_frames),
      initial_settings_(initial_settings),
      enable_http2_settings_grease_(enable_http2_settings_grease),
      greased_http2_frame_(greased_http2_frame),
      http2_end_stream_with_data_frame_(http2_end_stream_with_data_frame),
      enable_priority_update_(enable_priority_update),
      go_away_on_ip_change_(go_away_on_ip_change),
      time_func_(time_func),
      network_quality_estimator_(network_quality_estimator),
      cleanup_sessions_on_ip_address_changed_(
          cleanup_sessions_on_ip_address_changed) {
RunSpdySessionPoolRust(34, [&](uint32_t operation, [[maybe_unused]] uint64_t a, [[maybe_unused]] uint64_t b, [[maybe_unused]] uint64_t c) -> int64_t {
    switch(operation) {
      case 0: { return cleanup_sessions_on_ip_address_changed_; }
      case 1: { NetworkChangeNotifier::AddIPAddressObserver(this);
return 0; }
      case 2: { return !!ssl_client_context_; }
      case 3: { ssl_client_context_->AddObserver(this);
return 0; }
    }
    NOTREACHED();
  });
}

SpdySessionPool::~SpdySessionPool() {
RunSpdySessionPoolRust(35, [&](uint32_t operation, [[maybe_unused]] uint64_t a, [[maybe_unused]] uint64_t b, [[maybe_unused]] uint64_t c) -> int64_t {
    switch(operation) {
      case 0: { 
#if DCHECK_IS_ON()
       for (const auto& request_info : spdy_session_request_map_) {
         DCHECK(request_info.second.request_set.empty());
       }
#endif
    
return 0; }
      case 1: { CloseAllSessions();
return 0; }
      case 2: { return !sessions_.empty(); }
      case 3: { RemoveUnavailableSession((*sessions_.begin())->GetWeakPtr());
return 0; }
      case 4: { return !!ssl_client_context_; }
      case 5: { ssl_client_context_->RemoveObserver(this);
return 0; }
      case 6: { return cleanup_sessions_on_ip_address_changed_; }
      case 7: { NetworkChangeNotifier::RemoveIPAddressObserver(this);
return 0; }
    }
    NOTREACHED();
  });
}

int SpdySessionPool::CreateAvailableSessionFromSocketHandle(
    const SpdySessionKey& key,
    std::unique_ptr<StreamSocketHandle> stream_socket_handle,
    const NetLogWithSource& net_log,
    const MultiplexedSessionCreationInitiator session_creation_initiator,
    base::WeakPtr<SpdySession>* session,
    std::optional<ConnectionManagementConfig> connection_management_config,
    SpdySessionInitiator spdy_session_initiator) {
std::unique_ptr<SpdySession> new_session;
  base::flat_set<std::string> dns_aliases;
  std::optional<base::expected<base::WeakPtr<SpdySession>,int>> inserted;

  const int64_t rust_result = RunSpdySessionPoolRust(25, [&](uint32_t operation, [[maybe_unused]] uint64_t a, [[maybe_unused]] uint64_t b, [[maybe_unused]] uint64_t c) -> int64_t {
    switch(operation) {
      case 0: { TRACE_EVENT0(NetTracingCategory(),"SpdySessionPool::CreateAvailableSessionFromSocketHandle");
return 0; }
      case 1: { new_session=CreateSession(key,net_log.net_log(),session_creation_initiator,spdy_session_initiator,connection_management_config);
return 0; }
      case 2: { dns_aliases=stream_socket_handle->socket()->GetDnsAliases();
return 0; }
      case 3: { new_session->InitializeWithSocketHandle(std::move(stream_socket_handle),this);
return 0; }
      case 4: { inserted=InsertSession(key,std::move(new_session),net_log,std::move(dns_aliases),true); return inserted->has_value(); }
      case 5: { *session=std::move(inserted->value());
return 0; }
      case 6: { return inserted->error(); }
    }
    NOTREACHED();
  });
return static_cast<int>(rust_result);
}

base::expected<base::WeakPtr<SpdySession>, int>
SpdySessionPool::CreateAvailableSessionFromSocket(
    const SpdySessionKey& key,
    std::unique_ptr<StreamSocket> socket_stream,
    const LoadTimingInfo::ConnectTiming& connect_timing,
    const NetLogWithSource& net_log,
    SpdySessionInitiator spdy_session_initiator) {
std::unique_ptr<SpdySession> new_session;
  base::flat_set<std::string> dns_aliases;
  std::optional<base::expected<base::WeakPtr<SpdySession>,int>> result;

  RunSpdySessionPoolRust(26, [&](uint32_t operation, [[maybe_unused]] uint64_t a, [[maybe_unused]] uint64_t b, [[maybe_unused]] uint64_t c) -> int64_t {
    switch(operation) {
      case 0: { TRACE_EVENT0(NetTracingCategory(),"SpdySessionPool::CreateAvailableSessionFromSocket");
return 0; }
      case 1: { new_session=CreateSession(key,net_log.net_log(),MultiplexedSessionCreationInitiator::kUnknown,spdy_session_initiator,std::nullopt);
return 0; }
      case 2: { dns_aliases=socket_stream->GetDnsAliases();
return 0; }
      case 3: { new_session->InitializeWithSocket(std::move(socket_stream),connect_timing,this);
return 0; }
      case 4: { return base::FeatureList::IsEnabled(features::kSpdySessionForProxyAdditionalChecks); }
      case 5: { result=InsertSession(key,std::move(new_session),net_log,std::move(dns_aliases),a!=0);
return 0; }
    }
    NOTREACHED();
  });
return std::move(*result);
}

base::WeakPtr<SpdySession> SpdySessionPool::FindAvailableSession(
    const SpdySessionKey& key,
    bool enable_ip_based_pooling_for_h2,
    bool is_websocket,
    const NetLogWithSource& net_log) {
AvailableSessionMap::iterator it;
  SpdySessionRequestMap::iterator req_it;
  base::WeakPtr<SpdySession> result;

  RunSpdySessionPoolRust(0, [&](uint32_t operation, [[maybe_unused]] uint64_t a, [[maybe_unused]] uint64_t b, [[maybe_unused]] uint64_t c) -> int64_t {
    switch(operation) {
      case 0: { it = LookupAvailableSessionByKey(key); return it != available_sessions_.end(); }
      case 1: { return enable_ip_based_pooling_for_h2; }
      case 2: { return base::FeatureList::IsEnabled(features::kPostHandshakeIpPoolingAllEndpoints); }
      case 3: { req_it = spdy_session_request_map_.find(key); return req_it != spdy_session_request_map_.end(); }
      case 4: { return !req_it->second.ip_endpoints.empty(); }
      case 5: { return TryPoolToExistingIpSession(key, is_websocket, ConnectionEndpointMetadata(), req_it->second.ip_endpoints, req_it->second.dns_aliases, false, false); }
      case 6: { return is_websocket; }
      case 7: { return it->second->support_websocket(); }
      case 8: { return key == it->second->spdy_session_key(); }
      case 9: { UMA_HISTOGRAM_ENUMERATION("Net.SpdySessionGet", FOUND_EXISTING, SPDY_SESSION_GET_MAX);
       net_log.AddEventReferencingSource(NetLogEventType::HTTP2_SESSION_POOL_FOUND_EXISTING_SESSION, it->second->net_log().source());
return 0; }
      case 10: { UMA_HISTOGRAM_ENUMERATION("Net.SpdySessionGet", FOUND_EXISTING_FROM_IP_POOL, SPDY_SESSION_GET_MAX);
       net_log.AddEventReferencingSource(NetLogEventType::HTTP2_SESSION_POOL_FOUND_EXISTING_SESSION_FROM_IP_POOL, it->second->net_log().source());
return 0; }
      case 11: { result = it->second;
return 0; }
    }
    NOTREACHED();
  });
return result;
}

base::WeakPtr<SpdySession>
SpdySessionPool::FindMatchingIpSessionForServiceEndpoint(
    const SpdySessionKey& key,
    const ServiceEndpoint& service_endpoint,
    const base::flat_set<std::string>& dns_aliases) {
base::WeakPtr<SpdySession> result;
  RunSpdySessionPoolRust(22, [&](uint32_t operation, [[maybe_unused]] uint64_t a, [[maybe_unused]] uint64_t b, [[maybe_unused]] uint64_t c) -> int64_t {
    switch(operation) {
      case 0: { CHECK(!HasAvailableSession(key,true,false)); CHECK(key.socket_tag()==SocketTag());
return 0; }
      case 1: { result=FindMatchingIpSession(key,service_endpoint.ipv6_endpoints,dns_aliases); return !!result; }
      case 2: { result=FindMatchingIpSession(key,service_endpoint.ipv4_endpoints,dns_aliases);
return 0; }
    }
    NOTREACHED();
  });
return result;
}

bool SpdySessionPool::HasAvailableSession(const SpdySessionKey& key,
                                          bool enable_ip_based_pooling_for_h2,
                                          bool is_websocket) const {
AvailableSessionMap::const_iterator it;
  const int64_t rust_result = RunSpdySessionPoolRust(1, [&](uint32_t operation, [[maybe_unused]] uint64_t a, [[maybe_unused]] uint64_t b, [[maybe_unused]] uint64_t c) -> int64_t {
    switch(operation) {
      case 0: { it = available_sessions_.find(key); return it != available_sessions_.end(); }
      case 1: { return is_websocket; }
      case 2: { return it->second->support_websocket(); }
      case 3: { return enable_ip_based_pooling_for_h2; }
      case 4: { return key == it->second->spdy_session_key(); }
    }
    NOTREACHED();
  });
return rust_result != 0;
}

base::WeakPtr<SpdySession> SpdySessionPool::RequestSession(
    const SpdySessionKey& key,
    bool enable_ip_based_pooling_for_h2,
    bool is_websocket,
    const NetLogWithSource& net_log,
    base::RepeatingClosure on_blocking_request_destroyed_callback,
    SpdySessionRequest::Delegate* delegate,
    std::unique_ptr<SpdySessionRequest>* spdy_session_request,
    bool* is_blocking_request_for_session) {
base::WeakPtr<SpdySession> result;
  RequestInfoForKey* request_info = nullptr;

  RunSpdySessionPoolRust(2, [&](uint32_t operation, [[maybe_unused]] uint64_t a, [[maybe_unused]] uint64_t b, [[maybe_unused]] uint64_t c) -> int64_t {
    switch(operation) {
      case 0: { DCHECK(delegate);
return 0; }
      case 1: { result = FindAvailableSession(key, enable_ip_based_pooling_for_h2, is_websocket, net_log); return !!result; }
      case 2: { *is_blocking_request_for_session = true;
return 0; }
      case 3: { request_info = &spdy_session_request_map_[key]; return request_info->has_blocking_request; }
      case 4: { *is_blocking_request_for_session = a != 0;
return 0; }
      case 5: { *spdy_session_request = std::make_unique<SpdySessionRequest>(key, enable_ip_based_pooling_for_h2, is_websocket, *is_blocking_request_for_session, delegate, this);
       request_info->request_set.insert(spdy_session_request->get());
return 0; }
      case 6: { request_info->has_blocking_request = true;
return 0; }
      case 7: { return !!on_blocking_request_destroyed_callback; }
      case 8: { request_info->deferred_callbacks.push_back(on_blocking_request_destroyed_callback);
return 0; }
    }
    NOTREACHED();
  });
return result;
}

OnHostResolutionCallbackResult SpdySessionPool::OnHostResolutionComplete(
    const SpdySessionKey& key,
    bool is_websocket,
    base::span<const HostResolverEndpointResult> endpoint_results,
    const base::flat_set<std::string>& aliases) {
SpdySessionRequestMap::iterator req_it;
  AvailableSessionMap::iterator existing;

  const int64_t rust_result = RunSpdySessionPoolRust(29, [&](uint32_t operation, [[maybe_unused]] uint64_t a, [[maybe_unused]] uint64_t b, [[maybe_unused]] uint64_t c) -> int64_t {
    switch(operation) {
      case 0: { req_it=spdy_session_request_map_.find(key); return req_it!=spdy_session_request_map_.end(); }
      case 1: { existing=LookupAvailableSessionByKey(key); return existing!=available_sessions_.end(); }
      case 2: { return is_websocket; }
      case 3: { return existing->second->support_websocket(); }
      case 4: { return endpoint_results.size(); }
      case 5: { return TryPoolToExistingIpSession(key,is_websocket,endpoint_results[a].metadata,endpoint_results[a].ip_endpoints,aliases); }
      case 6: { return base::FeatureList::IsEnabled(features::kPostHandshakeIpPoolingAllEndpoints); }
      case 7: { req_it->second.ip_endpoints.clear();
return 0; }
      case 8: { return IsEndpointEligibleForH2(endpoint_results[a].metadata); }
      case 9: { AppendUniqueIpEndpoints(req_it->second.ip_endpoints,endpoint_results[a].ip_endpoints);
return 0; }
      case 10: { req_it->second.dns_aliases=aliases;
return 0; }
    }
    NOTREACHED();
  });
return rust_result == 0 ? OnHostResolutionCallbackResult::kContinue : OnHostResolutionCallbackResult::kMayBeDeletedAsync;
}

OnHostResolutionCallbackResult SpdySessionPool::OnHostResolutionComplete(
    const SpdySessionKey& key,
    bool is_websocket,
    base::span<const ServiceEndpoint> endpoint_results,
    const base::flat_set<std::string>& aliases) {
SpdySessionRequestMap::iterator req_it;
  AvailableSessionMap::iterator existing;

  const int64_t rust_result = RunSpdySessionPoolRust(30, [&](uint32_t operation, [[maybe_unused]] uint64_t a, [[maybe_unused]] uint64_t b, [[maybe_unused]] uint64_t c) -> int64_t {
    switch(operation) {
      case 0: { req_it=spdy_session_request_map_.find(key); return req_it!=spdy_session_request_map_.end(); }
      case 1: { existing=LookupAvailableSessionByKey(key); return existing!=available_sessions_.end(); }
      case 2: { return is_websocket; }
      case 3: { return existing->second->support_websocket(); }
      case 4: { return endpoint_results.size(); }
      case 5: { return TryPoolToExistingIpSession(key,is_websocket,endpoint_results[a].metadata,endpoint_results[a].ipv6_endpoints,aliases); }
      case 6: { return TryPoolToExistingIpSession(key,is_websocket,endpoint_results[a].metadata,endpoint_results[a].ipv4_endpoints,aliases); }
      case 7: { return base::FeatureList::IsEnabled(features::kPostHandshakeIpPoolingAllEndpoints); }
      case 8: { req_it->second.ip_endpoints.clear();
return 0; }
      case 9: { return IsEndpointEligibleForH2(endpoint_results[a].metadata); }
      case 10: { AppendUniqueIpEndpoints(req_it->second.ip_endpoints,endpoint_results[a].ipv6_endpoints);
return 0; }
      case 11: { AppendUniqueIpEndpoints(req_it->second.ip_endpoints,endpoint_results[a].ipv4_endpoints);
return 0; }
      case 12: { req_it->second.dns_aliases=aliases;
return 0; }
    }
    NOTREACHED();
  });
return rust_result == 0 ? OnHostResolutionCallbackResult::kContinue : OnHostResolutionCallbackResult::kMayBeDeletedAsync;
}

void SpdySessionPool::MakeSessionUnavailable(
    const base::WeakPtr<SpdySession>& available_session) {
std::set<SpdySessionKey>::const_iterator alias;
  RunSpdySessionPoolRust(3, [&](uint32_t operation, [[maybe_unused]] uint64_t a, [[maybe_unused]] uint64_t b, [[maybe_unused]] uint64_t c) -> int64_t {
    switch(operation) {
      case 0: { UnmapKey(available_session->spdy_session_key());
return 0; }
      case 1: { RemoveAliases(available_session->spdy_session_key());
return 0; }
      case 2: { alias = available_session->pooled_aliases().begin();
return 0; }
      case 3: { return alias != available_session->pooled_aliases().end(); }
      case 4: { UnmapKey(*alias);
return 0; }
      case 5: { RemoveAliases(*alias);
return 0; }
      case 6: { ++alias;
return 0; }
      case 7: { DCHECK(!IsSessionAvailable(available_session));
return 0; }
    }
    NOTREACHED();
  });
}

void SpdySessionPool::RemoveUnavailableSession(
    const base::WeakPtr<SpdySession>& unavailable_session) {
SessionSet::iterator it;
  std::unique_ptr<SpdySession> owned_session;

  RunSpdySessionPoolRust(4, [&](uint32_t operation, [[maybe_unused]] uint64_t a, [[maybe_unused]] uint64_t b, [[maybe_unused]] uint64_t c) -> int64_t {
    switch(operation) {
      case 0: { DCHECK(!IsSessionAvailable(unavailable_session));
return 0; }
      case 1: { unavailable_session->net_log().AddEvent(NetLogEventType::HTTP2_SESSION_POOL_REMOVE_SESSION);
return 0; }
      case 2: { it = sessions_.find(unavailable_session.get()); CHECK(it != sessions_.end());
return 0; }
      case 3: { owned_session.reset(*it); sessions_.erase(it);
return 0; }
      case 4: { NotifyOnSessionClosed(owned_session->spdy_session_key(), owned_session->WasEverUsedToCreateStreams());
return 0; }
    }
    NOTREACHED();
  });
}

// Make a copy of |sessions_| in the Close* functions below to avoid
// reentrancy problems. Since arbitrary functions get called by close
// handlers, it doesn't suffice to simply increment the iterator
// before closing.

void SpdySessionPool::CloseCurrentSessions(Error error) {
  CloseCurrentSessionsHelper(error, "Closing current sessions.",
                             false /* idle_only */);
}

void SpdySessionPool::CloseCurrentIdleSessions(const std::string& description) {
  CloseCurrentSessionsHelper(ERR_ABORTED, description, true /* idle_only */);
}

void SpdySessionPool::CloseAllSessions() {
SessionSet::const_iterator it;
  RunSpdySessionPoolRust(5, [&](uint32_t operation, [[maybe_unused]] uint64_t a, [[maybe_unused]] uint64_t b, [[maybe_unused]] uint64_t c) -> int64_t {
    switch(operation) {
      case 0: { it = sessions_.begin();
return 0; }
      case 1: { return it != sessions_.end(); }
      case 2: { return (*it)->IsDraining(); }
      case 3: { ++it;
return 0; }
      case 4: { CloseCurrentSessionsHelper(ERR_ABORTED, "Closing all sessions.", false);
return 0; }
    }
    NOTREACHED();
  });
}

void SpdySessionPool::MakeCurrentSessionsGoingAway(Error error) {
WeakSessionList current_sessions = GetCurrentSessions();
  RunSpdySessionPoolRust(6, [&](uint32_t operation, [[maybe_unused]] uint64_t a, [[maybe_unused]] uint64_t b, [[maybe_unused]] uint64_t c) -> int64_t {
    switch(operation) {
      case 0: { return current_sessions.size(); }
      case 1: { return !!base::span(current_sessions)[a]; }
      case 2: { base::span(current_sessions)[a]->MakeUnavailable(error);
return 0; }
      case 3: { base::span(current_sessions)[a]->StartGoingAway(kLastStreamId, error);
return 0; }
      case 4: { base::span(current_sessions)[a]->MaybeFinishGoingAway();
return 0; }
      case 5: { DCHECK(!IsSessionAvailable(base::span(current_sessions)[a]));
return 0; }
    }
    NOTREACHED();
  });
}

base::Value SpdySessionPool::SpdySessionPoolInfoToValue() const {
auto result=base::ListValue::with_capacity(sessions_.size());
  SessionSet::const_iterator it;

  RunSpdySessionPoolRust(24, [&](uint32_t operation, [[maybe_unused]] uint64_t a, [[maybe_unused]] uint64_t b, [[maybe_unused]] uint64_t c) -> int64_t {
    switch(operation) {
      case 0: { it=sessions_.begin();
return 0; }
      case 1: { return it!=sessions_.end(); }
      case 2: { result.Append((*it)->GetInfoAsValue());
return 0; }
      case 3: { ++it;
return 0; }
    }
    NOTREACHED();
  });
return base::Value(std::move(result));
}

void SpdySessionPool::OnIPAddressChanged(
    NetworkChangeNotifier::IPAddressChangeType change_type) {
RunSpdySessionPoolRust(7, [&](uint32_t operation, [[maybe_unused]] uint64_t a, [[maybe_unused]] uint64_t b, [[maybe_unused]] uint64_t c) -> int64_t {
    switch(operation) {
      case 0: { DCHECK(cleanup_sessions_on_ip_address_changed_);
return 0; }
      case 1: { return base::FeatureList::IsEnabled(net::features::kMaintainConnectionsOnIpv6TempAddrChange); }
      case 2: { return change_type == NetworkChangeNotifier::IP_ADDRESS_CHANGE_IPV6_TEMPADDR; }
      case 3: { return go_away_on_ip_change_; }
      case 4: { MakeCurrentSessionsGoingAway(ERR_NETWORK_CHANGED);
return 0; }
      case 5: { CloseCurrentSessions(ERR_NETWORK_CHANGED);
return 0; }
    }
    NOTREACHED();
  });
}

void SpdySessionPool::OnSSLConfigChanged(
    SSLClientContext::SSLConfigChangeType change_type) {
RunSpdySessionPoolRust(8, [&](uint32_t operation, [[maybe_unused]] uint64_t a, [[maybe_unused]] uint64_t b, [[maybe_unused]] uint64_t c) -> int64_t {
    switch(operation) {
      case 0: { return static_cast<int64_t>(change_type); }
      case 1: { return static_cast<int64_t>(SSLClientContext::SSLConfigChangeType::kSSLConfigChanged); }
      case 2: { return static_cast<int64_t>(SSLClientContext::SSLConfigChangeType::kCertDatabaseChanged); }
      case 3: { return static_cast<int64_t>(SSLClientContext::SSLConfigChangeType::kCertVerifierChanged); }
      case 4: { MakeCurrentSessionsGoingAway(ERR_NETWORK_CHANGED);
return 0; }
      case 5: { MakeCurrentSessionsGoingAway(ERR_CERT_DATABASE_CHANGED);
return 0; }
      case 6: { MakeCurrentSessionsGoingAway(ERR_CERT_VERIFIER_CHANGED);
return 0; }
    }
    NOTREACHED();
  });
}

void SpdySessionPool::OnSSLConfigForServersChanged(
    const base::flat_set<HostPortPair>& servers) {
WeakSessionList current_sessions = GetCurrentSessions();
  base::span<const ProxyServer> proxies;

  RunSpdySessionPoolRust(9, [&](uint32_t operation, [[maybe_unused]] uint64_t a, [[maybe_unused]] uint64_t b, [[maybe_unused]] uint64_t c) -> int64_t {
    switch(operation) {
      case 0: { return current_sessions.size(); }
      case 1: { return !!base::span(current_sessions)[a]; }
      case 2: { return servers.contains(base::span(current_sessions)[a]->host_port_pair()); }
      case 3: { proxies = base::span(current_sessions)[a]->spdy_session_key().proxy_chain().proxy_servers(); return proxies.size(); }
      case 4: { return proxies[b].is_http_like(); }
      case 5: { return proxies[b].is_http(); }
      case 6: { return servers.contains(proxies[b].host_port_pair()); }
      case 7: { base::span(current_sessions)[a]->MakeUnavailable(ERR_NETWORK_CHANGED);
return 0; }
      case 8: { base::span(current_sessions)[a]->StartGoingAway(kLastStreamId, ERR_NETWORK_CHANGED);
return 0; }
      case 9: { base::span(current_sessions)[a]->MaybeFinishGoingAway();
return 0; }
      case 10: { DCHECK(!IsSessionAvailable(base::span(current_sessions)[a]));
return 0; }
    }
    NOTREACHED();
  });
}

const base::flat_set<std::string>& SpdySessionPool::GetDnsAliasesForSessionKey(
    const SpdySessionKey& key) const {
DnsAliasesBySessionKeyMap::const_iterator it;
  const base::flat_set<std::string>* result = nullptr;

  RunSpdySessionPoolRust(23, [&](uint32_t operation, [[maybe_unused]] uint64_t a, [[maybe_unused]] uint64_t b, [[maybe_unused]] uint64_t c) -> int64_t {
    switch(operation) {
      case 0: { it=dns_aliases_by_session_key_.find(key); return it!=dns_aliases_by_session_key_.end(); }
      case 1: { result=&dns_alias_utility::EmptyDnsAliases();
return 0; }
      case 2: { result=&it->second;
return 0; }
    }
    NOTREACHED();
  });
return *result;
}

void SpdySessionPool::RemoveRequestForSpdySession(SpdySessionRequest* request) {
SpdySessionRequestMap::iterator iter;
  RunSpdySessionPoolRust(10, [&](uint32_t operation, [[maybe_unused]] uint64_t a, [[maybe_unused]] uint64_t b, [[maybe_unused]] uint64_t c) -> int64_t {
    switch(operation) {
      case 0: { DCHECK_EQ(this, request->spdy_session_pool());
return 0; }
      case 1: { iter = spdy_session_request_map_.find(request->key()); CHECK(iter != spdy_session_request_map_.end());
return 0; }
      case 2: { return request->is_blocking_request_for_session(); }
      case 3: { return !iter->second.deferred_callbacks.empty(); }
      case 4: { base::SingleThreadTaskRunner::GetCurrentDefault()->PostTask(FROM_HERE,
       base::BindOnce(&SpdySessionPool::UpdatePendingRequests, weak_ptr_factory_.GetWeakPtr(), request->key()));
return 0; }
      case 5: { DCHECK(iter->second.request_set.contains(request));
return 0; }
      case 6: { RemoveRequestInternal(iter, iter->second.request_set.find(request));
return 0; }
    }
    NOTREACHED();
  });
}

SpdySessionPool::RequestInfoForKey::RequestInfoForKey() = default;
SpdySessionPool::RequestInfoForKey::~RequestInfoForKey() = default;

bool SpdySessionPool::IsSessionAvailable(
    const base::WeakPtr<SpdySession>& session) const {
AvailableSessionMap::const_iterator it;
  const int64_t rust_result = RunSpdySessionPoolRust(11, [&](uint32_t operation, [[maybe_unused]] uint64_t a, [[maybe_unused]] uint64_t b, [[maybe_unused]] uint64_t c) -> int64_t {
    switch(operation) {
      case 0: { it = available_sessions_.begin();
return 0; }
      case 1: { return it != available_sessions_.end(); }
      case 2: { return it->second.get() == session.get(); }
      case 3: { ++it;
return 0; }
    }
    NOTREACHED();
  });
return rust_result != 0;
}

void SpdySessionPool::MapKeyToAvailableSession(
    const SpdySessionKey& key,
    const base::WeakPtr<SpdySession>& session,
    base::flat_set<std::string> dns_aliases) {
RunSpdySessionPoolRust(12, [&](uint32_t operation, [[maybe_unused]] uint64_t a, [[maybe_unused]] uint64_t b, [[maybe_unused]] uint64_t c) -> int64_t {
    switch(operation) {
      case 0: { DCHECK(sessions_.contains(session.get()));
return 0; }
      case 1: { auto result = available_sessions_.emplace(key, session); CHECK(result.second);
return 0; }
      case 2: { dns_aliases_by_session_key_[key] = std::move(dns_aliases);
return 0; }
    }
    NOTREACHED();
  });
}

SpdySessionPool::AvailableSessionMap::iterator
SpdySessionPool::LookupAvailableSessionByKey(
    const SpdySessionKey& key) {
  return available_sessions_.find(key);
}

void SpdySessionPool::UnmapKey(const SpdySessionKey& key) {
AvailableSessionMap::iterator it;
  RunSpdySessionPoolRust(13, [&](uint32_t operation, [[maybe_unused]] uint64_t a, [[maybe_unused]] uint64_t b, [[maybe_unused]] uint64_t c) -> int64_t {
    switch(operation) {
      case 0: { it = LookupAvailableSessionByKey(key); CHECK(it != available_sessions_.end());
return 0; }
      case 1: { available_sessions_.erase(it);
return 0; }
      case 2: { dns_aliases_by_session_key_.erase(key);
return 0; }
    }
    NOTREACHED();
  });
}

void SpdySessionPool::RemoveAliases(const SpdySessionKey& key) {
AliasMap::iterator it;
  RunSpdySessionPoolRust(14, [&](uint32_t operation, [[maybe_unused]] uint64_t a, [[maybe_unused]] uint64_t b, [[maybe_unused]] uint64_t c) -> int64_t {
    switch(operation) {
      case 0: { it = aliases_.begin();
return 0; }
      case 1: { return it != aliases_.end(); }
      case 2: { return it->second == key; }
      case 3: { auto old_it = it; ++it; aliases_.erase(old_it);
return 0; }
      case 4: { ++it;
return 0; }
    }
    NOTREACHED();
  });
}

SpdySessionPool::WeakSessionList SpdySessionPool::GetCurrentSessions() const {
WeakSessionList result;
  SessionSet::const_iterator it;

  RunSpdySessionPoolRust(15, [&](uint32_t operation, [[maybe_unused]] uint64_t a, [[maybe_unused]] uint64_t b, [[maybe_unused]] uint64_t c) -> int64_t {
    switch(operation) {
      case 0: { it = sessions_.begin();
return 0; }
      case 1: { return it != sessions_.end(); }
      case 2: { result.push_back((*it)->GetWeakPtr());
return 0; }
      case 3: { ++it;
return 0; }
    }
    NOTREACHED();
  });
return result;
}

void SpdySessionPool::CloseCurrentSessionsHelper(Error error,
                                                 const std::string& description,
                                                 bool idle_only) {
WeakSessionList current_sessions = GetCurrentSessions();
  RunSpdySessionPoolRust(16, [&](uint32_t operation, [[maybe_unused]] uint64_t a, [[maybe_unused]] uint64_t b, [[maybe_unused]] uint64_t c) -> int64_t {
    switch(operation) {
      case 0: { return current_sessions.size(); }
      case 1: { return !!base::span(current_sessions)[a]; }
      case 2: { return idle_only; }
      case 3: { return base::span(current_sessions)[a]->is_active(); }
      case 4: { return base::span(current_sessions)[a]->IsDraining(); }
      case 5: { base::span(current_sessions)[a]->CloseSessionOnError(error, description);
return 0; }
      case 6: { NotifyOnSessionClosed(base::span(current_sessions)[a]->spdy_session_key(), base::span(current_sessions)[a]->WasEverUsedToCreateStreams());
return 0; }
      case 7: { DCHECK(!IsSessionAvailable(base::span(current_sessions)[a]));
return 0; }
      case 8: { DCHECK(!base::span(current_sessions)[a] || base::span(current_sessions)[a]->IsDraining());
return 0; }
    }
    NOTREACHED();
  });
}

std::unique_ptr<SpdySession> SpdySessionPool::CreateSession(
    const SpdySessionKey& key,
    NetLog* net_log,
    const MultiplexedSessionCreationInitiator session_creation_initiator,
    SpdySessionInitiator spdy_session_initiator,
    std::optional<ConnectionManagementConfig> connection_management_config) {
AvailableSessionMap::iterator it;
  SSLConfigService* ssl_config=nullptr;
  std::unique_ptr<SpdySession> result;

  RunSpdySessionPoolRust(27, [&](uint32_t operation, [[maybe_unused]] uint64_t a, [[maybe_unused]] uint64_t b, [[maybe_unused]] uint64_t c) -> int64_t {
    switch(operation) {
      case 0: { UMA_HISTOGRAM_ENUMERATION("Net.SpdySessionGet",IMPORTED_FROM_SOCKET,SPDY_SESSION_GET_MAX);
return 0; }
      case 1: { it=LookupAvailableSessionByKey(key); return it!=available_sessions_.end(); }
      case 2: { CHECK(key!=it->second->spdy_session_key());
return 0; }
      case 3: { it->second->RemovePooledAlias(key);
return 0; }
      case 4: { UnmapKey(key);
return 0; }
      case 5: { RemoveAliases(key);
return 0; }
      case 6: { return connection_management_config.has_value(); }
      case 7: { AddConnectionManagementConfig(key,connection_management_config.value());
return 0; }
      case 8: { return !!ssl_client_context_; }
      case 9: { ssl_config=ssl_client_context_->ssl_config_service();
return 0; }
      case 10: { result=std::make_unique<SpdySession>(key,http_server_properties_,transport_security_state_,ssl_config,
       quic_supported_versions_,enable_sending_initial_data_,enable_ping_based_connection_checking_,
       is_http2_enabled_,is_quic_enabled_,session_max_recv_window_size_,session_max_queued_capped_frames_,
       initial_settings_,enable_http2_settings_grease_,greased_http2_frame_,http2_end_stream_with_data_frame_,
       enable_priority_update_,time_func_,network_quality_estimator_,net_log,session_creation_initiator,spdy_session_initiator);
return 0; }
    }
    NOTREACHED();
  });
return result;
}

base::expected<base::WeakPtr<SpdySession>, int> SpdySessionPool::InsertSession(
    const SpdySessionKey& key,
    std::unique_ptr<SpdySession> new_session,
    const NetLogWithSource& source_net_log,
    base::flat_set<std::string> dns_aliases,
    bool perform_post_insertion_checks) {
base::WeakPtr<SpdySession> available_session;
  IPEndPoint address;
  std::optional<base::expected<base::WeakPtr<SpdySession>,int>> result;

  RunSpdySessionPoolRust(28, [&](uint32_t operation, [[maybe_unused]] uint64_t a, [[maybe_unused]] uint64_t b, [[maybe_unused]] uint64_t c) -> int64_t {
    switch(operation) {
      case 0: { available_session=new_session->GetWeakPtr();
return 0; }
      case 1: { sessions_.insert(new_session.release());
return 0; }
      case 2: { MapKeyToAvailableSession(key,available_session,std::move(dns_aliases));
return 0; }
      case 3: { base::SingleThreadTaskRunner::GetCurrentDefault()->PostTask(FROM_HERE,
       base::BindOnce(&SpdySessionPool::UpdatePendingRequests,weak_ptr_factory_.GetWeakPtr(),key));
return 0; }
      case 4: { source_net_log.AddEventReferencingSource(NetLogEventType::HTTP2_SESSION_POOL_IMPORTED_SESSION_FROM_SOCKET,available_session->net_log().source());
return 0; }
      case 5: { return key.proxy_chain().is_direct(); }
      case 6: { return available_session->GetPeerAddress(&address); }
      case 7: { aliases_.insert(AliasMap::value_type(address,key));
return 0; }
      case 8: { return perform_post_insertion_checks; }
      case 9: { result=available_session;
return 0; }
      case 10: { return available_session->HasAcceptableTransportSecurity(); }
      case 11: { available_session->CloseSessionOnError(ERR_HTTP2_INADEQUATE_TRANSPORT_SECURITY,"");
return 0; }
      case 12: { NotifyOnConnectionFailure(key);
return 0; }
      case 13: { result=base::unexpected(ERR_HTTP2_INADEQUATE_TRANSPORT_SECURITY);
return 0; }
      case 14: { return available_session->ParseAlps(); }
      case 15: { DCHECK_NE(ERR_IO_PENDING,static_cast<int>(a));
return 0; }
      case 16: { result=base::unexpected(static_cast<int>(a));
return 0; }
    }
    NOTREACHED();
  });
return std::move(*result);
}

void SpdySessionPool::UpdatePendingRequests(const SpdySessionKey& key) {
AvailableSessionMap::iterator it;
  base::WeakPtr<SpdySession> new_session;
  SpdySessionRequestMap::iterator iter;
  RequestSet::iterator request;
  SpdySessionRequest::Delegate* delegate = nullptr;
  std::list<base::RepeatingClosure> deferred_requests;
  std::list<base::RepeatingClosure>::iterator deferred;

  RunSpdySessionPoolRust(17, [&](uint32_t operation, [[maybe_unused]] uint64_t a, [[maybe_unused]] uint64_t b, [[maybe_unused]] uint64_t c) -> int64_t {
    switch(operation) {
      case 0: { it=LookupAvailableSessionByKey(key); return it!=available_sessions_.end(); }
      case 1: { new_session=it->second->GetWeakPtr(); return key!=new_session->spdy_session_key(); }
      case 2: { return !!new_session; }
      case 3: { return new_session->IsAvailable(); }
      case 4: { iter=spdy_session_request_map_.find(key); return iter!=spdy_session_request_map_.end(); }
      case 5: { request=iter->second.request_set.begin();
return 0; }
      case 6: { return request!=iter->second.request_set.end(); }
      case 7: { return (*request)->is_websocket(); }
      case 8: { return new_session->support_websocket(); }
      case 9: { return (*request)->enable_ip_based_pooling_for_h2(); }
      case 10: { ++request;
return 0; }
      case 11: { delegate=(*request)->delegate();
return 0; }
      case 12: { RemoveRequestInternal(iter,request);
return 0; }
      case 13: { delegate->OnSpdySessionAvailable(new_session);
return 0; }
      case 14: { return iter->second.has_blocking_request; }
      case 15: { deferred_requests=std::move(iter->second.deferred_callbacks);
return 0; }
      case 16: { return iter->second.request_set.empty(); }
      case 17: { spdy_session_request_map_.erase(iter);
return 0; }
      case 18: { deferred=deferred_requests.begin();
return 0; }
      case 19: { return deferred!=deferred_requests.end(); }
      case 20: { auto callback=*deferred; callback.Run();
return 0; }
      case 21: { ++deferred;
return 0; }
      case 22: { new_session.reset();
return 0; }
    }
    NOTREACHED();
  });
}

void SpdySessionPool::RemoveRequestInternal(
    SpdySessionRequestMap::iterator request_map_iterator,
    RequestSet::iterator request_set_iterator) {
SpdySessionRequest* request = *request_set_iterator;
  RunSpdySessionPoolRust(18, [&](uint32_t operation, [[maybe_unused]] uint64_t a, [[maybe_unused]] uint64_t b, [[maybe_unused]] uint64_t c) -> int64_t {
    switch(operation) {
      case 0: { request_map_iterator->second.request_set.erase(request_set_iterator);
return 0; }
      case 1: { return request->is_blocking_request_for_session(); }
      case 2: { DCHECK(request_map_iterator->second.has_blocking_request); request_map_iterator->second.has_blocking_request=false;
return 0; }
      case 3: { return request_map_iterator->second.request_set.empty(); }
      case 4: { return request_map_iterator->second.deferred_callbacks.empty(); }
      case 5: { spdy_session_request_map_.erase(request_map_iterator);
return 0; }
      case 6: { request->OnRemovedFromPool();
return 0; }
    }
    NOTREACHED();
  });
}

base::WeakPtr<SpdySession> SpdySessionPool::FindMatchingIpSession(
    const SpdySessionKey& key,
    const std::vector<IPEndPoint>& ip_endpoints,
    const base::flat_set<std::string>& dns_aliases) {
std::pair<AliasMap::iterator,AliasMap::iterator> range;
  AliasMap::iterator alias;
  AvailableSessionMap::iterator available;
  SpdySessionKey::CompareForAliasingResult comparison;
  base::WeakPtr<SpdySession> result;

  RunSpdySessionPoolRust(31, [&](uint32_t operation, [[maybe_unused]] uint64_t a, [[maybe_unused]] uint64_t b, [[maybe_unused]] uint64_t c) -> int64_t {
    switch(operation) {
      case 0: { return ip_endpoints.size(); }
      case 1: { range=aliases_.equal_range(base::span(ip_endpoints)[a]); alias=range.first;
return 0; }
      case 2: { return alias!=range.second; }
      case 3: { ++alias;
return 0; }
      case 4: { return alias->second.socket_tag()==key.socket_tag(); }
      case 5: { available=LookupAvailableSessionByKey(alias->second); CHECK(available!=available_sessions_.end());
return 0; }
      case 6: { comparison=alias->second.CompareForAliasing(key); return comparison.is_potentially_aliasable; }
      case 7: { result=available->second; return result->VerifyDomainAuthentication(key.host_port_pair().host()); }
      case 8: { MapKeyToAvailableSession(key,result,dns_aliases);
return 0; }
      case 9: { result->AddPooledAlias(key);
return 0; }
      case 10: { result.reset();
return 0; }
    }
    NOTREACHED();
  });
return result;
}

void SpdySessionPool::AddConnectionManagementConfig(
    const SpdySessionKey& key,
    ConnectionManagementConfig& connection_management_config) {
RunSpdySessionPoolRust(19, [&](uint32_t operation, [[maybe_unused]] uint64_t a, [[maybe_unused]] uint64_t b, [[maybe_unused]] uint64_t c) -> int64_t {
    switch(operation) {
      case 0: { return base::FeatureList::IsEnabled(net::features::kConnectionKeepAliveForHttp2); }
      case 1: { return !!connection_management_config.connection_change_observer; }
      case 2: { return connection_change_notifier_map_.contains(key); }
      case 3: { connection_change_notifier_map_[key]=std::make_unique<ConnectionChangeNotifier>();
return 0; }
      case 4: { connection_change_notifier_map_[key]->AddObserver(connection_management_config.connection_change_observer.get());
return 0; }
    }
    NOTREACHED();
  });
}

void SpdySessionPool::NotifyOnNetworkEvent(net::NetworkChangeEvent event) {
  // TODO(crbug.com/453308537): We currently do not support
  // `NotifyOnNetworkEvent` since `SpdySessionPool` does not observe
  // `NetworkObserver`. We should add support, but for the time being, we can
  // get similar behavior via the `IPAddressObserver`, which would close the
  // connection on network changes.
  NOTIMPLEMENTED() << "SpdySessionPool does not support NotifyOnNetworkEvent";
}

void SpdySessionPool::NotifyOnSessionClosed(
    const SpdySessionKey& session_key,
    bool was_ever_used_to_create_streams) {
decltype(connection_change_notifier_map_)::iterator notifier;
  RunSpdySessionPoolRust(20, [&](uint32_t operation, [[maybe_unused]] uint64_t a, [[maybe_unused]] uint64_t b, [[maybe_unused]] uint64_t c) -> int64_t {
    switch(operation) {
      case 0: { notifier=connection_change_notifier_map_.find(session_key); return notifier!=connection_change_notifier_map_.end(); }
      case 1: { notifier->second->OnSessionClosed(was_ever_used_to_create_streams);
return 0; }
    }
    NOTREACHED();
  });
}

void SpdySessionPool::NotifyOnConnectionFailure(
    const SpdySessionKey& session_key) {
decltype(connection_change_notifier_map_)::iterator notifier;
  RunSpdySessionPoolRust(21, [&](uint32_t operation, [[maybe_unused]] uint64_t a, [[maybe_unused]] uint64_t b, [[maybe_unused]] uint64_t c) -> int64_t {
    switch(operation) {
      case 0: { notifier=connection_change_notifier_map_.find(session_key); return notifier!=connection_change_notifier_map_.end(); }
      case 1: { notifier->second->OnConnectionFailed();
return 0; }
    }
    NOTREACHED();
  });
}

bool SpdySessionPool::TryPoolToExistingIpSession(
    const SpdySessionKey& key,
    bool is_websocket,
    const ConnectionEndpointMetadata& metadata,
    base::span<const IPEndPoint> ip_endpoints,
    const base::flat_set<std::string>& dns_aliases,
    bool log_domain_match_failure,
    bool allow_socket_tag_change) {
std::pair<AliasMap::iterator,AliasMap::iterator> range;
  AliasMap::iterator alias_it;
  AvailableSessionMap::iterator available;
  SpdySessionKey::CompareForAliasingResult comparison;
  base::WeakPtr<SpdySession> session;
  std::optional<SpdySessionKey> old_key,new_key,new_pool_alias_key;
  base::flat_set<std::string> main_dns, pooled_dns;
  std::set<SpdySessionKey>::const_iterator pooled_it;

  const int64_t rust_result = RunSpdySessionPoolRust(32, [&](uint32_t operation, [[maybe_unused]] uint64_t a, [[maybe_unused]] uint64_t b, [[maybe_unused]] uint64_t c) -> int64_t {
    switch(operation) {
      case 0: { return IsEndpointEligibleForH2(metadata); }
      case 1: { return ip_endpoints.size(); }
      case 2: { range=aliases_.equal_range(ip_endpoints[a]); alias_it=range.first;
return 0; }
      case 3: { return alias_it!=range.second; }
      case 4: { ++alias_it;
return 0; }
      case 5: { available=LookupAvailableSessionByKey(alias_it->second); CHECK(available!=available_sessions_.end()); comparison=alias_it->second.CompareForAliasing(key);
return 0; }
      case 6: { return comparison.is_potentially_aliasable; }
      case 7: { return comparison.is_socket_tag_match; }
      case 8: { return allow_socket_tag_change; }
      case 9: { return is_websocket; }
      case 10: { return available->second->support_websocket(); }
      case 11: { session=available->second; return session->VerifyDomainAuthentication(key.host_port_pair().host()); }
      case 12: { return log_domain_match_failure; }
      case 13: { UMA_HISTOGRAM_ENUMERATION("Net.SpdyIPPoolDomainMatch",0,2);
return 0; }
      case 14: { UMA_HISTOGRAM_ENUMERATION("Net.SpdyIPPoolDomainMatch",1,2);
return 0; }
      case 15: { old_key.emplace(session->spdy_session_key());
            new_key.emplace(old_key->host_port_pair(),old_key->privacy_mode(),old_key->proxy_chain(),old_key->session_usage(),key.socket_tag(),old_key->network_anonymization_key(),old_key->secure_dns_policy(),old_key->disable_cert_verification_network_fetches(),old_key->target_network());
return 0; }
      case 16: { return available_sessions_.find(*new_key)!=available_sessions_.end(); }
      case 17: { return session->ChangeSocketTag(key.socket_tag()); }
      case 18: { DCHECK(session->spdy_session_key()==*new_key); return *new_key==key; }
      case 19: { main_dns=GetDnsAliasesForSessionKey(*old_key);
return 0; }
      case 20: { UnmapKey(*old_key);
return 0; }
      case 21: { MapKeyToAvailableSession(*new_key,session,std::move(main_dns));
return 0; }
      case 22: { aliases_.insert(AliasMap::value_type(alias_it->first,*new_key)); aliases_.erase(alias_it);
return 0; }
      case 23: { pooled_it=session->pooled_aliases().begin();
return 0; }
      case 24: { return pooled_it!=session->pooled_aliases().end(); }
      case 25: { return pooled_it->socket_tag()==key.socket_tag(); }
      case 26: { ++pooled_it;
return 0; }
      case 27: { pooled_dns=GetDnsAliasesForSessionKey(*pooled_it);
return 0; }
      case 28: { UnmapKey(*pooled_it);
return 0; }
      case 29: { new_pool_alias_key.emplace(pooled_it->host_port_pair(),pooled_it->privacy_mode(),pooled_it->proxy_chain(),pooled_it->session_usage(),key.socket_tag(),pooled_it->network_anonymization_key(),pooled_it->secure_dns_policy(),pooled_it->disable_cert_verification_network_fetches(),pooled_it->target_network());
            auto old=pooled_it; ++pooled_it; session->RemovePooledAlias(*old);
return 0; }
      case 30: { return available_sessions_.find(*new_pool_alias_key)==available_sessions_.end(); }
      case 31: { MapKeyToAvailableSession(*new_pool_alias_key,session,std::move(pooled_dns));
return 0; }
      case 32: { session->AddPooledAlias(*new_pool_alias_key);
return 0; }
      case 33: { return *new_pool_alias_key==key; }
      case 34: { MapKeyToAvailableSession(key,session,dns_aliases);
return 0; }
      case 35: { session->AddPooledAlias(key);
return 0; }
      case 36: { base::SingleThreadTaskRunner::GetCurrentDefault()->PostTask(FROM_HERE,
       base::BindOnce(&SpdySessionPool::UpdatePendingRequests,weak_ptr_factory_.GetWeakPtr(),key));
return 0; }
      case 37: { new_pool_alias_key.reset(); pooled_dns=base::flat_set<std::string>();
return 0; }
      case 38: { new_key.reset(); old_key.reset(); main_dns=base::flat_set<std::string>();
return 0; }
    }
    NOTREACHED();
  });
return rust_result != 0;
}

}  // namespace net
