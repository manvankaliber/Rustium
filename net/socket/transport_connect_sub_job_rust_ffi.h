// Source-only connected-family draft. Not compiled or integrated.
#ifndef NET_SOCKET_TRANSPORT_CONNECT_SUB_JOB_RUST_DRAFT_H_
#define NET_SOCKET_TRANSPORT_CONNECT_SUB_JOB_RUST_DRAFT_H_
#include "net/base/net_export.h"
#include "net/socket/connection_jobs_rust_ffi.h"
namespace net::transport_connect_sub_job_rust {
enum class Operation : uint32_t {
  Start = 0,
  Load = 1,
  Io = 2,
  Loop = 3,
  Connect = 4,
  Complete = 5,
  Started = 6,
};
enum class Action : uint32_t {
  State = 0,
  SetState = 1,
  CheckNone = 2,
  CheckLoop = 3,
  CheckOk = 4,
  NotReached = 5,
  HasWatcherFactory = 6,
  MakeWatcher = 7,
  MakeSocket = 8,
  AttemptEvent = 9,
  ApplyTag = 10,
  HasWebsocket = 11,
  WrapWebsocket = 12,
  Connect = 13,
  ResetSocket = 14,
  RecordAttempt = 15,
  Index = 16,
  Count = 17,
  Next = 18,
  ParentComplete = 19,
};
}
#endif
