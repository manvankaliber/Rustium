// Copyright 2015 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/base/backoff_entry_serializer.h"

#include <algorithm>
#include <ostream>
#include <utility>

#include "base/notreached.h"
#include "base/strings/string_number_conversions.h"
#include "base/time/tick_clock.h"
#include "base/values.h"
#include "net/base/backoff_entry.h"
#include "net/base/backoff_rust_ffi.h"

extern "C" size_t backoff_rust_list_len(const void* ctx) {
  return static_cast<const base::ListValue*>(ctx)->size();
}
extern "C" uint8_t backoff_rust_list_kind(const void* ctx, size_t index) {
  const auto& value = (*static_cast<const base::ListValue*>(ctx))[index];
  if (value.is_int()) {
    return 1;
  }
  if (value.is_double()) {
    return 2;
  }
  if (value.is_string()) {
    return 3;
  }
  return 0;
}
extern "C" int32_t backoff_rust_list_integer(const void* ctx, size_t index) {
  return (*static_cast<const base::ListValue*>(ctx))[index].GetInt();
}
extern "C" int64_t backoff_rust_list_double_seconds(const void* ctx,
                                                    size_t index) {
  return base::Seconds(
             (*static_cast<const base::ListValue*>(ctx))[index].GetDouble())
      .InMicroseconds();
}
extern "C" BackoffRustParsed backoff_rust_list_parse(const void* ctx,
                                                     size_t index) {
  int64_t value = 0;
  bool ok = base::StringToInt64(
      (*static_cast<const base::ListValue*>(ctx))[index].GetString(), &value);
  return {value, static_cast<uint8_t>(ok)};
}
extern "C" int64_t backoff_rust_delta_subtract(int64_t a, int64_t b) {
  return (base::Microseconds(a) - base::Microseconds(b)).InMicroseconds();
}
extern "C" int64_t backoff_rust_wall_add(int64_t a, int64_t b) {
  return (base::Time::FromInternalValue(a) + base::Microseconds(b))
      .ToInternalValue();
}

namespace net {

base::ListValue BackoffEntrySerializer::SerializeToList(
    const BackoffEntry& entry,
    base::Time time_now) {
  base::ListValue serialized;
  serialized.Append(SerializationFormatVersion::kVersion2);
  serialized.Append(entry.failure_count());
  const auto release = entry.GetReleaseTime().ToInternalValue();
  const auto now = entry.GetTimeTicksNow().ToInternalValue();
  auto times =
      backoff_rust_serialize_times(release, now, time_now.ToInternalValue());
  serialized.Append(base::NumberToString(times.first));
  serialized.Append(base::NumberToString(times.second));

  return serialized;
}

std::unique_ptr<BackoffEntry> BackoffEntrySerializer::DeserializeFromList(
    const base::ListValue& serialized,
    const BackoffEntry::Policy* policy,
    const base::TickClock* tick_clock,
    base::Time time_now) {
  auto decoded = backoff_rust_decode(&serialized);
  if (!decoded.valid) {
    return nullptr;
  }
  auto entry = std::make_unique<BackoffEntry>(policy, tick_clock);
  for (int n = 0; n < decoded.failures; ++n) {
    entry->InformOfRequest(false);
  }
  auto duration = backoff_rust_restore_duration(
      decoded.duration, decoded.absolute, time_now.ToInternalValue());
  if (!duration.valid) {
    return nullptr;
  }
  const base::TimeTicks release_time =
      entry->BackoffDurationToReleaseTime(base::Microseconds(duration.value));
  if (!backoff_rust_finite_release(release_time.ToInternalValue())) {
    return nullptr;
  }
  entry->SetCustomReleaseTime(release_time);

  return entry;
}

}  // namespace net
