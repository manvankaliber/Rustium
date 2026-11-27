// Copyright 2013 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_SPDY_SPDY_READ_QUEUE_H_
#define NET_SPDY_SPDY_READ_QUEUE_H_

#include <stdint.h>

#include <cstddef>
#include <memory>

#include "base/containers/span.h"
#include "base/memory/raw_ptr_exclusion.h"
#include "net/base/net_export.h"

namespace net {

class SpdyBuffer;

// A FIFO queue of incoming data from a SPDY connection. Useful for
// SpdyStream delegates.
class NET_EXPORT_PRIVATE SpdyReadQueue {
 public:
  SpdyReadQueue();

  SpdyReadQueue(const SpdyReadQueue&) = delete;
  SpdyReadQueue& operator=(const SpdyReadQueue&) = delete;

  ~SpdyReadQueue();

  // Returns whether there's anything in the queue.
  bool IsEmpty() const;

  // Returns the total number of bytes in the queue.
  size_t GetTotalSize() const;

  // Enqueues the bytes in |buffer|.
  void Enqueue(std::unique_ptr<SpdyBuffer> buffer);

  // Dequeues up to `out.size()` bytes into `out`. Returns the number of bytes
  // dequeued.
  size_t Dequeue(base::span<uint8_t> out);

  // Removes all bytes from the queue.
  void Clear();

 private:
  // Null until the first enqueue; otherwise owns one strong reference to the
  // Rust FIFO, buffer handles and accounting. Rust creates the state as part of
  // that enqueue, and releases the owner reference after destruction drains it.
  // Clear snapshots retain the Rust state independently of this object's life.
  // Read-only clear/destructor callbacks retain the original pre-clear view;
  // reentrant mutation ends that view and destruction cannot resurrect storage.
  // No Rust borrow spans callbacks. Rust Rc allocation, not PartitionAlloc.
  RAW_PTR_EXCLUSION void* state_;
};

}  // namespace net

#endif  // NET_SPDY_SPDY_READ_QUEUE_H_
