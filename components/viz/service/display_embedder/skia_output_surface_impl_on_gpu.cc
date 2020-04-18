// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/viz/service/display_embedder/skia_output_surface_impl_on_gpu.h"

#include "base/atomic_sequence_num.h"
#include "base/callback_helpers.h"
#include "base/synchronization/waitable_event.h"
#include "components/viz/service/display/output_surface_frame.h"
#include "components/viz/service/gl/gpu_service_impl.h"
#include "gpu/command_buffer/common/swap_buffers_complete_params.h"
#include "gpu/command_buffer/service/gpu_preferences.h"
#include "gpu/command_buffer/service/mailbox_manager.h"
#include "gpu/command_buffer/service/scheduler.h"
#include "gpu/command_buffer/service/sync_point_manager.h"
#include "gpu/command_buffer/service/texture_base.h"
#include "gpu/command_buffer/service/texture_manager.h"
#include "gpu/ipc/service/image_transport_surface.h"
#include "third_party/skia/include/private/SkDeferredDisplayList.h"
#include "ui/gl/gl_bindings.h"
#include "ui/gl/gl_context.h"
#include "ui/gl/gl_surface.h"
#include "ui/gl/gl_version_info.h"

namespace viz {
namespace {

base::AtomicSequenceNumber g_next_command_buffer_id;

}  // namespace

YUVResourceMetadata::YUVResourceMetadata(
    std::vector<ResourceMetadata> metadatas,
    SkYUVColorSpace yuv_color_space)
    : metadatas_(std::move(metadatas)), yuv_color_space_(yuv_color_space) {
  DCHECK(metadatas_.size() == 2 || metadatas_.size() == 3);
}

YUVResourceMetadata::YUVResourceMetadata(YUVResourceMetadata&& other) = default;

YUVResourceMetadata::~YUVResourceMetadata() = default;

YUVResourceMetadata& YUVResourceMetadata::operator=(
    YUVResourceMetadata&& other) = default;

SkiaOutputSurfaceImplOnGpu::SkiaOutputSurfaceImplOnGpu(
    GpuServiceImpl* gpu_service,
    gpu::SurfaceHandle surface_handle,
    const DidSwapBufferCompleteCallback& did_swap_buffer_complete_callback,
    const BufferPresentedCallback& buffer_presented_callback)
    : command_buffer_id_(gpu::CommandBufferId::FromUnsafeValue(
          g_next_command_buffer_id.GetNext() + 1)),
      gpu_service_(gpu_service),
      surface_handle_(surface_handle),
      did_swap_buffer_complete_callback_(did_swap_buffer_complete_callback),
      buffer_presented_callback_(buffer_presented_callback),
      weak_ptr_factory_(this) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  weak_ptr_ = weak_ptr_factory_.GetWeakPtr();

  sync_point_client_state_ =
      gpu_service_->sync_point_manager()->CreateSyncPointClientState(
          gpu::CommandBufferNamespace::VIZ_OUTPUT_SURFACE, command_buffer_id_,
          gpu_service_->skia_output_surface_sequence_id());

  surface_ = gpu::ImageTransportSurface::CreateNativeSurface(
      weak_ptr_factory_.GetWeakPtr(), surface_handle_, gl::GLSurfaceFormat());
  DCHECK(surface_);

  if (!gpu_service_->CreateGrContextIfNecessary(surface_.get())) {
    LOG(FATAL) << "Failed to create GrContext";
    // TODO(penghuang): handle the failure.
  }

  DCHECK(gpu_service_->context_for_skia());
  DCHECK(gpu_service_->gr_context());

  if (!gpu_service_->context_for_skia()->MakeCurrent(surface_.get())) {
    LOG(FATAL) << "Failed to make current.";
    // TODO(penghuang): Handle the failure.
  }

  capabilities_.flipped_output_surface = surface_->FlipsVertically();

