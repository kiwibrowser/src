// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/loader/resource_buffer.h"

#include <math.h>

#include "base/logging.h"

namespace content {

// A circular buffer allocator.
//
// We keep track of the starting offset (alloc_start_) and the ending offset
// (alloc_end_).  There are two layouts to keep in mind:
//
// #1:
//    ------------[XXXXXXXXXXXXXXXXXXXXXXX]----
//                ^                        ^
//                start                    end
//
// #2:
//    XXXXXXXXXX]---------------------[XXXXXXXX
//               ^                    ^
//               end                  start
//
// If end <= start, then we have the buffer wraparound case (depicted second).
// If the buffer is empty, then start and end will be set to -1.
//
// Allocations are always contiguous.

ResourceBuffer::ResourceBuffer()
    : buf_size_(0),
      min_alloc_size_(0),
      max_alloc_size_(0),
      alloc_start_(-1),
      alloc_end_(-1) {
}

ResourceBuffer::~ResourceBuffer() {
}

bool ResourceBuffer::Initialize(int buffer_size,
                                int min_allocation_size,
                                int max_allocation_size) {
  CHECK(!IsInitialized());

  // It would be wasteful if these are not multiples of min_allocation_size.
  CHECK_EQ(0, buffer_size % min_allocation_size);
  CHECK_EQ(0, max_allocation_size % min_allocation_size);

  buf_size_ = buffer_size;
  min_alloc_size_ = min_allocation_size;
  max_alloc_size_ = max_allocation_size;

  return shared_mem_.CreateAndMapAnonymous(buf_size_);
}

bool ResourceBuffer::IsInitialized() const {
  return shared_mem_.memory() != nullptr;
}

base::SharedMemory& ResourceBuffer::GetSharedMemory() {
  CHECK(IsInitialized());
  return shared_mem_;
}

bool ResourceBuffer::CanAllocate() const {
  CHECK(IsInitialized());

  if (alloc_start_ == -1)
    return true;

  int diff = alloc_end_ - alloc_start_;
  if (diff > 0)
    return (buf_size_ - diff) >= min_alloc_size_;

  return -diff >= min_alloc_size_;
}

char* ResourceBuffer::Allocate(int* size) {
  CHECK(CanAllocate());

  int alloc_offset = 0;
  int alloc_size;

  if (alloc_start_ == -1) {
    // This is the first allocation.
    alloc_start_ = 0;
    alloc_end_ = buf_size_;
    alloc_size = buf_size_;
  } else if (alloc_start_ < alloc_end_) {
    // Append the next allocation if it fits.  Otherwise, wraparound.
    //
    // NOTE: We could look to see if a larger allocation is possible by
    // wrapping around sooner, but instead we just look to fill the space at
    // the end of the buffer provided that meets the min_alloc_size_
    // requirement.
    //
    if ((buf_size_ - alloc_end_) >= min_alloc_size_) {
      alloc_offset = alloc_end_;
      alloc_size = buf_size_ - alloc_end_;
      alloc_end_ = buf_size_;
    } else {
      // It must be possible to allocate a least min_alloc_size_.
      CHECK(alloc_start_ >= min_alloc_size_);
      alloc_size = alloc_start_;
      alloc_end_ = alloc_start_;
    }
  } else {
    // This is the wraparound case.
    CHECK(alloc_end_ < alloc_start_);
    alloc_offset = alloc_end_;
    alloc_size = alloc_start_ - alloc_end_;
    alloc_end_ = alloc_start_;
  }

  // Make sure alloc_size does not exceed max_alloc_size_.  We store the
  // current value of alloc_size, so that we can use ShrinkLastAllocation to
  // trim it back.  This allows us to reuse the alloc_end_ adjustment logic.

  alloc_sizes_.push(alloc_size);

  if (alloc_size > max_alloc_size_) {
    alloc_size = max_alloc_size_;
    ShrinkLastAllocation(alloc_size);
  }

  *size = alloc_size;
  return static_cast<char*>(shared_mem_.memory()) + alloc_offset;
}

int ResourceBuffer::GetLastAllocationOffset() const {
  CHECK(!alloc_sizes_.empty());
  CHECK(alloc_end_ >= alloc_sizes_.back());
  return alloc_end_ - alloc_sizes_.back();
}

void ResourceBuffer::ShrinkLastAllocation(int new_size) {
  CHECK(!alloc_sizes_.empty());

  int aligned_size = (new_size / min_alloc_size_) * min_alloc_size_;
  if (aligned_size < new_size)
    aligned_size += min_alloc_size_;

  CHECK_LE(new_size, aligned_size);
  CHECK_GE(alloc_sizes_.back(), aligned_size);

  int* last_allocation_size = &alloc_sizes_.back();
  alloc_end_ -= (*last_allocation_size - aligned_size);
  *last_allocation_size = aligned_size;
}

void ResourceBuffer::RecycleLeastRecentlyAllocated() {
  CHECK(!alloc_sizes_.empty());
  int allocation_size = alloc_sizes_.front();
  alloc_sizes_.pop();

  alloc_start_ += allocation_size;
  CHECK(alloc_start_ <= buf_size_);

  if (alloc_start_ == alloc_end_) {
    CHECK(alloc_sizes_.empty());
    alloc_start_ = -1;
    alloc_end_ = -1;
  } else if (alloc_start_ == buf_size_) {
    CHECK(!alloc_sizes_.empty());
    alloc_start_ = 0;
  }
}

}  // namespace content
