// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_VIZ_SERVICE_DISPLAY_EMBEDDER_SKIA_OUTPUT_SURFACE_IMPL_ON_GPU_H_
#define COMPONENTS_VIZ_SERVICE_DISPLAY_EMBEDDER_SKIA_OUTPUT_SURFACE_IMPL_ON_GPU_H_

#include "base/containers/circular_deque.h"
#include "base/macros.h"
#include "base/threading/thread_checker.h"
#include "build/build_config.h"
#include "components/viz/common/quads/render_pass.h"
#include "components/viz/common/resources/resource_metadata.h"
#include "components/viz/service/display/output_surface.h"
#include "components/viz/service/display/output_surface_frame.h"
#include "gpu/command_buffer/common/sync_token.h"
#include "gpu/ipc/common/surface_handle.h"
#include "gpu/ipc/in_process_command_buffer.h"
#include "gpu/ipc/service/image_transport_surface_delegate.h"
#include "third_party/skia/include/core/SkSurface.h"

class SkDeferredDisplayList;

namespace base {
class WaitableEvent;
}

namespace gl {
class GLSurface;
}

namespace gpu {
class SyncPointClientState;
}

namespace viz {

class GpuServiceImpl;

// Metadata for YUV promise SkImage.
class YUVResourceMetadata {
 public:
  YUVResourceMetadata(std::vector<ResourceMetadata> metadatas,
                      SkYUVColorSpace yuv_color_space);
  YUVResourceMetadata(YUVResourceMetadata&& other);
  ~YUVResourceMetadata();
  YUVResourceMetadata& operator=(YUVResourceMetadata&& other);

  const std::vector<ResourceMetadata>& metadatas() const { return metadatas_; }
  SkYUVColorSpace yuv_color_space() const { return yuv_color_space_; }
  const sk_sp<SkImage> image() const { return image_; }
  void set_image(sk_sp<SkImage> image) { image_ = image; }
  const gfx::Size size() const { return metadatas_[0].size; }

 private:
  // Metadatas for YUV planes.
  std::vector<ResourceMetadata> metadatas_;

  SkYUVColorSpace yuv_color_space_;

  // The image copied from YUV textures, it is for fullfilling the promise
  // image.
  // TODO(penghuang): Remove it when Skia supports drawing YUV textures
  // directly.
  sk_sp<SkImage> image_;

  DISALLOW_COPY_AND_ASSIGN(YUVResourceMetadata);
};

// The SkiaOutputSurface implementation running on the GPU thread. This class
// should be created, used and destroyed on the GPU thread.
class SkiaOutputSurfaceImplOnGpu : public gpu::ImageTransportSurfaceDelegate {
 public:
  using DidSwapBufferCompleteCallback =
      base::RepeatingCallback<void(gpu::SwapBuffersCompleteParams)>;
  using BufferPresentedCallback =
      base::RepeatingCallback<void(const gfx::PresentationFeedback& feedback)>;
  SkiaOutputSurfaceImplOnGpu(
      GpuServiceImpl* gpu_service,
      gpu::SurfaceHandle surface_handle,
      const DidSwapBufferCompleteCallback& did_swap_buffer_complete_callback,
      const BufferPresentedCallback& buffer_presented_callback);
  ~SkiaOutputSurfaceImplOnGpu() override;

  gpu::CommandBufferId command_buffer_id() const { return command_buffer_id_; }
  const OutputSurface::Capabilities capabilities() const {
    return capabilities_;
  }
  const base::WeakPtr<SkiaOutputSurfaceImplOnGpu>& weak_ptr() const {
    return weak_ptr_;
  }

  void Reshape(const gfx::Size& size,
               float device_scale_factor,
               const gfx::ColorSpace& color_space,
               bool has_alpha,
               bool use_stencil,
               SkSurfaceCharacterization* characterization,
               base::WaitableEvent* event);
  void SwapBuffers(OutputSurfaceFrame frame,
                   std::unique_ptr<SkDeferredDisplayList> ddl,
                   std::vector<YUVResourceMetadata*> yuv_resource_metadatas,
                   uint64_t sync_fence_release);
  void FinishPaintRenderPass(
      RenderPassId id,
      std::unique_ptr<SkDeferredDisplayList> ddl,
      std::vector<YUVResourceMetadata*> yuv_resource_metadatas,
      uint64_t sync_fence_release);
  void RemoveRenderPassResource(std::vector<RenderPassId> ids);

  // Fullfill callback for promise SkImage created from a resource.
  void FullfillPromiseTexture(const ResourceMetadata& metadata,
                              GrBackendTexture* backend_texture);
  // Fullfill callback for promise SkImage created from YUV resources.
  void FullfillPromiseTexture(const YUVResourceMetadata& metadata,
                              GrBackendTexture* backend_texture);
  // Fullfill callback for promise SkImage created from a render pass.
  void FullfillPromiseTexture(const RenderPassId id,
                              GrBackendTexture* backend_texture);

 private:
// gpu::ImageTransportSurfaceDelegate implementation:
#if defined(OS_WIN)
  void DidCreateAcceleratedSurfaceChildWindow(
      gpu::SurfaceHandle parent_window,
      gpu::SurfaceHandle child_window) override;
#endif
  void DidSwapBuffersComplete(gpu::SwapBuffersCompleteParams params) override;
  const gpu::gles2::FeatureInfo* GetFeatureInfo() const override;
  const gpu::GpuPreferences& GetGpuPreferences() const override;
  void SetSnapshotRequestedCallback(const base::Closure& callback) override;
  void BufferPresented(const gfx::PresentationFeedback& feedback) override;
  void AddFilter(IPC::MessageFilter* message_filter) override;
  int32_t GetRouteID() const override;

  void BindOrCopyTextureIfNecessary(gpu::TextureBase* texture_base);
  void PreprocessYUVResources(
      std::vector<YUVResourceMetadata*> yuv_resource_metadatas);

  // Generage the next swap ID and push it to our pending swap ID queues.
  void OnSwapBuffers();

  const gpu::CommandBufferId command_buffer_id_;
  GpuServiceImpl* const gpu_service_;
  const gpu::SurfaceHandle surface_handle_;
  DidSwapBufferCompleteCallback did_swap_buffer_complete_callback_;
  BufferPresentedCallback buffer_presented_callback_;
  scoped_refptr<gpu::SyncPointClientState> sync_point_client_state_;
  gpu::GpuPreferences gpu_preferences_;
  scoped_refptr<gl::GLSurface> surface_;
  sk_sp<SkSurface> sk_surface_;
  OutputSurface::Capabilities capabilities_;

  // Offscreen surfaces for render passes. It can only be accessed on GPU
  // thread.
  base::flat_map<RenderPassId, sk_sp<SkSurface>> offscreen_surfaces_;

  // ID is pushed each time we begin a swap, and popped each time we present or
  // complete a swap.
  base::circular_deque<uint64_t> pending_swap_completed_ids_;
  uint64_t swap_id_ = 0;

  THREAD_CHECKER(thread_checker_);

  base::WeakPtr<SkiaOutputSurfaceImplOnGpu> weak_ptr_;
  base::WeakPtrFactory<SkiaOutputSurfaceImplOnGpu> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(SkiaOutputSurfaceImplOnGpu);
};

}  // namespace viz

#endif  // COMPONENTS_VIZ_SERVICE_DISPLAY_EMBEDDER_SKIA_OUTPUT_SURFACE_IMPL_ON_GPU_H_
