// Copyright 2018 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stddef.h>
#include <stdint.h>

#include <memory>

#include "base/compiler_specific.h"
#include "base/containers/span.h"
#include "net/base/io_buffer.h"
#include "net/dns/dns_query.h"

// Entry point for LibFuzzer.
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  auto packet = base::MakeRefCounted<net::IOBufferWithSize>(size);
  // LibFuzzer supplies an initialized size-byte range; empty input may be null.
  if (size != 0) {
    packet->span().copy_from(UNSAFE_BUFFERS(base::span(data, size)));
  }
  auto out = std::make_unique<net::DnsQuery>(packet);
  out->Parse(size);
  return 0;
}
