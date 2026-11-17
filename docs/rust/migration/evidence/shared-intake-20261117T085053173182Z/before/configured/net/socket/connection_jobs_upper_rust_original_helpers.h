// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#ifndef NET_SOCKET_CONNECTION_JOBS_UPPER_RUST_ORIGINAL_HELPERS_H_
#define NET_SOCKET_CONNECTION_JOBS_UPPER_RUST_ORIGINAL_HELPERS_H_
#include <iterator>
#include <string>
#include <string_view>
#include <utility>
#include <vector>
#include "base/check_op.h"
#include "base/logging.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/stringprintf.h"
#include "build/buildflag.h"
#include "net/base/host_port_pair.h"
#include "net/base/proxy_chain.h"
#include "net/base/proxy_server.h"
#include "net/log/net_log_with_source.h"
#include "net/socket/connection_jobs_rust_original_helpers.h"
#include "net/socket/connect_job.h"
#include "net/socket/connect_job_rust_original.h"
#include "net/net_buildflags.h"
#include "url/scheme_host_port.h"

namespace net {
static_assert(static_cast<int>(OnHostResolutionCallbackResult::kContinue)==
              static_cast<int>(ConnectJobRustOriginalHostResolutionCallbackResult::kContinue));
static_assert(static_cast<int>(OnHostResolutionCallbackResult::kMayBeDeletedAsync)==
              static_cast<int>(ConnectJobRustOriginalHostResolutionCallbackResult::kMayBeDeletedAsync));
// Parent adds only this test/reference storage friend to native ProxyServer and
// ProxyChain. Public native values cross external-provider boundaries unchanged;
// no reinterpret_cast, frozen-type conversion or hidden Rust validation call.
struct ConnectionJobsUpperRustOriginalStorage {
  static CommonConnectJobParamsRustOriginal CommonParams(
      const CommonConnectJobParams& source) {
    return CommonConnectJobParamsRustOriginal(
      source.client_socket_factory,source.host_resolver,source.http_auth_cache,
      source.http_auth_handler_factory,source.spdy_session_pool,source.quic_supported_versions,
      source.quic_session_pool,source.proxy_delegate,source.http_user_agent_settings,
      source.ssl_client_context,source.socket_performance_watcher_factory,
      source.network_quality_estimator,source.net_log,source.websocket_endpoint_lock_manager,
      source.http_server_properties,source.alpn_protos,source.application_settings,
      source.ignore_certificate_errors,source.enable_early_data);
  }
  static bool Https(const ProxyServer& server) {
    return connection_jobs_rust_original_helpers::IsHttps(server);
  }
  static bool Quic(const ProxyServer& server) {
    return connection_jobs_rust_original_helpers::IsQuic(server);
  }
  static const HostPortPair& Host(const ProxyServer& server) {
    return connection_jobs_rust_original_helpers::HostPort(server);
  }
  static bool HostEqual(const HostPortPair& a, const HostPortPair& b) {
    return a.port() == b.port() && a.host() == b.host();
  }
  // Frozen original HPP policy, over public native storage views. front() uses
  // the same proven nonempty precondition as original string[0].
  static std::string HostForURL(const HostPortPair& pair) {
    const std::string& host = pair.host();
    if (host.find('\0') != std::string::npos) {
      std::string host_for_log(host);
      size_t nullpos;
      while ((nullpos = host_for_log.find('\0')) != std::string::npos) {
        host_for_log.replace(nullpos, 1, "%00");
      }
      LOG(DFATAL) << "Host has a null char: " << host_for_log;
    }
    if (host.find(':') != std::string::npos) {
      DCHECK_NE(host.front(), '[');
      return base::StringPrintf("[%s]", host.c_str());
    }
    return host;
  }
  static std::string HostString(const HostPortPair& pair) {
    std::string ret(HostForURL(pair));
    ret += ':';
    ret += base::NumberToString(pair.port());
    return ret;
  }
  static HostPortPair FromSchemeHostPort(const url::SchemeHostPort& input) {
    DCHECK(input.IsValid());
    std::string_view host = input.host();
    if (host.size() >= 2 && host.front() == '[' && host.back() == ']') {
      host = host.substr(1, host.size() - 2);
    }
    // Existing native HPP ctor only constructs port/string storage.
    return HostPortPair(host, input.port());
  }
  static bool Valid(const ProxyChain& chain) {
    if (!chain.proxy_server_list_.has_value()) { return false; }
    if (chain.proxy_server_list_->empty()) { return true; }
    bool allow_quic = false;
#if BUILDFLAG(ENABLE_QUIC_PROXY_SUPPORT)
    allow_quic = true;
#endif
    if (chain.proxy_server_list_->size() == 1) {
      const auto& server = chain.proxy_server_list_->at(0);
      bool valid = server.scheme() != ProxyServer::SCHEME_INVALID;
      if (Quic(server)) { valid = valid && allow_quic; }
      return valid;
    }
    DCHECK_GT(chain.proxy_server_list_->size(), 1u);
#if BUILDFLAG(ENABLE_BRACKETED_PROXY_URIS)
    bool seen_quic = false;
    bool seen_https = false;
    for (const auto& server : *chain.proxy_server_list_) {
      if (Quic(server)) {
        if (seen_https) { return false; }
        seen_quic = true;
      } else if (Https(server)) {
        seen_https = true;
      } else {
        return false;
      }
    }
    return !seen_quic || allow_quic;
#else
    return false;
#endif
  }
  static ProxyChain Prefix(const ProxyChain& chain, size_t length) {
    DCHECK(chain.IsValid());
    DCHECK_LE(length, chain.length());
    ProxyChain result;
    const auto& servers = chain.proxy_server_list_.value();
    result.proxy_server_list_.emplace(servers.begin(),
                                      std::next(servers.begin(), length));
    result.opaque_data_ = chain.opaque_data_;
    // Original private constructor validation/invalid-state reset, independent
    // of the migrated native ProxyChain constructor policy.
    if (!Valid(result)) { result = ProxyChain(); }
    CHECK(result.IsValid());
    return result;
  }
  static void BeginReferencing(const NetLogWithSource& log, NetLogEventType type,
                                const NetLogSource& source) {
    log.BeginEvent(type, [&] {
      base::DictValue result;
      if (source.id == 0) { return result; }
      base::DictValue dependency;
      dependency.Set("type", static_cast<int>(source.type));
      dependency.Set("id", static_cast<int>(source.id));
      result.Set("source_dependency", std::move(dependency));
      return result;
    });
  }
};
}  // namespace net
#endif  // NET_SOCKET_CONNECTION_JOBS_UPPER_RUST_ORIGINAL_HELPERS_H_
