// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include "net/socket/connection_jobs_family_flow_fixture.h"

namespace net::connection_jobs_family_flow {
void Trace::Add(int kind,int64_t a,int64_t b) {
  checksum=checksum*131+static_cast<uint64_t>(kind);
  checksum=checksum*131+static_cast<uint64_t>(a);
  checksum=checksum*131+static_cast<uint64_t>(b);
  events.push_back({kind,a,b});
 }

Trace::Trace() = default;

Trace::~Trace() = default;

Trace::Trace(const Trace&) = default;

Trace::Trace(Trace&&) noexcept = default;

Trace& Trace::operator=(const Trace&) = default;

Trace& Trace::operator=(Trace&&) noexcept = default;
Route::Route() = default;
Route::~Route() = default;
Route::Route(const Route&) = default;
Route::Route(Route&&) = default;
Route& Route::operator=(const Route&) = default;
Route& Route::operator=(Route&&) = default;

}  // namespace net::connection_jobs_family_flow
