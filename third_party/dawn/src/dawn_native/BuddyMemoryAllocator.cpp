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

#include "dawn_native/BuddyMemoryAllocator.h"

#include "common/Math.h"
#include "dawn_native/ResourceHeapAllocator.h"

namespace dawn_native {

    BuddyMemoryAllocator::BuddyMemoryAllocator(uint64_t maxSystemSize,
                                               uint64_t memoryBlockSize,
                                               ResourceHeapAllocator* heapAllocator)
        : mMemoryBlockSize(memoryBlockSize),
          mBuddyBlockAllocator(maxSystemSize),
          mHeapAllocator(heapAllocator) {
        ASSERT(memoryBlockSize <= maxSystemSize);
        ASSERT(IsPowerOfTwo(mMemoryBlockSize));
        ASSERT(maxSystemSize % mMemoryBlockSize == 0);

        mTrackedSubAllocations.resize(maxSystemSize / mMemoryBlockSize);
    }

    uint64_t BuddyMemoryAllocator::GetMemoryIndex(uint64_t offset) const {
        ASSERT(offset != BuddyAllocator::kInvalidOffset);
        return offset / mMemoryBlockSize;
    }

    ResultOrError<ResourceMemoryAllocation> BuddyMemoryAllocator::Allocate(uint64_t allocationSize,
                                                                           uint64_t alignment) {
        ResourceMemoryAllocation invalidAllocation = ResourceMemoryAllocation{};

        if (allocationSize == 0) {
            return std::move(invalidAllocation);
        }

        // Check the unaligned size to avoid overflowing NextPowerOfTwo.
        if (allocationSize > mMemoryBlockSize) {
            return std::move(invalidAllocation);
        }

        // Round allocation size to nearest power-of-two.
        allocationSize = NextPowerOfTwo(allocationSize);

        // Allocation cannot exceed the memory size.
        if (allocationSize > mMemoryBlockSize) {
            return std::move(invalidAllocation);
        }

        // Attempt to sub-allocate a block of the requested size.
        const uint64_t blockOffset = mBuddyBlockAllocator.Allocate(allocationSize, alignment);
        if (blockOffset == BuddyAllocator::kInvalidOffset) {
            return std::move(invalidAllocation);
        }

        const uint64_t memoryIndex = GetMemoryIndex(blockOffset);
        if (mTrackedSubAllocations[memoryIndex].refcount == 0) {
            // Transfer ownership to this allocator
            std::unique_ptr<ResourceHeapBase> memory;
            DAWN_TRY_ASSIGN(memory, mHeapAllocator->AllocateResourceHeap(mMemoryBlockSize));
            mTrackedSubAllocations[memoryIndex] = {/*refcount*/ 0, std::move(memory)};
        }

        mTrackedSubAllocations[memoryIndex].refcount++;

        AllocationInfo info;
        info.mBlockOffset = blockOffset;
        info.mMethod = AllocationMethod::kSubAllocated;

        // Allocation offset is always local to the memory.
        const uint64_t memoryOffset = blockOffset % mMemoryBlockSize;

        return ResourceMemoryAllocation{
            info, memoryOffset, mTrackedSubAllocations[memoryIndex].mMemoryAllocation.get()};
    }

    void BuddyMemoryAllocator::Deallocate(const ResourceMemoryAllocation& allocation) {
        const AllocationInfo info = allocation.GetInfo();

        ASSERT(info.mMethod == AllocationMethod::kSubAllocated);

        const uint64_t memoryIndex = GetMemoryIndex(info.mBlockOffset);

        ASSERT(mTrackedSubAllocations[memoryIndex].refcount > 0);
        mTrackedSubAllocations[memoryIndex].refcount--;

        if (mTrackedSubAllocations[memoryIndex].refcount == 0) {
            mHeapAllocator->DeallocateResourceHeap(
                std::move(mTrackedSubAllocations[memoryIndex].mMemoryAllocation));
        }

        mBuddyBlockAllocator.Deallocate(info.mBlockOffset);
    }

    uint64_t BuddyMemoryAllocator::GetMemoryBlockSize() const {
        return mMemoryBlockSize;
    }

    uint64_t BuddyMemoryAllocator::ComputeTotalNumOfHeapsForTesting() const {
        uint64_t count = 0;
        for (const TrackedSubAllocations& allocation : mTrackedSubAllocations) {
            if (allocation.refcount > 0) {
                count++;
            }
        }
        return count;
    }

}  // namespace dawn_native
