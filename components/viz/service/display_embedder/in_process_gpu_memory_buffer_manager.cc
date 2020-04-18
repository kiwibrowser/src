// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/viz/service/display_embedder/in_process_gpu_memory_buffer_manager.h"

#include "gpu/ipc/common/gpu_memory_buffer_impl.h"
#include "gpu/ipc/common/gpu_memory_buffer_support.h"
#include "gpu/ipc/in_process_command_buffer.h"
#include "gpu/ipc/service/gpu_channel_manager.h"
#include "gpu/ipc/service/gpu_memory_buffer_factory.h"

namespace viz {

InProcessGpuMemoryBufferManager::InProcessGpuMemoryBufferManager(
    gpu::GpuChannelManager* channel_manager)
    : gpu_memory_buffer_support_(new gpu::GpuMemoryBufferSupport()),
      client_id_(gpu::InProcessCommandBuffer::kGpuMemoryBufferClientId),
      channel_manager_(channel_manager),
      weak_factory_(this) {
  weak_ptr_ = weak_factory_.GetWeakPtr();
}

InProcessGpuMemoryBufferManager::~InProcessGpuMemoryBufferManager() {}

std::unique_ptr<gfx::GpuMemoryBuffer>
InProcessGpuMemoryBufferManager::CreateGpuMemoryBuffer(
    const gfx::Size& size,
    gfx::BufferFormat format,
    gfx::BufferUsage usage,
    gpu::SurfaceHandle surface_handle) {
  gfx::GpuMemoryBufferId id(next_gpu_memory_id_++);
  gfx::GpuMemoryBufferHandle buffer_handle =
      channel_manager_->gpu_memory_buffer_factory()->CreateGpuMemoryBuffer(
          id, size, format, usage, client_id_, surface_handle);
  return gpu_memory_buffer_support_->CreateGpuMemoryBufferImplFromHandle(
      buffer_handle, size, format, usage,
      base::Bind(&InProcessGpuMemoryBufferManager::DestroyGpuMemoryBuffer,
                 weak_ptr_, id, client_id_));
}

void InProcessGpuMemoryBufferManager::SetDestructionSyncToken(
    gfx::GpuMemoryBuffer* buffer,
    const gpu::SyncToken& sync_token) {
  static_cast<gpu::GpuMemoryBufferImpl*>(buffer)->set_destruction_sync_token(
      sync_token);
}

void InProcessGpuMemoryBufferManager::DestroyGpuMemoryBuffer(
    gfx::GpuMemoryBufferId id,
    int client_id,
    const gpu::SyncToken& sync_token) {
  channel_manager_->DestroyGpuMemoryBuffer(id, client_id, sync_token);
}

}  // namespace viz
