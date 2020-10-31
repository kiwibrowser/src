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

#include "dawn_native/BuddyAllocator.h"

#include "common/Assert.h"
#include "common/Math.h"

namespace dawn_native {

    BuddyAllocator::BuddyAllocator(uint64_t maxSize) : mMaxBlockSize(maxSize) {
        ASSERT(IsPowerOfTwo(maxSize));

        mFreeLists.resize(Log2(mMaxBlockSize) + 1);

        // Insert the level0 free block.
        mRoot = new BuddyBlock(maxSize, /*offset*/ 0);
        mFreeLists[0] = {mRoot};
    }

    BuddyAllocator::~BuddyAllocator() {
        if (mRoot) {
            DeleteBlock(mRoot);
        }
    }

    uint64_t BuddyAllocator::ComputeTotalNumOfFreeBlocksForTesting() const {
        return ComputeNumOfFreeBlocks(mRoot);
    }

    uint64_t BuddyAllocator::ComputeNumOfFreeBlocks(BuddyBlock* block) const {
        if (block->mState == BlockState::Free) {
            return 1;
        } else if (block->mState == BlockState::Split) {
            return ComputeNumOfFreeBlocks(block->split.pLeft) +
                   ComputeNumOfFreeBlocks(block->split.pLeft->pBuddy);
        }
        return 0;
    }

    uint32_t BuddyAllocator::ComputeLevelFromBlockSize(uint64_t blockSize) const {
        // Every level in the buddy system can be indexed by order-n where n = log2(blockSize).
        // However, mFreeList zero-indexed by level.
        // For example, blockSize=4 is Level1 if MAX_BLOCK is 8.
        return Log2(mMaxBlockSize) - Log2(blockSize);
    }

    uint64_t BuddyAllocator::GetNextFreeAlignedBlock(size_t allocationBlockLevel,
                                                     uint64_t alignment) const {
        ASSERT(IsPowerOfTwo(alignment));
        // The current level is the level that corresponds to the allocation size. The free list may
        // not contain a block at that level until a larger one gets allocated (and splits).
        // Continue to go up the tree until such a larger block exists.
        //
        // Even if the block exists at the level, it cannot be used if it's offset is unaligned.
        // When the alignment is also a power-of-two, we simply use the next free block whose size
        // is greater than or equal to the alignment value.
        //
        //  After one 8-byte allocation:
        //
        //  Level          --------------------------------
        //      0       32 |               S              |
        //                 --------------------------------
        //      1       16 |       S       |       F2     |       S - split
        //                 --------------------------------       F - free
        //      2       8  |   Aa  |   F1  |              |       A - allocated
        //                 --------------------------------
        //
        //  Allocate(size=8, alignment=8) will be satisfied by using F1.
        //  Allocate(size=8, alignment=4) will be satified by using F1.
        //  Allocate(size=8, alignment=16) will be satisified by using F2.
        //
        for (size_t ii = 0; ii <= allocationBlockLevel; ++ii) {
            size_t currLevel = allocationBlockLevel - ii;
            BuddyBlock* freeBlock = mFreeLists[currLevel].head;
            if (freeBlock && (freeBlock->mOffset % alignment == 0)) {
                return currLevel;
            }
        }
        return kInvalidOffset;  // No free block exists at any level.
    }

    // Inserts existing free block into the free-list.
    // Called by allocate upon splitting to insert a child block into a free-list.
    // Note: Always insert into the head of the free-list. As when a larger free block at a lower
    // level was split, there were no smaller free blocks at a higher level to allocate.
    void BuddyAllocator::InsertFreeBlock(BuddyBlock* block, size_t level) {
        ASSERT(block->mState == BlockState::Free);

        // Inserted block is now the front (no prev).
        block->free.pPrev = nullptr;

        // Old head is now the inserted block's next.
        block->free.pNext = mFreeLists[level].head;

        // Block already in HEAD position (ex. right child was inserted first).
        if (mFreeLists[level].head != nullptr) {
            // Old head's previous is the inserted block.
            mFreeLists[level].head->free.pPrev = block;
        }

        mFreeLists[level].head = block;
    }

    void BuddyAllocator::RemoveFreeBlock(BuddyBlock* block, size_t level) {
        ASSERT(block->mState == BlockState::Free);

        if (mFreeLists[level].head == block) {
            // Block is in HEAD position.
            mFreeLists[level].head = mFreeLists[level].head->free.pNext;
        } else {
            // Block is after HEAD position.
            BuddyBlock* pPrev = block->free.pPrev;
            BuddyBlock* pNext = block->free.pNext;

            ASSERT(pPrev != nullptr);
            ASSERT(pPrev->mState == BlockState::Free);

            pPrev->free.pNext = pNext;

            if (pNext != nullptr) {
                ASSERT(pNext->mState == BlockState::Free);
                pNext->free.pPrev = pPrev;
            }
        }
    }

