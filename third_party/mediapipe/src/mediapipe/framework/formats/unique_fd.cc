#include "mediapipe/framework/formats/unique_fd.h"

#include <unistd.h>

#include "absl/base/attributes.h"  // from @com_google_absl
#include "absl/log/absl_log.h"     // from @com_google_absl
#include "absl/status/statusor.h"  // from @com_google_absl
#include "mediapipe/framework/port/ret_check.h"


namespace mediapipe {

namespace {


}  // namespace

UniqueFd& UniqueFd::operator=(UniqueFd&& move) {
  if (this == &move) {
    return *this;
  }

  Reset();

  if (move.fd_ != -1) {
    fd_ = move.fd_;
    move.fd_ = -1;
  }

  return *this;
}

absl::StatusOr<UniqueFd> UniqueFd::Dup() const {
  RET_CHECK(IsValid());
  int dup_fd = dup(Get());
  return UniqueFd(dup_fd);
}

// Releases ownership of the file descriptor and returns it.
ABSL_MUST_USE_RESULT int UniqueFd::Release() {
  if (!IsValid()) {
    return -1;
  }

  int fd = fd_;
  fd_ = -1;
  return fd;
}

// Closes a wrapped file descriptor and resets the wrapper.
void UniqueFd::Reset(int new_fd) {
  if (IsValid()) {
    if (::close(fd_) != 0) {
      ABSL_LOG(ERROR) << "Failed to close fd: " << fd_;
    }
    fd_ = -1;
  }

  if (new_fd != -1) {
    fd_ = new_fd;
  }
}

}  // namespace mediapipe
