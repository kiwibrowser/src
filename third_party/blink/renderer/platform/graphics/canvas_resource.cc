// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/graphics/canvas_resource.h"

#include "components/viz/common/resources/single_release_callback.h"
#include "components/viz/common/resources/transferable_resource.h"
#include "gpu/GLES2/gl2extchromium.h"
#include "gpu/command_buffer/client/gpu_memory_buffer_manager.h"
#include "gpu/command_buffer/common/gpu_memory_buffer_support.h"
#include "third_party/blink/public/platform/platform.h"
#include "third_party/blink/renderer/platform/graphics/canvas_resource_provider.h"
#include "third_party/blink/renderer/platform/graphics/gpu/shared_gpu_context.h"
#include "third_party/blink/renderer/platform/graphics/static_bitmap_image.h"
#include "third_party/skia/include/gpu/GrContext.h"
#include "ui/gfx/color_space.h"

namespace blink {

CanvasResource::CanvasResource(base::WeakPtr<CanvasResourceProvider> provider,
                               SkFilterQuality filter_quality,
                               const CanvasColorParams& color_params)
    : provider_(std::move(provider)),
      filter_quality_(filter_quality),
      color_params_(color_params) {}

CanvasResource::~CanvasResource() {
  // Sync token should have been waited on in sub-class implementation of
  // Abandon().
  DCHECK(!sync_token_for_release_.HasData());
}

bool CanvasResource::IsBitmap() {
  return false;
}

scoped_refptr<StaticBitmapImage> CanvasResource::Bitmap() {
  NOTREACHED();
  return nullptr;
}

gpu::gles2::GLES2Interface* CanvasResource::ContextGL() const {
  if (!ContextProviderWrapper())
    return nullptr;
  return ContextProviderWrapper()->ContextProvider()->ContextGL();
}

void CanvasResource::SetSyncTokenForRelease(const gpu::SyncToken& token) {
  sync_token_for_release_ = token;
}

void CanvasResource::WaitSyncTokenBeforeRelease() {
  auto* gl = ContextGL();
  if (sync_token_for_release_.HasData() && gl) {
    gl->WaitSyncTokenCHROMIUM(sync_token_for_release_.GetData());
  }
  sync_token_for_release_.Clear();
}

static void ReleaseFrameResources(
    base::WeakPtr<CanvasResourceProvider> resource_provider,
    scoped_refptr<CanvasResource> resource,
    const gpu::SyncToken& sync_token,
    bool lost_resource) {
  resource->SetSyncTokenForRelease(sync_token);
  if (lost_resource) {
    resource->Abandon();
  }
  if (resource_provider && !lost_resource && resource->IsRecycleable()) {
    resource_provider->RecycleResource(std::move(resource));
  }
}

bool CanvasResource::PrepareTransferableResource(
    viz::TransferableResource* out_resource,
    std::unique_ptr<viz::SingleReleaseCallback>* out_callback) {
  DCHECK(IsValid());

  // This should never be called on unaccelerated canvases (for now).

  // Gpu compositing is a prerequisite for accelerated 2D canvas
  // TODO: For WebGL to use this, we must add software composing support.
  DCHECK(SharedGpuContext::IsGpuCompositingEnabled());
  auto* gl = ContextGL();
  DCHECK(gl);

  const gpu::Mailbox& mailbox = GetOrCreateGpuMailbox();
  if (mailbox.IsZero())
    return false;

  *out_resource = viz::TransferableResource::MakeGLOverlay(
      mailbox, GLFilter(), TextureTarget(), GetSyncToken(), gfx::Size(Size()),
      IsOverlayCandidate());

  out_resource->color_space = color_params_.GetSamplerGfxColorSpace();

  scoped_refptr<CanvasResource> this_ref(this);
  auto func = WTF::Bind(&ReleaseFrameResources, provider_,
                        WTF::Passed(std::move(this_ref)));
  *out_callback = viz::SingleReleaseCallback::Create(std::move(func));
  return true;
}

GrContext* CanvasResource::GetGrContext() const {
  if (!ContextProviderWrapper())
    return nullptr;
  return ContextProviderWrapper()->ContextProvider()->GetGrContext();
}

GLenum CanvasResource::GLFilter() const {
  return filter_quality_ == kNone_SkFilterQuality ? GL_NEAREST : GL_LINEAR;
}

// CanvasResourceBitmap
//==============================================================================

CanvasResourceBitmap::CanvasResourceBitmap(
    scoped_refptr<StaticBitmapImage> image,
    base::WeakPtr<CanvasResourceProvider> provider,
    SkFilterQuality filter_quality,
    const CanvasColorParams& color_params)
    : CanvasResource(std::move(provider), filter_quality, color_params),
      image_(std::move(image)) {}

scoped_refptr<CanvasResourceBitmap> CanvasResourceBitmap::Create(
    scoped_refptr<StaticBitmapImage> image,
    base::WeakPtr<CanvasResourceProvider> provider,
    SkFilterQuality filter_quality,
    const CanvasColorParams& color_params) {
  scoped_refptr<CanvasResourceBitmap> resource =
      AdoptRef(new CanvasResourceBitmap(std::move(image), std::move(provider),
                                        filter_quality, color_params));
  if (resource->IsValid())
    return resource;
  return nullptr;
}

bool CanvasResourceBitmap::IsValid() const {
  if (!image_)
    return false;
  return image_->IsValid();
}

bool CanvasResourceBitmap::IsAccelerated() const {
  return image_->IsTextureBacked();
}

scoped_refptr<CanvasResource> CanvasResourceBitmap::MakeAccelerated(
    base::WeakPtr<WebGraphicsContext3DProviderWrapper>
        context_provider_wrapper) {
  if (IsAccelerated())
    return base::WrapRefCounted(this);
  if (!context_provider_wrapper)
    return nullptr;
  scoped_refptr<StaticBitmapImage> accelerated_image =
      image_->MakeAccelerated(context_provider_wrapper);
  // passing nullptr for the resource provider argument creates an orphan
  // CanvasResource, which implies that it internal resources will not be
  // recycled.
  scoped_refptr<CanvasResource> accelerated_resource =
      Create(accelerated_image, nullptr, FilterQuality(), ColorParams());
  if (!accelerated_resource)
    return nullptr;
  return accelerated_resource;
}

scoped_refptr<CanvasResource> CanvasResourceBitmap::MakeUnaccelerated() {
  if (!IsAccelerated())
    return base::WrapRefCounted(this);
  scoped_refptr<StaticBitmapImage> unaccelerated_image =
      image_->MakeUnaccelerated();
  // passing nullptr for the resource provider argument creates an orphan
  // CanvasResource, which implies that it internal resources will not be
  // recycled.
  scoped_refptr<CanvasResource> unaccelerated_resource =
      Create(unaccelerated_image, nullptr, FilterQuality(), ColorParams());
  return unaccelerated_resource;
}

void CanvasResourceBitmap::TearDown() {
  WaitSyncTokenBeforeRelease();
  // We must not disassociate the mailbox from the texture object here because
  // the texture may be recycled by skia and the associated cached mailbox
  // stored by GraphicsContext3DUtils.cpp must remain valid.
  image_ = nullptr;
}

IntSize CanvasResourceBitmap::Size() const {
  if (!image_)
    return IntSize(0, 0);
  return IntSize(image_->width(), image_->height());
}

GLenum CanvasResourceBitmap::TextureTarget() const {
  return GL_TEXTURE_2D;
}

bool CanvasResourceBitmap::IsBitmap() {
  return true;
}

scoped_refptr<StaticBitmapImage> CanvasResourceBitmap::Bitmap() {
  return image_;
}

const gpu::Mailbox& CanvasResourceBitmap::GetOrCreateGpuMailbox() {
  DCHECK(image_);  // Calling code should check IsValid() before calling this.
  image_->EnsureMailbox(kUnverifiedSyncToken, GLFilter());
  return image_->GetMailbox();
}

bool CanvasResourceBitmap::HasGpuMailbox() const {
  return image_ && image_->HasMailbox();
}

const gpu::SyncToken& CanvasResourceBitmap::GetSyncToken() {
  DCHECK(image_);  // Calling code should check IsValid() before calling this.
  return image_->GetSyncToken();
}

base::WeakPtr<WebGraphicsContext3DProviderWrapper>
CanvasResourceBitmap::ContextProviderWrapper() const {
  if (!image_)
    return nullptr;
  return image_->ContextProviderWrapper();
}

// CanvasResourceGpuMemoryBuffer
//==============================================================================

CanvasResourceGpuMemoryBuffer::CanvasResourceGpuMemoryBuffer(
    const IntSize& size,
    const CanvasColorParams& color_params,
    base::WeakPtr<WebGraphicsContext3DProviderWrapper> context_provider_wrapper,
    base::WeakPtr<CanvasResourceProvider> provider,
    SkFilterQuality filter_quality)
    : CanvasResource(provider, filter_quality, color_params),
      context_provider_wrapper_(std::move(context_provider_wrapper)),
      color_params_(color_params) {
  if (!context_provider_wrapper_)
    return;
  auto* gl = context_provider_wrapper_->ContextProvider()->ContextGL();
  auto* gr = context_provider_wrapper_->ContextProvider()->GetGrContext();
  if (!gl || !gr)
    return;
  gpu::GpuMemoryBufferManager* gpu_memory_buffer_manager =
      Platform::Current()->GetGpuMemoryBufferManager();
  if (!gpu_memory_buffer_manager)
    return;
  gpu_memory_buffer_ = gpu_memory_buffer_manager->CreateGpuMemoryBuffer(
      gfx::Size(size.Width(), size.Height()),
      color_params_.GetBufferFormat(),  // Use format
      gfx::BufferUsage::SCANOUT, gpu::kNullSurfaceHandle);
  if (!gpu_memory_buffer_) {
    return;
  }
  image_id_ = gl->CreateImageCHROMIUM(gpu_memory_buffer_->AsClientBuffer(),
                                      size.Width(), size.Height(),
                                      color_params_.GLInternalFormat());
  if (!image_id_) {
    gpu_memory_buffer_ = nullptr;
    return;
  }

  gpu_memory_buffer_->SetColorSpace(color_params.GetStorageGfxColorSpace());
  gl->GenTextures(1, &texture_id_);
  const GLenum target = TextureTarget();
  gl->BindTexture(target, texture_id_);
  gl->BindTexImage2DCHROMIUM(target, image_id_);
}

CanvasResourceGpuMemoryBuffer::~CanvasResourceGpuMemoryBuffer() {
  TearDown();
}

GLenum CanvasResourceGpuMemoryBuffer::TextureTarget() const {
  return gpu::GetPlatformSpecificTextureTarget();
}

IntSize CanvasResourceGpuMemoryBuffer::Size() const {
  return IntSize(gpu_memory_buffer_->GetSize().width(),
                 gpu_memory_buffer_->GetSize().height());
}

scoped_refptr<CanvasResourceGpuMemoryBuffer>
CanvasResourceGpuMemoryBuffer::Create(
    const IntSize& size,
    const CanvasColorParams& color_params,
    base::WeakPtr<WebGraphicsContext3DProviderWrapper> context_provider_wrapper,
    base::WeakPtr<CanvasResourceProvider> provider,
    SkFilterQuality filter_quality) {
  scoped_refptr<CanvasResourceGpuMemoryBuffer> resource =
      AdoptRef(new CanvasResourceGpuMemoryBuffer(
          size, color_params, std::move(context_provider_wrapper), provider,
          filter_quality));
  if (resource->IsValid())
    return resource;
  return nullptr;
}

void CanvasResourceGpuMemoryBuffer::TearDown() {
  WaitSyncTokenBeforeRelease();
  if (!context_provider_wrapper_ || !image_id_)
    return;
  auto* gl = context_provider_wrapper_->ContextProvider()->ContextGL();
  if (gl && image_id_)
    gl->DestroyImageCHROMIUM(image_id_);
  if (gl && texture_id_)
    gl->DeleteTextures(1, &texture_id_);
  image_id_ = 0;
  texture_id_ = 0;
  gpu_memory_buffer_ = nullptr;
}

const gpu::Mailbox& CanvasResourceGpuMemoryBuffer::GetOrCreateGpuMailbox() {
  auto* gl = ContextGL();
  DCHECK(gl);  // caller should already have early exited if !gl.
  if (gpu_mailbox_.IsZero() && gl) {
    gl->GenMailboxCHROMIUM(gpu_mailbox_.name);
    gl->ProduceTextureDirectCHROMIUM(texture_id_, gpu_mailbox_.name);
    mailbox_needs_new_sync_token_ = true;
  }
  return gpu_mailbox_;
}

bool CanvasResourceGpuMemoryBuffer::HasGpuMailbox() const {
  return !gpu_mailbox_.IsZero();
}

const gpu::SyncToken& CanvasResourceGpuMemoryBuffer::GetSyncToken() {
  if (mailbox_needs_new_sync_token_) {
    auto* gl = ContextGL();
    DCHECK(gl);  // caller should already have early exited if !gl.
    mailbox_needs_new_sync_token_ = false;
    gl->GenUnverifiedSyncTokenCHROMIUM(sync_token_.GetData());
  }
  return sync_token_;
}

void CanvasResourceGpuMemoryBuffer::CopyFromTexture(GLuint source_texture,
                                                    GLenum format,
                                                    GLenum type) {
  if (!IsValid())
    return;

  ContextGL()->CopyTextureCHROMIUM(
      source_texture, 0 /*sourceLevel*/, TextureTarget(), texture_id_,
      0 /*destLevel*/, format, type, false /*unpackFlipY*/,
      false /*unpackPremultiplyAlpha*/, false /*unpackUnmultiplyAlpha*/);
  mailbox_needs_new_sync_token_ = true;
}

base::WeakPtr<WebGraphicsContext3DProviderWrapper>
CanvasResourceGpuMemoryBuffer::ContextProviderWrapper() const {
  return context_provider_wrapper_;
}

}  // namespace blink
