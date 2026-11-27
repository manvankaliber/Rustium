// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include "base/check.h"
#include "base/check_op.h"
#include "base/compiler_specific.h"
#include "base/strings/string_util.h"
#include "net/base/host_port_pair.h"
#include "net/base/host_port_rust_ffi.h"
#include "net/base/host_port_rust_reference.h"

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  // SAFETY: libFuzzer supplies size initialized bytes for the call.
  std::string input = size ? UNSAFE_BUFFERS(std::string(
                                 reinterpret_cast<const char*>(data), size))
                           : std::string();
  std::string old = "sentinel", current = old;
  int op = 12345, np = op;
  bool a = net::HostPortRustOriginalParseHostAndPort(input, &old, &op);
  bool b = net::host_port_rust::ParseHostAndPort(input, &current, &np);
  CHECK_EQ(a, b);
  CHECK_EQ(old, current);
  CHECK_EQ(op, np);
  old = input;
  current = input;
  op = np = 12345;
  a = net::HostPortRustOriginalParseHostAndPort(old, &old, &op);
  b = net::host_port_rust::ParseHostAndPort(current, &current, &np);
  CHECK_EQ(a, b);
  CHECK_EQ(old, current);
  CHECK_EQ(op, np);
  auto reference = net::HostPortRustReference::FromString(input);
  auto pair = net::HostPortPair::FromString(input);
  CHECK_EQ(reference.host(), pair.host());
  CHECK_EQ(reference.port(), pair.port());
  CHECK_EQ(reference.IsEmpty(), pair.IsEmpty());
  uint16_t port = size ? data[0] : 0;
  net::HostPortRustReference r(input, port);
  net::HostPortPair p(input, port);
  CHECK_EQ(r.IsEmpty(), p.IsEmpty());
  if (base::IsStringUTF8AllowingNoncharacters(input)) {
    auto rv = r.ToValue();
    auto pv = p.ToValue();
    CHECK_EQ(rv, pv);
    auto r2 = net::HostPortRustReference::FromValue(rv);
    auto p2 = net::HostPortPair::FromValue(pv);
    CHECK_EQ(r2.has_value(), p2.has_value());
    if (r2) {
      CHECK_EQ(r2->host(), p2->host());
      CHECK_EQ(r2->port(), p2->port());
    }
  }
  if (input.find('\0') == std::string::npos &&
      !(input.starts_with('[') && input.find(':') != std::string::npos)) {
    CHECK_EQ(r.HostForURL(), p.HostForURL());
    CHECK_EQ(r.ToString(), p.ToString());
  }
  net::HostPortRustReference r3("other", 80);
  net::HostPortPair p3("other", 80);
  CHECK_EQ(r == r3, p == p3);
  CHECK_EQ(r < r3, p < p3);
  return 0;
}
