// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_LOADER_RESOURCE_BUFFER_H_
#define CONTENT_BROWSER_LOADER_RESOURCE_BUFFER_H_

#include "base/containers/queue.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/shared_memory.h"
#include "content/common/content_export.h"

namespace content {

// ResourceBuffer implements a simple "circular buffer" allocation strategy.
// Allocations are recycled in FIFO order.
//
// You can think of the ResourceBuffer as a FIFO.  The Allocate method reserves
// space in the buffer.  Allocate may be called multiple times until the buffer
// is fully reserved (at which point CanAllocate returns false).  Allocations
// are freed in FIFO order via a call to RecycleLeastRecentlyAllocated.
//
// ResourceBuffer is reference-counted for the benefit of consumers, who need
// to ensure that ResourceBuffer stays alive while they are using its memory.
//
// EXAMPLE USAGE:
//
//   // Writes data into the ResourceBuffer, and returns the location (byte
//   // offset and count) of the bytes written into the ResourceBuffer's shared
//   // memory buffer.
//   void WriteToBuffer(ResourceBuffer* buf, int* offset, int* count) {
//     DCHECK(buf->CanAllocate());
//
//     *offset = -1;
//     *count = 0;
//
//     int size;
//     char* ptr = buf->Allocate(&size);
//     if (!ptr) { /* handle error */ }
//
//     int bytes_read = static_cast<int>(fread(ptr, 1, size, file_pointer_));
//     if (!bytes_read) { /* handle error */ }
//
//     if (bytes_read < size)
//       buf->ShrinkLastAllocation(bytes_read);
//
//     *offset = buf->GetLastAllocationOffset();
//     *count = bytes_read;
//   }
//
// NOTE: As the above example illustrates, the ResourceBuffer keeps track of
// the last allocation made.  Calling ShrinkLastAllocation is optional, as it
// just helps the ResourceBuffer optimize storage and be more aggressive about
// returning larger allocations from the Allocate method.
//
class CONTENT_EXPORT ResourceBuffer
    : public base::RefCountedThreadSafe<ResourceBuffer> {
 public:
  ResourceBuffer();

  // Initialize the shared memory buffer.  It will be buffer_size bytes in
  // length.  The min/max_allocation_size parameters control the behavior of
  // the Allocate method.  It will prefer to return segments that are
  // max_allocation_size in length, but will return segments less than that if
  // space is limited.  It will not return allocations smaller than
  // min_allocation_size.
  bool Initialize(int buffer_size,
                  int min_allocation_size,
                  int max_allocation_size);
  bool IsInitialized() const;

  // Returns a reference to the underlying shared memory.
  base::SharedMemory& GetSharedMemory();

  // Returns true if Allocate will succeed.
  bool CanAllocate() const;

  // Returns a pointer into the shared memory buffer or NULL if the buffer is
  // already fully allocated.  The returned size will be max_allocation_size
  // unless the buffer is close to being full.
  char* Allocate(int* size);

  // Returns the offset into the shared memory buffer where the last allocation
  // returned by Allocate can be found.
  int GetLastAllocationOffset() const;

  // Called to reduce the size of the last allocation returned by Allocate.  It
  // is OK for new_size to match the current size of the last allocation.
  void ShrinkLastAllocation(int new_size);

  // Called to allow reuse of memory that was previously allocated.  See notes
  // above the class for more details about this method.
  void RecycleLeastRecentlyAllocated();

 private:
  friend class base::RefCountedThreadSafe<ResourceBuffer>;
  ~ResourceBuffer();

  base::SharedMemory shared_mem_;

  int buf_size_;
  int min_alloc_size_;
  int max_alloc_size_;

  // These point to the range of the shared memory that is currently allocated.
  // If alloc_start_ is -1, then the range is empty and nothing is allocated.
  // Otherwise, alloc_start_ points to the start of the allocated range, and
  // alloc_end_ points just beyond the end of the previous allocation.  In the
  // wraparound case, alloc_end_ <= alloc_start_.  See resource_buffer.cc for
  // more details about these members.
  int alloc_start_;
  int alloc_end_;

  base::queue<int> alloc_sizes_;

  DISALLOW_COPY_AND_ASSIGN(ResourceBuffer);
};

}  // namespace content

#endif  // CONTENT_BROWSER_LOADER_RESOURCE_BUFFER_H_
