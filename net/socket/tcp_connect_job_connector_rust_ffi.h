// Source-only connected-family draft. Not compiled or integrated.
#ifndef NET_SOCKET_TCP_CONNECT_JOB_CONNECTOR_RUST_DRAFT_H_
#define NET_SOCKET_TCP_CONNECT_JOB_CONNECTOR_RUST_DRAFT_H_
#include "net/base/net_export.h"
#include "net/socket/connection_jobs_rust_ffi.h"
namespace net::tcp_connect_job_connector_rust {
enum class Operation : uint32_t {
  Advance = 0,
  Load = 1,
  Io = 2,
  Loop = 3,
  Obtain = 4,
  Connect = 5,
  Complete = 6,
  Verify = 7,
  Failed = 8,
  Done = 9,
};
enum class Action : uint32_t {
  State = 0,
  SetState = 1,
  CheckNotDone = 2,
  CheckLoop = 3,
  CheckResult = 4,
  CheckOk = 5,
  CheckNoAddress = 6,
  CheckAddress = 7,
  CheckNoMore = 8,
  NotReached = 9,
  GetIp = 10,
  IpError = 11,
  TakeIp = 12,
  FindEndpoint = 13,
  HasEndpoint = 14,
  IsStale = 15,
  SaveMetadata = 16,
  ClearMetadata = 17,
  HasMetadata = 18,
  UseMetadata = 19,
  StartNull = 20,
  StampStart = 21,
  HasWatcher = 22,
  MakeWatcher = 23,
  MakeSocket = 24,
  ApplyTag = 25,
  HasWebsocket = 26,
  WrapWebsocket = 27,
  StartEvent = 28,
  Connect = 29,
  CompleteEvent = 30,
  CompleteErrorEvent = 31,
  Established = 32,
  Crypto = 33,
  UsableEvent = 34,
  FinalMetadata = 35,
  AddressV6 = 36,
  FreshPrefer = 37,
  StalePrefer = 38,
  RecordAttempt = 39,
  ResetSocket = 40,
  ResetAddress = 41,
  DoneEvent = 42,
  DoneErrorEvent = 43,
  ParentComplete = 44,
};
}
extern "C" NET_EXPORT_PRIVATE bool connection_jobs_rust_connector_flag(uint32_t,int64_t);
#endif
