// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef GPU_IPC_SERVICE_GPU_MEMORY_BUFFER_FACTORY_ANDROID_HARDWARE_BUFFER_H_
#define GPU_IPC_SERVICE_GPU_MEMORY_BUFFER_FACTORY_ANDROID_HARDWARE_BUFFER_H_

#include "gpu/command_buffer/service/image_factory.h"
#include "gpu/ipc/service/gpu_ipc_service_export.h"
#include "gpu/ipc/service/gpu_memory_buffer_factory.h"

namespace gl {
class GLImage;
}

namespace gpu {

class GPU_IPC_SERVICE_EXPORT GpuMemoryBufferFactoryAndroidHardwareBuffer
    : public GpuMemoryBufferFactory,
      public ImageFactory {
 public:
  GpuMemoryBufferFactoryAndroidHardwareBuffer();
  ~GpuMemoryBufferFactoryAndroidHardwareBuffer() override;

  // Overridden from GpuMemoryBufferFactory:
  gfx::GpuMemoryBufferHandle CreateGpuMemoryBuffer(
      gfx::GpuMemoryBufferId id,
      const gfx::Size& size,
      gfx::BufferFormat format,
      gfx::BufferUsage usage,
      int client_id,
      SurfaceHandle surface_handle) override;
  void DestroyGpuMemoryBuffer(gfx::GpuMemoryBufferId id,
                              int client_id) override;
  ImageFactory* AsImageFactory() override;

  // Overridden from ImageFactory:
  scoped_refptr<gl::GLImage> CreateImageForGpuMemoryBuffer(
      const gfx::GpuMemoryBufferHandle& handle,
      const gfx::Size& size,
      gfx::BufferFormat format,
      unsigned internalformat,
      int client_id,
      SurfaceHandle surface_handle) override;
  scoped_refptr<gl::GLImage> CreateAnonymousImage(const gfx::Size& size,
                                                  gfx::BufferFormat format,
                                                  gfx::BufferUsage usage,
                                                  unsigned internalformat,
                                                  bool* is_cleared) override;
  unsigned RequiredTextureType() override;

 private:
  DISALLOW_COPY_AND_ASSIGN(GpuMemoryBufferFactoryAndroidHardwareBuffer);
};

}  // namespace gpu

#endif  // GPU_IPC_SERVICE_GPU_MEMORY_BUFFER_FACTORY_ANDROID_HARDWARE_BUFFER_H_
