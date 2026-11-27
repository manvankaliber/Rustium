// Copyright 2014 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// The GZipHeader class allows you to parse a gzip header, such as you
// might find at the beginning of a file compressed by gzip (ie, a .gz
// file), or at the beginning of an HTTP response that uses a gzip
// Content-Encoding. See RFC 1952 for the specification for the gzip
// header.
//
// The model is that you call ReadMore() for each chunk of bytes
// you've read from a file or socket.
//

#ifndef NET_FILTER_GZIP_HEADER_H_
#define NET_FILTER_GZIP_HEADER_H_

#include <stddef.h>
#include <stdint.h>

#include "base/containers/span.h"
#include "net/base/net_export.h"
#include "net/filter/gzip_header_state.h"

namespace net {

class NET_EXPORT GZipHeader {
 public:
  enum Status {
    INCOMPLETE_HEADER,  // don't have all the bits yet...
    COMPLETE_HEADER,    // complete, valid header
    INVALID_HEADER,     // found something invalid in the header
  };

  GZipHeader();

  GZipHeader(const GZipHeader&) = delete;
  GZipHeader& operator=(const GZipHeader&) = delete;

  ~GZipHeader();

  // Wipe the slate clean and start from scratch.
  void Reset();

  // Attempt to parse the given buffer as the next installment of
  // bytes from a gzip header. If the bytes we've seen so far do not
  // yet constitute a complete gzip header, return
  // INCOMPLETE_HEADER. If these bytes do not constitute a *valid*
  // gzip header, return INVALID_HEADER. When we've seen a complete
  // gzip header, return COMPLETE_HEADER and set `header_end` to the offset
  // of the first byte beyond the gzip header (i.e., it's the number of bytes of
  // `inbuf` the are part of the header).
  Status ReadMore(base::span<const uint8_t> inbuf, size_t& header_end);

  // Returns true if `inbuf` starts with a gzip header, possibly followed by
  // additional bytes.
  static bool HasGZipHeader(base::span<const uint8_t> inbuf);

 private:
  // Inline state for the Rust parser. Phase zero is the initial state.
  // Preserve the existing object size/alignment; no Rust heap handle is needed.
  gzip_header_internal::State state_;
};

}  // namespace net

#endif  // NET_FILTER_GZIP_HEADER_H_
