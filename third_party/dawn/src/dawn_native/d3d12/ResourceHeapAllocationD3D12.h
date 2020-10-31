// Copyright 2019 The Dawn Authors
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

#ifndef DAWNNATIVE_D3D12_RESOURCEHEAPALLOCATIOND3D12_H_
#define DAWNNATIVE_D3D12_RESOURCEHEAPALLOCATIOND3D12_H_

#include "dawn_native/ResourceMemoryAllocation.h"
#include "dawn_native/d3d12/d3d12_platform.h"

namespace dawn_native { namespace d3d12 {

    class Heap;

    class ResourceHeapAllocation : public ResourceMemoryAllocation {
      public:
        ResourceHeapAllocation() = default;
        ResourceHeapAllocation(const AllocationInfo& info,
                               uint64_t offset,
                               ComPtr<ID3D12Resource> resource,
                               Heap* heap);
        ~ResourceHeapAllocation() override = default;

        void Invalidate() override;

        ID3D12Resource* GetD3D12Resource() const;
        D3D12_GPU_VIRTUAL_ADDRESS GetGPUPointer() const;

      private:
        ComPtr<ID3D12Resource> mResource;
    };

}}  // namespace dawn_native::d3d12

#endif  // DAWNNATIVE_D3D12_RESOURCEHEAPALLOCATIOND3D12_H_
