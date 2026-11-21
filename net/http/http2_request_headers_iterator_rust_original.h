// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#ifndef NET_HTTP_HTTP2_REQUEST_HEADERS_ITERATOR_RUST_ORIGINAL_H_
#define NET_HTTP_HTTP2_REQUEST_HEADERS_ITERATOR_RUST_ORIGINAL_H_

#include <string>

namespace net::http2_original_helpers {

// Reference-only iterator policy over a native HeaderVector storage view.
// Headers can be public HttpRequestHeaders (required by HttpStream's virtual
// signature) or the independently original container. GetHeaderVector() is an
// inline const storage accessor in both inspected headers. No public Iterator
// or Rust step helper is called, and there is no copying/conversion of headers.
template <class Headers>
class RequestHeadersIterator {
 public:
  explicit RequestHeadersIterator(const Headers& headers)
      : curr_(headers.GetHeaderVector().begin()),
        end_(headers.GetHeaderVector().end()) {}

  RequestHeadersIterator(const RequestHeadersIterator&) = delete;
  RequestHeadersIterator& operator=(const RequestHeadersIterator&) = delete;

  bool GetNext() {
    // Exact frozen HttpRequestHeadersRustOriginal::Iterator::GetNext body.
    if (!started_) {
      started_ = true;
      return curr_ != end_;
    }

    if (curr_ == end_) {
      return false;
    }

    ++curr_;
    return curr_ != end_;
  }

  const std::string& name() const { return curr_->key; }
  const std::string& value() const { return curr_->value; }

 private:
  bool started_ = false;
  typename Headers::HeaderVector::const_iterator curr_;
  const typename Headers::HeaderVector::const_iterator end_;
};

}  // namespace net::http2_original_helpers

#endif  // NET_HTTP_HTTP2_REQUEST_HEADERS_ITERATOR_RUST_ORIGINAL_H_
