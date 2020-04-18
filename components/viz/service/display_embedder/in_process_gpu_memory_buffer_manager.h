// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_VIZ_SERVICE_DISPLAY_EMBEDDER_IN_PROCESS_GPU_MEMORY_BUFFER_MANAGER_H_
#define COMPONENTS_VIZ_SERVICE_DISPLAY_EMBEDDER_IN_PROCESS_GPU_MEMORY_BUFFER_MANAGER_H_

#include "base/memory/weak_ptr.h"
#include "gpu/command_buffer/client/gpu_memory_buffer_manager.h"

namespace gpu {
class GpuChannelManager;
class GpuMemoryBufferSupport;
}

namespace viz {

class InProcessGpuMemoryBufferManager : public gpu::GpuMemoryBufferManager {
 public:
  explicit InProcessGpuMemoryBufferManager(
      gpu::GpuChannelManager* channel_manager);

  ~InProcessGpuMemoryBufferManager() override;

  // gpu::GpuMemoryBufferManager:
  std::unique_ptr<gfx::GpuMemoryBuffer> CreateGpuMemoryBuffer(
      const gfx::Size& size,
      gfx::BufferFormat format,
      gfx::BufferUsage usage,
      gpu::SurfaceHandle surface_handle) override;
  void SetDestructionSyncToken(gfx::GpuMemoryBuffer* buffer,
                               const gpu::SyncToken& sync_token) override;

 private:
  void DestroyGpuMemoryBuffer(gfx::GpuMemoryBufferId id,
                              int client_id,
                              const gpu::SyncToken& sync_token);
  std::unique_ptr<gpu::GpuMemoryBufferSupport> gpu_memory_buffer_support_;
  const int client_id_;
  int next_gpu_memory_id_ = 1;
  gpu::GpuChannelManager* channel_manager_;
  base::WeakPtr<InProcessGpuMemoryBufferManager> weak_ptr_;
  base::WeakPtrFactory<InProcessGpuMemoryBufferManager> weak_factory_;
  DISALLOW_COPY_AND_ASSIGN(InProcessGpuMemoryBufferManager);
};

}  // namespace viz

#endif  // COMPONENTS_VIZ_SERVICE_DISPLAY_EMBEDDER_IN_PROCESS_GPU_MEMORY_BUFFER_MANAGER_H_
