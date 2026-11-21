// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_SOCKET_TRANSPORT_CLIENT_SOCKET_POOL_IDLE_RUST_ORIGINAL_H_
#define NET_SOCKET_TRANSPORT_CLIENT_SOCKET_POOL_IDLE_RUST_ORIGINAL_H_

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

// Independent, executable adaptation of the original at bb1359ebe90ffbefb5f67a85bdf001f44415de3d.
// Provider calls are replaced with scripted observations, but their order and
// short-circuiting are retained. It does not call the Rust implementation.
namespace net::transport_idle_original {

enum class Reason { kNone, kFlush, kExpired, kRemoteClosed, kUnexpectedData,
                    kReturnedClosed, kStaleGeneration };

struct Socket {
  bool used = false;
  bool connected_and_idle = false;
  bool connected = false;
  int64_t elapsed = 0;
  std::string observations = {};
  std::vector<bool> used_results = {};
  size_t used_index = 0;

  bool WasEverUsed();
  bool IsConnectedAndIdle();
  bool IsConnected();
};

Reason IsUsable(Socket& socket);
Reason Cleanup(Socket& socket, bool force, int64_t used_timeout,
               int64_t unused_timeout);
Reason Release(Socket& socket, int64_t socket_generation,
               int64_t group_generation);
// Returns sockets.size() if no usable socket exists; closed records receive the
// original close reason. Selection itself preserves the original list order.
size_t Assign(std::vector<Socket>& sockets, std::vector<Reason>& closed);

}  // namespace net::transport_idle_original

#endif  // NET_SOCKET_TRANSPORT_CLIENT_SOCKET_POOL_IDLE_RUST_ORIGINAL_H_
