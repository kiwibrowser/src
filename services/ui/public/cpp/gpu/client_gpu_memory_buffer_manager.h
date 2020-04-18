// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_UI_PUBLIC_CPP_GPU_CLIENT_GPU_MEMORY_BUFFER_MANAGER_H_
#define SERVICES_UI_PUBLIC_CPP_GPU_CLIENT_GPU_MEMORY_BUFFER_MANAGER_H_

#include <memory>
#include <set>
#include <vector>

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "base/threading/thread.h"
#include "gpu/command_buffer/client/gpu_memory_buffer_manager.h"
#include "services/ui/public/interfaces/gpu.mojom.h"

namespace base {
class WaitableEvent;
}

namespace gpu {
class GpuMemoryBufferSupport;
}

namespace ui {

class ClientGpuMemoryBufferManager : public gpu::GpuMemoryBufferManager {
 public:
  explicit ClientGpuMemoryBufferManager(mojom::GpuPtr gpu);
  ~ClientGpuMemoryBufferManager() override;

 private:
  void InitThread(mojom::GpuPtrInfo gpu_info);
  void TearDownThread();
  void DisconnectGpuOnThread();
  void AllocateGpuMemoryBufferOnThread(const gfx::Size& size,
                                       gfx::BufferFormat format,
                                       gfx::BufferUsage usage,
                                       gfx::GpuMemoryBufferHandle* handle,
                                       base::WaitableEvent* wait);
  void OnGpuMemoryBufferAllocatedOnThread(
      gfx::GpuMemoryBufferHandle* ret_handle,
      base::WaitableEvent* wait,
      const gfx::GpuMemoryBufferHandle& handle);
  void DeletedGpuMemoryBuffer(gfx::GpuMemoryBufferId id,
                              const gpu::SyncToken& sync_token);

  // Overridden from gpu::GpuMemoryBufferManager:
  std::unique_ptr<gfx::GpuMemoryBuffer> CreateGpuMemoryBuffer(
      const gfx::Size& size,
      gfx::BufferFormat format,
      gfx::BufferUsage usage,
      gpu::SurfaceHandle surface_handle) override;
  void SetDestructionSyncToken(gfx::GpuMemoryBuffer* buffer,
                               const gpu::SyncToken& sync_token) override;

  int counter_ = 0;
  // TODO(sad): Explore the option of doing this from an existing thread.
  base::Thread thread_;
  mojom::GpuPtr gpu_;
  base::WeakPtr<ClientGpuMemoryBufferManager> weak_ptr_;
  std::set<base::WaitableEvent*> pending_allocation_waiters_;
  std::unique_ptr<gpu::GpuMemoryBufferSupport> gpu_memory_buffer_support_;
  base::WeakPtrFactory<ClientGpuMemoryBufferManager> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(ClientGpuMemoryBufferManager);
};

}  // namespace ui

#endif  // SERVICES_UI_PUBLIC_CPP_GPU_CLIENT_GPU_MEMORY_BUFFER_MANAGER_H_
