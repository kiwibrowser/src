// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef GPU_COMMAND_BUFFER_CLIENT_GPU_MEMORY_BUFFER_MANAGER_H_
#define GPU_COMMAND_BUFFER_CLIENT_GPU_MEMORY_BUFFER_MANAGER_H_

#include <memory>

#include "gpu/gpu_export.h"
#include "gpu/ipc/common/surface_handle.h"
#include "ui/gfx/geometry/size.h"
#include "ui/gfx/gpu_memory_buffer.h"

namespace gpu {

struct SyncToken;

class GPU_EXPORT GpuMemoryBufferManager {
 public:
  GpuMemoryBufferManager();
  virtual ~GpuMemoryBufferManager();

  // Creates a GpuMemoryBuffer that can be shared with another process. It can
  // be called on any thread.
  virtual std::unique_ptr<gfx::GpuMemoryBuffer> CreateGpuMemoryBuffer(
      const gfx::Size& size,
      gfx::BufferFormat format,
      gfx::BufferUsage usage,
      gpu::SurfaceHandle surface_handle) = 0;

  // Associates destruction sync point with |buffer|. It can be called on any
  // thread.
  virtual void SetDestructionSyncToken(gfx::GpuMemoryBuffer* buffer,
                                       const gpu::SyncToken& sync_token) = 0;
};

}  // namespace gpu

#endif  // GPU_COMMAND_BUFFER_CLIENT_GPU_MEMORY_BUFFER_MANAGER_H_
