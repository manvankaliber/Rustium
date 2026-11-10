#include "absl/status/status.h"         // from @com_google_absl
#include "absl/status/status_macros.h"  // from @com_google_absl
#include "mediapipe/framework/formats/tensor.h"


namespace mediapipe {

bool Tensor::AllocateAhwbMapToSsbo() const { return false; }
bool Tensor::InsertAhwbToSsboFence() const { return false; }
void Tensor::MoveAhwbStuff(Tensor* src) {}
absl::Status Tensor::ReleaseAhwbStuff() { return absl::OkStatus(); }
void* Tensor::MapAhwbToCpuRead() const { return nullptr; }
void* Tensor::MapAhwbToCpuWrite() const { return nullptr; }
void Tensor::TrackAhwbUsage(uint64_t key) const {}
void Tensor::MarkAhwbUsage() const {}
bool Tensor::ready_as_ahwb() const { return false; }


}  // namespace mediapipe
