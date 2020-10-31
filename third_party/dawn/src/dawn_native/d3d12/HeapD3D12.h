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

#ifndef DAWNNATIVE_D3D12_HEAPD3D12_H_
#define DAWNNATIVE_D3D12_HEAPD3D12_H_

#include "dawn_native/ResourceHeap.h"
#include "dawn_native/d3d12/PageableD3D12.h"
#include "dawn_native/d3d12/d3d12_platform.h"

namespace dawn_native { namespace d3d12 {

    class Device;

    // This class is used to represent ID3D12Heap allocations, as well as an implicit heap
    // representing a directly allocated resource. It inherits from Pageable because each Heap must
    // be represented in the ResidencyManager.
    class Heap : public ResourceHeapBase, public Pageable {
      public:
        Heap(ComPtr<ID3D12Pageable> d3d12Pageable, MemorySegment memorySegment, uint64_t size);

        ID3D12Heap* GetD3D12Heap() const;

      private:
        ComPtr<ID3D12Heap> mD3d12Heap;
    };
}}  // namespace dawn_native::d3d12

#endif  // DAWNNATIVE_D3D12_HEAPD3D12_H_
