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

#ifndef DAWNNATIVE_D3D12_RESOURCEALLOCATORMANAGERD3D12_H_
#define DAWNNATIVE_D3D12_RESOURCEALLOCATORMANAGERD3D12_H_

#include "common/SerialQueue.h"
#include "dawn_native/BuddyMemoryAllocator.h"
#include "dawn_native/d3d12/HeapAllocatorD3D12.h"
#include "dawn_native/d3d12/ResourceHeapAllocationD3D12.h"

#include <array>

namespace dawn_native { namespace d3d12 {

    class Device;

    // Resource heap types + flags combinations are named after the D3D constants.
    // https://docs.microsoft.com/en-us/windows/win32/api/d3d12/ne-d3d12-d3d12_heap_flags
    // https://docs.microsoft.com/en-us/windows/win32/api/d3d12/ne-d3d12-d3d12_heap_type
    enum ResourceHeapKind {

        // Resource heap tier 2
        // Allows resource heaps to contain all buffer and textures types.
        // This enables better heap re-use by avoiding the need for separate heaps and
        // also reduces fragmentation.
        Readback_AllBuffersAndTextures,
        Upload_AllBuffersAndTextures,
        Default_AllBuffersAndTextures,

        // Resource heap tier 1
        // Resource heaps only support types from a single resource category.
        Readback_OnlyBuffers,
        Upload_OnlyBuffers,
        Default_OnlyBuffers,

        Default_OnlyNonRenderableOrDepthTextures,
        Default_OnlyRenderableOrDepthTextures,

        EnumCount,
        InvalidEnum = EnumCount,
    };

    // Manages a list of resource allocators used by the device to create resources using
    // multiple allocation methods.
    class ResourceAllocatorManager {
      public:
        ResourceAllocatorManager(Device* device);

        ResultOrError<ResourceHeapAllocation> AllocateMemory(
            D3D12_HEAP_TYPE heapType,
            const D3D12_RESOURCE_DESC& resourceDescriptor,
            D3D12_RESOURCE_STATES initialUsage);

        void DeallocateMemory(ResourceHeapAllocation& allocation);

        void Tick(Serial lastCompletedSerial);

      private:
        void FreeMemory(ResourceHeapAllocation& allocation);

        ResultOrError<ResourceHeapAllocation> CreatePlacedResource(
            D3D12_HEAP_TYPE heapType,
            const D3D12_RESOURCE_DESC& requestedResourceDescriptor,
            const D3D12_CLEAR_VALUE* optimizedClearValue,
            D3D12_RESOURCE_STATES initialUsage);

        ResultOrError<ResourceHeapAllocation> CreateCommittedResource(
            D3D12_HEAP_TYPE heapType,
            const D3D12_RESOURCE_DESC& resourceDescriptor,
            const D3D12_CLEAR_VALUE* optimizedClearValue,
            D3D12_RESOURCE_STATES initialUsage);

        Device* mDevice;
        uint32_t mResourceHeapTier;

        static constexpr uint64_t kMaxHeapSize = 32ll * 1024ll * 1024ll * 1024ll;  // 32GB
        static constexpr uint64_t kMinHeapSize = 4ll * 1024ll * 1024ll;            // 4MB

        std::array<std::unique_ptr<BuddyMemoryAllocator>, ResourceHeapKind::EnumCount>
            mSubAllocatedResourceAllocators;
        std::array<std::unique_ptr<HeapAllocator>, ResourceHeapKind::EnumCount> mHeapAllocators;

        SerialQueue<ResourceHeapAllocation> mAllocationsToDelete;
    };

}}  // namespace dawn_native::d3d12

#endif  // DAWNNATIVE_D3D12_RESOURCEALLOCATORMANAGERD3D12_H_
