// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_CAPTURE_VIDEO_CHROMEOS_LOCAL_GPU_MEMORY_BUFFER_MANAGER_H_
#define MEDIA_CAPTURE_VIDEO_CHROMEOS_LOCAL_GPU_MEMORY_BUFFER_MANAGER_H_

#include <gbm.h>

#include "gpu/command_buffer/client/gpu_memory_buffer_manager.h"
#include "media/capture/capture_export.h"

namespace media {

// A local, as opposed to the default IPC-based, implementation of
// gfx::GpuMemoryBufferManager which interacts with the DRM render node device
// directly.  The LocalGpuMemoryBufferManager is only for testing purposes and
// should not be used in production.
class CAPTURE_EXPORT LocalGpuMemoryBufferManager
    : public gpu::GpuMemoryBufferManager {
 public:
  LocalGpuMemoryBufferManager();

  // gpu::GpuMemoryBufferManager implementation
  ~LocalGpuMemoryBufferManager() override;
  std::unique_ptr<gfx::GpuMemoryBuffer> CreateGpuMemoryBuffer(
      const gfx::Size& size,
      gfx::BufferFormat format,
      gfx::BufferUsage usage,
      gpu::SurfaceHandle surface_handle) override;
  void SetDestructionSyncToken(gfx::GpuMemoryBuffer* buffer,
                               const gpu::SyncToken& sync_token) override;

 private:
  gbm_device* gbm_device_;

  DISALLOW_COPY_AND_ASSIGN(LocalGpuMemoryBufferManager);
};

}  // namespace media

#endif  // MEDIA_CAPTURE_VIDEO_CHROMEOS_LOCAL_GPU_MEMORY_BUFFER_MANAGER_H_
