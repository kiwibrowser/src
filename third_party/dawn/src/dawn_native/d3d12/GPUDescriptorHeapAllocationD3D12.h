// Copyright 2020 The Dawn Authors
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

#ifndef DAWNNATIVE_D3D12_GPUDESCRIPTORHEAPALLOCATION_H_
#define DAWNNATIVE_D3D12_GPUDESCRIPTORHEAPALLOCATION_H_

#include "common/Serial.h"
#include "dawn_native/d3d12/d3d12_platform.h"

namespace dawn_native { namespace d3d12 {

    // Wrapper for a handle into a GPU-only descriptor heap.
    class GPUDescriptorHeapAllocation {
      public:
        GPUDescriptorHeapAllocation() = default;
        GPUDescriptorHeapAllocation(D3D12_GPU_DESCRIPTOR_HANDLE baseDescriptor,
                                    Serial lastUsageSerial,
                                    Serial heapSerial);

        D3D12_GPU_DESCRIPTOR_HANDLE GetBaseDescriptor() const;
        Serial GetLastUsageSerial() const;
        Serial GetHeapSerial() const;

      private:
        D3D12_GPU_DESCRIPTOR_HANDLE mBaseDescriptor = {0};
        Serial mLastUsageSerial = 0;
        Serial mHeapSerial = 0;
    };

}}  // namespace dawn_native::d3d12

#endif  // DAWNNATIVE_D3D12_CPUDESCRIPTORHEAPALLOCATION_H_
