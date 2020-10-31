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

#include "common/SlabAllocator.h"

#include "common/Assert.h"
#include "common/Math.h"

#include <algorithm>
#include <cstdlib>
#include <limits>
#include <new>

// IndexLinkNode

SlabAllocatorImpl::IndexLinkNode::IndexLinkNode(Index index, Index nextIndex)
    : index(index), nextIndex(nextIndex) {
}

// Slab

SlabAllocatorImpl::Slab::Slab(std::unique_ptr<char[]> allocation, IndexLinkNode* head)
    : allocation(std::move(allocation)),
      freeList(head),
      prev(nullptr),
      next(nullptr),
      blocksInUse(0) {
}

SlabAllocatorImpl::Slab::Slab(Slab&& rhs) = default;

SlabAllocatorImpl::SentinelSlab::SentinelSlab() : Slab(nullptr, nullptr) {
}

SlabAllocatorImpl::SentinelSlab::SentinelSlab(SentinelSlab&& rhs) = default;

SlabAllocatorImpl::SentinelSlab::~SentinelSlab() {
    Slab* slab = this->next;
    while (slab != nullptr) {
        Slab* next = slab->next;
        ASSERT(slab->blocksInUse == 0);
        slab->~Slab();
        slab = next;
    }
}

// SlabAllocatorImpl

SlabAllocatorImpl::Index SlabAllocatorImpl::kInvalidIndex =
    std::numeric_limits<SlabAllocatorImpl::Index>::max();

SlabAllocatorImpl::SlabAllocatorImpl(Index blocksPerSlab,
                                     uint32_t objectSize,
                                     uint32_t objectAlignment)
    : mAllocationAlignment(std::max(static_cast<uint32_t>(alignof(Slab)), objectAlignment)),
      mSlabBlocksOffset(Align(sizeof(Slab), objectAlignment)),
      mIndexLinkNodeOffset(Align(objectSize, alignof(IndexLinkNode))),
      mBlockStride(Align(mIndexLinkNodeOffset + sizeof(IndexLinkNode), objectAlignment)),
      mBlocksPerSlab(blocksPerSlab),
      mTotalAllocationSize(
          // required allocation size
          static_cast<size_t>(mSlabBlocksOffset) + mBlocksPerSlab * mBlockStride +
          // Pad the allocation size by mAllocationAlignment so that the aligned allocation still
          // fulfills the required size.
          mAllocationAlignment) {
    ASSERT(IsPowerOfTwo(mAllocationAlignment));
}

SlabAllocatorImpl::SlabAllocatorImpl(SlabAllocatorImpl&& rhs)
    : mAllocationAlignment(rhs.mAllocationAlignment),
      mSlabBlocksOffset(rhs.mSlabBlocksOffset),
      mIndexLinkNodeOffset(rhs.mIndexLinkNodeOffset),
      mBlockStride(rhs.mBlockStride),
      mBlocksPerSlab(rhs.mBlocksPerSlab),
      mTotalAllocationSize(rhs.mTotalAllocationSize),
      mAvailableSlabs(std::move(rhs.mAvailableSlabs)),
      mFullSlabs(std::move(rhs.mFullSlabs)),
      mRecycledSlabs(std::move(rhs.mRecycledSlabs)) {
}

SlabAllocatorImpl::~SlabAllocatorImpl() = default;

SlabAllocatorImpl::IndexLinkNode* SlabAllocatorImpl::OffsetFrom(
    IndexLinkNode* node,
    std::make_signed_t<Index> offset) const {
    return reinterpret_cast<IndexLinkNode*>(reinterpret_cast<char*>(node) +
                                            static_cast<intptr_t>(mBlockStride) * offset);
}

SlabAllocatorImpl::IndexLinkNode* SlabAllocatorImpl::NodeFromObject(void* object) const {
    return reinterpret_cast<SlabAllocatorImpl::IndexLinkNode*>(static_cast<char*>(object) +
                                                               mIndexLinkNodeOffset);
}

void* SlabAllocatorImpl::ObjectFromNode(IndexLinkNode* node) const {
    return static_cast<void*>(reinterpret_cast<char*>(node) - mIndexLinkNodeOffset);
}

bool SlabAllocatorImpl::IsNodeInSlab(Slab* slab, IndexLinkNode* node) const {
    char* firstObjectPtr = reinterpret_cast<char*>(slab) + mSlabBlocksOffset;
    IndexLinkNode* firstNode = NodeFromObject(firstObjectPtr);
    IndexLinkNode* lastNode = OffsetFrom(firstNode, mBlocksPerSlab - 1);
    return node >= firstNode && node <= lastNode && node->index < mBlocksPerSlab;
}