    uint64_t BuddyAllocator::Allocate(uint64_t allocationSize, uint64_t alignment) {
        if (allocationSize == 0 || allocationSize > mMaxBlockSize) {
            return kInvalidOffset;
        }

        // Compute the level
        const uint32_t allocationSizeToLevel = ComputeLevelFromBlockSize(allocationSize);

        ASSERT(allocationSizeToLevel < mFreeLists.size());

        uint64_t currBlockLevel = GetNextFreeAlignedBlock(allocationSizeToLevel, alignment);

        // Error when no free blocks exist (allocator is full)
        if (currBlockLevel == kInvalidOffset) {
            return kInvalidOffset;
        }

        // Split free blocks level-by-level.
        // Terminate when the current block level is equal to the computed level of the requested
        // allocation.
        BuddyBlock* currBlock = mFreeLists[currBlockLevel].head;

        for (; currBlockLevel < allocationSizeToLevel; currBlockLevel++) {
            ASSERT(currBlock->mState == BlockState::Free);

            // Remove curr block (about to be split).
            RemoveFreeBlock(currBlock, currBlockLevel);

            // Create two free child blocks (the buddies).
            const uint64_t nextLevelSize = currBlock->mSize / 2;
            BuddyBlock* leftChildBlock = new BuddyBlock(nextLevelSize, currBlock->mOffset);
            BuddyBlock* rightChildBlock =
                new BuddyBlock(nextLevelSize, currBlock->mOffset + nextLevelSize);

            // Remember the parent to merge these back upon de-allocation.
            rightChildBlock->pParent = currBlock;
            leftChildBlock->pParent = currBlock;

            // Make them buddies.
            leftChildBlock->pBuddy = rightChildBlock;
            rightChildBlock->pBuddy = leftChildBlock;

            // Insert the children back into the free list into the next level.
            // The free list does not require a specific order. However, an order is specified as
            // it's ideal to allocate lower addresses first by having the leftmost child in HEAD.
            InsertFreeBlock(rightChildBlock, currBlockLevel + 1);
            InsertFreeBlock(leftChildBlock, currBlockLevel + 1);

            // Curr block is now split.
            currBlock->mState = BlockState::Split;
            currBlock->split.pLeft = leftChildBlock;

            // Decend down into the next level.
            currBlock = leftChildBlock;
        }

        // Remove curr block from free-list (now allocated).
        RemoveFreeBlock(currBlock, currBlockLevel);
        currBlock->mState = BlockState::Allocated;

        return currBlock->mOffset;
    }

    void BuddyAllocator::Deallocate(uint64_t offset) {
        BuddyBlock* curr = mRoot;

        // TODO(bryan.bernhart@intel.com): Optimize de-allocation.
        // Passing allocationSize directly will avoid the following level-by-level search;
        // however, it requires the size information to be stored outside the allocator.

        // Search for the free block node that corresponds to the block offset.
        size_t currBlockLevel = 0;
        while (curr->mState == BlockState::Split) {
            if (offset < curr->split.pLeft->pBuddy->mOffset) {
                curr = curr->split.pLeft;
            } else {
                curr = curr->split.pLeft->pBuddy;
            }

            currBlockLevel++;
        }

        ASSERT(curr->mState == BlockState::Allocated);

        // Ensure the block is at the correct level
        ASSERT(currBlockLevel == ComputeLevelFromBlockSize(curr->mSize));

        // Mark curr free so we can merge.
        curr->mState = BlockState::Free;

        // Merge the buddies (LevelN-to-Level0).
        while (currBlockLevel > 0 && curr->pBuddy->mState == BlockState::Free) {
            // Remove the buddy.
            RemoveFreeBlock(curr->pBuddy, currBlockLevel);

            BuddyBlock* parent = curr->pParent;

            // The buddies were inserted in a specific order but
            // could be deleted in any order.
            DeleteBlock(curr->pBuddy);
            DeleteBlock(curr);

            // Parent is now free.
            parent->mState = BlockState::Free;

            // Ascend up to the next level (parent block).
            curr = parent;
            currBlockLevel--;
        }

        InsertFreeBlock(curr, currBlockLevel);
    }

    // Helper which deletes a block in the tree recursively (post-order).
    void BuddyAllocator::DeleteBlock(BuddyBlock* block) {
        ASSERT(block != nullptr);

        if (block->mState == BlockState::Split) {
            // Delete the pair in same order we inserted.
            DeleteBlock(block->split.pLeft->pBuddy);
            DeleteBlock(block->split.pLeft);
        }
        delete block;
    }

}  // namespace dawn_native
