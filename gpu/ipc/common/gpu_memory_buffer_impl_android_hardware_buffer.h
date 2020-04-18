// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef GPU_IPC_COMMON_GPU_MEMORY_BUFFER_IMPL_ANDROID_HARDWARE_BUFFER_H_
#define GPU_IPC_COMMON_GPU_MEMORY_BUFFER_IMPL_ANDROID_HARDWARE_BUFFER_H_

#include "base/android/scoped_hardware_buffer_handle.h"
#include "gpu/gpu_export.h"
#include "gpu/ipc/common/gpu_memory_buffer_impl.h"

namespace gpu {

// Implementation of GPU memory buffer based on AHardwareBuffer.
class GPU_EXPORT GpuMemoryBufferImplAndroidHardwareBuffer
    : public GpuMemoryBufferImpl {
 public:
  ~GpuMemoryBufferImplAndroidHardwareBuffer() override;

  static constexpr gfx::GpuMemoryBufferType kBufferType =
      gfx::ANDROID_HARDWARE_BUFFER;

  static std::unique_ptr<GpuMemoryBufferImplAndroidHardwareBuffer> Create(
      gfx::GpuMemoryBufferId id,
      const gfx::Size& size,
      gfx::BufferFormat format,
      gfx::BufferUsage usage,
      const DestructionCallback& callback);

  static std::unique_ptr<GpuMemoryBufferImplAndroidHardwareBuffer>
  CreateFromHandle(const gfx::GpuMemoryBufferHandle& handle,
                   const gfx::Size& size,
                   gfx::BufferFormat format,
                   gfx::BufferUsage usage,
                   const DestructionCallback& callback);

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

 private:
  GpuMemoryBufferImplAndroidHardwareBuffer(
      gfx::GpuMemoryBufferId id,
      const gfx::Size& size,
      gfx::BufferFormat format,
      const DestructionCallback& callback,
      base::android::ScopedHardwareBufferHandle handle);

  base::android::ScopedHardwareBufferHandle hardware_buffer_handle_;

  DISALLOW_COPY_AND_ASSIGN(GpuMemoryBufferImplAndroidHardwareBuffer);
};

}  // namespace gpu

#endif  // GPU_IPC_COMMON_GPU_MEMORY_BUFFER_IMPL_ANDROID_HARDWARE_BUFFER_H_