  // Get stencil bits from the default frame buffer.
  auto* current_gl = gpu_service_->context_for_skia()->GetCurrentGL();
  const auto* version = current_gl->Version;
  auto* api = current_gl->Api;
  GLint stencil_bits = 0;
  if (version->is_desktop_core_profile) {
    api->glGetFramebufferAttachmentParameterivEXTFn(
        GL_FRAMEBUFFER, GL_STENCIL, GL_FRAMEBUFFER_ATTACHMENT_STENCIL_SIZE,
        &stencil_bits);
  } else {
    api->glGetIntegervFn(GL_STENCIL_BITS, &stencil_bits);
  }

  capabilities_.supports_stencil = stencil_bits > 0;
}

SkiaOutputSurfaceImplOnGpu::~SkiaOutputSurfaceImplOnGpu() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
}

void SkiaOutputSurfaceImplOnGpu::Reshape(
    const gfx::Size& size,
    float device_scale_factor,
    const gfx::ColorSpace& color_space,
    bool has_alpha,
    bool use_stencil,
    SkSurfaceCharacterization* characterization,
    base::WaitableEvent* event) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);

  std::unique_ptr<base::ScopedClosureRunner> scoped_runner;
  if (event) {
    scoped_runner = std::make_unique<base::ScopedClosureRunner>(
        base::BindOnce(&base::WaitableEvent::Signal, base::Unretained(event)));
  }

  if (!gpu_service_->context_for_skia()->MakeCurrent(surface_.get())) {
    LOG(FATAL) << "Failed to make current.";
    // TODO(penghuang): Handle the failure.
  }
  gl::GLSurface::ColorSpace surface_color_space =
      color_space == gfx::ColorSpace::CreateSCRGBLinear()
          ? gl::GLSurface::ColorSpace::SCRGB_LINEAR
          : gl::GLSurface::ColorSpace::UNSPECIFIED;
  if (!surface_->Resize(size, device_scale_factor, surface_color_space,
                        has_alpha)) {
    LOG(FATAL) << "Failed to resize.";
    // TODO(penghuang): Handle the failure.
  }
  DCHECK(gpu_service_->context_for_skia()->IsCurrent(surface_.get()));
  DCHECK(gpu_service_->gr_context());

  SkSurfaceProps surface_props =
      SkSurfaceProps(0, SkSurfaceProps::kLegacyFontHost_InitType);

  GrGLFramebufferInfo framebuffer_info;
  framebuffer_info.fFBOID = 0;
  const auto* version_info = gpu_service_->context_for_skia()->GetVersionInfo();
  framebuffer_info.fFormat = version_info->is_es ? GL_BGRA8_EXT : GL_RGBA8;

  GrBackendRenderTarget render_target(size.width(), size.height(), 0, 8,
                                      framebuffer_info);

  sk_surface_ = SkSurface::MakeFromBackendRenderTarget(
      gpu_service_->gr_context(), render_target, kBottomLeft_GrSurfaceOrigin,
      kBGRA_8888_SkColorType, nullptr, &surface_props);
  DCHECK(sk_surface_);

  if (characterization) {
    sk_surface_->characterize(characterization);
    DCHECK(characterization->isValid());
  }
}

void SkiaOutputSurfaceImplOnGpu::SwapBuffers(
    OutputSurfaceFrame frame,
    std::unique_ptr<SkDeferredDisplayList> ddl,
    std::vector<YUVResourceMetadata*> yuv_resource_metadatas,
    uint64_t sync_fence_release) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  DCHECK(ddl);
  DCHECK(sk_surface_);

  if (!gpu_service_->context_for_skia()->MakeCurrent(surface_.get())) {
    LOG(FATAL) << "Failed to make current.";
    // TODO(penghuang): Handle the failure.
  }

  PreprocessYUVResources(std::move(yuv_resource_metadatas));

  sk_surface_->draw(ddl.get());
  gpu_service_->gr_context()->flush();
  OnSwapBuffers();
  surface_->SwapBuffers(
      base::BindRepeating([](const gfx::PresentationFeedback&) {}));
  sync_point_client_state_->ReleaseFenceSync(sync_fence_release);
}

