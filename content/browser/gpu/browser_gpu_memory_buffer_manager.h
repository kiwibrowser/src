// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_GPU_BROWSER_GPU_MEMORY_BUFFER_MANAGER_H_
#define CONTENT_BROWSER_GPU_BROWSER_GPU_MEMORY_BUFFER_MANAGER_H_

#include <stddef.h>
#include <stdint.h>

#include <memory>
#include <utility>

#include "base/callback.h"
#include "base/containers/hash_tables.h"
#include "base/hash.h"
#include "base/macros.h"
#include "base/trace_event/memory_dump_provider.h"
#include "content/browser/gpu/gpu_process_host.h"
#include "content/common/content_export.h"
#include "gpu/command_buffer/client/gpu_memory_buffer_manager.h"
#include "gpu/ipc/common/surface_handle.h"
#include "gpu/ipc/host/gpu_memory_buffer_support.h"

namespace gpu {
class GpuMemoryBufferSupport;
}

namespace content {

class CONTENT_EXPORT BrowserGpuMemoryBufferManager
    : public gpu::GpuMemoryBufferManager,
      public base::trace_event::MemoryDumpProvider {
 public:
  using CreateCallback =
      base::OnceCallback<void(const gfx::GpuMemoryBufferHandle& handle)>;
  using AllocationCallback = CreateCallback;

  BrowserGpuMemoryBufferManager(int gpu_client_id,
                                uint64_t gpu_client_tracing_id);
  ~BrowserGpuMemoryBufferManager() override;

  static BrowserGpuMemoryBufferManager* current();

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

  void AllocateGpuMemoryBufferForChildProcess(gfx::GpuMemoryBufferId id,
                                              const gfx::Size& size,
                                              gfx::BufferFormat format,
                                              gfx::BufferUsage usage,
                                              int child_client_id,
                                              AllocationCallback callback);
  void ChildProcessDeletedGpuMemoryBuffer(
      gfx::GpuMemoryBufferId id,
      int child_client_id,
      const gpu::SyncToken& sync_token);
  void ProcessRemoved(int client_id);

  bool IsNativeGpuMemoryBufferConfiguration(gfx::BufferFormat format,
                                            gfx::BufferUsage usage) const;

 private:
  struct BufferInfo {
    BufferInfo(const gfx::Size& size,
               gfx::GpuMemoryBufferType type,
               gfx::BufferFormat format,
               gfx::BufferUsage usage,
               int gpu_host_id);
    BufferInfo(const BufferInfo& other);
    ~BufferInfo();

    gfx::Size size;
    gfx::GpuMemoryBufferType type = gfx::EMPTY_BUFFER;
    gfx::BufferFormat format = gfx::BufferFormat::RGBA_8888;
    gfx::BufferUsage usage = gfx::BufferUsage::GPU_READ;
    int gpu_host_id = 0;
    base::UnguessableToken shared_memory_guid;
  };

  struct CreateGpuMemoryBufferRequest;

  std::unique_ptr<gfx::GpuMemoryBuffer> AllocateGpuMemoryBufferForSurface(
      const gfx::Size& size,
      gfx::BufferFormat format,
      gfx::BufferUsage usage,
      gpu::SurfaceHandle surface_handle);

  // Functions that handle synchronous buffer creation requests.
  void HandleCreateGpuMemoryBufferOnIO(CreateGpuMemoryBufferRequest* request);
  void HandleGpuMemoryBufferCreatedOnIO(
      CreateGpuMemoryBufferRequest* request,
      const gfx::GpuMemoryBufferHandle& handle);

  // Functions that implement asynchronous buffer creation.
  void CreateGpuMemoryBufferOnIO(gfx::GpuMemoryBufferId id,
                                 const gfx::Size& size,
                                 gfx::BufferFormat format,
                                 gfx::BufferUsage usage,
                                 gpu::SurfaceHandle surface_handle,
                                 int client_id,
                                 CreateCallback callback);
  void GpuMemoryBufferCreatedOnIO(gfx::GpuMemoryBufferId id,
                                  gpu::SurfaceHandle surface_handle,
                                  int client_id,
                                  int gpu_host_id,
                                  CreateCallback callback,
                                  const gfx::GpuMemoryBufferHandle& handle,
                                  GpuProcessHost::BufferCreationStatus status);
  void DestroyGpuMemoryBufferOnIO(gfx::GpuMemoryBufferId id,
                                  int client_id,
                                  const gpu::SyncToken& sync_token);

  uint64_t ClientIdToTracingProcessId(int client_id) const;

  std::unique_ptr<gpu::GpuMemoryBufferSupport> gpu_memory_buffer_support_;

  const gpu::GpuMemoryBufferConfigurationSet native_configurations_;
  const int gpu_client_id_;
  const uint64_t gpu_client_tracing_id_;
  int next_gpu_memory_id_ = 1;

  // Stores info about buffers for all clients. This should only be accessed
  // on the IO thread.
  using BufferMap = base::hash_map<gfx::GpuMemoryBufferId, BufferInfo>;
  using ClientMap = base::hash_map<int, BufferMap>;
  ClientMap clients_;

  DISALLOW_COPY_AND_ASSIGN(BrowserGpuMemoryBufferManager);
};

}  // namespace content

#endif  // CONTENT_BROWSER_GPU_BROWSER_GPU_MEMORY_BUFFER_MANAGER_H_
