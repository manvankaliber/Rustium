// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#ifndef NET_HTTP_CONTENT_DISPOSITION_SERVICES_H_
#define NET_HTTP_CONTENT_DISPOSITION_SERVICES_H_
#include <cstdint>

#include "third_party/rust/cxx/v1/cxx.h"
namespace net::content_disposition_internal {
struct Conversion;
// Synchronous whole-string conversion. No input pointer is retained. Output
// owns its bytes; input slices are immutable, non-aliasing borrowed storage.
Conversion convert(rust::Slice<const uint8_t> input,
                   rust::Slice<const uint8_t> charset,
                   uint8_t mode);
}  // namespace net::content_disposition_internal
#endif
