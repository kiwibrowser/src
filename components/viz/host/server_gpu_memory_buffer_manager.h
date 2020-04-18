// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_VIZ_HOST_SERVER_GPU_MEMORY_BUFFER_MANAGER_H_
#define COMPONENTS_VIZ_HOST_SERVER_GPU_MEMORY_BUFFER_MANAGER_H_

#include <memory>

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/single_thread_task_runner.h"
#include "base/synchronization/waitable_event.h"
#include "base/trace_event/memory_dump_provider.h"
#include "components/viz/host/viz_host_export.h"
#include "gpu/command_buffer/client/gpu_memory_buffer_manager.h"
#include "gpu/ipc/host/gpu_memory_buffer_support.h"

namespace gpu {
class GpuMemoryBufferSupport;
}

namespace viz {

namespace mojom {
class GpuService;
}

// This GpuMemoryBufferManager implementation is for [de]allocating gpu memory
// from the gpu process over the mojom.GpuService api.
// Note that |CreateGpuMemoryBuffer()| can be called on any thread. All the rest
// of the functions must be called on the thread this object is created on.
class VIZ_HOST_EXPORT ServerGpuMemoryBufferManager
    : public gpu::GpuMemoryBufferManager,
      public base::trace_event::MemoryDumpProvider {
 public:
  ServerGpuMemoryBufferManager(
      mojom::GpuService* gpu_service,
      int client_id,
      std::unique_ptr<gpu::GpuMemoryBufferSupport> gpu_memory_buffer_support);
  ~ServerGpuMemoryBufferManager() override;

  void DestroyGpuMemoryBuffer(gfx::GpuMemoryBufferId id,
                              int client_id,
                              const gpu::SyncToken& sync_token);

  void DestroyAllGpuMemoryBufferForClient(int client_id);

  void AllocateGpuMemoryBuffer(
      gfx::GpuMemoryBufferId id,
      int client_id,
      const gfx::Size& size,
      gfx::BufferFormat format,
      gfx::BufferUsage usage,
      gpu::SurfaceHandle surface_handle,
      base::OnceCallback<void(const gfx::GpuMemoryBufferHandle&)> callback);

  // Overridden from gpu::GpuMemoryBufferManager:
  std::unique_ptr<gfx::GpuMemoryBuffer> CreateGpuMemoryBuffer(
      const gfx::Size& size,
      gfx::BufferFormat format,
      gfx::BufferUsage usage,
      gpu::SurfaceHandle surface_handle) override;
  void SetDestructionSyncToken(gfx::GpuMemoryBuffer* buffer,
                               const gpu::SyncToken& sync_token) override;

  // Overridden from base::trace_event::MemoryDumpProvider:
  bool OnMemoryDump(const base::trace_event::MemoryDumpArgs& args,
                    base::trace_event::ProcessMemoryDump* pmd) override;

 private:
  uint64_t ClientIdToTracingId(int client_id) const;
  void OnGpuMemoryBufferAllocated(
      int client_id,
      size_t buffer_size_in_bytes,
      base::OnceCallback<void(const gfx::GpuMemoryBufferHandle&)> callback,
      const gfx::GpuMemoryBufferHandle& handle);

  mojom::GpuService* gpu_service_;
  const int client_id_;
  int next_gpu_memory_id_ = 1;

  struct BufferInfo {
    BufferInfo();
    ~BufferInfo();
    gfx::GpuMemoryBufferType type = gfx::EMPTY_BUFFER;
    size_t buffer_size_in_bytes = 0;
    base::UnguessableToken shared_memory_guid;
  };

  using AllocatedBuffers =
      std::unordered_map<gfx::GpuMemoryBufferId,
                         BufferInfo,
                         BASE_HASH_NAMESPACE::hash<gfx::GpuMemoryBufferId>>;
  std::unordered_map<int, AllocatedBuffers> allocated_buffers_;
  std::unordered_set<int> pending_buffers_;

  std::unique_ptr<gpu::GpuMemoryBufferSupport> gpu_memory_buffer_support_;

  const gpu::GpuMemoryBufferConfigurationSet native_configurations_;
  scoped_refptr<base::SingleThreadTaskRunner> task_runner_;
  base::WeakPtr<ServerGpuMemoryBufferManager> weak_ptr_;
  base::WeakPtrFactory<ServerGpuMemoryBufferManager> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(ServerGpuMemoryBufferManager);
};

}  // namespace viz

#endif  // COMPONENTS_VIZ_HOST_SERVER_GPU_MEMORY_BUFFER_MANAGER_H_
