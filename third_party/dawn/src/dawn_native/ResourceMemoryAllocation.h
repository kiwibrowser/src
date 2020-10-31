// Copyright 2018 The Dawn Authors
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

#ifndef DAWNNATIVE_RESOURCEMEMORYALLOCATION_H_
#define DAWNNATIVE_RESOURCEMEMORYALLOCATION_H_

#include <cstdint>

namespace dawn_native {

    class ResourceHeapBase;

    // Allocation method determines how memory was sub-divided.
    // Used by the device to get the allocator that was responsible for the allocation.
    enum class AllocationMethod {

        // Memory not sub-divided.
        kDirect,

        // Memory sub-divided using one or more blocks of various sizes.
        kSubAllocated,

        // Memory was allocated outside of Dawn.
        kExternal,

        // Memory not allocated or freed.
        kInvalid
    };

    // Metadata that describes how the allocation was allocated.
    struct AllocationInfo {
        // AllocationInfo contains a separate offset to not confuse block vs memory offsets.
        // The block offset is within the entire allocator memory range and only required by the
        // buddy sub-allocator to get the corresponding memory. Unlike the block offset, the
        // allocation offset is always local to the memory.
        uint64_t mBlockOffset = 0;

        AllocationMethod mMethod = AllocationMethod::kInvalid;
    };

    // Handle into a resource heap pool.
    class ResourceMemoryAllocation {
      public:
        ResourceMemoryAllocation();
        ResourceMemoryAllocation(const AllocationInfo& info,
                                 uint64_t offset,
                                 ResourceHeapBase* resourceHeap,
                                 uint8_t* mappedPointer = nullptr);
        virtual ~ResourceMemoryAllocation() = default;

        ResourceHeapBase* GetResourceHeap() const;
        uint64_t GetOffset() const;
        uint8_t* GetMappedPointer() const;
        AllocationInfo GetInfo() const;

        virtual void Invalidate();

      private:
        AllocationInfo mInfo;
        uint64_t mOffset;
        ResourceHeapBase* mResourceHeap;
        uint8_t* mMappedPointer;
    };
}  // namespace dawn_native

#endif  // DAWNNATIVE_RESOURCEMEMORYALLOCATION_H_
