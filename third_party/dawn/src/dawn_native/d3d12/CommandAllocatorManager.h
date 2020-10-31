// Copyright 2017 The Dawn Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef DAWNNATIVE_D3D12_COMMANDALLOCATORMANAGER_H_
#define DAWNNATIVE_D3D12_COMMANDALLOCATORMANAGER_H_

#include "dawn_native/d3d12/d3d12_platform.h"

#include "common/SerialQueue.h"
#include "dawn_native/Error.h"

#include <bitset>

namespace dawn_native { namespace d3d12 {

    class Device;

    class CommandAllocatorManager {
      public:
        CommandAllocatorManager(Device* device);

        // A CommandAllocator that is reserved must be used on the next ExecuteCommandLists
        // otherwise its commands may be reset before execution has completed on the GPU
        ResultOrError<ID3D12CommandAllocator*> ReserveCommandAllocator();
        MaybeError Tick(uint64_t lastCompletedSerial);

      private:
        Device* device;

        // This must be at least 2 because the Device and Queue use separate command allocators
        static constexpr unsigned int kMaxCommandAllocators = 32;
        unsigned int mAllocatorCount;

        struct IndexedCommandAllocator {
            ComPtr<ID3D12CommandAllocator> commandAllocator;
            unsigned int index;
        };

        ComPtr<ID3D12CommandAllocator> mCommandAllocators[kMaxCommandAllocators];
        std::bitset<kMaxCommandAllocators> mFreeAllocators;
        SerialQueue<IndexedCommandAllocator> mInFlightCommandAllocators;
    };

}}  // namespace dawn_native::d3d12

#endif  // DAWNNATIVE_D3D12_COMMANDALLOCATORMANAGER_H_
