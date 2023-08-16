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

#include "dawn_native/d3d12/CPUDescriptorHeapAllocationD3D12.h"
#include "dawn_native/Error.h"

namespace dawn_native { namespace d3d12 {

    CPUDescriptorHeapAllocation::CPUDescriptorHeapAllocation(
        D3D12_CPU_DESCRIPTOR_HANDLE baseDescriptor,
        uint32_t heapIndex)
        : mBaseDescriptor(baseDescriptor), mHeapIndex(heapIndex) {
    }

    D3D12_CPU_DESCRIPTOR_HANDLE CPUDescriptorHeapAllocation::GetBaseDescriptor() const {
        ASSERT(IsValid());
        return mBaseDescriptor;
    }

    D3D12_CPU_DESCRIPTOR_HANDLE CPUDescriptorHeapAllocation::OffsetFrom(
        uint32_t sizeIncrementInBytes,
        uint32_t offsetInDescriptorCount) const {
        ASSERT(IsValid());
        D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle = mBaseDescriptor;
        cpuHandle.ptr += sizeIncrementInBytes * offsetInDescriptorCount;
        return cpuHandle;
    }

    uint32_t CPUDescriptorHeapAllocation::GetHeapIndex() const {
        ASSERT(mHeapIndex >= 0);
        return mHeapIndex;
    }

    bool CPUDescriptorHeapAllocation::IsValid() const {
        return mBaseDescriptor.ptr != 0;
    }

    void CPUDescriptorHeapAllocation::Invalidate() {
        mBaseDescriptor = {0};
    }

}}  // namespace dawn_native::d3d12
