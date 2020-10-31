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

#include "dawn_native/d3d12/GPUDescriptorHeapAllocationD3D12.h"

namespace dawn_native { namespace d3d12 {

    GPUDescriptorHeapAllocation::GPUDescriptorHeapAllocation(
        D3D12_GPU_DESCRIPTOR_HANDLE baseDescriptor,
        Serial lastUsageSerial,
        Serial heapSerial)
        : mBaseDescriptor(baseDescriptor),
          mLastUsageSerial(lastUsageSerial),
          mHeapSerial(heapSerial) {
    }

    D3D12_GPU_DESCRIPTOR_HANDLE GPUDescriptorHeapAllocation::GetBaseDescriptor() const {
        return mBaseDescriptor;
    }

    Serial GPUDescriptorHeapAllocation::GetLastUsageSerial() const {
        return mLastUsageSerial;
    }

    Serial GPUDescriptorHeapAllocation::GetHeapSerial() const {
        return mHeapSerial;
    }
}}  // namespace dawn_native::d3d12
