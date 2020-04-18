// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_TEST_TEST_IN_PROCESS_CONTEXT_PROVIDER_H_
#define CC_TEST_TEST_IN_PROCESS_CONTEXT_PROVIDER_H_

#include <stdint.h>

#include <memory>

#include "base/single_thread_task_runner.h"
#include "base/synchronization/lock.h"
#include "cc/test/test_image_factory.h"
#include "components/viz/common/gpu/context_provider.h"
#include "components/viz/common/gpu/raster_context_provider.h"
#include "components/viz/test/test_gpu_memory_buffer_manager.h"
#include "gpu/config/gpu_feature_info.h"

class GrContext;

namespace gpu {
class GLInProcessContext;
class RasterInProcessContext;
}

namespace skia_bindings {
class GrContextForGLES2Interface;
}

namespace cc {

std::unique_ptr<gpu::GLInProcessContext> CreateTestInProcessContext();

class TestInProcessContextProvider
    : public base::RefCountedThreadSafe<TestInProcessContextProvider>,
      public viz::ContextProvider,
      public viz::RasterContextProvider {
 public:
  explicit TestInProcessContextProvider(bool enable_oop_rasterization);

  // viz::ContextProvider / viz::RasterContextProvider implementation.
  void AddRef() const override;
  void Release() const override;
  gpu::ContextResult BindToCurrentThread() override;
  gpu::gles2::GLES2Interface* ContextGL() override;
  gpu::raster::RasterInterface* RasterInterface() override;
  gpu::ContextSupport* ContextSupport() override;
  class GrContext* GrContext() override;
  viz::ContextCacheController* CacheController() override;
  base::Lock* GetLock() override;
  const gpu::Capabilities& ContextCapabilities() const override;
  const gpu::GpuFeatureInfo& GetGpuFeatureInfo() const override;
  void AddObserver(viz::ContextLostObserver* obs) override {}
  void RemoveObserver(viz::ContextLostObserver* obs) override {}

 protected:
  friend class base::RefCountedThreadSafe<TestInProcessContextProvider>;
  ~TestInProcessContextProvider() override;

 private:
  viz::TestGpuMemoryBufferManager gpu_memory_buffer_manager_;
  TestImageFactory image_factory_;

  // Used if support_gles2_interface.
  std::unique_ptr<gpu::GLInProcessContext> gles2_context_;
  std::unique_ptr<gpu::raster::RasterInterface> raster_implementation_gles2_;
  std::unique_ptr<skia_bindings::GrContextForGLES2Interface> gr_context_;

  // Used if !support_gles2_interface.
  std::unique_ptr<gpu::RasterInProcessContext> raster_context_;

  std::unique_ptr<viz::ContextCacheController> cache_controller_;
  base::Lock context_lock_;
  gpu::GpuFeatureInfo gpu_feature_info_;
};

}  // namespace cc

#endif  // CC_TEST_TEST_IN_PROCESS_CONTEXT_PROVIDER_H_
