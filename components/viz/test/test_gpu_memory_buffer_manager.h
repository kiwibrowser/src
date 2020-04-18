// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_VIZ_TEST_TEST_GPU_MEMORY_BUFFER_MANAGER_H_
#define COMPONENTS_VIZ_TEST_TEST_GPU_MEMORY_BUFFER_MANAGER_H_

#include <memory>

#include "base/macros.h"
#include "base/synchronization/lock.h"
#include "gpu/command_buffer/client/gpu_memory_buffer_manager.h"

namespace viz {

class TestGpuMemoryBufferManager : public gpu::GpuMemoryBufferManager {
 public:
  TestGpuMemoryBufferManager();
  ~TestGpuMemoryBufferManager() override;

  std::unique_ptr<TestGpuMemoryBufferManager>
  CreateClientGpuMemoryBufferManager();
  int GetClientId() { return client_id_; }

  void OnGpuMemoryBufferDestroyed(gfx::GpuMemoryBufferId gpu_memory_buffer_id);

  // Overridden from gpu::GpuMemoryBufferManager:
  std::unique_ptr<gfx::GpuMemoryBuffer> CreateGpuMemoryBuffer(
      const gfx::Size& size,
      gfx::BufferFormat format,
      gfx::BufferUsage usage,
      gpu::SurfaceHandle surface_handle) override;
  void SetDestructionSyncToken(gfx::GpuMemoryBuffer* buffer,
                               const gpu::SyncToken& sync_token) override;

 private:
  // Buffers allocated by this manager.
  int last_gpu_memory_buffer_id_ = 1000;
  // Use of |buffers_| must be protected by the lock.
  base::Lock buffers_lock_;
  std::map<int, gfx::GpuMemoryBuffer*> buffers_;

  // Parent information for child managers.
  int client_id_ = -1;
  TestGpuMemoryBufferManager* parent_gpu_memory_buffer_manager_ = nullptr;

  // Child infomration for parent managers.
  int last_client_id_ = 5000;
  std::map<int, TestGpuMemoryBufferManager*> clients_;

  DISALLOW_COPY_AND_ASSIGN(TestGpuMemoryBufferManager);
};

}  // namespace viz

#endif  // COMPONENTS_VIZ_TEST_TEST_GPU_MEMORY_BUFFER_MANAGER_H_
