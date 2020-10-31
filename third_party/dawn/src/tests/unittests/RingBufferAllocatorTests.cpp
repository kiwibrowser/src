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

#include <gtest/gtest.h>

#include "dawn_native/RingBufferAllocator.h"

using namespace dawn_native;

constexpr uint64_t RingBufferAllocator::kInvalidOffset;

// Number of basic tests for Ringbuffer
TEST(RingBufferAllocatorTests, BasicTest) {
    constexpr uint64_t sizeInBytes = 64000;
    RingBufferAllocator allocator(sizeInBytes);

    // Ensure no requests exist on empty buffer.
    EXPECT_TRUE(allocator.Empty());

    ASSERT_EQ(allocator.GetSize(), sizeInBytes);

    // Ensure failure upon sub-allocating an oversized request.
    ASSERT_EQ(allocator.Allocate(sizeInBytes + 1, 0), RingBufferAllocator::kInvalidOffset);

    // Fill the entire buffer with two requests of equal size.
    ASSERT_EQ(allocator.Allocate(sizeInBytes / 2, 1), 0u);
    ASSERT_EQ(allocator.Allocate(sizeInBytes / 2, 2), 32000u);

    // Ensure the buffer is full.
    ASSERT_EQ(allocator.Allocate(1, 3), RingBufferAllocator::kInvalidOffset);
}

// Tests that several ringbuffer allocations do not fail.
TEST(RingBufferAllocatorTests, RingBufferManyAlloc) {
    constexpr uint64_t maxNumOfFrames = 64000;
    constexpr uint64_t frameSizeInBytes = 4;

    RingBufferAllocator allocator(maxNumOfFrames * frameSizeInBytes);

    size_t offset = 0;
    for (size_t i = 0; i < maxNumOfFrames; ++i) {
        offset = allocator.Allocate(frameSizeInBytes, i);
        ASSERT_EQ(offset, i * frameSizeInBytes);
    }
}

// Tests ringbuffer sub-allocations of the same serial are correctly tracked.
TEST(RingBufferAllocatorTests, AllocInSameFrame) {
    constexpr uint64_t maxNumOfFrames = 3;
    constexpr uint64_t frameSizeInBytes = 4;

    RingBufferAllocator allocator(maxNumOfFrames * frameSizeInBytes);

    //    F1
    //  [xxxx|--------]
    size_t offset = allocator.Allocate(frameSizeInBytes, 1);

    //    F1   F2
    //  [xxxx|xxxx|----]

    offset = allocator.Allocate(frameSizeInBytes, 2);

    //    F1     F2
    //  [xxxx|xxxxxxxx]

    offset = allocator.Allocate(frameSizeInBytes, 2);

    ASSERT_EQ(offset, 8u);
    ASSERT_EQ(allocator.GetUsedSize(), frameSizeInBytes * 3);

    allocator.Deallocate(2);

    ASSERT_EQ(allocator.GetUsedSize(), 0u);
    EXPECT_TRUE(allocator.Empty());
}

// Tests ringbuffer sub-allocation at various offsets.
TEST(RingBufferAllocatorTests, RingBufferSubAlloc) {
    constexpr uint64_t maxNumOfFrames = 10;
    constexpr uint64_t frameSizeInBytes = 4;

    RingBufferAllocator allocator(maxNumOfFrames * frameSizeInBytes);

    // Sub-alloc the first eight frames.
    Serial serial = 1;
    for (size_t i = 0; i < 8; ++i) {
        allocator.Allocate(frameSizeInBytes, serial);
        serial += 1;
    }

    // Each frame corrresponds to the serial number (for simplicity).
    //
    //    F1   F2   F3   F4   F5   F6   F7   F8
    //  [xxxx|xxxx|xxxx|xxxx|xxxx|xxxx|xxxx|xxxx|--------]
    //

    // Ensure an oversized allocation fails (only 8 bytes left)
    ASSERT_EQ(allocator.Allocate(frameSizeInBytes * 3, serial + 1),
              RingBufferAllocator::kInvalidOffset);
    ASSERT_EQ(allocator.GetUsedSize(), frameSizeInBytes * 8);

    // Reclaim the first 3 frames.
    allocator.Deallocate(3);

    //                 F4   F5   F6   F7   F8
    //  [------------|xxxx|xxxx|xxxx|xxxx|xxxx|--------]
    //
    ASSERT_EQ(allocator.GetUsedSize(), frameSizeInBytes * 5);

    // Re-try the over-sized allocation.
    size_t offset = allocator.Allocate(frameSizeInBytes * 3, serial);

    //        F9       F4   F5   F6   F7   F8
    //  [xxxxxxxxxxxx|xxxx|xxxx|xxxx|xxxx|xxxx|xxxxxxxx]
    //                                         ^^^^^^^^ wasted

    // In this example, Deallocate(8) could not reclaim the wasted bytes. The wasted bytes
    // were added to F9's sub-allocation.
    // TODO(bryan.bernhart@intel.com): Decide if Deallocate(8) should free these wasted bytes.

    ASSERT_EQ(offset, 0u);
    ASSERT_EQ(allocator.GetUsedSize(), frameSizeInBytes * maxNumOfFrames);

    // Ensure we are full.
    ASSERT_EQ(allocator.Allocate(frameSizeInBytes, serial + 1),
              RingBufferAllocator::kInvalidOffset);

    // Reclaim the next two frames.
    allocator.Deallocate(5);

    //        F9       F4   F5   F6   F7   F8
    //  [xxxxxxxxxxxx|----|----|xxxx|xxxx|xxxx|xxxxxxxx]
    //
    ASSERT_EQ(allocator.GetUsedSize(), frameSizeInBytes * 8);

    // Sub-alloc the chunk in the middle.
    serial += 1;
    offset = allocator.Allocate(frameSizeInBytes * 2, serial);

    ASSERT_EQ(offset, frameSizeInBytes * 3);
    ASSERT_EQ(allocator.GetUsedSize(), frameSizeInBytes * maxNumOfFrames);

    //        F9         F10      F6   F7   F8
    //  [xxxxxxxxxxxx|xxxxxxxxx|xxxx|xxxx|xxxx|xxxxxxxx]
    //

    // Ensure we are full.
    ASSERT_EQ(allocator.Allocate(frameSizeInBytes, serial + 1),
              RingBufferAllocator::kInvalidOffset);

    // Reclaim all.
    allocator.Deallocate(maxNumOfFrames);

    EXPECT_TRUE(allocator.Empty());
}

// Checks if ringbuffer sub-allocation does not overflow.
TEST(RingBufferAllocatorTests, RingBufferOverflow) {
    Serial serial = 1;

    RingBufferAllocator allocator(std::numeric_limits<uint64_t>::max());

    ASSERT_EQ(allocator.Allocate(1, serial), 0u);
    ASSERT_EQ(allocator.Allocate(std::numeric_limits<uint64_t>::max(), serial + 1),
              RingBufferAllocator::kInvalidOffset);
}
