// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#ifndef NET_SPDY_SPDY_BUFFER_RUST_FFI_H_
#define NET_SPDY_SPDY_BUFFER_RUST_FFI_H_

#include <cstddef>
#include <cstdint>

enum class SpdyBufferRustCopyAction : uint8_t {
  kCopy = 0,
  kEmpty = 1,
  kTooLarge = 2,
};

using SpdyBufferRustPrepare = size_t (*)(void*, size_t);
using SpdyBufferRustInvoke = void (*)(void*, size_t, size_t, uint32_t);
using SpdyBufferRustConsume = void (*)(void*, size_t, uint32_t);

extern "C" size_t spdy_buffer_rust_initial_offset();
extern "C" SpdyBufferRustCopyAction spdy_buffer_rust_copy_action(size_t size);
extern "C" size_t spdy_buffer_rust_remaining(size_t total, size_t offset);
// Non-unwinding callbacks use a stack context independent of the buffer owner.
// Prepare commits offset before copying callbacks; invoke sees the fixed copy.
extern "C" void spdy_buffer_rust_consume(size_t total,
                                        size_t offset,
                                        size_t amount,
                                        uint32_t source,
                                        void* context,
                                        SpdyBufferRustPrepare prepare,
                                        SpdyBufferRustInvoke invoke);
extern "C" void spdy_buffer_rust_discard(size_t total,
                                        size_t offset,
                                        void* context,
                                        SpdyBufferRustConsume consume);

#endif  // NET_SPDY_SPDY_BUFFER_RUST_FFI_H_
