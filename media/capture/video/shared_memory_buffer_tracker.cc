// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/capture/video/shared_memory_buffer_tracker.h"

#include "base/logging.h"
#include "ui/gfx/geometry/size.h"

namespace media {

SharedMemoryBufferTracker::SharedMemoryBufferTracker() = default;

SharedMemoryBufferTracker::~SharedMemoryBufferTracker() = default;

bool SharedMemoryBufferTracker::Init(const gfx::Size& dimensions,
                                     VideoPixelFormat format) {
  DVLOG(2) << __func__ << "allocating ShMem of " << dimensions.ToString();
  set_dimensions(dimensions);
  // |dimensions| can be 0x0 for trackers that do not require memory backing.
  set_max_pixel_count(dimensions.GetArea());
  set_pixel_format(format);
  return provider_.InitForSize(
      VideoCaptureFormat(dimensions, 0.0f, format).ImageAllocationSize());
}

std::unique_ptr<VideoCaptureBufferHandle>
SharedMemoryBufferTracker::GetMemoryMappedAccess() {
  return provider_.GetHandleForInProcessAccess();
}

mojo::ScopedSharedBufferHandle SharedMemoryBufferTracker::GetHandleForTransit(
    bool read_only) {
  return provider_.GetHandleForInterProcessTransit(read_only);
}

base::SharedMemoryHandle
SharedMemoryBufferTracker::GetNonOwnedSharedMemoryHandleForLegacyIPC() {
  return provider_.GetNonOwnedSharedMemoryHandleForLegacyIPC();
}

}  // namespace media
