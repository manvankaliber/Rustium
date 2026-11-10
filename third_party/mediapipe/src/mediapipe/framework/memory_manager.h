// Copyright 2024 The MediaPipe Authors.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
#ifndef MEDIAPIPE_FRAMEWORK_MEMORY_MANAGER_H_
#define MEDIAPIPE_FRAMEWORK_MEMORY_MANAGER_H_

#include <memory>

// Defines MEDIAPIPE_TENSOR_USE_AHWB
#include "mediapipe/framework/port.h"
#include "mediapipe/gpu/multi_pool.h"


namespace mediapipe {

// Owns buffer pools to provide access to pooled buffer objects. Access is
// managed via shared_ptrs to allow clients of buffer objects to control their
// lifetime.
//
// Example usage:
// 1) Instantiate the MemoryManager and pass it to the kMemoryManagerService
//    before graph initialization:
//    CalculatorGraph graph;
//    graph.SetServiceObject(kMemoryManagerService,
//                           std::make_shared<MemoryManager>());
//    graph.Initialize(...);
// 2) Lookup MemoryManager instance in Calculator::Open():
//    absl::Status Calculator::Open(CalculatorContext* cc) {
//      auto memory_manager_service = cc->Service(kMemoryManagerService);
//      if (memory_manager_service.IsAvailable()) {
//        memory_manager_ = &memory_manager_service.GetObject();
//      } else {
//        ABSL_LOG(WARNING) << "MemoryManager not available";
//      }
//      ...
//    }
//    private:
//       MemoryManager* memory_manager_ = nullptr;
// 3) Pass Calculator::memory_manager_ to the Tensor class constructor:
//       Tensor tensor(Tensor::ElementType::kFloat32,
//                     Tensor::Shape{kTensorSize}, &memory_manager_);


// Dummy MemoryManager class for platforms that don't use AHWB.
class MemoryManager {
 public:
  explicit MemoryManager(MultiPoolOptions options = kDefaultMultiPoolOptions,
                         bool prefer_ahwb = false) {}
  bool PreferAhwb() const { return false; }
};


}  //  namespace mediapipe

#endif  // MEDIAPIPE_FRAMEWORK_MEMORY_MANAGER_H_
