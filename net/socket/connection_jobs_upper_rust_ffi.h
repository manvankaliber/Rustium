// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#ifndef NET_SOCKET_CONNECTION_JOBS_UPPER_RUST_FFI_H_
#define NET_SOCKET_CONNECTION_JOBS_UPPER_RUST_FFI_H_
#include "base/memory/raw_ptr.h"
#include <cstdint>
#include "base/check.h"
#include "net/base/net_export.h"
namespace net::connection_jobs_upper_rust {
using Service = bool (*)(void*, uint32_t, uint32_t, int64_t, int64_t*);
extern "C" NET_EXPORT_PRIVATE int64_t connection_jobs_rust_run(
    uint32_t module, uint32_t operation, int64_t argument, void* context,
    Service service);
// Shared same-thread invocation observations, never part of any job layout.
// Each derived destructor invalidates at entry, before its own fields die.
struct NET_EXPORT_PRIVATE Invocation {
  explicit Invocation(const void* identity);
  ~Invocation();
  using ClearObservers = void (*)(Invocation*);
  void Register(ClearObservers clear = nullptr);
  void Unregister();
  bool Live() const { return identity != nullptr; }
  static void Invalidate(const void* identity);
  raw_ptr<const void> identity;
  raw_ptr<Invocation> previous = nullptr;
  bool registered = false;
  ClearObservers clear_observers = nullptr;
  static Invocation*& Active();
};
}  // namespace net::connection_jobs_upper_rust
#endif  // NET_SOCKET_CONNECTION_JOBS_UPPER_RUST_FFI_H_
