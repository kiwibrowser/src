// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef GPU_IPC_COMMON_GPU_MEMORY_BUFFER_IMPL_SHARED_MEMORY_H_
#define GPU_IPC_COMMON_GPU_MEMORY_BUFFER_IMPL_SHARED_MEMORY_H_

#include <stddef.h>

#include <memory>

#include "base/macros.h"
#include "gpu/gpu_export.h"
#include "gpu/ipc/common/gpu_memory_buffer_impl.h"

namespace gpu {

// Implementation of GPU memory buffer based on shared memory.
class GPU_EXPORT GpuMemoryBufferImplSharedMemory : public GpuMemoryBufferImpl {
 public:
  ~GpuMemoryBufferImplSharedMemory() override;

  static constexpr gfx::GpuMemoryBufferType kBufferType =
      gfx::SHARED_MEMORY_BUFFER;

  static std::unique_ptr<GpuMemoryBufferImplSharedMemory> Create(
      gfx::GpuMemoryBufferId id,
      const gfx::Size& size,
      gfx::BufferFormat format,
      gfx::BufferUsage usage,
      const DestructionCallback& callback);

  static gfx::GpuMemoryBufferHandle CreateGpuMemoryBuffer(
      gfx::GpuMemoryBufferId id,
      const gfx::Size& size,
      gfx::BufferFormat format,
      gfx::BufferUsage usage);

  static std::unique_ptr<GpuMemoryBufferImplSharedMemory> CreateFromHandle(
      const gfx::GpuMemoryBufferHandle& handle,
      const gfx::Size& size,
      gfx::BufferFormat format,
      gfx::BufferUsage usage,
      const DestructionCallback& callback);

  static bool IsUsageSupported(gfx::BufferUsage usage);
  static bool IsConfigurationSupported(gfx::BufferFormat format,
                                       gfx::BufferUsage usage);
  static bool IsSizeValidForFormat(const gfx::Size& size,
                                   gfx::BufferFormat format);

  static base::Closure AllocateForTesting(const gfx::Size& size,
                                          gfx::BufferFormat format,
                                          gfx::BufferUsage usage,
                                          gfx::GpuMemoryBufferHandle* handle);

  // Overridden from gfx::GpuMemoryBuffer:
  bool Map() override;
  void* memory(size_t plane) override;
  void Unmap() override;
  int stride(size_t plane) const override;
  gfx::GpuMemoryBufferHandle GetHandle() const override;
  base::trace_event::MemoryAllocatorDumpGuid GetGUIDForTracing(
      uint64_t tracing_process_id) const override;

 private:
  GpuMemoryBufferImplSharedMemory(
      gfx::GpuMemoryBufferId id,
      const gfx::Size& size,
      gfx::BufferFormat format,
      gfx::BufferUsage usage,
      const DestructionCallback& callback,
      std::unique_ptr<base::SharedMemory> shared_memory,
      size_t offset,
      int stride);

  std::unique_ptr<base::SharedMemory> shared_memory_;
  size_t offset_;
  int stride_;

  DISALLOW_COPY_AND_ASSIGN(GpuMemoryBufferImplSharedMemory);
};

}  // namespace gpu

#endif  // GPU_IPC_COMMON_GPU_MEMORY_BUFFER_IMPL_SHARED_MEMORY_H_
