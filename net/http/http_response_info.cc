// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include "net/http/http_response_info.h"

#include <utility>
#include <vector>

#include "base/check.h"
#include "base/logging.h"
#include "base/memory/raw_ptr_exclusion.h"
#include "base/memory/stack_allocated.h"
#include "base/numerics/safe_conversions.h"
#include "base/pickle.h"
#include "net/cert/signed_certificate_timestamp.h"
#include "net/cert/x509_certificate.h"
#include "net/http/http_response_headers.h"
#include "net/http/http_response_info_rust_ffi.h"
#include "net/ssl/ssl_cert_request_info.h"
#include "net/ssl/ssl_connection_status_flags.h"
#include "third_party/boringssl/src/include/openssl/ssl.h"
namespace net {
static_assert(static_cast<int>(HttpConnectionInfo::kMaxValue) == 42);
static_assert(
    static_cast<int>(
        AlternateProtocolUsage::ALTERNATE_PROTOCOL_USAGE_UNSPECIFIED_REASON) ==
    7);
static_assert(SSL_CONNECTION_VERSION_SHIFT == 20 &&
              SSL_CONNECTION_VERSION_MASK == 7);
static_assert(SSL_CONNECTION_VERSION_TLS1_3 == 6 &&
              SSL_CONNECTION_VERSION_SSL3 == 2);
namespace {
using namespace response_info_rust;
struct Services {
  STACK_ALLOCATED();

