// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include "net/spdy/http2_write_queue_original_stream_access.h"

#include "net/spdy/spdy_stream.h"

namespace net {

base::WeakPtr<SpdyStream> Http2WriteQueueOriginalStreamAccess::GetWeakPtr(
    SpdyStream& stream) {
  return stream.weak_ptr_factory_.GetWeakPtr();
}

}  // namespace net
