// Copyright 2016 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/spdy/http2_priority_dependencies.h"

#include <array>

#include "net/spdy/http2_priority_rust_ffi.h"

namespace net {

void Http2PriorityDependencies::StateDeleter::operator()(
    Http2PriorityRustState* state) const {
  http2_priority_rust_delete(state);
}

Http2PriorityDependencies::Http2PriorityDependencies()
    : state_(http2_priority_rust_new()) {}
Http2PriorityDependencies::~Http2PriorityDependencies() = default;

Http2PriorityDependencies::Http2PriorityDependencies(
    const Http2PriorityDependencies& other)
    : state_(http2_priority_rust_clone(other.state_.get())) {}

Http2PriorityDependencies& Http2PriorityDependencies::operator=(
    const Http2PriorityDependencies& other) {
  if (this != &other) {
    state_.reset(http2_priority_rust_clone(other.state_.get()));
  }
  return *this;
}

void Http2PriorityDependencies::OnStreamCreation(
    spdy::SpdyStreamId id,
    spdy::SpdyPriority priority,
    spdy::SpdyStreamId* parent_stream_id,
    int* weight,
    bool* exclusive) {
  Http2PriorityRustUpdate out;
  if (!http2_priority_rust_create(state_.get(), id, priority, &out)) {
    return;
  }
  // Preserve original writes, including signed/unsigned aliased parent/weight
  // storage. A real parent with ID zero still performs the final parent write.
  *parent_stream_id = 0;
  *exclusive = true;
  *weight = out.weight;
  if (out.has_parent) {
    *parent_stream_id = out.parent;
  }
}

void Http2PriorityDependencies::OnStreamDestruction(spdy::SpdyStreamId id) {
  http2_priority_rust_destroy(state_.get(), id);
}

std::vector<Http2PriorityDependencies::DependencyUpdate>
Http2PriorityDependencies::OnStreamUpdate(spdy::SpdyStreamId id,
                                          spdy::SpdyPriority new_priority) {
  std::vector<DependencyUpdate> result;
  std::array<Http2PriorityRustUpdate, 2> out;
  const size_t count =
      http2_priority_rust_update(state_.get(), id, new_priority, out.data());
  // Missing streams and unchanged dependencies return no updates. Allocate
  // native result storage only after Rust has determined the required size.
  result.reserve(count);
  for (size_t i = 0; i < count; ++i) {
    result.push_back(
        {out[i].id, out[i].parent, out[i].weight, out[i].exclusive != 0});
  }
  return result;
}

}  // namespace net
