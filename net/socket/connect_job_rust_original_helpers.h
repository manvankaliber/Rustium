// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#ifndef NET_SOCKET_CONNECT_JOB_RUST_ORIGINAL_HELPERS_H_
#define NET_SOCKET_CONNECT_JOB_RUST_ORIGINAL_HELPERS_H_

#include <string_view>
#include <utility>

#include "base/check_op.h"
#include "base/values.h"
#include "net/log/net_log_event_type.h"
#include "net/log/net_log_source.h"
#include "net/log/net_log_with_source.h"

namespace net::connect_job_rust_original_helpers {

inline constexpr int kPending = -1;
inline constexpr int kAborted = -3;
inline constexpr int kTimedOut = -7;

// Original NetLogParamsWithInt: never call migrated NetLogNumberValue policy.
inline base::DictValue ParamsWithInt(std::string_view name, int value) {
  base::DictValue params;
  params.Set(name, value);
  return params;
}

// Composes the independently frozen original EndEventWithNetErrorCode and
// EndEventWithIntParams bodies. Only native event dispatch/value ownership is shared.
inline void EndEventWithNetErrorCode(const NetLogWithSource& log,
                                    NetLogEventType event_type,
                                    int net_error) {
  DCHECK_NE(kPending, net_error);
  if (net_error >= 0) {
    log.EndEvent(event_type);
  } else {
    log.EndEvent(event_type,
                 [&] { return ParamsWithInt("net_error", net_error); });
  }
}

// Original NetLogSource value policy, with kInvalidId frozen at0.
inline base::DictValue SourceParameters(const NetLogSource& source) {
  if (source.id == 0) {
    return base::DictValue();
  }
  base::DictValue dict;
  dict.Set("type", static_cast<int>(source.type));
  dict.Set("id", static_cast<int>(source.id));
  base::DictValue event_params;
  event_params.Set("source_dependency", std::move(dict));
  return event_params;
}

inline void AddEventReferencingSource(const NetLogWithSource& log,
                                     NetLogEventType event_type,
                                     const NetLogSource& source) {
  log.AddEvent(event_type, [&] { return SourceParameters(source); });
}

}  // namespace net::connect_job_rust_original_helpers
#endif  // NET_SOCKET_CONNECT_JOB_RUST_ORIGINAL_HELPERS_H_
