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

#ifndef DAWNNATIVE_D3D12_SHADERVISIBLEDESCRIPTORALLOCATOR_H_
#define DAWNNATIVE_D3D12_SHADERVISIBLEDESCRIPTORALLOCATOR_H_

#include "dawn_native/Error.h"
#include "dawn_native/RingBufferAllocator.h"
#include "dawn_native/d3d12/PageableD3D12.h"
#include "dawn_native/d3d12/d3d12_platform.h"

#include <list>

// |ShaderVisibleDescriptorAllocator| allocates a variable-sized block of descriptors from a GPU
// descriptor heap pool.
// Internally, it manages a list of heaps using a ringbuffer block allocator. The heap is in one
// of two states: switched in or out. Only a switched in heap can be bound to the pipeline. If
// the heap is full, the caller must switch-in a new heap before re-allocating and the old one
// is returned to the pool.
namespace dawn_native { namespace d3d12 {

    class Device;
    class GPUDescriptorHeapAllocation;

    class ShaderVisibleDescriptorHeap : public Pageable {
      public:
        ShaderVisibleDescriptorHeap(ComPtr<ID3D12DescriptorHeap> d3d12DescriptorHeap,
                                    uint64_t size);
        ID3D12DescriptorHeap* GetD3D12DescriptorHeap() const;

      private:
        ComPtr<ID3D12DescriptorHeap> mD3d12DescriptorHeap;
    };

    class ShaderVisibleDescriptorAllocator {
      public:
        static ResultOrError<std::unique_ptr<ShaderVisibleDescriptorAllocator>> Create(
            Device* device,
            D3D12_DESCRIPTOR_HEAP_TYPE heapType);

        ShaderVisibleDescriptorAllocator(Device* device, D3D12_DESCRIPTOR_HEAP_TYPE heapType);

        // Returns true if the allocation was successful, when false is returned the current heap is
        // full and AllocateAndSwitchShaderVisibleHeap() must be called.
        bool AllocateGPUDescriptors(uint32_t descriptorCount,
                                    Serial pendingSerial,
                                    D3D12_CPU_DESCRIPTOR_HANDLE* baseCPUDescriptor,
                                    GPUDescriptorHeapAllocation* allocation);

        void Tick(uint64_t completedSerial);

        ID3D12DescriptorHeap* GetShaderVisibleHeap() const;
        MaybeError AllocateAndSwitchShaderVisibleHeap();

        // For testing purposes only.
        Serial GetShaderVisibleHeapSerialForTesting() const;
        uint64_t GetShaderVisibleHeapSizeForTesting() const;
        uint64_t GetShaderVisiblePoolSizeForTesting() const;
        bool IsShaderVisibleHeapLockedResidentForTesting() const;
        bool IsLastShaderVisibleHeapInLRUForTesting() const;

        bool IsAllocationStillValid(const GPUDescriptorHeapAllocation& allocation) const;

      private:
        struct SerialDescriptorHeap {
            Serial heapSerial;
            std::unique_ptr<ShaderVisibleDescriptorHeap> heap;
        };

        std::unique_ptr<ShaderVisibleDescriptorHeap> mHeap;
        RingBufferAllocator mAllocator;
        std::list<SerialDescriptorHeap> mPool;
        D3D12_DESCRIPTOR_HEAP_TYPE mHeapType;

        Device* mDevice;

        // The serial value of 0 means the shader-visible heaps have not been allocated.
        // This value is never returned in the GPUDescriptorHeapAllocation after
        // AllocateGPUDescriptors() is called.
        Serial mHeapSerial = 0;

        uint32_t mSizeIncrement;
    };
}}  // namespace dawn_native::d3d12

#endif  // DAWNNATIVE_D3D12_SHADERVISIBLEDESCRIPTORALLOCATOR_H_