void SkiaOutputSurfaceImplOnGpu::FinishPaintRenderPass(
    RenderPassId id,
    std::unique_ptr<SkDeferredDisplayList> ddl,
    std::vector<YUVResourceMetadata*> yuv_resource_metadatas,
    uint64_t sync_fence_release) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  DCHECK(ddl);

  if (!gpu_service_->context_for_skia()->MakeCurrent(surface_.get())) {
    LOG(FATAL) << "Failed to make current.";
    // TODO(penghuang): Handle resize failure.
  }

  PreprocessYUVResources(std::move(yuv_resource_metadatas));

  auto& surface = offscreen_surfaces_[id];
  SkSurfaceCharacterization characterization;
  // TODO(penghuang): Using characterization != ddl->characterization(), when
  // the SkSurfaceCharacterization::operator!= is implemented in Skia.
  if (!surface || !surface->characterize(&characterization) ||
      characterization != ddl->characterization()) {
    surface = SkSurface::MakeRenderTarget(
        gpu_service_->gr_context(), ddl->characterization(), SkBudgeted::kNo);
    DCHECK(surface);
  }
  surface->draw(ddl.get());
  surface->flush();
  sync_point_client_state_->ReleaseFenceSync(sync_fence_release);
}

void SkiaOutputSurfaceImplOnGpu::RemoveRenderPassResource(
    std::vector<RenderPassId> ids) {
  DCHECK(!ids.empty());
  for (const auto& id : ids) {
    auto it = offscreen_surfaces_.find(id);
    DCHECK(it != offscreen_surfaces_.end());
    offscreen_surfaces_.erase(it);
  }
}

void SkiaOutputSurfaceImplOnGpu::FullfillPromiseTexture(
    const ResourceMetadata& metadata,
    GrBackendTexture* backend_texture) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  auto* mailbox_manager = gpu_service_->mailbox_manager();
  auto* texture_base = mailbox_manager->ConsumeTexture(metadata.mailbox);
  if (!texture_base) {
    DLOG(ERROR) << "Failed to full fill the promise texture.";
    return;
  }
  BindOrCopyTextureIfNecessary(texture_base);
  GrGLTextureInfo texture_info;
  texture_info.fTarget = texture_base->target();
  texture_info.fID = texture_base->service_id();
  texture_info.fFormat = *metadata.backend_format.getGLFormat();
  *backend_texture =
      GrBackendTexture(metadata.size.width(), metadata.size.height(),
                       metadata.mip_mapped, texture_info);
}

void SkiaOutputSurfaceImplOnGpu::FullfillPromiseTexture(
    const YUVResourceMetadata& yuv_metadata,
    GrBackendTexture* backend_texture) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);

  if (yuv_metadata.image())
    *backend_texture = yuv_metadata.image()->getBackendTexture(true);
  DLOG_IF(ERROR, !backend_texture->isValid())
      << "Failed to full fill the promise texture from yuv resources.";
}

void SkiaOutputSurfaceImplOnGpu::FullfillPromiseTexture(
    const RenderPassId id,
    GrBackendTexture* backend_texture) {
  auto it = offscreen_surfaces_.find(id);
  DCHECK(it != offscreen_surfaces_.end());
  sk_sp<SkSurface>& surface = it->second;
  *backend_texture =
      surface->getBackendTexture(SkSurface::kFlushRead_BackendHandleAccess);
  DLOG_IF(ERROR, !backend_texture->isValid())
      << "Failed to full fill the promise texture created from RenderPassId:"
      << id;
}

#if defined(OS_WIN)
void SkiaOutputSurfaceImplOnGpu::DidCreateAcceleratedSurfaceChildWindow(
    gpu::SurfaceHandle parent_window,
    gpu::SurfaceHandle child_window) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  NOTIMPLEMENTED();
}
#endif

void SkiaOutputSurfaceImplOnGpu::DidSwapBuffersComplete(
    gpu::SwapBuffersCompleteParams params) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  params.swap_response.swap_id = pending_swap_completed_ids_.front();
  pending_swap_completed_ids_.pop_front();
  did_swap_buffer_complete_callback_.Run(params);
}

const gpu::gles2::FeatureInfo* SkiaOutputSurfaceImplOnGpu::GetFeatureInfo()
    const {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  NOTIMPLEMENTED();
  return nullptr;
}

const gpu::GpuPreferences& SkiaOutputSurfaceImplOnGpu::GetGpuPreferences()
    const {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  NOTIMPLEMENTED();
  return gpu_preferences_;
}

