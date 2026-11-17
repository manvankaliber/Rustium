// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license in LICENSE.
#include "net/dns/host_resolver_cache_rust_test_support.h"

namespace net::host_resolver_cache_rust_test {
Clocks::Wall::Wall() = default;
Clocks::Wall::~Wall() = default;
base::Time Clocks::Wall::Now() const {
      events.push_back('w');
      std::function<void()> next = std::exchange(hook, {});
      if (next) {
        next();
      }
      return now;
    }

Clocks::Ticks::Ticks() = default;
Clocks::Ticks::~Ticks() = default;
base::TimeTicks Clocks::Ticks::NowTicks() const {
      if (wall) {
        wall->events.push_back('t');
      }
      std::function<void()> next = std::exchange(hook, {});
      if (next) {
        next();
      }
      return now;
    }

}  // namespace net
