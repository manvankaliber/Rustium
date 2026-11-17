#include "net/socket/connection_jobs_upper_rust_original_helpers.h"
#include "net/base/net_error_rust_reference.h"
#include "net/log/net_log_values_rust_reference.h"
// Copyright 2024 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/socket/stream_socket_handle_rust_original.h"

#include <memory>

#include "net/base/load_timing_info.h"
#include "net/log/net_log_with_source.h"
#include "net/socket/stream_socket.h"

namespace net {

StreamSocketHandleRustOriginal::StreamSocketHandleRustOriginal() = default;

StreamSocketHandleRustOriginal::~StreamSocketHandleRustOriginal() = default;

void StreamSocketHandleRustOriginal::SetSocket(std::unique_ptr<StreamSocket> socket) {
  socket_ = std::move(socket);
}

std::unique_ptr<StreamSocket> StreamSocketHandleRustOriginal::PassSocket() {
  return std::move(socket_);
}

bool StreamSocketHandleRustOriginal::GetLoadTimingInfo(
    bool is_reused,
    LoadTimingInfo* load_timing_info) const {
  if (socket_) {
    load_timing_info->socket_log_id = socket_->NetLog().source().id;
  } else {
    // Only return load timing information when there's a socket.
    return false;
  }

  load_timing_info->socket_reused = is_reused;

  // No times if the socket is reused.
  if (is_reused) {
    return true;
  }

  load_timing_info->connect_timing = connect_timing_;
  return true;
}

void StreamSocketHandleRustOriginal::AddHigherLayeredPool(HigherLayeredPoolRustOriginal* pool) {}

void StreamSocketHandleRustOriginal::RemoveHigherLayeredPool(HigherLayeredPoolRustOriginal* pool) {}

}  // namespace net
