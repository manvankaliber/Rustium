// Source-only connected-family draft. Not compiled or integrated.
#ifndef NET_SOCKET_CONNECT_JOB_FACTORY_RUST_DRAFT_H_
#define NET_SOCKET_CONNECT_JOB_FACTORY_RUST_DRAFT_H_
#include "net/base/net_export.h"
#include "net/socket/connection_jobs_rust_ffi.h"
namespace net::connect_job_factory_rust {
enum class Operation : uint32_t {
  Create = 0,
};
enum class Action : uint32_t {
  IsSsl = 0,
  IsTransport = 1,
  IsHttp = 2,
  CheckSocks = 3,
  Ssl = 4,
  Transport = 5,
  Http = 6,
  Socks = 7,
};
}
extern "C" NET_EXPORT_PRIVATE bool connection_jobs_rust_factory_has_input(bool);
#endif
