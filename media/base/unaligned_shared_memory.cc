// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/base/unaligned_shared_memory.h"

#include <limits>

#include "base/logging.h"
#include "base/sys_info.h"

namespace media {

UnalignedSharedMemory::UnalignedSharedMemory(
    const base::SharedMemoryHandle& handle,
    bool read_only)
    : shm_(handle, read_only), misalignment_(0) {}

UnalignedSharedMemory::~UnalignedSharedMemory() = default;

bool UnalignedSharedMemory::MapAt(off_t offset, size_t size) {
  if (offset < 0) {
    DLOG(ERROR) << "Invalid offset";
    return false;
  }

  /* |   |   |   |   |   |  shm pages
   *       |                offset (may exceed max size_t)
   *       |-----------|    size
   *     |-|                misalignment
   *     |                  adjusted offset
   *     |-------------|    requested mapping
   */
  // Note: result of % computation may be off_t or size_t, depending on the
  // relative ranks of those types. In any case we assume that
  // VMAllocationGranularity() fits in both types, so the final result does too.
  size_t misalignment = offset % base::SysInfo::VMAllocationGranularity();

  // Above this |size|, |size| + |misalignment| overflows.
  size_t max_size = std::numeric_limits<size_t>::max() - misalignment;
  if (size > max_size) {
    DLOG(ERROR) << "Invalid size";
    return false;
  }

  off_t adjusted_offset = offset - static_cast<off_t>(misalignment);
  if (!shm_.MapAt(adjusted_offset, size + misalignment)) {
    DLOG(ERROR) << "Failed to map shared memory";
    return false;
  }

  misalignment_ = misalignment;
  return true;
}

void* UnalignedSharedMemory::memory() const {
  return static_cast<uint8_t*>(shm_.memory()) + misalignment_;
}

}  // namespace media
