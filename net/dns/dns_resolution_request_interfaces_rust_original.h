// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#ifndef NET_DNS_DNS_RESOLUTION_REQUEST_INTERFACES_RUST_ORIGINAL_H_
#define NET_DNS_DNS_RESOLUTION_REQUEST_INTERFACES_RUST_ORIGINAL_H_

#include <optional>
#include <string>
#include "base/component_export.h"
#include "base/containers/flat_set.h"
#include "base/containers/span.h"
#include "net/base/address_list.h"
#include "net/base/completion_once_callback.h"
#include "net/base/host_port_rust_reference.h"
#include "net/base/request_priority.h"
#include "net/dns/dns_resolution_host_cache_rust_original.h"
#include "net/dns/public/host_resolver_results.h"
#include "net/dns/public/resolution_details.h"
#include "net/dns/public/resolve_error_info.h"

namespace net::dns_resolution_original {

class ResolveHostRequestOriginal {
 public:
  // Destruction cancels the request if running asynchronously, causing the
  // callback to never be invoked.
  virtual ~ResolveHostRequestOriginal() = default;

  // Starts the request and returns a network error code.
  //
  // If the request could not be handled synchronously, returns
  // |ERR_IO_PENDING|, and completion will be signaled later via |callback|.
  // On any other returned value, the request was handled synchronously and
  // |callback| will not be invoked.
  //
  // Results in ERR_NAME_NOT_RESOLVED if the hostname is not resolved. More
  // detail about the underlying error can be retrieved using
  // GetResolveErrorInfo().
  //
  // The parent HostResolver must still be alive when Start() is called,  but
  // if it is destroyed before an asynchronous result completes, the request
  // will be automatically cancelled.
  //
  // If cancelled before |callback| is invoked, it will never be invoked.
  virtual int Start(CompletionOnceCallback callback) = 0;

  // Address record (A or AAAA) results of the request. Should only be called
  // after Start() signals completion, either by invoking the callback or by
  // returning a result other than |ERR_IO_PENDING|. May empty for non-address
  // requests.
  //
  // TODO(crbug.com/40203587): Remove and replace all usage with
  // GetEndpointResults().
  virtual const AddressList& GetAddressResults() const = 0;

  // Endpoint results for `A`, `AAAA`, `UNSPECIFIED`, or `HTTPS` requests.
  // Should only be called after Start() signals completion, either by
  // invoking the callback or by returning a result other than
  // `ERR_IO_PENDING`. May return empty for non-address/HTTPS requests.
  virtual base::span<const HostResolverEndpointResult> GetEndpointResults()
      const = 0;

  // Text record (TXT) results of the request. Should only be called after
  // Start() signals completion, either by invoking the callback or by
  // returning a result other than |ERR_IO_PENDING|. May return empty for
  // non-TXT requests.
  virtual base::span<const std::string> GetTextResults() const = 0;

  // Hostname record (SRV or PTR) results of the request. For SRV results,
  // hostnames are ordered according to their priorities and weights. See RFC
  // 2782. May return empty for non-SRV/PTR requests.
  //
  // Should only be called after Start() signals completion, either by
  // invoking the callback or by returning a result other than
  // |ERR_IO_PENDING|.
  virtual base::span<const ::net::HostPortRustReference> GetHostnameResults() const = 0;

  // Any DNS record aliases, such as CNAME aliases, found as a result of an
  // address query. Includes all known aliases, e.g. from A, AAAA, or HTTPS,
  // not just from the address used for the connection, in no particular
  // order. Should only be called after Start() signals completion, either by
  // invoking the callback or by returning a result other than
  // `ERR_IO_PENDING`. Returns a list of aliases that has been fixed up and
  // canonicalized (as URL hostnames), and thus may differ from the results
  // stored directly in the AddressList. May return empty for
  // non-address/HTTPS requests.
  //
  // If `ResolveHostParameters::include_canonical_name` was true, alias
  // results will always be the single "canonical name" received from the
  // system resolver without URL hostname canonicalization (or an empty set or
  // `nullptr` in the unusual case that the system resolver did not give a
  // canonical name).
  virtual const base::flat_set<std::string>& GetDnsAliasResults() const = 0;

  // Error info for the request.
  //
  // Should only be called after Start() signals completion, either by
  // invoking the callback or by returning a result other than
  // |ERR_IO_PENDING|.
  virtual ResolveErrorInfo GetResolveErrorInfo() const = 0;

