// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include "net/http/http_response_info_rust_test_support.h"
int main(int argc, char** argv) {
  auto cert_owner = LoadCertificate();
  certificate = cert_owner.get();
  HttpResponseInfo empty;
  HttpResponseInfoOriginal old;
  Equal(empty, old);
  for (int connection = 0; connection <= 42; ++connection) {
    empty.connection_info = old.connection_info =
        static_cast<net::HttpConnectionInfo>(connection);
    CHECK(empty.DidUseQuic() == old.DidUseQuic());
  }
  for (int version = 0; version < 256; ++version) {
    base::Pickle p;
    p.WriteInt(version);
    Parse(base::as_byte_span(Bytes(p)), 123);
  }
  // Explicit null/empty input and stable backing lifetime.
  Parse(base::span<const uint8_t>(), 123);
  // Exhaustive prefix truncation of full current production representations.
  for (int i = 0; i < 128; ++i) {
    auto n = Next();
    HttpResponseInfo a;
    HttpResponseInfoOriginal b;
    Fill(a, n);
    Fill(b, n);
    Equal(a, b);
    for (bool skip : {false, true}) {
      for (bool trunc : {false, true}) {
        auto p = a.MakePickle(skip, trunc), q = b.MakePickle(skip, trunc);
        CHECK(Bytes(*p) == Bytes(*q));
        auto ps = a.MakePickleWithSignedBodySizeForTesting(skip, trunc, -1),
             qs = b.MakePickleWithSignedBodySizeForTesting(skip, trunc, -1);
        CHECK(Bytes(*ps) == Bytes(*qs));

        std::string owned = Bytes(*p);
        for (size_t cut = 0; cut <= owned.size(); ++cut) {
          ParsePrefix(owned, cut, n);
        }
      }
    }
  }
  const int count = argc > 1 ? std::atoi(argv[1]) : 1000000;
  for (int i = 0; i < count; ++i) {
    uint64_t n = Next();
    uint32_t flags = (static_cast<uint32_t>(n) & 0xffffff00u) | 3u;
    uint32_t extra = static_cast<uint32_t>(n >> 32) & 31u;
    auto p = Legacy(flags, (flags & (1u << 31)) ? extra : 0, n);
    std::string bytes = Bytes(p);
    size_t size = (n & 3) ? bytes.size()
                          : static_cast<size_t>(Next() % (bytes.size() + 1));
    if ((i % 8) == 0) {
      for (char& v : bytes) {
        v = static_cast<char>(Next());
      }
    }
    if ((i % 8) == 0) {
      Parse(base::as_byte_span(bytes).first(size), n);
    } else {
      ParsePrefix(bytes, size, n);
    }
  }
  std::printf(
      "PASS: current serialization bytes, legacy formats, malformed prefixes, "
      "copy/assignment/self-assignment, all connection enums; %d differential "
      "state inputs plus exhaustive current prefixes\n",
      count);
}
