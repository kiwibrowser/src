// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_CAPTURE_VIDEO_SHARED_MEMORY_BUFFER_TRACKER_H_
#define MEDIA_CAPTURE_VIDEO_SHARED_MEMORY_BUFFER_TRACKER_H_

#include "media/capture/video/shared_memory_handle_provider.h"
#include "media/capture/video/video_capture_buffer_handle.h"
#include "media/capture/video/video_capture_buffer_tracker.h"

namespace gfx {
class Size;
}

namespace media {

// Tracker specifics for SharedMemory.
class SharedMemoryBufferTracker final : public VideoCaptureBufferTracker {
 public:
  SharedMemoryBufferTracker();
  ~SharedMemoryBufferTracker() override;

  bool Init(const gfx::Size& dimensions, VideoPixelFormat format) override;

  // Implementation of VideoCaptureBufferTracker:
  std::unique_ptr<VideoCaptureBufferHandle> GetMemoryMappedAccess() override;
  mojo::ScopedSharedBufferHandle GetHandleForTransit(bool read_only) override;
  base::SharedMemoryHandle GetNonOwnedSharedMemoryHandleForLegacyIPC() override;

 private:
  SharedMemoryHandleProvider provider_;

  DISALLOW_COPY_AND_ASSIGN(SharedMemoryBufferTracker);
};

}  // namespace media

#endif  // MEDIA_CAPTURE_VIDEO_SHARED_MEMORY_BUFFER_TRACKER_H_
