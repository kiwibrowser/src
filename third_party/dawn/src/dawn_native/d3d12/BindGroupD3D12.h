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

#ifndef DAWNNATIVE_D3D12_BINDGROUPD3D12_H_
#define DAWNNATIVE_D3D12_BINDGROUPD3D12_H_

#include "dawn_native/BindGroup.h"

#include "dawn_native/d3d12/d3d12_platform.h"

#include "dawn_native/d3d12/DescriptorHeapAllocator.h"

namespace dawn_native { namespace d3d12 {

    class Device;

    class BindGroup : public BindGroupBase {
      public:
        BindGroup(Device* device, const BindGroupDescriptor* descriptor);

        void AllocateDescriptors(const DescriptorHeapHandle& cbvSrvUavHeapStart,
                                 uint32_t* cbvUavSrvHeapOffset,
                                 const DescriptorHeapHandle& samplerHeapStart,
                                 uint32_t* samplerHeapOffset);
        uint32_t GetCbvUavSrvHeapOffset() const;
        uint32_t GetSamplerHeapOffset() const;

        bool TestAndSetCounted(uint64_t heapSerial, uint32_t indexInSubmit);

      private:
        uint32_t mCbvUavSrvHeapOffset;
        uint32_t mSamplerHeapOffset;

        uint64_t mHeapSerial = 0;
        uint32_t mIndexInSubmit = 0;
    };

}}  // namespace dawn_native::d3d12

#endif  // DAWNNATIVE_D3D12_BINDGROUPD3D12_H_
