// Copyright 2016 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_SPDY_HTTP2_PRIORITY_DEPENDENCIES_H_
#define NET_SPDY_HTTP2_PRIORITY_DEPENDENCIES_H_

#include <memory>
#include <vector>

#include "net/base/net_export.h"
#include "net/third_party/quiche/src/quiche/http2/core/spdy_protocol.h"

struct Http2PriorityRustState;

namespace net {

// A helper class encapsulating the state and logic to set the priority fields
// for HTTP/2 streams based on their spdy::SpdyPriority and the ordering of
// creation and deletion of the streams. This implentation includes a gross hack
// in which the HTTP/2 weight is set to a transformation of the
// spdy::SpdyPriority value in order to support servers which do not honor
// HTTP/2 stream dependencies and instead treat the weight value like a SPDY/3
// priority.
// TODO(rch): Eliminate this gross hack when servers no longer act like this.
class NET_EXPORT_PRIVATE Http2PriorityDependencies {
 public:
  Http2PriorityDependencies();
  ~Http2PriorityDependencies();

  // Preserve the formerly implicit copy API with independent owned state.
  Http2PriorityDependencies(const Http2PriorityDependencies& other);
  Http2PriorityDependencies& operator=(const Http2PriorityDependencies& other);

  // Called when a stream is created. This is used for both client-initiated
  // and server-initiated (pushed) streams.
  // On return, |*parent_stream_id| is set to the stream id that should become
  // the parent of this stream, |*exclusive| is set to whether that dependency
  // should be exclusive, and |*weight| is set to the relative weight for the
  // created stream given this priority.
  void OnStreamCreation(spdy::SpdyStreamId id,
                        spdy::SpdyPriority priority,
                        spdy::SpdyStreamId* parent_stream_id,
                        int* weight,
                        bool* exclusive);

  // Called when a stream is destroyed.
  void OnStreamDestruction(spdy::SpdyStreamId id);

  struct DependencyUpdate {
    spdy::SpdyStreamId id;
    spdy::SpdyStreamId parent_stream_id;
    int weight;
    bool exclusive;
  };

  // Called when a stream's priority has changed. Returns a list of
  // dependency updates that should be sent to the server to describe
  // the requested priority change. The updates should be sent in the
  // given order.
  std::vector<DependencyUpdate> OnStreamUpdate(spdy::SpdyStreamId id,
                                               spdy::SpdyPriority new_priority);

 private:
  struct StateDeleter {
    void operator()(Http2PriorityRustState* state) const;
  };
  // Layout changed: rebuild all callers; never use old-DLL object methods.
  // Rust owns the full priority ordering, ID index, storage and weight policy.
  std::unique_ptr<Http2PriorityRustState, StateDeleter> state_;
};

}  // namespace net

#endif  // NET_SPDY_HTTP2_PRIORITY_DEPENDENCIES_H_
