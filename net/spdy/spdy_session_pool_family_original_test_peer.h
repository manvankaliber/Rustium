// Copyright 2026 The Chromium Authors
// BSD-style license: see LICENSE.
#ifndef NET_SPDY_SPDY_SESSION_POOL_FAMILY_ORIGINAL_TEST_PEER_H_
#define NET_SPDY_SPDY_SESSION_POOL_FAMILY_ORIGINAL_TEST_PEER_H_
#include "net/spdy/spdy_session_pool_family_original.h"
namespace net {
// Uses the already-declared original-only friend. No production layout access.
class SpdySessionPoolOriginalPeer {
 public:
  static void SetEnableSendingInitialData(SpdySessionPoolOriginal& pool, bool enabled) {
    pool.enable_sending_initial_data_ = enabled;
  }
};
}  // namespace net
#endif
