// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#ifndef NET_SPDY_HTTP2_WRITE_QUEUE_ORIGINAL_STREAM_ACCESS_H_
#define NET_SPDY_HTTP2_WRITE_QUEUE_ORIGINAL_STREAM_ACCESS_H_

#include "base/component_export.h"
#include "base/memory/weak_ptr.h"

namespace net {
class SpdyStream;

// Test-only, out-of-line clone of the frozen original getter. The single
// reviewed friend grants access only to this named class, with no storage or
// production behavior change. Each call reads the same object's native factory;
// it never calls the newly Rust-backed public SpdyStream::GetWeakPtr().
class COMPONENT_EXPORT(HTTP2_WRITE_QUEUE_STREAM_REFERENCE)
    Http2WriteQueueOriginalStreamAccess final {
 public:
  static base::WeakPtr<SpdyStream> GetWeakPtr(SpdyStream& stream);
};
}  // namespace net
#endif  // NET_SPDY_HTTP2_WRITE_QUEUE_ORIGINAL_STREAM_ACCESS_H_