  // Information about the result's staleness in the host cache. Only
  // available if results were received from the host cache, otherwise
  // returns nullopt.
  //
  // Should only be called after Start() signals completion, either by
  // invoking the callback or by returning a result other than
  // |ERR_IO_PENDING|.
  virtual const std::optional<::net::dns_resolution_original::HostCache::EntryStaleness>& GetStaleInfo()
      const = 0;

  // Changes the priority of the specified request. Can only be called while
  // the request is running (after Start() returns |ERR_IO_PENDING| and before
  // the callback is invoked).
  virtual void ChangeRequestPriority(RequestPriority priority) {}

  // Returns details about how the host resolution was performed. Only
  // available after the request has completed. Returns std::nullopt if the
  // resolution is not completed or failed.
  virtual std::optional<ResolutionDetails> GetResolutionDetails() const = 0;
};

class COMPONENT_EXPORT(DNS_RESOLUTION_ORIGINAL) ServiceEndpointRequestOriginal {
 public:
  class Delegate {
   public:
    virtual ~Delegate() = default;

    // Called when the request has updated endpoints.
    virtual void OnServiceEndpointsUpdated() = 0;

    // Called when all queries are responded or an error occurred.
    // Note that this can be called without OnServiceEndpointsUpdated().
    virtual void OnServiceEndpointRequestFinished(int rv) = 0;
  };

  virtual ~ServiceEndpointRequestOriginal() = default;

  // Starts resolving service endpoints. `delegate` is used only when this
  // method returns ERR_IO_PENDING. When the return value is other than
  // ERR_IO_PENDING, resolution completed (or an error occurred)
  // synchronously, and GetEndpointResults() will return finalized results.
  virtual int Start(Delegate* delegate) = 0;

  // The current available service endpoints. May return stale results when
  // the cache usage is ALLOWED. These can be changed over time while
  // resolution is still ongoing. Changes are signaled by a call to the
  // delegate's OnServiceEndpointsUpdated(). Results are finalized when
  // Start() finished synchronously (returning other than ERR_IO_PENDING), or
  // delegate's OnServiceEndpointRequestFinished() is called.
  virtual base::span<const ServiceEndpoint> GetEndpointResults() = 0;

  // Any DNS record aliases, such as CNAME aliases, found as a result of
  // addresses and HTTPS queries. These can be changed over time while
  // resolution is still ongoing. See also the comment on
  // Request::GetDnsAliasResults() for details.
  virtual const base::flat_set<std::string>& GetDnsAliasResults() = 0;

  // True if the client of this request can attempt cryptographic handshakes.
  // If false, the provided service endpoints via GetEndpointResults() are not
  // finalized to the point to allow completing transactions, and data or
  // cryptographic handshakes must not be sent. This can be changed over time
  // while resolution is still ongoing.
  // TODO(crbug.com/41493696): Consider renaming this to
  // `IsSvcbResolutionCompleted()` when Chrome supports HTTPS follow-up
  // queries.
  virtual bool EndpointsCryptoReady() = 0;

  // Returns the error info of this request. This can be changed over time
  // while resolution is still ongoing. In general, should be called only
  // after resolution completed.
  virtual ResolveErrorInfo GetResolveErrorInfo() = 0;

  // Staleness about the current endpoint results. Only available if results
  // were received from the host cache, otherwise returns nullptr.
  // This can be changed over time while resolution is still ongoing, e.g.,
  // clearing to nullptr if the cached results were intermediate stale results
  // and these are replaced with fresh results.
  virtual const ::net::dns_resolution_original::HostCache::EntryStaleness* GetStaleInfo() const = 0;

  // True when the current endpoints are a stale result returned only as a
  // preliminary results while the resolver retrieves fresh results. This is
  // equivalent to checking the staleness from GetStaleInfo() while the
  // request isn't final. This can be changed over time while resolution is
  // still ongoing.
  virtual bool IsStaleWhileRefreshing() const = 0;

  // Change the priority of this request.
  virtual void ChangeRequestPriority(RequestPriority priority) = 0;

  // Returns details about how the host resolution was performed. Returns
  // std::nullopt if the resolution is not completed or failed.
  virtual std::optional<ResolutionDetails> GetResolutionDetails() const = 0;

  // TODO(crbug.com/403373872): Remove this method once we identify the cause
  // of the bug.
  // Returns a string representation of the state of the request.
  virtual std::string DebugString() const;
};

}  // namespace net::dns_resolution_original
#endif  // NET_DNS_DNS_RESOLUTION_REQUEST_INTERFACES_RUST_ORIGINAL_H_
