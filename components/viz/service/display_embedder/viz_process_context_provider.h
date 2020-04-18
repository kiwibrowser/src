// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_VIZ_SERVICE_DISPLAY_EMBEDDER_VIZ_PROCESS_CONTEXT_PROVIDER_H_
#define COMPONENTS_VIZ_SERVICE_DISPLAY_EMBEDDER_VIZ_PROCESS_CONTEXT_PROVIDER_H_

#include <stdint.h>

#include <memory>

#include "base/compiler_specific.h"
#include "base/synchronization/lock.h"
#include "components/viz/common/gpu/context_cache_controller.h"
#include "components/viz/common/gpu/context_provider.h"
#include "components/viz/service/viz_service_export.h"
#include "gpu/command_buffer/common/context_creation_attribs.h"
#include "gpu/ipc/in_process_command_buffer.h"
#include "ui/gfx/native_widget_types.h"

class GrContext;

namespace gpu {
class GLInProcessContext;
class GpuChannelManagerDelegate;
class GpuMemoryBufferManager;
class ImageFactory;
struct SharedMemoryLimits;
}  // namespace gpu

namespace skia_bindings {
class GrContextForGLES2Interface;
}

namespace viz {

// A ContextProvider used in the viz process to setup an InProcessCommandBuffer
// for the display compositor.
class VIZ_SERVICE_EXPORT VizProcessContextProvider
    : public base::RefCountedThreadSafe<VizProcessContextProvider>,
      public ContextProvider {
 public:
  VizProcessContextProvider(
      scoped_refptr<gpu::InProcessCommandBuffer::Service> service,
      gpu::SurfaceHandle surface_handle,
      gpu::GpuMemoryBufferManager* gpu_memory_buffer_manager,
      gpu::ImageFactory* image_factory,
      gpu::GpuChannelManagerDelegate* gpu_channel_manager_delegate,
      const gpu::SharedMemoryLimits& limits);

  // ContextProvider implementation.
  void AddRef() const override;
  void Release() const override;
  gpu::ContextResult BindToCurrentThread() override;
  gpu::gles2::GLES2Interface* ContextGL() override;
  gpu::ContextSupport* ContextSupport() override;
  class GrContext* GrContext() override;
  ContextCacheController* CacheController() override;
  base::Lock* GetLock() override;
  const gpu::Capabilities& ContextCapabilities() const override;
  const gpu::GpuFeatureInfo& GetGpuFeatureInfo() const override;
  void AddObserver(ContextLostObserver* obs) override;
  void RemoveObserver(ContextLostObserver* obs) override;

  uint32_t GetCopyTextureInternalFormat();

  void SetUpdateVSyncParametersCallback(
      const gpu::InProcessCommandBuffer::UpdateVSyncParametersCallback&
          callback);

 protected:
  friend class base::RefCountedThreadSafe<VizProcessContextProvider>;
  ~VizProcessContextProvider() override;

 private:
  const gpu::ContextCreationAttribs attributes_;

  base::Lock context_lock_;
  std::unique_ptr<gpu::GLInProcessContext> context_;
  gpu::ContextResult context_result_;
  std::unique_ptr<skia_bindings::GrContextForGLES2Interface> gr_context_;
  std::unique_ptr<ContextCacheController> cache_controller_;
};

}  // namespace viz

#endif  // COMPONENTS_VIZ_SERVICE_DISPLAY_EMBEDDER_VIZ_PROCESS_CONTEXT_PROVIDER_H_
