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

#ifndef DAWNNATIVE_BUDDYALLOCATOR_H_
#define DAWNNATIVE_BUDDYALLOCATOR_H_

#include <cstddef>
#include <cstdint>
#include <limits>
#include <vector>

namespace dawn_native {

    // Buddy allocator uses the buddy memory allocation technique to satisfy an allocation request.
    // Memory is split into halves until just large enough to fit to the request. This
    // requires the allocation size to be a power-of-two value. The allocator "allocates" a block by
    // returning the starting offset whose size is guaranteed to be greater than or equal to the
    // allocation size. To deallocate, the same offset is used to find the corresponding block.
    //
    // Internally, it manages a free list to track free blocks in a full binary tree.
    // Every index in the free list corresponds to a level in the tree. That level also determines
    // the size of the block to be used to satisfy the request. The first level (index=0) represents
    // the root whose size is also called the max block size.
    //
    class BuddyAllocator {
      public:
        BuddyAllocator(uint64_t maxSize);
        ~BuddyAllocator();

        // Required methods.
        uint64_t Allocate(uint64_t allocationSize, uint64_t alignment = 1);
        void Deallocate(uint64_t offset);

        // For testing purposes only.
        uint64_t ComputeTotalNumOfFreeBlocksForTesting() const;

        static constexpr uint64_t kInvalidOffset = std::numeric_limits<uint64_t>::max();

      private:
        uint32_t ComputeLevelFromBlockSize(uint64_t blockSize) const;
        uint64_t GetNextFreeAlignedBlock(size_t allocationBlockLevel, uint64_t alignment) const;

        enum class BlockState { Free, Split, Allocated };

        struct BuddyBlock {
            BuddyBlock(uint64_t size, uint64_t offset)
                : mOffset(offset), mSize(size), mState(BlockState::Free) {
                free.pPrev = nullptr;
                free.pNext = nullptr;
            }

            uint64_t mOffset;
            uint64_t mSize;

            // Pointer to this block's buddy, iff parent is split.
            // Used to quickly merge buddy blocks upon de-allocate.
            BuddyBlock* pBuddy = nullptr;
            BuddyBlock* pParent = nullptr;

            // Track whether this block has been split or not.
            BlockState mState;

            struct FreeLinks {
                BuddyBlock* pPrev;
                BuddyBlock* pNext;
            };

            struct SplitLink {
                BuddyBlock* pLeft;
            };

            union {
                // Used upon allocation.
                // Avoids searching for the next free block.
                FreeLinks free;

                // Used upon de-allocation.
                // Had this block split upon allocation, it and it's buddy is to be deleted.
                SplitLink split;
            };
        };

        void InsertFreeBlock(BuddyBlock* block, size_t level);
        void RemoveFreeBlock(BuddyBlock* block, size_t level);
        void DeleteBlock(BuddyBlock* block);

        uint64_t ComputeNumOfFreeBlocks(BuddyBlock* block) const;

        // Keep track the head and tail (for faster insertion/removal).
        struct BlockList {
            BuddyBlock* head = nullptr;  // First free block in level.
            // TODO(bryan.bernhart@intel.com): Track the tail.
        };

        BuddyBlock* mRoot = nullptr;  // Used to deallocate non-free blocks.

        uint64_t mMaxBlockSize = 0;

        // List of linked-lists of free blocks where the index is a level that
        // corresponds to a power-of-two sized block.
        std::vector<BlockList> mFreeLists;
    };

}  // namespace dawn_native

#endif  // DAWNNATIVE_BUDDYALLOCATOR_H_
