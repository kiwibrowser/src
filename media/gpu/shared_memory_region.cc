// Copyright (c) 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/gpu/shared_memory_region.h"

namespace media {

SharedMemoryRegion::SharedMemoryRegion(const base::SharedMemoryHandle& handle,
                                       off_t offset,
                                       size_t size,
                                       bool read_only)
    : shm_(handle, read_only), offset_(offset), size_(size) {}

SharedMemoryRegion::SharedMemoryRegion(const BitstreamBuffer& bitstream_buffer,
                                       bool read_only)
    : SharedMemoryRegion(bitstream_buffer.handle(),
                         bitstream_buffer.offset(),
                         bitstream_buffer.size(),
                         read_only) {}

bool SharedMemoryRegion::Map() {
  return shm_.MapAt(offset_, size_);
}

void* SharedMemoryRegion::memory() {
  return shm_.memory();
}

}  // namespace media
