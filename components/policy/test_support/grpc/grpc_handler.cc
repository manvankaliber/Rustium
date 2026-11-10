// Copyright 2021 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/policy/test_support/grpc/grpc_handler.h"

#include "base/check.h"

namespace policy {
namespace test {

GrpcHandler::GrpcHandler(ServerReactorTracker* server_reactor_tracker)
    : server_reactor_tracker_(server_reactor_tracker) {
  DCHECK(server_reactor_tracker_);
}

GrpcHandler::~GrpcHandler() = default;

}  // namespace test
}  // namespace policy
