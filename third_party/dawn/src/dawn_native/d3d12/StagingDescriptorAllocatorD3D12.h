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

#ifndef DAWNNATIVE_D3D12_STAGINGDESCRIPTORALLOCATOR_H_
#define DAWNNATIVE_D3D12_STAGINGDESCRIPTORALLOCATOR_H_

#include "dawn_native/Error.h"

#include "dawn_native/d3d12/CPUDescriptorHeapAllocationD3D12.h"

#include <vector>

// |StagingDescriptorAllocator| allocates a fixed-size block of descriptors from a CPU
// descriptor heap pool.
// Internally, it manages a list of heaps using a fixed-size block allocator. The fixed-size
// block allocator is backed by a list of free blocks (free-list). The heap is in one of two
// states: AVAILABLE or not. To allocate, the next free block is removed from the free-list
// and the corresponding heap offset is returned. The AVAILABLE heap always has room for
// at-least one free block. If no AVAILABLE heap exists, a new heap is created and inserted
// back into the pool to be immediately used. To deallocate, the block corresponding to the
// offset is inserted back into the free-list.
namespace dawn_native { namespace d3d12 {

    class Device;

    class StagingDescriptorAllocator {
      public:
        StagingDescriptorAllocator() = default;
        StagingDescriptorAllocator(Device* device,
                                   uint32_t descriptorCount,
                                   uint32_t heapSize,
                                   D3D12_DESCRIPTOR_HEAP_TYPE heapType);
        ~StagingDescriptorAllocator();

        ResultOrError<CPUDescriptorHeapAllocation> AllocateCPUDescriptors();

        // Will call Deallocate when the serial is passed.
        ResultOrError<CPUDescriptorHeapAllocation> AllocateTransientCPUDescriptors();

        void Deallocate(CPUDescriptorHeapAllocation* allocation);

        uint32_t GetSizeIncrement() const;

        void Tick(Serial completedSerial);

      private:
        using Index = uint16_t;

        struct NonShaderVisibleBuffer {
            ComPtr<ID3D12DescriptorHeap> heap;
            std::vector<Index> freeBlockIndices;
        };

        MaybeError AllocateCPUHeap();

        Index GetFreeBlockIndicesSize() const;

        std::vector<uint32_t> mAvailableHeaps;  // Indices into the pool.
        std::vector<NonShaderVisibleBuffer> mPool;

        Device* mDevice;

        uint32_t mSizeIncrement;  // Size of the descriptor (in bytes).
        uint32_t mBlockSize;      // Size of the block of descriptors (in bytes).
        uint32_t mHeapSize;       // Size of the heap (in number of descriptors).

        D3D12_DESCRIPTOR_HEAP_TYPE mHeapType;

        SerialQueue<CPUDescriptorHeapAllocation> mAllocationsToDelete;
    };

}}  // namespace dawn_native::d3d12

#endif  // DAWNNATIVE_D3D12_STAGINGDESCRIPTORALLOCATOR_H_