 public:
  // Synchronous stack-only services; pointers never escape the Rust call.
  RAW_PTR_EXCLUSION HttpResponseInfo* output = nullptr;
  RAW_PTR_EXCLUSION const HttpResponseInfo* input = nullptr;
  RAW_PTR_EXCLUSION base::PickleIterator* iter = nullptr;
  RAW_PTR_EXCLUSION base::Pickle* pickle = nullptr;
  RAW_PTR_EXCLUSION bool* truncated = nullptr;
  std::string host;
  std::vector<std::string> aliases;
};
ResponseInfoRustReply Invoke(void* opaque,
                             uint32_t op,
                             uint32_t field,
                             int64_t value) {
  auto& s = *static_cast<Services*>(opaque);
  auto* out = s.output;
  const auto* in = s.input;
  switch (op) {
    case kGet:
      switch (field) {
        case 0:
          return {static_cast<int64_t>(in->was_cached), true};
        case 1:
          return {static_cast<int64_t>(in->was_fetched_via_spdy), true};
        case 2:
          return {static_cast<int64_t>(in->was_alpn_negotiated), true};
        case 3:
          return {static_cast<int64_t>(in->did_use_server_http_auth), true};
        case 4:
          return {static_cast<int64_t>(in->unused_since_prefetch), true};
        case 5:
          return {static_cast<int64_t>(in->restricted_prefetch), true};
        case 6:
          return {static_cast<int64_t>(in->did_use_shared_dictionary), true};
        case 7:
          return {static_cast<int64_t>(in->ssl_info.pkp_bypassed), true};
        case 8:
          return {static_cast<int64_t>(in->ssl_info.encrypted_client_hello),
                  true};
        case 9:
          return {static_cast<int64_t>(in->connection_info), true};
        case 10:
          return {static_cast<int64_t>(in->ssl_info.cert_status), true};
        case 11:
          return {static_cast<int64_t>(in->ssl_info.connection_status), true};
        case 12:
          return {static_cast<int64_t>(in->ssl_info.key_exchange_group), true};
        case 13:
          return {static_cast<int64_t>(in->ssl_info.peer_signature_algorithm),
                  true};
        case 14:
          return {static_cast<int64_t>(in->request_time.ToInternalValue()),
                  true};
        case 15:
          return {static_cast<int64_t>(in->response_time.ToInternalValue()),
                  true};
        case 16:
          return {static_cast<int64_t>(
                      in->original_response_time.ToInternalValue()),
                  true};
        case 17:
          return {
              static_cast<int64_t>((in->stale_revalidate_timeout - base::Time())
                                       .InMicroseconds()),
              true};
        case 18:
          return {static_cast<int64_t>(in->encoded_body_size->InBytes()), true};
        case 19:
          return {static_cast<int64_t>(in->zstd_uncompressed_body_size.value()),
                  true};
        case 20:
          return {static_cast<int64_t>(in->ssl_info.is_valid()), true};
        case 21:
          return {static_cast<int64_t>(in->vary_data.is_valid()), true};
        case 22:
          return {static_cast<int64_t>(in->proxy_chain.IsValid()), true};
        case 23:
          return {static_cast<int64_t>(in->proxy_chain.is_direct()), true};
        case 24:
          return {static_cast<int64_t>(in->dns_aliases.size()), true};
        case 25:
          return {static_cast<int64_t>(in->encoded_body_size.has_value()),
                  true};
        case 26:
          return {
              static_cast<int64_t>(in->zstd_uncompressed_body_size.has_value()),
              true};
        case 27:
          return {static_cast<int64_t>(in->network_accessed), true};
        case 28:
          return {static_cast<int64_t>(in->cache_entry_status), true};
        case 29:
          return {static_cast<int64_t>(in->async_revalidation_requested), true};
        case 30:
          return {static_cast<int64_t>(in->alternate_protocol_usage), true};
        case 31:
          return {static_cast<int64_t>(in->did_send_available_dictionary),
                  true};
        default:
          NOTREACHED();
      }
    case kSet:
      switch (field) {
        case 0:
          out->was_cached = value != 0;
          break;
        case 1:
          out->was_fetched_via_spdy = value != 0;
          break;
        case 2:
          out->was_alpn_negotiated = value != 0;
          break;
        case 3:
          out->did_use_server_http_auth = value != 0;
          break;
        case 4:
          out->unused_since_prefetch = value != 0;
          break;
        case 5:
          out->restricted_prefetch = value != 0;
          break;
        case 6:
          out->did_use_shared_dictionary = value != 0;
          break;
        case 7:
          out->ssl_info.pkp_bypassed = value != 0;
          break;
        case 8:
          out->ssl_info.encrypted_client_hello = value != 0;
          break;
        case 9:
          out->connection_info = static_cast<HttpConnectionInfo>(value);
          break;
        case 10:
          out->ssl_info.cert_status = static_cast<CertStatus>(value);
          break;
        case 11:
          out->ssl_info.connection_status = static_cast<int>(value);
          break;
        case 12:
          out->ssl_info.key_exchange_group = static_cast<uint16_t>(value);
          break;
        case 13:
          out->ssl_info.peer_signature_algorithm = static_cast<uint16_t>(value);
          break;
        case 14:
          out->request_time = base::Time::FromInternalValue(value);
          break;
        case 15:
          out->response_time = base::Time::FromInternalValue(value);
          break;
        case 16:
          out->original_response_time = base::Time::FromInternalValue(value);
          break;
        case 17:
          out->stale_revalidate_timeout =
              base::Time() + base::Microseconds(value);
          break;
        case 18:
          out->encoded_body_size = base::ByteSize(base::as_unsigned(value));
          break;
        case 19:
          out->zstd_uncompressed_body_size = value;
          break;
        case 27:
          out->network_accessed = value != 0;
          break;
        case 28:
          out->cache_entry_status =
              static_cast<HttpResponseInfo::CacheEntryStatus>(value);
          break;
        case 29:
          out->async_revalidation_requested = value != 0;
          break;
        case 30:
          out->alternate_protocol_usage =
              static_cast<AlternateProtocolUsage>(value);
          break;
        case 31:
          out->did_send_available_dictionary = value != 0;
          break;
        default:
          NOTREACHED();
      }
      return {0, true};
    case kReadInt: {
      int v = 0;
      bool ok = s.iter->ReadInt(&v);
      return {v, ok};
    }
    case kReadInt64: {
      int64_t v = 0;
      bool ok = s.iter->ReadInt64(&v);
      return {v, ok};
    }
    case kReadUInt32: {
      uint32_t v = 0;
      bool ok = s.iter->ReadUInt32(&v);
      return {v, ok};
    }
    case kReadUInt16: {
      uint16_t v = 0;
      bool ok = s.iter->ReadUInt16(&v);
      return {v, ok};
    }
    case kWriteInt:
      s.pickle->WriteInt(static_cast<int>(value));
      return {0, true};
    case kWriteInt64:
      s.pickle->WriteInt64(static_cast<int64_t>(value));
      return {0, true};
    case kWriteUInt32:
      s.pickle->WriteUInt32(static_cast<uint32_t>(value));
      return {0, true};
    case kWriteUInt16:
      s.pickle->WriteUInt16(static_cast<uint16_t>(value));
      return {0, true};
    case kReadHeaders:
      out->headers = base::MakeRefCounted<HttpResponseHeaders>(s.iter);
      return {out->headers->response_code(), true};
    case kReadCert:
      out->ssl_info.cert = X509Certificate::CreateFromPickle(s.iter);
      return {0, out->ssl_info.cert != nullptr};
    case kReadSct: {
      scoped_refptr<ct::SignedCertificateTimestamp> sct =
          ct::SignedCertificateTimestamp::CreateFromPickle(s.iter);
      return {0, sct != nullptr};
    }
    case kReadVary:
      return {0, out->vary_data.InitFromPickle(s.iter)};
    case kReadHost:
      return {0, s.iter->ReadString(&s.host)};
    case kSetEndpoint: {
      IPAddress address;
      // IP parsing is a primitive service. Rust decides when to attempt it and
      // whether the port read has succeeded before any public mutation.
      bool ok = address.AssignFromIPLiteral(s.host);
      if (!ok) {
        ok = ParseURLHostnameToAddress(s.host, &address);
      }
      if (ok) {
        out->remote_endpoint =
            IPEndPoint(address, static_cast<uint16_t>(value));
      }
      return {0, true};
    }
    case kReadAlpn:
      return {0, s.iter->ReadString(&out->alpn_negotiated_protocol)};
    case kReadAlias: {
      std::string alias;
      if (!s.iter->ReadString(&alias)) {
        return {0, false};
      }
      s.aliases.push_back(std::move(alias));
      return {0, true};
    }
    case kCommitAliases:
      out->dns_aliases = base::flat_set<std::string>(std::move(s.aliases));
      return {0, true};
    case kReadProxy: {
      // The dependency primitive advances the same iterator by reference.
      auto chain = ProxyChain::InitFromPickle(*s.iter);
      if (!chain) {
        return {0, false};
      }
      out->proxy_chain = std::move(*chain);
      return {0, true};
    }
    case kWriteHeaders:
      in->headers->Persist(s.pickle, static_cast<int>(value));
      return {0, true};
    case kWriteCert:
      in->ssl_info.cert->Persist(s.pickle);
      return {0, true};
    case kWriteVary:
      in->vary_data.Persist(s.pickle);
      return {0, true};
    case kWriteHost:
      s.pickle->WriteString(in->remote_endpoint.ToStringWithoutPort());
      s.pickle->WriteUInt16(in->remote_endpoint.port());
      return {0, true};
    case kWriteAlpn:
      s.pickle->WriteString(in->alpn_negotiated_protocol);
      return {0, true};
    case kWriteAlias:
      s.pickle->WriteString(
          *(in->dns_aliases.begin() + static_cast<ptrdiff_t>(value)));
      return {0, true};
    case kWriteProxy:
      in->proxy_chain.Persist(s.pickle);
      return {0, true};
    case kCipherEcdhe: {
      const SSL_CIPHER* cipher =
          SSL_get_cipher_by_value(static_cast<uint16_t>(value));
      return {cipher && SSL_CIPHER_get_kx_nid(cipher) == NID_kx_ecdhe, true};
    }
    case kSetTruncated:
      *s.truncated = value != 0;
      return {0, true};
    case kCopyField:
      switch (field) {
        case 0:
          out->was_cached = in->was_cached;
          break;
        case 1:
          out->cache_entry_status = in->cache_entry_status;
          break;
        case 2:
          out->network_accessed = in->network_accessed;
          break;
        case 3:
          out->was_fetched_via_spdy = in->was_fetched_via_spdy;
          break;
        case 4:
          out->was_alpn_negotiated = in->was_alpn_negotiated;
          break;
        case 5:
          out->proxy_chain = in->proxy_chain;
          break;
        case 6:
          out->did_use_server_http_auth = in->did_use_server_http_auth;
          break;
        case 7:
          out->unused_since_prefetch = in->unused_since_prefetch;
          break;
        case 8:
          out->restricted_prefetch = in->restricted_prefetch;
          break;
        case 9:
          out->async_revalidation_requested = in->async_revalidation_requested;
          break;
        case 10:
          out->stale_revalidate_timeout = in->stale_revalidate_timeout;
          break;
        case 11:
          out->remote_endpoint = in->remote_endpoint;
          break;
        case 12:
          out->alpn_negotiated_protocol = in->alpn_negotiated_protocol;
          break;
        case 13:
          out->alternate_protocol_usage = in->alternate_protocol_usage;
          break;
        case 14:
          out->connection_info = in->connection_info;
          break;
        case 15:
          out->request_time = in->request_time;
          break;
        case 16:
          out->response_time = in->response_time;
          break;
        case 17:
          out->original_response_time = in->original_response_time;
          break;
        case 18:
          out->resolve_error_info = in->resolve_error_info;
          break;
        case 19:
          out->auth_challenge = in->auth_challenge;
          break;
        case 20:
          out->cert_request_info = in->cert_request_info;
          break;
        case 21:
          out->ssl_info = in->ssl_info;
          break;
        case 22:
          out->headers = in->headers;
          break;
        case 23:
          out->vary_data = in->vary_data;
          break;
        case 24:
          out->dns_aliases = in->dns_aliases;
          break;
        case 25:
          out->did_send_available_dictionary =
              in->did_send_available_dictionary;
          break;
        case 26:
          out->did_use_shared_dictionary = in->did_use_shared_dictionary;
          break;
        case 27:
          out->encoded_body_size = in->encoded_body_size;
          break;
        case 28:
          out->zstd_uncompressed_body_size = in->zstd_uncompressed_body_size;
          break;
        default:
          NOTREACHED();
      }
      return {0, true};
    case kUnexpectedVersion:
      DLOG(ERROR) << "unexpected response info version: " << value;
      return {0, true};
    default:
      NOTREACHED();
  }
}
ResponseInfoRustBridge Bridge(Services& s) {
  return {&s, Invoke};
}
}  // namespace
HttpResponseInfo::HttpResponseInfo() {
  Services s{.output = this, .input = this};
  rustium_response_info(Bridge(s), 0, false, false);
}
HttpResponseInfo::HttpResponseInfo(const HttpResponseInfo& rhs) {
  Services s{.output = this, .input = &rhs};
  rustium_response_info(Bridge(s), 1, false, false);
}
HttpResponseInfo& HttpResponseInfo::operator=(const HttpResponseInfo& rhs) {
  Services s{.output = this, .input = &rhs};
  rustium_response_info(Bridge(s), 1, this == &rhs, false);
  return *this;
}
HttpResponseInfo::~HttpResponseInfo() = default;
bool HttpResponseInfo::InitFromPickle(base::PickleIterator iter,
                                      RAW_PTR_EXCLUSION bool* truncated) {
  Services s{
      .output = this, .input = this, .iter = &iter, .truncated = truncated};
  return rustium_response_info(Bridge(s), 2, false, false);
}
std::unique_ptr<base::Pickle> HttpResponseInfo::MakePickle(
    bool skip,
    bool truncated) const {
  std::optional<int64_t> size;
  if (encoded_body_size.has_value()) {
    size = encoded_body_size->InBytes();
  }
  return MakePickleImpl(skip, truncated, size);
}
std::unique_ptr<base::Pickle>
HttpResponseInfo::MakePickleWithSignedBodySizeForTesting(bool skip,
                                                         bool truncated,
                                                         int64_t size) const {
  return MakePickleImpl(skip, truncated, size);
}
std::unique_ptr<base::Pickle> HttpResponseInfo::MakePickleImpl(
    bool skip,
    bool truncated,
    std::optional<int64_t> size) const {
  // signed size is a primitive override, selected by the Rust persistence
  // policy.
  struct WriteServices {
    STACK_ALLOCATED();

   public:
    Services base;
    std::optional<int64_t> size;
  };
  auto pickle = std::make_unique<base::Pickle>();
  pickle->Reserve(10900);
  WriteServices s{{.input = this, .pickle = pickle.get()}, size};
  ResponseInfoRustBridge bridge{&s,
                                [](void* p, uint32_t op, uint32_t field,
                                   int64_t value) -> ResponseInfoRustReply {
                                  auto& w = *static_cast<WriteServices*>(p);
                                  if (op == kGet && field == kBodyPresent) {
                                    return {w.size.has_value(), true};
                                  }
                                  if (op == kGet && field == kBody) {
                                    return {w.size.value(), true};
                                  }
                                  return Invoke(&w.base, op, field, value);
                                }};
  rustium_response_info(bridge, 3, skip, truncated);
  return pickle;
}
bool HttpResponseInfo::DidUseQuic() const {
  Services s{.input = this};
  return rustium_response_info(Bridge(s), 4, false, false);
}
bool HttpResponseInfo::WasFetchedViaProxy() const {
  Services s{.input = this};
  return rustium_response_info(Bridge(s), 5, false, false);
}
}  // namespace net
