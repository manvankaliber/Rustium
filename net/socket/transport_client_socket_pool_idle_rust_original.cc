// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/socket/transport_client_socket_pool_idle_rust_original.h"

namespace net::transport_idle_original {

bool Socket::WasEverUsed() {
  observations += 'W';
  return used;
}
bool Socket::IsConnectedAndIdle() {
  observations += 'I';
  return connected_and_idle;
}
bool Socket::IsConnected() {
  observations += 'C';
  return connected;
}

Reason IsUsable(Socket& socket) {
  // Adapted from IdleSocket::IsUsable at bb1359eb. The StreamSocket virtual
  // observations are scripted above, not replaced by the Rust decisions.
  if (socket.WasEverUsed()) {
    if (!socket.IsConnectedAndIdle()) {
      if (!socket.IsConnected()) {
        return Reason::kRemoteClosed;
      }
      return Reason::kUnexpectedData;
    }
    return Reason::kNone;
  }
  if (!socket.IsConnected()) {
    return Reason::kRemoteClosed;
  }
  return Reason::kNone;
}

Reason Cleanup(Socket& socket, bool force, int64_t used_timeout,
               int64_t unused_timeout) {
  bool should_clean_up = force;
  Reason reason = force ? Reason::kFlush : Reason::kNone;
  int64_t timeout = socket.WasEverUsed() ? used_timeout : unused_timeout;
  if (socket.elapsed >= timeout) {
    should_clean_up = true;
    reason = Reason::kExpired;
  }
  Reason usability = IsUsable(socket);
  if (usability != Reason::kNone) {
    should_clean_up = true;
    reason = usability;
  }
  return should_clean_up ? reason : Reason::kNone;
}

Reason Release(Socket& socket, int64_t socket_generation,
               int64_t group_generation) {
  if (!socket.IsConnectedAndIdle()) {
    if (!socket.IsConnected()) {
      return Reason::kReturnedClosed;
    }
    return Reason::kUnexpectedData;
  }
  if (socket_generation != group_generation) {
    return Reason::kStaleGeneration;
  }
  return Reason::kNone;
}

size_t Assign(std::vector<Socket>& sockets, std::vector<Reason>& closed) {
  size_t candidate = sockets.size();
  size_t oldest = sockets.size();
  closed.clear();
  for (size_t i = 0; i < sockets.size(); ++i) {
    Reason usability = IsUsable(sockets[i]);
    closed.push_back(usability);
    if (usability != Reason::kNone) {
      continue;
    }
    if (oldest == sockets.size()) {
      oldest = i;
    }
    if (sockets[i].WasEverUsed()) {
      candidate = i;
    }
  }
  if (candidate == sockets.size()) {
    candidate = oldest;
  }
  if (candidate != sockets.size()) {
    // Original handout reads WasEverUsed again for reuse metadata.
    sockets[candidate].WasEverUsed();
  }
  return candidate;
}

}  // namespace net::transport_idle_original
