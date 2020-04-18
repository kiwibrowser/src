// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef GPU_IPC_SERVICE_GPU_MEMORY_BUFFER_FACTORY_H_
#define GPU_IPC_SERVICE_GPU_MEMORY_BUFFER_FACTORY_H_

#include <memory>
#include <vector>

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "gpu/ipc/common/surface_handle.h"
#include "gpu/ipc/service/gpu_ipc_service_export.h"
#include "ui/gfx/geometry/size.h"
#include "ui/gfx/gpu_memory_buffer.h"

namespace gpu {

class ImageFactory;

class GPU_IPC_SERVICE_EXPORT GpuMemoryBufferFactory {
 public:
  virtual ~GpuMemoryBufferFactory() = default;

  // Creates a new factory instance for native GPU memory buffers. Returns null
  // if native buffers are not supported.
  static std::unique_ptr<GpuMemoryBufferFactory> CreateNativeType();

  // Creates a new GPU memory buffer instance. A valid handle is returned on
  // success. It can be called on any thread.
  virtual gfx::GpuMemoryBufferHandle CreateGpuMemoryBuffer(
      gfx::GpuMemoryBufferId id,
      const gfx::Size& size,
      gfx::BufferFormat format,
      gfx::BufferUsage usage,
      int client_id,
      SurfaceHandle surface_handle) = 0;

  // Destroys GPU memory buffer identified by |id|.
  // It can be called on any thread.
  virtual void DestroyGpuMemoryBuffer(gfx::GpuMemoryBufferId id,
                                      int client_id) = 0;

  // Type-checking downcast routine.
  virtual ImageFactory* AsImageFactory() = 0;

 protected:
  GpuMemoryBufferFactory() = default;

 private:
  DISALLOW_COPY_AND_ASSIGN(GpuMemoryBufferFactory);
};

}  // namespace gpu

#endif  // GPU_IPC_SERVICE_GPU_MEMORY_BUFFER_FACTORY_H_