void SlabAllocatorImpl::PushFront(Slab* slab, IndexLinkNode* node) const {
    ASSERT(IsNodeInSlab(slab, node));

    IndexLinkNode* head = slab->freeList;
    if (head == nullptr) {
        node->nextIndex = kInvalidIndex;
    } else {
        ASSERT(IsNodeInSlab(slab, head));
        node->nextIndex = head->index;
    }
    slab->freeList = node;

    ASSERT(slab->blocksInUse != 0);
    slab->blocksInUse--;
}

SlabAllocatorImpl::IndexLinkNode* SlabAllocatorImpl::PopFront(Slab* slab) const {
    ASSERT(slab->freeList != nullptr);

    IndexLinkNode* head = slab->freeList;
    if (head->nextIndex == kInvalidIndex) {
        slab->freeList = nullptr;
    } else {
        ASSERT(IsNodeInSlab(slab, head));
        slab->freeList = OffsetFrom(head, head->nextIndex - head->index);
        ASSERT(IsNodeInSlab(slab, slab->freeList));
    }

    ASSERT(slab->blocksInUse < mBlocksPerSlab);
    slab->blocksInUse++;
    return head;
}

void SlabAllocatorImpl::SentinelSlab::Prepend(SlabAllocatorImpl::Slab* slab) {
    if (this->next != nullptr) {
        this->next->prev = slab;
    }
    slab->prev = this;
    slab->next = this->next;
    this->next = slab;
}

void SlabAllocatorImpl::Slab::Splice() {
    SlabAllocatorImpl::Slab* originalPrev = this->prev;
    SlabAllocatorImpl::Slab* originalNext = this->next;

    this->prev = nullptr;
    this->next = nullptr;

    ASSERT(originalPrev != nullptr);

    // Set the originalNext's prev pointer.
    if (originalNext != nullptr) {
        originalNext->prev = originalPrev;
    }

    // Now, set the originalNext as the originalPrev's new next.
    originalPrev->next = originalNext;
}

void* SlabAllocatorImpl::Allocate() {
    if (mAvailableSlabs.next == nullptr) {
        GetNewSlab();
    }

    Slab* slab = mAvailableSlabs.next;
    IndexLinkNode* node = PopFront(slab);
    ASSERT(node != nullptr);

    // Move full slabs to a separate list, so allocate can always return quickly.
    if (slab->blocksInUse == mBlocksPerSlab) {
        slab->Splice();
        mFullSlabs.Prepend(slab);
    }

    return ObjectFromNode(node);
}

void SlabAllocatorImpl::Deallocate(void* ptr) {
    IndexLinkNode* node = NodeFromObject(ptr);

    ASSERT(node->index < mBlocksPerSlab);
    void* firstAllocation = ObjectFromNode(OffsetFrom(node, -node->index));
    Slab* slab = reinterpret_cast<Slab*>(static_cast<char*>(firstAllocation) - mSlabBlocksOffset);
    ASSERT(slab != nullptr);

    bool slabWasFull = slab->blocksInUse == mBlocksPerSlab;

    ASSERT(slab->blocksInUse != 0);
    PushFront(slab, node);

    if (slabWasFull) {
        // Slab is in the full list. Move it to the recycled list.
        ASSERT(slab->freeList != nullptr);
        slab->Splice();
        mRecycledSlabs.Prepend(slab);
    }

    // TODO(enga): Occasionally prune slabs if |blocksInUse == 0|.
    // Doing so eagerly hurts performance.
}

void SlabAllocatorImpl::GetNewSlab() {
    // Should only be called when there are no available slabs.
    ASSERT(mAvailableSlabs.next == nullptr);

    if (mRecycledSlabs.next != nullptr) {
        // If the recycled list is non-empty, swap their contents.
        std::swap(mAvailableSlabs.next, mRecycledSlabs.next);

        // We swapped the next pointers, so the prev pointer is wrong.
        // Update it here.
        mAvailableSlabs.next->prev = &mAvailableSlabs;
        ASSERT(mRecycledSlabs.next == nullptr);
        return;
    }

    // TODO(enga): Use aligned_alloc with C++17.
    auto allocation = std::unique_ptr<char[]>(new char[mTotalAllocationSize]);
    char* alignedPtr = AlignPtr(allocation.get(), mAllocationAlignment);

    char* dataStart = alignedPtr + mSlabBlocksOffset;

    IndexLinkNode* node = NodeFromObject(dataStart);
    for (uint32_t i = 0; i < mBlocksPerSlab; ++i) {
        new (OffsetFrom(node, i)) IndexLinkNode(i, i + 1);
    }

    IndexLinkNode* lastNode = OffsetFrom(node, mBlocksPerSlab - 1);
    lastNode->nextIndex = kInvalidIndex;

    mAvailableSlabs.Prepend(new (alignedPtr) Slab(std::move(allocation), node));
}
