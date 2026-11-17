// Copyright 2023 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/dns/host_resolver_cache_source_reference.h"

#include <optional>

#include "base/values.h"

namespace net::host_resolver_cache_reference {

base::Value OriginalHostResolverSourceToValue(HostResolverSource source) {
  return base::Value(static_cast<int>(source));
}

std::optional<HostResolverSource> OriginalHostResolverSourceFromValue(
    const base::Value& value) {
  std::optional<int> value_int = value.GetIfInt();
  if (!value_int.has_value() || value_int.value() < 0 ||
      value_int.value() > static_cast<int>(HostResolverSource::MAX)) {
    return std::nullopt;
  }

  return static_cast<HostResolverSource>(value_int.value());
}

}  // namespace net
