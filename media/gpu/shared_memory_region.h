// Copyright (c) 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_GPU_SHARED_MEMORY_REGION_H_
#define MEDIA_GPU_SHARED_MEMORY_REGION_H_

#include "base/memory/shared_memory_handle.h"
#include "media/base/bitstream_buffer.h"
#include "media/base/unaligned_shared_memory.h"

namespace media {

// Helper class to access a region of a SharedMemory. Different from
// SharedMemory, in which the |offset| of function MapAt() must be aligned to
// the value of |SysInfo::VMAllocationGranularity()|, the |offset| of a
// SharedMemoryRegion needs not to be aligned, this class hides the details
// and returns the mapped address of the given offset.
//
// TODO(sandersd): This is now a trivial wrapper around
// media::UnalignedSharedMemory. Switch all users over and delete
// SharedMemoryRegion.
class SharedMemoryRegion {
 public:
  // Creates a SharedMemoryRegion.
  // The mapped memory region begins at |offset| bytes from the start of the
  // shared memory and the length is |size|. It will take the ownership of
  // the |handle| and release the resource when being destroyed. Different
  // from SharedMemory, the |offset| needs not to be aligned to the value of
  // |SysInfo::VMAllocationGranularity()|.
  SharedMemoryRegion(const base::SharedMemoryHandle& handle,
                     off_t offset,
                     size_t size,
                     bool read_only);

  // Creates a SharedMemoryRegion from the given |bistream_buffer|.
  SharedMemoryRegion(const BitstreamBuffer& bitstream_buffer, bool read_only);

  // Maps the shared memory into the caller's address space.
  // Return true on success, false otherwise.
  bool Map();

  // Gets a pointer to the mapped region if it has been mapped via Map().
  // Returns |nullptr| if it is not mapped. The returned pointer points
  // to the memory at the offset previously passed to the constructor.
  void* memory();

  size_t size() const { return size_; }

 private:
  UnalignedSharedMemory shm_;
  off_t offset_;
  size_t size_;

  DISALLOW_COPY_AND_ASSIGN(SharedMemoryRegion);
};

}  // namespace media

#endif  // MEDIA_GPU_SHARED_MEMORY_REGION_H_
