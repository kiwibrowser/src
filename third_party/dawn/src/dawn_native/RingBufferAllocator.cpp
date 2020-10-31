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

#include "dawn_native/RingBufferAllocator.h"

// Note: Current RingBufferAllocator implementation uses two indices (start and end) to implement a
// circular queue. However, this approach defines a full queue when one element is still unused.
//
// For example, [E,E,E,E] would be equivelent to [U,U,U,U].
//                 ^                                ^
//                S=E=1                            S=E=1
//
// The latter case is eliminated by counting used bytes >= capacity. This definition prevents
// (the last) byte and requires an extra variable to count used bytes. Alternatively, we could use
// only two indices that keep increasing (unbounded) but can be still indexed using bit masks.
// However, this 1) requires the size to always be a power-of-two and 2) remove tests that check
// used bytes.
// TODO(bryan.bernhart@intel.com): Follow-up with ringbuffer optimization.
namespace dawn_native {

    RingBufferAllocator::RingBufferAllocator(uint64_t maxSize) : mMaxBlockSize(maxSize) {
    }

    void RingBufferAllocator::Deallocate(Serial lastCompletedSerial) {
        // Reclaim memory from previously recorded blocks.
        for (Request& request : mInflightRequests.IterateUpTo(lastCompletedSerial)) {
            mUsedStartOffset = request.endOffset;
            mUsedSize -= request.size;
        }

        // Dequeue previously recorded requests.
        mInflightRequests.ClearUpTo(lastCompletedSerial);
    }

    uint64_t RingBufferAllocator::GetSize() const {
        return mMaxBlockSize;
    }

    uint64_t RingBufferAllocator::GetUsedSize() const {
        return mUsedSize;
    }

    bool RingBufferAllocator::Empty() const {
        return mInflightRequests.Empty();
    }

    // Sub-allocate the ring-buffer by requesting a chunk of the specified size.
    // This is a serial-based resource scheme, the life-span of resources (and the allocations) get
    // tracked by GPU progress via serials. Memory can be reused by determining if the GPU has
    // completed up to a given serial. Each sub-allocation request is tracked in the serial offset
    // queue, which identifies an existing (or new) frames-worth of resources. Internally, the
    // ring-buffer maintains offsets of 3 "memory" states: Free, Reclaimed, and Used. This is done
    // in FIFO order as older frames would free resources before newer ones.
    uint64_t RingBufferAllocator::Allocate(uint64_t allocationSize, Serial serial) {
        // Check if the buffer is full by comparing the used size.
        // If the buffer is not split where waste occurs (e.g. cannot fit new sub-alloc in front), a
        // subsequent sub-alloc could fail where the used size was previously adjusted to include
        // the wasted.
        if (mUsedSize >= mMaxBlockSize) {
            return kInvalidOffset;
        }

        // Ensure adding allocationSize does not overflow.
        const uint64_t remainingSize = (mMaxBlockSize - mUsedSize);
        if (allocationSize > remainingSize) {
            return kInvalidOffset;
        }

        uint64_t startOffset = kInvalidOffset;

        // Check if the buffer is NOT split (i.e sub-alloc on ends)
        if (mUsedStartOffset <= mUsedEndOffset) {
            // Order is important (try to sub-alloc at end first).
            // This is due to FIFO order where sub-allocs are inserted from left-to-right (when not
            // wrapped).
            if (mUsedEndOffset + allocationSize <= mMaxBlockSize) {
                startOffset = mUsedEndOffset;
                mUsedEndOffset += allocationSize;
                mUsedSize += allocationSize;
                mCurrentRequestSize += allocationSize;
            } else if (allocationSize <= mUsedStartOffset) {  // Try to sub-alloc at front.
                // Count the space at the end so that a subsequent
                // sub-alloc cannot not succeed when the buffer is full.
                const uint64_t requestSize = (mMaxBlockSize - mUsedEndOffset) + allocationSize;

                startOffset = 0;
                mUsedEndOffset = allocationSize;
                mUsedSize += requestSize;
                mCurrentRequestSize += requestSize;
            }
        } else if (mUsedEndOffset + allocationSize <=
                   mUsedStartOffset) {  // Otherwise, buffer is split where sub-alloc must be
                                        // in-between.
            startOffset = mUsedEndOffset;
            mUsedEndOffset += allocationSize;
            mUsedSize += allocationSize;
            mCurrentRequestSize += allocationSize;
        }

        if (startOffset != kInvalidOffset) {
            Request request;
            request.endOffset = mUsedEndOffset;
            request.size = mCurrentRequestSize;

            mInflightRequests.Enqueue(std::move(request), serial);
            mCurrentRequestSize = 0;  // reset
        }

        return startOffset;
    }
}  // namespace dawn_native
