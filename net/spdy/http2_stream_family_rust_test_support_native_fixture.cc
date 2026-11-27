// Copyright 2026 The Chromium Authors
// BSD-style license: see LICENSE.
#include "net/spdy/http2_stream_family_rust_test_support.h"

namespace net::http2_family_test {

Inputs::Inputs() {
  early[":status"] = "103";
  early["link"] = "</asset.js>; rel=preload";
  final[":status"] = "200";
  final["content-type"] = "text/plain";
  trailers["digest"] = "fixture";
}

Inputs::~Inputs() = default;

Inputs::Inputs(Inputs&&) = default;

Inputs& Inputs::operator=(Inputs&&) = default;

}  // namespace net::http2_family_test
