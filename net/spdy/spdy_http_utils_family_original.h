// Copyright 2012 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_SPDY_FAMILY_ORIGINAL_SPDY_HTTP_UTILS_H_
#define NET_SPDY_FAMILY_ORIGINAL_SPDY_HTTP_UTILS_H_

#include <optional>

#include "base/memory/scoped_refptr.h"
#include "base/types/expected.h"
#include "base/component_export.h"
#include "net/base/request_priority.h"
#include "net/third_party/quiche/src/quiche/common/http/http_header_block.h"
#include "net/third_party/quiche/src/quiche/http2/core/spdy_framer.h"
#include "net/third_party/quiche/src/quiche/http2/core/spdy_protocol.h"

class GURL;

namespace net {

class HttpResponseInfo;
struct HttpRequestInfo;
class HttpRequestHeaders;
class HttpResponseHeaders;

// HTTP Extensible Priorities header (in lowercase HTTP2/3).
// RFC 9218.
COMPONENT_EXPORT(HTTP2_STREAM_FAMILY_REFERENCE) extern const char* const kHttp2PriorityHeaderOriginal;

// Convert a quiche::HttpHeaderBlock into an HttpResponseInfo with some checks.
// `headers` input parameter with the quiche::HttpHeaderBlock.
// `response` output parameter for the HttpResponseInfo.
// Returns OK if successfully converted.  An error is returned if the
// quiche::HttpHeaderBlock is incomplete (e.g. missing 'status' or 'version') or
// checks fail.
COMPONENT_EXPORT(HTTP2_STREAM_FAMILY_REFERENCE) int SpdyHeadersToHttpResponseOriginal(const quiche::HttpHeaderBlock& headers,
                                         HttpResponseInfo* response);

// Converts a quiche::HttpHeaderBlock object into an HttpResponseHeaders object
// by creating a string with embedded nul bytes instead of newlines and then
// parsing it to the HttpResponseHeaders constructor to be parsed. Exposed for
// testing.
// TODO(crbug.com/40282642): Remove this once it is no longer needed.
COMPONENT_EXPORT(HTTP2_STREAM_FAMILY_REFERENCE) base::expected<scoped_refptr<HttpResponseHeaders>, int>
SpdyHeadersToHttpResponseHeadersUsingRawStringOriginal(
    const quiche::HttpHeaderBlock& headers);

// Converts a quiche::HttpHeaderBlock object into an HttpResponseHeaders object
// by using the HttpResponseHeaders::Builder API. Exposed for testing.
// TODO(crbug.com/40282642): Merge this back into
// SpdyHeadersToHttpResponseOriginal() when
// SpdyHeadersToHttpResponseHeadersUsingRawStringOriginal() is removed.
COMPONENT_EXPORT(HTTP2_STREAM_FAMILY_REFERENCE) base::expected<scoped_refptr<HttpResponseHeaders>, int>
SpdyHeadersToHttpResponseHeadersUsingBuilderOriginal(
    const quiche::HttpHeaderBlock& headers);

// Create a quiche::HttpHeaderBlock from HttpRequestInfo and
// HttpRequestHeaders.
COMPONENT_EXPORT(HTTP2_STREAM_FAMILY_REFERENCE) void CreateSpdyHeadersFromHttpRequestOriginal(
    const HttpRequestInfo& info,
    std::optional<RequestPriority> priority,
    const HttpRequestHeaders& request_headers,
    quiche::HttpHeaderBlock* headers);

// Create a quiche::HttpHeaderBlock from HttpRequestInfo and
// HttpRequestHeaders, with the given protocol for extended CONNECT.
// The request's method must be `CONNECT`.
COMPONENT_EXPORT(HTTP2_STREAM_FAMILY_REFERENCE) void CreateSpdyHeadersFromHttpRequestForExtendedConnectOriginal(
    const HttpRequestInfo& info,
    std::optional<RequestPriority> priority,
    const std::string& ext_connect_protocol,
    const HttpRequestHeaders& request_headers,
    quiche::HttpHeaderBlock* headers);

// Create a quiche::HttpHeaderBlock from HttpRequestInfo and HttpRequestHeaders
// for a WebSockets over HTTP/2 request.
COMPONENT_EXPORT(HTTP2_STREAM_FAMILY_REFERENCE) void CreateSpdyHeadersFromHttpRequestForWebSocketOriginal(
    const GURL& url,
    const HttpRequestHeaders& request_headers,
    quiche::HttpHeaderBlock* headers);

COMPONENT_EXPORT(HTTP2_STREAM_FAMILY_REFERENCE) spdy::SpdyPriority ConvertRequestPriorityToSpdyPriorityOriginal(
    RequestPriority priority);

COMPONENT_EXPORT(HTTP2_STREAM_FAMILY_REFERENCE) RequestPriority
ConvertSpdyPriorityToRequestPriorityOriginal(spdy::SpdyPriority priority);

}  // namespace net

#endif  // NET_SPDY_FAMILY_ORIGINAL_SPDY_HTTP_UTILS_H_
