// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_POLICY_TEST_SUPPORT_REMOTE_COMMANDS_SERVICE_HANDLER_H_
#define COMPONENTS_POLICY_TEST_SUPPORT_REMOTE_COMMANDS_SERVICE_HANDLER_H_

#include "components/policy/test_support/grpc/grpc_unary_handler.h"
#include "components/policy/test_support/remote_commands_service.grpc.pb.h"

namespace remote_commands {

inline constexpr char kSendRemoteCommandRpcName[] = "SendRemoteCommand";
inline constexpr char kWaitRemoteCommandResultRpcName[] = "WaitRemoteCommandResult";
inline constexpr char kWaitRemoteCommandAckedRpcName[] = "WaitRemoteCommandAcked";

// Bind the standard generated service to the generic asynchronous RPC helpers.
struct RemoteCommandsServiceHandler {
  using SendRemoteCommand = policy::test::GrpcUnaryHandler<
      RemoteCommandsService, SendRemoteCommandRequest, SendRemoteCommandResponse,
      kSendRemoteCommandRpcName>;
  using WaitRemoteCommandResult = policy::test::GrpcUnaryHandler<
      RemoteCommandsService, WaitRemoteCommandResultRequest,
      WaitRemoteCommandResultResponse, kWaitRemoteCommandResultRpcName>;
  using WaitRemoteCommandAcked = policy::test::GrpcUnaryHandler<
      RemoteCommandsService, WaitRemoteCommandAckedRequest,
      WaitRemoteCommandAckedResponse, kWaitRemoteCommandAckedRpcName>;
};

}  // namespace remote_commands

#endif  // COMPONENTS_POLICY_TEST_SUPPORT_REMOTE_COMMANDS_SERVICE_HANDLER_H_
