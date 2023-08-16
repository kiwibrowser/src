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

#ifndef COMMON_SLABALLOCATOR_H_
#define COMMON_SLABALLOCATOR_H_

#include "common/PlacementAllocated.h"

#include <cstdint>
#include <memory>
#include <type_traits>

// The SlabAllocator allocates objects out of one or more fixed-size contiguous "slabs" of memory.
// This makes it very quick to allocate and deallocate fixed-size objects because the allocator only
// needs to index an offset into pre-allocated memory. It is similar to a pool-allocator that
// recycles memory from previous allocations, except multiple allocations are hosted contiguously in
// one large slab.
//
// Internally, the SlabAllocator stores slabs as a linked list to avoid extra indirections indexing
// into an std::vector. To service an allocation request, the allocator only needs to know the first
// currently available slab. There are three backing linked lists: AVAILABLE, FULL, and RECYCLED.
// A slab that is AVAILABLE can be used to immediately service allocation requests. Once it has no
// remaining space, it is moved to the FULL state. When a FULL slab sees any deallocations, it is
// moved to the RECYCLED state. The RECYCLED state is separate from the AVAILABLE state so that
// deallocations don't immediately prepend slabs to the AVAILABLE list, and change the current slab
// servicing allocations. When the AVAILABLE list becomes empty is it swapped with the RECYCLED
// list.
//
// Allocated objects are placement-allocated with some extra info at the end (we'll call the Object
// plus the extra bytes a "block") used to specify the constant index of the block in its parent
// slab, as well as the index of the next available block. So, following the block next-indices
// forms a linked list of free blocks.
//
// Slab creation: When a new slab is allocated, sufficient memory is allocated for it, and then the
// slab metadata plus all of its child blocks are placement-allocated into the memory. Indices and
// next-indices are initialized to form the free-list of blocks.
//
// Allocation: When an object is allocated, if there is no space available in an existing slab, a
// new slab is created (or an old slab is recycled). The first block of the slab is removed and
// returned.
//
// Deallocation: When an object is deallocated, it can compute the pointer to its parent slab
// because it stores the index of its own allocation. That block is then prepended to the slab's
// free list.
class SlabAllocatorImpl {
  public:
    // Allocations host their current index and the index of the next free block.
    // Because this is an index, and not a byte offset, it can be much smaller than a size_t.
    // TODO(enga): Is uint8_t sufficient?
    using Index = uint16_t;

    SlabAllocatorImpl(SlabAllocatorImpl&& rhs);

  protected:
    // This is essentially a singly linked list using indices instead of pointers,
    // so we store the index of "this" in |this->index|.
    struct IndexLinkNode : PlacementAllocated {
        IndexLinkNode(Index index, Index nextIndex);

        const Index index;  // The index of this block in the slab.
        Index nextIndex;    // The index of the next available block. kInvalidIndex, if none.
    };

    struct Slab : PlacementAllocated {
        // A slab is placement-allocated into an aligned pointer from a separate allocation.
        // Ownership of the allocation is transferred to the slab on creation.
        // | ---------- allocation --------- |
        // | pad | Slab | data ------------> |
        Slab(std::unique_ptr<char[]> allocation, IndexLinkNode* head);
        Slab(Slab&& rhs);

        void Splice();

        std::unique_ptr<char[]> allocation;
        IndexLinkNode* freeList;
        Slab* prev;
        Slab* next;
        Index blocksInUse;
    };

    SlabAllocatorImpl(Index blocksPerSlab, uint32_t objectSize, uint32_t objectAlignment);
    ~SlabAllocatorImpl();

    // Allocate a new block of memory.
    void* Allocate();

    // Deallocate a block of memory.
    void Deallocate(void* ptr);

  private:
    // The maximum value is reserved to indicate the end of the list.
    static Index kInvalidIndex;

    // Get the IndexLinkNode |offset| slots away.
    IndexLinkNode* OffsetFrom(IndexLinkNode* node, std::make_signed_t<Index> offset) const;

    // Compute the pointer to the IndexLinkNode from an allocated object.
    IndexLinkNode* NodeFromObject(void* object) const;

    // Compute the pointer to the object from an IndexLinkNode.
    void* ObjectFromNode(IndexLinkNode* node) const;

    bool IsNodeInSlab(Slab* slab, IndexLinkNode* node) const;

    // The Slab stores a linked-list of free allocations.
    // PushFront/PopFront adds/removes an allocation from the free list.
    void PushFront(Slab* slab, IndexLinkNode* node) const;
    IndexLinkNode* PopFront(Slab* slab) const;

    // Replace the current slab with a new one, and chain the old one off of it.
    // Both slabs may still be used for for allocation/deallocation, but older slabs
    // will be a little slower to get allocations from.
    void GetNewSlab();

    const uint32_t mAllocationAlignment;

    // | Slab | pad | Obj | pad | Node | pad | Obj | pad | Node | pad | ....
    // | -----------|                              mSlabBlocksOffset
    // |            | ---------------------- |     mBlockStride
    // |            | ----------|                  mIndexLinkNodeOffset
    // | --------------------------------------> (mSlabBlocksOffset + mBlocksPerSlab * mBlockStride)

    // A Slab is metadata, followed by the aligned memory to allocate out of. |mSlabBlocksOffset| is
    // the offset to the start of the aligned memory region.
    const uint32_t mSlabBlocksOffset;

    // The IndexLinkNode is stored after the Allocation itself. This is the offset to it.
    const uint32_t mIndexLinkNodeOffset;

    // Because alignment of allocations may introduce padding, |mBlockStride| is the
    // distance between aligned blocks of (Allocation + IndexLinkNode)
    const uint32_t mBlockStride;

    const Index mBlocksPerSlab;  // The total number of blocks in a slab.

    const size_t mTotalAllocationSize;

    struct SentinelSlab : Slab {
        SentinelSlab();
        ~SentinelSlab();

        SentinelSlab(SentinelSlab&& rhs);

        void Prepend(Slab* slab);
    };

    SentinelSlab mAvailableSlabs;  // Available slabs to service allocations.
    SentinelSlab mFullSlabs;       // Full slabs. Stored here so we can skip checking them.
    SentinelSlab mRecycledSlabs;   // Recycled slabs. Not immediately added to |mAvailableSlabs| so
                                   // we don't thrash the current "active" slab.
};

template <typename T>
class SlabAllocator : public SlabAllocatorImpl {
  public:
    SlabAllocator(size_t totalObjectBytes,
                  uint32_t objectSize = sizeof(T),
                  uint32_t objectAlignment = alignof(T))
        : SlabAllocatorImpl(totalObjectBytes / objectSize, objectSize, objectAlignment) {
    }

    template <typename... Args>
    T* Allocate(Args&&... args) {
        void* ptr = SlabAllocatorImpl::Allocate();
        return new (ptr) T(std::forward<Args>(args)...);
    }

    void Deallocate(T* object) {
        SlabAllocatorImpl::Deallocate(object);
    }
};

#endif  // COMMON_SLABALLOCATOR_H_
