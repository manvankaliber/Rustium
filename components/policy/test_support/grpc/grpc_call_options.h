// Copyright 2021 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_POLICY_TEST_SUPPORT_GRPC_GRPC_CALL_OPTIONS_H_
#define COMPONENTS_POLICY_TEST_SUPPORT_GRPC_GRPC_CALL_OPTIONS_H_

#include <grpcpp/grpcpp.h>

namespace grpc {
class ClientContext;
}

namespace policy {
namespace test {

// Various options that control gRPC ClientContext behavior.
class GrpcCallOptions {
 public:
  GrpcCallOptions();
  ~GrpcCallOptions();

  // Sets the client call deadline.
  void SetDeadline(int64_t deadline_ms);

  // Applies the options to a give grpc client |context|.
  void ApplyOptionsToContext(grpc::ClientContext* context) const;

  // Converts an int64_t to gRPC's gpr_timespec.
  static gpr_timespec ToGprTimespec(int64_t duration_ms);

 private:
  // gRPC read request deadline.
  int64_t deadline_ms_;
};

}  // namespace test
}  // namespace policy

#endif  // COMPONENTS_POLICY_TEST_SUPPORT_GRPC_GRPC_CALL_OPTIONS_H_
