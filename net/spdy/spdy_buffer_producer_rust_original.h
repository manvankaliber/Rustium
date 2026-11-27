// Copyright 2013 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_SPDY_SPDY_BUFFER_PRODUCER_RUST_ORIGINAL_H_
#define NET_SPDY_SPDY_BUFFER_PRODUCER_RUST_ORIGINAL_H_

#include <memory>

#include "base/compiler_specific.h"
#include "base/component_export.h"

namespace net {

class SpdyBufferOriginal;

// An object which provides a SpdyBufferOriginal for writing. We pass these
// around instead of SpdyBuffers since some buffers have to be
// generated "just in time".
class COMPONENT_EXPORT(SPDY_BUFFER_RUST_REFERENCE) SpdyBufferProducerOriginal {
 public:
  SpdyBufferProducerOriginal();

  // Produces the buffer to be written. Will be called at most once.
  virtual std::unique_ptr<SpdyBufferOriginal> ProduceBuffer() = 0;

  SpdyBufferProducerOriginal(const SpdyBufferProducerOriginal&) = delete;
  SpdyBufferProducerOriginal& operator=(const SpdyBufferProducerOriginal&) = delete;

  virtual ~SpdyBufferProducerOriginal();
};

// A simple wrapper around a single SpdyBufferOriginal.
class COMPONENT_EXPORT(SPDY_BUFFER_RUST_REFERENCE) SimpleBufferProducerOriginal : public SpdyBufferProducerOriginal {
 public:
  explicit SimpleBufferProducerOriginal(std::unique_ptr<SpdyBufferOriginal> buffer);

  SimpleBufferProducerOriginal(const SimpleBufferProducerOriginal&) = delete;
  SimpleBufferProducerOriginal& operator=(const SimpleBufferProducerOriginal&) = delete;

  ~SimpleBufferProducerOriginal() override;

  std::unique_ptr<SpdyBufferOriginal> ProduceBuffer() override;

 private:
  std::unique_ptr<SpdyBufferOriginal> buffer_;
};

}  // namespace net

#endif  // NET_SPDY_SPDY_BUFFER_PRODUCER_RUST_ORIGINAL_H_