void SkiaOutputSurfaceImplOnGpu::SetSnapshotRequestedCallback(
    const base::Closure& callback) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  NOTIMPLEMENTED();
}

void SkiaOutputSurfaceImplOnGpu::BufferPresented(
    const gfx::PresentationFeedback& feedback) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  buffer_presented_callback_.Run(feedback);
}

void SkiaOutputSurfaceImplOnGpu::AddFilter(IPC::MessageFilter* message_filter) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  NOTIMPLEMENTED();
}

int32_t SkiaOutputSurfaceImplOnGpu::GetRouteID() const {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  NOTIMPLEMENTED();
  return 0;
}

void SkiaOutputSurfaceImplOnGpu::BindOrCopyTextureIfNecessary(
    gpu::TextureBase* texture_base) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  if (gpu_service_->gpu_preferences().use_passthrough_cmd_decoder)
    return;
  // If a texture created with non-passthrough command buffer and bind with
  // an image, the Chrome will defer copying the image to the texture until
  // the texture is used. It is for implementing low latency drawing and
  // avoiding unnecessary texture copy. So we need check the texture image
  // state, and bind or copy the image to the texture if necessary.
  auto* texture = static_cast<gpu::gles2::Texture*>(texture_base);
  gpu::gles2::Texture::ImageState image_state;
  auto* image = texture->GetLevelImage(GL_TEXTURE_2D, 0, &image_state);
  if (image && image_state == gpu::gles2::Texture::UNBOUND) {
    glBindTexture(texture_base->target(), texture_base->service_id());
    if (image->BindTexImage(texture_base->target())) {
    } else {
      texture->SetLevelImageState(texture_base->target(), 0,
                                  gpu::gles2::Texture::COPIED);
      if (!image->CopyTexImage(texture_base->target()))
        LOG(ERROR) << "Failed to copy a gl image to texture.";
    }
  }
}

void SkiaOutputSurfaceImplOnGpu::PreprocessYUVResources(
    std::vector<YUVResourceMetadata*> yuv_resource_metadatas) {
  // Create SkImage for fullfilling YUV promise image, before drawing the ddl.
  // TODO(penghuang): Remove the extra step when Skia supports drawing YUV
  // textures directly.
  auto* mailbox_manager = gpu_service_->mailbox_manager();
  for (auto* yuv_metadata : yuv_resource_metadatas) {
    const auto& metadatas = yuv_metadata->metadatas();
    DCHECK(metadatas.size() == 2 || metadatas.size() == 3);
    GrBackendTexture backend_textures[3];
    size_t i = 0;
    for (const auto& metadata : metadatas) {
      auto* texture_base = mailbox_manager->ConsumeTexture(metadata.mailbox);
      if (!texture_base)
        break;
      BindOrCopyTextureIfNecessary(texture_base);
      GrGLTextureInfo texture_info;
      texture_info.fTarget = texture_base->target();
      texture_info.fID = texture_base->service_id();
      texture_info.fFormat = *metadata.backend_format.getGLFormat();
      backend_textures[i++] =
          GrBackendTexture(metadata.size.width(), metadata.size.height(),
                           GrMipMapped::kNo, texture_info);
    }

    if (i != metadatas.size())
      continue;

    sk_sp<SkImage> image;
    if (metadatas.size() == 2) {
      image = SkImage::MakeFromNV12TexturesCopy(
          gpu_service_->gr_context(), yuv_metadata->yuv_color_space(),
          backend_textures, kTopLeft_GrSurfaceOrigin,
          nullptr /* image_color_space */);
      DCHECK(image);
    } else {
      image = SkImage::MakeFromYUVTexturesCopy(
          gpu_service_->gr_context(), yuv_metadata->yuv_color_space(),
          backend_textures, kTopLeft_GrSurfaceOrigin,
          nullptr /* image_color_space */);
      DCHECK(image);
    }
    yuv_metadata->set_image(std::move(image));
  }
}

void SkiaOutputSurfaceImplOnGpu::OnSwapBuffers() {
  uint64_t swap_id = swap_id_++;
  pending_swap_completed_ids_.push_back(swap_id);
}

}  // namespace viz
