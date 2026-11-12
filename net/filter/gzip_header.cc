// Copyright 2014 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/filter/gzip_header.h"

#include "base/containers/span_rust.h"
#include "net/filter/gzip_header_ffi.rs.h"

namespace net {

GZipHeader::GZipHeader() = default;

GZipHeader::~GZipHeader() = default;

void GZipHeader::Reset() {
  // The inline initial state is shared with the Rust parser. Keep the existing
  // C++ object layout and avoid a heap allocation or bridge call on
  // construction.
  state_ = {};
}

GZipHeader::Status GZipHeader::ReadMore(base::span<const uint8_t> inbuf,
                                        size_t& header_end) {
  // Rust borrows one complete input chunk and retains no reference to it.
  // The C++ output may be uninitialized. Give Rust an initialized local and
  // commit it only on completion, preserving the public output-only contract.
  size_t parsed_end = 0;
  Status status;
  if (inbuf.size() == 1) {
    // Avoid constructing a slice for fragmented one-byte reads. All parsing
    // and state transitions still run in Rust, through the same parser core.
    status = static_cast<Status>(
        gzip_header_internal::read_byte(inbuf[0], state_, parsed_end));
  } else {
    status = static_cast<Status>(gzip_header_internal::read_more(
        base::SpanToRustSlice(inbuf), state_, parsed_end));
  }
  if (status == COMPLETE_HEADER) {
    header_end = parsed_end;
  }
  return status;
}

bool GZipHeader::HasGZipHeader(base::span<const uint8_t> inbuf) {
  return gzip_header_internal::has_header(base::SpanToRustSlice(inbuf));
}

}  // namespace net
